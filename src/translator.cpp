#include <string>
#include <vector>
#include <iomanip>
#include <iostream>

#include "translator.h"
#include "token.h"

using namespace std;
using namespace ck_token;
using namespace ck_ast;

void push_byte(vector<unsigned char>& bytemap, unsigned char n) {
	bytemap.push_back(n);
};

void push(vector<unsigned char>& bytemap, int n, const void* p) {
	for (int i = 0; i < n; ++i)
		bytemap.push_back(((unsigned char*) p)[i]);
};

void push_raise(vector<unsigned char>& bytemap, const wstring& s) {	
	push_byte(bytemap, ck_bytecodes::THROW_STRING);
	
	int size = s.size();
	
	push(bytemap, sizeof(int), &size);
	
	for (int i = 0; i < size; ++i) {
		wchar_t c = s[i];
		push(bytemap, sizeof(wchar_t), &c);
	}
};



// Due the parsing of loops, switch/case, functions
// translator has to preserve the type of enclosing statement 
// to allow saving template jump points:
// 
// Sample code:
// 
// while (...) {
//     ...
//     break; <-- this should jump to the loop end
//     ...
//     continue; <-- this should jump to the loop start
//     ...
// }
// 
// And bytecodes:
// 
// .loop_condition:
// ...
// Check condition
// ...
// JMP_IF_ZERO .loop_end
// ...
// JMP .loop_end <-- this is what break turns into => append address of jump address to jmp_1
// ...
// JMP .loop_condition <-- this is what continue turns into => append address of jump address to jmp_2
// ...
// .loop_end:

// In any other cases this should produce runtime throw statement
const int BREAK_PLACEMENT_NONE     = -1; // Nothing
const int BREAK_PLACEMENT_LOOP     =  2; // Loops: while, for
const int BREAK_PLACEMENT_SWCASE   =  3; // Switch/case
const int BREAK_PLACEMENT_FUNCTION =  4; // Function return statements
const int BREAK_PLACEMENT_BLOCK    =  5; // Indicate that current step is inside of the block to properly clear scopes

#define VISIT(x) visit (bytemap, lineno_table, x)

// Single structure representing address replacement block.
// Stack is used to represent stacked templates like: loop inside loop inside if inside function ...
struct address_template {
	int placement_type = BREAK_PLACEMENT_NONE;
	int start_address  = 0;
	vector<int>* jmp_1 = nullptr;
	vector<int>* jmp_2 = nullptr;
	
	address_template() {};
	address_template(int type) : placement_type(type) {};
};

vector<address_template> placement_address;
address_template none_placement(BREAK_PLACEMENT_NONE);

address_template& lookup_address(int placement_type) {
	if (placement_address.size() == 0)
		return none_placement;
	
	for (int i = placement_address.size() - 1; i >= 0; --i)
		// return > break, continue
		if ((placement_type == BREAK_PLACEMENT_LOOP || placement_type == BREAK_PLACEMENT_SWCASE) && placement_address[i].placement_type == BREAK_PLACEMENT_FUNCTION)
			return none_placement;
		else if (placement_address[i].placement_type == placement_type)
			return placement_address[i];
	
	return none_placement;
};

void push_address(int placement_type, int start_address, vector<int>* jmp_1, vector<int>* jmp_2) {
	address_template at;
	at.placement_type = placement_type;
	at.start_address = start_address;
	at.jmp_1 = jmp_1;
	at.jmp_2 = jmp_2;
	placement_address.push_back(at);
};

void pop_address() {
	if (placement_address.size() != 0)
		placement_address.pop_back();
};

void pop_address(vector<unsigned char>& bytemap, int jmp_1, int jmp_2) {
	if (placement_address.size() == 0)
		return;
	
	address_template at = placement_address.back();
	
	if (at.jmp_1) {
		vector<int>& j1 = *at.jmp_1;
		
		for (int i = 0; i < j1.size(); ++i)
			for (int k = j1[i]; k < j1[i] + sizeof(int); ++k)
				bytemap[k] = ((unsigned char*) &jmp_1)[k - j1[i]];
	}
	
	if (at.jmp_2) {
		vector<int>& j2 = *at.jmp_2;
		
		for (int i = 0; i < j2.size(); ++i) 
			for (int k = j2[i]; k < j2[i] + sizeof(int); ++k)
				bytemap[k] = ((unsigned char*) &jmp_2)[k - j2[i]];
	}
	
	placement_address.pop_back();
};


// Tracking line numbers change
int last_lineno = 0;
int last_lineno_addr = 0;
int jump_address_offset = 0;

int relative_address(vector<unsigned char>& bytemap) {
	return (int) bytemap.size() + jump_address_offset;
};

int absolute_address(vector<unsigned char>& bytemap) {
	return bytemap.size();
};

void visit(vector<unsigned char>& bytemap, vector<int>& lineno_table, ASTNode* n) {
	if (!n)
		return;
	
	// Each time the line number changes, it will be appended to the bytemap.
	// XXX: Use Line Number Table
	if (n->lineno != last_lineno) {
		last_lineno = n->lineno;
		last_lineno_addr = bytemap.size();
		
		lineno_table.push_back(last_lineno);        // Record: [lineno|start]
		lineno_table.push_back(last_lineno_addr);
		
		push_byte(bytemap, ck_bytecodes::LINENO);
		push(bytemap, sizeof(int), &last_lineno);
	}
	
	switch(n->type) {
		case ASTROOT: {
			ASTNode* t = n->left;
			while (t) {
				VISIT(t);
				t = t->next;
			}
			break;
		}
		
		case INTEGER: {
			push_byte(bytemap, ck_bytecodes::PUSH_CONST_INT);
			push(bytemap, sizeof(int64_t), (n->objectlist->object));
			break;
		}
		
		case DOUBLE: {
			push_byte(bytemap, ck_bytecodes::PUSH_CONST_DOUBLE);
			push(bytemap, sizeof(double), (n->objectlist->object));
			break;
		}
		
		case BOOLEAN: {
			push_byte(bytemap, ck_bytecodes::PUSH_CONST_BOOLEAN);
			push(bytemap, 1, (n->objectlist->object));
			break;
		}
		
		case TNULL: {
			push_byte(bytemap, ck_bytecodes::PUSH_CONST_NULL);
			break;
		}
		
		case UNDEFINED: {
			push_byte(bytemap, ck_bytecodes::PUSH_CONST_UNDEFINED);
			break;
		}
		
		case STRING: { // [size|string]
			push_byte(bytemap, ck_bytecodes::PUSH_CONST_STRING);
			
			wstring& s = *(wstring*) n->objectlist->object;
			int size = s.size();
			
			push(bytemap, sizeof(int), &size);
			
			for (int i = 0; i < size; ++i) {
				wchar_t c = s[i];
				push(bytemap, sizeof(wchar_t), &c);
			}
			break;
		}
		
		case NAME: { // [size|name]
			push_byte(bytemap, ck_bytecodes::LOAD_VAR);
			
			wstring& s = *(wstring*) n->objectlist->object;
			int size = s.size();
			
			push(bytemap, sizeof(int), &size);
			
			for (int i = 0; i < size; ++i) {
				wchar_t c = s[i];
				push(bytemap, sizeof(wchar_t), &c);
			}
			break;
		}
		
		case EXPRESSION: {
			// Standard of execution says:
			// ---> After every expression evaluation, there is only 
			//      result ot this level must left in the object stack.
			// So, EXPRESSION node expects just one object to be in stack, and, 
			// as it should be, it will be popped out after evaluation.
			
			ASTNode* t = n->left;
			while (t) {
				VISIT(t);
				t = t->next;
			}
			
			push_byte(bytemap, ck_bytecodes::VSTACK_POP);
			break;
		}
	
		case ARRAY: {
			// Initialize all objects into object stack than call PUSH_CONST_ARRAY for [size]
			
			int size = 0;
			ASTNode* t = n->left;
			while (t) {
				VISIT(t);
				t = t->next;
				++size;
			}
			
			push_byte(bytemap, ck_bytecodes::PUSH_CONST_ARRAY);
			push(bytemap, sizeof(int), &size);
			break;
		}
	
		case OBJECT: { // [size|str1_size|str1|...|strN_size|strN]
			int size = 0;
			ASTNode* t = n->left;
			while (t) {
				VISIT(t);
				t = t->next;
				++size;
			}
			
			push_byte(bytemap, ck_bytecodes::PUSH_CONST_OBJECT);
			push(bytemap, sizeof(int), &size);
			
			ASTObjectList* list = n->objectlist;
			while (list) {
				wstring* str = (wstring*) list->object;
				list = list->next;
				size = str->size();
				push(bytemap, sizeof(int), &size);
				
				for (int i = 0; i < size; ++i) {
					wchar_t c = (*str)[i];
					push(bytemap, sizeof(wchar_t), &c);
				}	
			}
			break;
		}
	
		case DEFINE: { // [amount|name1|ops1|...|nameN|opsN]		
			// ops  = has_value | safe  | local | const
			//        1000/0000   0100    0010    0001
		
			ASTNode* t = n->left;
			while (t) {
				VISIT(t);
				t = t->next;
			}
			
			int amount = 0;
			ASTObjectList* list = n->objectlist;
			while (list) {
				list = list->next;
				list = list->next;
				++amount;
			}
			
			push_byte(bytemap, ck_bytecodes::DEFINE_VAR);
			push(bytemap, sizeof(int), &amount);
			
			list = n->objectlist;
			while (list) {
				int* ops = (int*) list->object;
				list = list->next;
				wstring& str = *(wstring*) list->object;
				list = list->next;
				int size = str.size();
				
				push(bytemap, sizeof(int), &size);
				
				for (int i = 0; i < size; ++i) {
					wchar_t c = str[i];
					push(bytemap, sizeof(wchar_t), &c);
				}
				
				push_byte(bytemap, (unsigned char) (*ops) & 0b1111);
			}
			
			break;
		}
	
		case CALL: {
			// STACK:
			// arg0
			// ....
			// argN
			// ref  <-- reference to object or nothing
			// key  <-- member key or nothing
			
			int argc = 0;
			ASTNode* t = n->left->next;
			while (t) {
				VISIT(t);
				t = t->next;
				++argc;
			}
			
			if (n->left->type == FIELD) { // CALL_FIELD [name]
			
				VISIT(n->left->left);  // -> ref
				
				// STACK:
				// ref
				
				push_byte(bytemap, ck_bytecodes::CALL_FIELD);
			
				push(bytemap, sizeof(int), &argc);
				
				wstring& s = *(wstring*) n->left->objectlist->object;
				int size = s.size();
				
				push(bytemap, sizeof(int), &size);
				
				for (int i = 0; i < size; ++i) {
					wchar_t c = s[i];
					push(bytemap, sizeof(wchar_t), &c);
				}
				
				// STACK:
				// return
			} else if (n->left->type == MEMBER) { // CALL_MEMBER
				
				VISIT(n->left->left); // -> ref
				
				// STACK:
				// ref
				
				VISIT(n->left->right);       // -> key
				
				// STACK:
				// ref
				// key
				
				push_byte(bytemap, ck_bytecodes::CALL_MEMBER);
			
				push(bytemap, sizeof(int), &argc);
				
				// STACK:
				// return
			} else if (n->left->type == NAME) {
				
				// STACK:
				
				push_byte(bytemap, ck_bytecodes::CALL_NAME);
			
				push(bytemap, sizeof(int), &argc);
				
				wstring& s = *(wstring*) n->left->objectlist->object;
				int size = s.size();
				
				push(bytemap, sizeof(int), &size);
				
				for (int i = 0; i < size; ++i) {
					wchar_t c = s[i];
					push(bytemap, sizeof(wchar_t), &c);
				}
				
				// STACK:
				// return
				
			} else {
				
				VISIT(n->left);
				
				// STACK:
				// obj
				
				push_byte(bytemap, ck_bytecodes::CALL);
			
				push(bytemap, sizeof(int), &argc);
				
				// STACK:
				// return
			}
			
			break;
		}
		
		case ASSIGN: {
			if (n->left->type == FIELD) { // STORE_FIELD [name]
			
				VISIT(n->left->left);
				
				// STACK:
				// ref
				
				VISIT(n->right);
				
				// STACK:
				// ref
				// op2 (ref.member = op2)
				
				push_byte(bytemap, ck_bytecodes::VSTACK_DUP);
				push_byte(bytemap, ck_bytecodes::VSTACK_SWAP1);
				
				// STACK:
				// op2 (ref.member = op2)
				// ref
				// op2 (ref.member = op2)
				
				push_byte(bytemap, ck_bytecodes::STORE_FIELD);
				
				wstring& s = *(wstring*) n->left->objectlist->object;
				int size = s.size();
				
				push(bytemap, sizeof(int), &size);
				
				for (int i = 0; i < size; ++i) {
					wchar_t c = s[i];
					push(bytemap, sizeof(wchar_t), &c);
				}
				
				// STACK:
				// op2 (ref.member = op2)
			} else if (n->left->type == MEMBER) { // STORE_MEMBER [name]
			
				VISIT(n->left->left);
				VISIT(n->left->right);
				
				// STACK:
				// ref
				// key
				
				VISIT(n->right);
				
				// STACK:
				// ref
				// key
				// op2 (ref[member] = op2)
				
				push_byte(bytemap, ck_bytecodes::VSTACK_DUP);
				push_byte(bytemap, ck_bytecodes::VSTACK_SWAP1);
				push_byte(bytemap, ck_bytecodes::VSTACK_SWAP2);
				
				// STACK:
				// op2 (ref[member] = op2)
				// ref
				// key
				// op2 (ref[member] = op2)
				
				push_byte(bytemap, ck_bytecodes::STORE_MEMBER);
				
				// STACK:
				// op2 (ref.member = op2)
			} else if (n->left->type == NAME) { // STORE_VAR [name]
				
				VISIT(n->right);
				
				// STACK:
				// op2 (name += op2)
				
				push_byte(bytemap, ck_bytecodes::VSTACK_DUP);
				
				// STACK:
				// op2 (name = op2)
				// op2 (name = op2)
				
				push_byte(bytemap, ck_bytecodes::STORE_VAR);
				
				wstring& s = *(wstring*) n->left->objectlist->object;
				int size = s.size();
				
				push(bytemap, sizeof(int), &size);
				
				for (int i = 0; i < size; ++i) {
					wchar_t c = s[i];
					push(bytemap, sizeof(wchar_t), &c);
				}
				
				// STACK:
				// op2 (name = op2)
			}
			
			break;
		}
		
		case ASSIGN_ADD    :
		case ASSIGN_SUB    :
		case ASSIGN_MUL    :
		case ASSIGN_DIV    :
		case ASSIGN_BITRSH :
		case ASSIGN_BITLSH :
		case ASSIGN_BITURSH:
		case ASSIGN_BITULSH:
		case ASSIGN_DIR    :
		case ASSIGN_PATH   :
		case ASSIGN_MOD    :
		case ASSIGN_BITOR  :
		case ASSIGN_BITAND :
		case ASSIGN_BITXOR :
		case ASSIGN_HASH   : {			
			if (n->left->type == FIELD) { // STORE_FIELD [name]
			
				VISIT(n->left->left);
				push_byte(bytemap, ck_bytecodes::VSTACK_DUP);
				
				// STACK:
				// ref
				// ref
				
				push_byte(bytemap, ck_bytecodes::LOAD_FIELD);
				
				wstring& s = *(wstring*) n->left->objectlist->object;
				int size = s.size();
				
				push(bytemap, sizeof(int), &size);
				
				for (int i = 0; i < size; ++i) {
					wchar_t c = s[i];
					push(bytemap, sizeof(wchar_t), &c);
				}
				
				// STACK:
				// ref
				// val
				
				VISIT(n->right);
				
				// STACK:
				// ref
				// val
				// op2 (ref.member += op2)
				
				push_byte(bytemap, ck_bytecodes::OPERATOR);
				switch (n->type) {
					case ASSIGN_ADD:     push_byte(bytemap, ck_bytecodes::OPT_ADD    ); break;
					case ASSIGN_SUB:     push_byte(bytemap, ck_bytecodes::OPT_SUB    ); break;
					case ASSIGN_MUL:     push_byte(bytemap, ck_bytecodes::OPT_MUL    ); break;
					case ASSIGN_DIV:     push_byte(bytemap, ck_bytecodes::OPT_DIV    ); break;
					case ASSIGN_BITRSH:  push_byte(bytemap, ck_bytecodes::OPT_BITRSH ); break;
					case ASSIGN_BITLSH:  push_byte(bytemap, ck_bytecodes::OPT_BITLSH ); break;
					case ASSIGN_BITURSH: push_byte(bytemap, ck_bytecodes::OPT_BITURSH); break;
					case ASSIGN_DIR:     push_byte(bytemap, ck_bytecodes::OPT_DIR    ); break;
					case ASSIGN_PATH:    push_byte(bytemap, ck_bytecodes::OPT_PATH   ); break;
					case ASSIGN_MOD:     push_byte(bytemap, ck_bytecodes::OPT_MOD    ); break;
					case ASSIGN_BITOR:   push_byte(bytemap, ck_bytecodes::OPT_BITOR  ); break;
					case ASSIGN_BITAND:  push_byte(bytemap, ck_bytecodes::OPT_BITAND ); break;
					case ASSIGN_BITXOR:  push_byte(bytemap, ck_bytecodes::OPT_BITXOR ); break;
					case ASSIGN_HASH:    push_byte(bytemap, ck_bytecodes::OPT_HASH   ); break;
				}
				
				// STACK:
				// ref
				// result
				
				push_byte(bytemap, ck_bytecodes::VSTACK_DUP);
				push_byte(bytemap, ck_bytecodes::VSTACK_SWAP1);
				
				// STACK:
				// result
				// ref
				// result
				
				push_byte(bytemap, ck_bytecodes::STORE_FIELD);
				
				push(bytemap, sizeof(int), &size);
				
				for (int i = 0; i < size; ++i) {
					wchar_t c = s[i];
					push(bytemap, sizeof(wchar_t), &c);
				}
				
				// STACK:
				// result
			} else if (n->left->type == MEMBER) { // STORE_MEMBER [name]
			
				VISIT(n->left->left);
				push_byte(bytemap, ck_bytecodes::VSTACK_DUP);
				VISIT(n->left->right);
				push_byte(bytemap, ck_bytecodes::VSTACK_DUP);
				push_byte(bytemap, ck_bytecodes::VSTACK_SWAP1);
				
				// STACK:
				// ref
				// key
				// ref
				// key
				
				push_byte(bytemap, ck_bytecodes::LOAD_MEMBER);
				
				// STACK:
				// ref
				// key
				// val
				
				VISIT(n->right);
				
				// STACK:
				// ref
				// key
				// val
				// op2 (ref[member] += op2)
				
				push_byte(bytemap, ck_bytecodes::OPERATOR);
				switch (n->type) {
					case ASSIGN_ADD:     push_byte(bytemap, ck_bytecodes::OPT_ADD    ); break;
					case ASSIGN_SUB:     push_byte(bytemap, ck_bytecodes::OPT_SUB    ); break;
					case ASSIGN_MUL:     push_byte(bytemap, ck_bytecodes::OPT_MUL    ); break;
					case ASSIGN_DIV:     push_byte(bytemap, ck_bytecodes::OPT_DIV    ); break;
					case ASSIGN_BITRSH:  push_byte(bytemap, ck_bytecodes::OPT_BITRSH ); break;
					case ASSIGN_BITLSH:  push_byte(bytemap, ck_bytecodes::OPT_BITLSH ); break;
					case ASSIGN_BITURSH: push_byte(bytemap, ck_bytecodes::OPT_BITURSH); break;
					case ASSIGN_DIR:     push_byte(bytemap, ck_bytecodes::OPT_DIR    ); break;
					case ASSIGN_PATH:    push_byte(bytemap, ck_bytecodes::OPT_PATH   ); break;
					case ASSIGN_MOD:     push_byte(bytemap, ck_bytecodes::OPT_MOD    ); break;
					case ASSIGN_BITOR:   push_byte(bytemap, ck_bytecodes::OPT_BITOR  ); break;
					case ASSIGN_BITAND:  push_byte(bytemap, ck_bytecodes::OPT_BITAND ); break;
					case ASSIGN_BITXOR:  push_byte(bytemap, ck_bytecodes::OPT_BITXOR ); break;
					case ASSIGN_HASH:    push_byte(bytemap, ck_bytecodes::OPT_HASH   ); break;
				}
				
				// STACK:
				// ref
				// key
				// result
				
				push_byte(bytemap, ck_bytecodes::VSTACK_DUP);
				push_byte(bytemap, ck_bytecodes::VSTACK_SWAP1);
				push_byte(bytemap, ck_bytecodes::VSTACK_SWAP2);
				
				// STACK:
				// result
				// ref
				// key
				// result
				
				push_byte(bytemap, ck_bytecodes::STORE_MEMBER);
				
				// STACK:
				// result
			} else if (n->left->type == NAME) { // STORE_VAR [name]
				// STORE_VAR
				
				push_byte(bytemap, ck_bytecodes::LOAD_VAR);
				
				wstring& s = *(wstring*) n->left->objectlist->object;
				int size = s.size();
				
				push(bytemap, sizeof(int), &size);
				
				for (int i = 0; i < size; ++i) {
					wchar_t c = s[i];
					push(bytemap, sizeof(wchar_t), &c);
				}
				
				VISIT(n->right);
				
				// STACK:
				// val
				// op2 (name += op2)
				
				push_byte(bytemap, ck_bytecodes::OPERATOR);
				switch (n->type) {
					case ASSIGN_ADD:     push_byte(bytemap, ck_bytecodes::OPT_ADD    ); break;
					case ASSIGN_SUB:     push_byte(bytemap, ck_bytecodes::OPT_SUB    ); break;
					case ASSIGN_MUL:     push_byte(bytemap, ck_bytecodes::OPT_MUL    ); break;
					case ASSIGN_DIV:     push_byte(bytemap, ck_bytecodes::OPT_DIV    ); break;
					case ASSIGN_BITRSH:  push_byte(bytemap, ck_bytecodes::OPT_BITRSH ); break;
					case ASSIGN_BITLSH:  push_byte(bytemap, ck_bytecodes::OPT_BITLSH ); break;
					case ASSIGN_BITURSH: push_byte(bytemap, ck_bytecodes::OPT_BITURSH); break;
					case ASSIGN_DIR:     push_byte(bytemap, ck_bytecodes::OPT_DIR    ); break;
					case ASSIGN_PATH:    push_byte(bytemap, ck_bytecodes::OPT_PATH   ); break;
					case ASSIGN_MOD:     push_byte(bytemap, ck_bytecodes::OPT_MOD    ); break;
					case ASSIGN_BITOR:   push_byte(bytemap, ck_bytecodes::OPT_BITOR  ); break;
					case ASSIGN_BITAND:  push_byte(bytemap, ck_bytecodes::OPT_BITAND ); break;
					case ASSIGN_BITXOR:  push_byte(bytemap, ck_bytecodes::OPT_BITXOR ); break;
					case ASSIGN_HASH:    push_byte(bytemap, ck_bytecodes::OPT_HASH   ); break;
				}
				push_byte(bytemap, ck_bytecodes::VSTACK_DUP);
				
				// STACK:
				// result
				// result
				
				push_byte(bytemap, ck_bytecodes::STORE_VAR);
				
				push(bytemap, sizeof(int), &size);
				
				for (int i = 0; i < size; ++i) {
					wchar_t c = s[i];
					push(bytemap, sizeof(wchar_t), &c);
				}
				
				// STACK:
				// result
			}
			
			break;
		}
	
		case FIELD: {
			VISIT(n->left);
			
			push_byte(bytemap, ck_bytecodes::LOAD_FIELD);
			
			wstring& s = *(wstring*) n->objectlist->object;
			int size = s.size();
			
			push(bytemap, sizeof(int), &size);
			
			for (int i = 0; i < size; ++i) {
				wchar_t c = s[i];
				push(bytemap, sizeof(wchar_t), &c);
			}
			break;
		}
	
		case MEMBER: {
			// ...         --> VSTACK += ref <-- top-1
			// ...         --> VSTACK += key <-- top
			// LOAD_MEMBER
			
			VISIT(n->left);
			VISIT(n->right);
			
			push_byte(bytemap, ck_bytecodes::LOAD_MEMBER);
			break;
		}
	
		case POS_INC:
		case POS_DEC: {
			if (n->left->type == FIELD) { // STORE_FIELD [name]
			
				VISIT(n->left->left);
				push_byte(bytemap, ck_bytecodes::VSTACK_DUP);
				
				// STACK:
				// ref
				// ref
				
				push_byte(bytemap, ck_bytecodes::LOAD_FIELD);
				
				wstring& s = *(wstring*) n->left->objectlist->object;
				int size = s.size();
				
				push(bytemap, sizeof(int), &size);
				
				for (int i = 0; i < size; ++i) {
					wchar_t c = s[i];
					push(bytemap, sizeof(wchar_t), &c);
				}
				
				// STACK:
				// ref
				// val
				
				push_byte(bytemap, ck_bytecodes::VSTACK_DUP);
				push_byte(bytemap, ck_bytecodes::VSTACK_SWAP1);
				
				// STACK:
				// val
				// ref
				// val
				
				push_byte(bytemap, ck_bytecodes::UNARY_OPERATOR);
				if (n->type == POS_INC)
					push_byte(bytemap, ck_bytecodes::OPT_INC);
				else
					push_byte(bytemap, ck_bytecodes::OPT_DEC);
				
				// STACK:
				// val
				// ref
				// result
				
				push_byte(bytemap, ck_bytecodes::STORE_FIELD);
				
				push(bytemap, sizeof(int), &size);
				
				for (int i = 0; i < size; ++i) {
					wchar_t c = s[i];
					push(bytemap, sizeof(wchar_t), &c);
				}
				
				// STACK:
				// val
			} else if (n->left->type == MEMBER) { // STORE_MEMBER [name]
			
				VISIT(n->left->left);
				push_byte(bytemap, ck_bytecodes::VSTACK_DUP);
				VISIT(n->left->right);
				push_byte(bytemap, ck_bytecodes::VSTACK_DUP);
				push_byte(bytemap, ck_bytecodes::VSTACK_SWAP1);
				
				// STACK:
				// ref
				// key
				// ref
				// key
				
				push_byte(bytemap, ck_bytecodes::LOAD_MEMBER);
				
				// STACK:
				// ref
				// key
				// val
				
				push_byte(bytemap, ck_bytecodes::VSTACK_DUP);
				
				// STACK:
				// ref
				// key
				// val
				// val
				
				push_byte(bytemap, ck_bytecodes::UNARY_OPERATOR);
				if (n->type == POS_INC)
					push_byte(bytemap, ck_bytecodes::OPT_INC);
				else
					push_byte(bytemap, ck_bytecodes::OPT_DEC);
				
				// STACK:
				// ref
				// key
				// val
				// result

				push_byte(bytemap, ck_bytecodes::VSTACK_SWAP1);
				
				// STACK:
				// ref
				// val
				// key
				// result

				push_byte(bytemap, ck_bytecodes::VSTACK_SWAP2);
				
				// STACK:
				// val
				// ref
				// key
				// result
				
				push_byte(bytemap, ck_bytecodes::STORE_MEMBER);
				
				// STACK:
				// val
			} else if (n->left->type == NAME) { // STORE_VAR [name]
				// STORE_VAR
				
				push_byte(bytemap, ck_bytecodes::LOAD_VAR);
				
				wstring& s = *(wstring*) n->left->objectlist->object;
				int size = s.size();
				
				push(bytemap, sizeof(int), &size);
				
				for (int i = 0; i < size; ++i) {
					wchar_t c = s[i];
					push(bytemap, sizeof(wchar_t), &c);
				}
				
				// STACK:
				// val
				
				push_byte(bytemap, ck_bytecodes::VSTACK_DUP);
				push_byte(bytemap, ck_bytecodes::UNARY_OPERATOR);
				if (n->type == POS_INC)
					push_byte(bytemap, ck_bytecodes::OPT_INC);
				else
					push_byte(bytemap, ck_bytecodes::OPT_DEC);
				
				// STACK:
				// val
				// result
				
				push_byte(bytemap, ck_bytecodes::STORE_VAR);
				
				push(bytemap, sizeof(int), &size);
				
				for (int i = 0; i < size; ++i) {
					wchar_t c = s[i];
					push(bytemap, sizeof(wchar_t), &c);
				}
				
				// STACK:
				// val
			}
			
			break;
		}
		
		case PRE_INC:
		case PRE_DEC: {
			if (n->left->type == FIELD) { // STORE_FIELD [name]
			
				VISIT(n->left->left);
				push_byte(bytemap, ck_bytecodes::VSTACK_DUP);
				
				// STACK:
				// ref
				// ref
				
				push_byte(bytemap, ck_bytecodes::LOAD_FIELD);
				
				wstring& s = *(wstring*) n->left->objectlist->object;
				int size = s.size();
				
				push(bytemap, sizeof(int), &size);
				
				for (int i = 0; i < size; ++i) {
					wchar_t c = s[i];
					push(bytemap, sizeof(wchar_t), &c);
				}
				
				// STACK:
				// ref
				// val
				
				push_byte(bytemap, ck_bytecodes::UNARY_OPERATOR);
				if (n->type == PRE_INC)
					push_byte(bytemap, ck_bytecodes::OPT_INC);
				else
					push_byte(bytemap, ck_bytecodes::OPT_DEC);
				
				// STACK:
				// ref
				// result
				
				push_byte(bytemap, ck_bytecodes::VSTACK_DUP);
				push_byte(bytemap, ck_bytecodes::VSTACK_SWAP1);
				
				// STACK:
				// result
				// ref
				// result
				
				push_byte(bytemap, ck_bytecodes::STORE_FIELD);
				
				push(bytemap, sizeof(int), &size);
				
				for (int i = 0; i < size; ++i) {
					wchar_t c = s[i];
					push(bytemap, sizeof(wchar_t), &c);
				}
				
				// STACK:
				// result
			} else if (n->left->type == MEMBER) { // STORE_MEMBER [name]
			
				VISIT(n->left->left);
				push_byte(bytemap, ck_bytecodes::VSTACK_DUP);
				VISIT(n->left->right);
				push_byte(bytemap, ck_bytecodes::VSTACK_DUP);
				push_byte(bytemap, ck_bytecodes::VSTACK_SWAP1);
				
				// STACK:
				// ref
				// key
				// ref
				// key
				
				push_byte(bytemap, ck_bytecodes::LOAD_MEMBER);
				
				// STACK:
				// ref
				// key
				// val
				
				push_byte(bytemap, ck_bytecodes::VSTACK_DUP);
				
				// STACK:
				// ref
				// key
				// val
				
				push_byte(bytemap, ck_bytecodes::UNARY_OPERATOR);
				if (n->type == PRE_INC)
					push_byte(bytemap, ck_bytecodes::OPT_INC);
				else
					push_byte(bytemap, ck_bytecodes::OPT_DEC);
				
				// STACK:
				// ref
				// key
				// result
				
				push_byte(bytemap, ck_bytecodes::VSTACK_DUP);
				
				// STACK:
				// ref
				// key
				// result
				// result

				push_byte(bytemap, ck_bytecodes::VSTACK_SWAP1);
				
				// STACK:
				// ref
				// result
				// key
				// result

				push_byte(bytemap, ck_bytecodes::VSTACK_SWAP2);
				
				// STACK:
				// result
				// ref
				// key
				// result
				
				push_byte(bytemap, ck_bytecodes::STORE_MEMBER);
				
				// STACK:
				// result
			} else if (n->left->type == NAME) { // STORE_VAR [name]
				// STORE_VAR
				
				push_byte(bytemap, ck_bytecodes::LOAD_VAR);
				
				wstring& s = *(wstring*) n->left->objectlist->object;
				int size = s.size();
				
				push(bytemap, sizeof(int), &size);
				
				for (int i = 0; i < size; ++i) {
					wchar_t c = s[i];
					push(bytemap, sizeof(wchar_t), &c);
				}
				
				// STACK:
				// val
				
				push_byte(bytemap, ck_bytecodes::UNARY_OPERATOR);
				if (n->type == PRE_INC)
					push_byte(bytemap, ck_bytecodes::OPT_INC);
				else
					push_byte(bytemap, ck_bytecodes::OPT_DEC);
				
				push_byte(bytemap, ck_bytecodes::VSTACK_DUP);
				
				// STACK:
				// result
				// result
				
				push_byte(bytemap, ck_bytecodes::STORE_VAR);
				
				push(bytemap, sizeof(int), &size);
				
				for (int i = 0; i < size; ++i) {
					wchar_t c = s[i];
					push(bytemap, sizeof(wchar_t), &c);
				}
				
				// STACK:
				// result
			}
			
			break;
		}
	
		case PLUS     : { VISIT(n->left); VISIT(n->right); push_byte(bytemap, ck_bytecodes::OPERATOR); push_byte(bytemap, ck_bytecodes::OPT_ADD     ); break; }
		case MINUS    : { VISIT(n->left); VISIT(n->right); push_byte(bytemap, ck_bytecodes::OPERATOR); push_byte(bytemap, ck_bytecodes::OPT_SUB     ); break; }
		case MUL      : { VISIT(n->left); VISIT(n->right); push_byte(bytemap, ck_bytecodes::OPERATOR); push_byte(bytemap, ck_bytecodes::OPT_MUL     ); break; }
		case DIV      : { VISIT(n->left); VISIT(n->right); push_byte(bytemap, ck_bytecodes::OPERATOR); push_byte(bytemap, ck_bytecodes::OPT_DIV     ); break; }
		case BITRSH   : { VISIT(n->left); VISIT(n->right); push_byte(bytemap, ck_bytecodes::OPERATOR); push_byte(bytemap, ck_bytecodes::OPT_BITRSH  ); break; }
		case BITLSH   : { VISIT(n->left); VISIT(n->right); push_byte(bytemap, ck_bytecodes::OPERATOR); push_byte(bytemap, ck_bytecodes::OPT_BITLSH  ); break; }
		case BITURSH  : { VISIT(n->left); VISIT(n->right); push_byte(bytemap, ck_bytecodes::OPERATOR); push_byte(bytemap, ck_bytecodes::OPT_BITURSH ); break; }
		case BITULSH  : { VISIT(n->left); VISIT(n->right); push_byte(bytemap, ck_bytecodes::OPERATOR); push_byte(bytemap, ck_bytecodes::OPT_BITULSH ); break; }
		case BITNOT   : { VISIT(n->left); VISIT(n->right); push_byte(bytemap, ck_bytecodes::OPERATOR); push_byte(bytemap, ck_bytecodes::OPT_BITNOT  ); break; }
		case DIR      : { VISIT(n->left); VISIT(n->right); push_byte(bytemap, ck_bytecodes::OPERATOR); push_byte(bytemap, ck_bytecodes::OPT_DIR     ); break; }
		case PATH     : { VISIT(n->left); VISIT(n->right); push_byte(bytemap, ck_bytecodes::OPERATOR); push_byte(bytemap, ck_bytecodes::OPT_PATH    ); break; }
		case MOD      : { VISIT(n->left); VISIT(n->right); push_byte(bytemap, ck_bytecodes::OPERATOR); push_byte(bytemap, ck_bytecodes::OPT_MOD     ); break; }
		case BITOR    : { VISIT(n->left); VISIT(n->right); push_byte(bytemap, ck_bytecodes::OPERATOR); push_byte(bytemap, ck_bytecodes::OPT_BITOR   ); break; }
		case BITAND   : { VISIT(n->left); VISIT(n->right); push_byte(bytemap, ck_bytecodes::OPERATOR); push_byte(bytemap, ck_bytecodes::OPT_BITAND  ); break; }
		case HASH     : { VISIT(n->left); VISIT(n->right); push_byte(bytemap, ck_bytecodes::OPERATOR); push_byte(bytemap, ck_bytecodes::OPT_HASH    ); break; }
		case EQ       : { VISIT(n->left); VISIT(n->right); push_byte(bytemap, ck_bytecodes::OPERATOR); push_byte(bytemap, ck_bytecodes::OPT_EQ      ); break; }
		case NEQ      : { VISIT(n->left); VISIT(n->right); push_byte(bytemap, ck_bytecodes::OPERATOR); push_byte(bytemap, ck_bytecodes::OPT_NEQ     ); break; }
		case LEQ      : { VISIT(n->left); VISIT(n->right); push_byte(bytemap, ck_bytecodes::OPERATOR); push_byte(bytemap, ck_bytecodes::OPT_LEQ     ); break; }
		case NLEQ     : { VISIT(n->left); VISIT(n->right); push_byte(bytemap, ck_bytecodes::OPERATOR); push_byte(bytemap, ck_bytecodes::OPT_NLEQ    ); break; }
		case GT       : { VISIT(n->left); VISIT(n->right); push_byte(bytemap, ck_bytecodes::OPERATOR); push_byte(bytemap, ck_bytecodes::OPT_GT      ); break; }
		case GE       : { VISIT(n->left); VISIT(n->right); push_byte(bytemap, ck_bytecodes::OPERATOR); push_byte(bytemap, ck_bytecodes::OPT_GE      ); break; }
		case LT       : { VISIT(n->left); VISIT(n->right); push_byte(bytemap, ck_bytecodes::OPERATOR); push_byte(bytemap, ck_bytecodes::OPT_LT      ); break; }
		case LE       : { VISIT(n->left); VISIT(n->right); push_byte(bytemap, ck_bytecodes::OPERATOR); push_byte(bytemap, ck_bytecodes::OPT_LE      ); break; }
		case PUSH     : { VISIT(n->left); VISIT(n->right); push_byte(bytemap, ck_bytecodes::OPERATOR); push_byte(bytemap, ck_bytecodes::OPT_PUSH    ); break; }
		case ARROW    : { VISIT(n->left); VISIT(n->right); push_byte(bytemap, ck_bytecodes::OPERATOR); push_byte(bytemap, ck_bytecodes::OPT_ARROW   ); break; }
		case BITXOR   : { VISIT(n->left); VISIT(n->right); push_byte(bytemap, ck_bytecodes::OPERATOR); push_byte(bytemap, ck_bytecodes::OPT_BITXOR  ); break; }
		case AS       : { VISIT(n->left); VISIT(n->right); push_byte(bytemap, ck_bytecodes::OPERATOR); push_byte(bytemap, ck_bytecodes::OPT_AS      ); break; }
		case ISTYPEOF : { VISIT(n->left); VISIT(n->right); push_byte(bytemap, ck_bytecodes::OPERATOR); push_byte(bytemap, ck_bytecodes::OPT_ISTYPEOF); break; }
		
		// Lazy calculations
		case OR: {
			// A || B
			// if A is true then return value of A
			// else
			//   return value of A || B
		
			//  VISIT(left)
			//  VSTACK_DUP
			//  JMP_IF_ZERO secondary_jmp
			//  JMP .end_jmp
			// .secondary_jmp:
			//  VISIT(right)
			//  OPERATOR ||
			// .end_jmp:
			
			VISIT(n->left);
			
			// Duplicate A value on stack
			push_byte(bytemap, ck_bytecodes::VSTACK_DUP);
			
			push_byte(bytemap, ck_bytecodes::JMP_IF_ZERO);
			
			// Insert jump point if A is false
			int secondary_jmp_addr = absolute_address(bytemap);
			push(bytemap, sizeof(int), &secondary_jmp_addr);
			
			push_byte(bytemap, ck_bytecodes::JMP);
			
			// Insert jump point to the end of expression
			int end_jmp_addr = absolute_address(bytemap);
			push(bytemap, sizeof(int), &end_jmp_addr);
			
			// Insert address
			int secondary_jmp = relative_address(bytemap);
			for (int i = 0; i < sizeof(int); ++i) 
				bytemap[i + secondary_jmp_addr] = ((unsigned char*) &secondary_jmp)[i];
			
			VISIT(n->right);
			
			push_byte(bytemap, ck_bytecodes::OPERATOR); 
			push_byte(bytemap, ck_bytecodes::OPT_OR);
			
			// Insert address
			int end_jmp = relative_address(bytemap);
			for (int i = 0; i < sizeof(int); ++i) 
				bytemap[i + end_jmp_addr] = ((unsigned char*) &end_jmp)[i];

			break; 
		}
		case AND: {
			// A && B
			// if A is true then return value of A
			// else
			//   return value of A && B
		
			//  VISIT(left)
			//  VSTACK_DUP
			//  JMP_IF_ZERO end_jmp
			//  VISIT(right)
			//  OPERATOR &&
			// .end_jmp:
			
			VISIT(n->left);
			
			// Duplicate A value on stack
			push_byte(bytemap, ck_bytecodes::VSTACK_DUP);
			
			push_byte(bytemap, ck_bytecodes::JMP_IF_ZERO);
			
			// Insert jump point to the end of expression
			int end_jmp_addr = absolute_address(bytemap);
			push(bytemap, sizeof(int), &end_jmp_addr);
			
			VISIT(n->right);
			
			push_byte(bytemap, ck_bytecodes::OPERATOR); 
			push_byte(bytemap, ck_bytecodes::OPT_AND);
			
			// Insert address
			int end_jmp = relative_address(bytemap);
			for (int i = 0; i < sizeof(int); ++i) 
				bytemap[i + end_jmp_addr] = ((unsigned char*) &end_jmp)[i];

			break; 
		}
	
		case DOG   : { VISIT(n->left); push_byte(bytemap, ck_bytecodes::UNARY_OPERATOR); push_byte(bytemap, ck_bytecodes::OPT_DOG);    break; }
		case NOT   : { VISIT(n->left); push_byte(bytemap, ck_bytecodes::UNARY_OPERATOR); push_byte(bytemap, ck_bytecodes::OPT_NOT);    break; }
		case POS   : { VISIT(n->left); push_byte(bytemap, ck_bytecodes::UNARY_OPERATOR); push_byte(bytemap, ck_bytecodes::OPT_POS);    break; }
		case NEG   : { VISIT(n->left); push_byte(bytemap, ck_bytecodes::UNARY_OPERATOR); push_byte(bytemap, ck_bytecodes::OPT_NEG);    break; }
		case TYPEOF: { VISIT(n->left); push_byte(bytemap, ck_bytecodes::UNARY_OPERATOR); push_byte(bytemap, ck_bytecodes::OPT_TYPEOF); break; }
		
		case BLOCK: {
			push_byte(bytemap, ck_bytecodes::VSTATE_PUSH_SCOPE);
			push_address(BREAK_PLACEMENT_BLOCK, 0, nullptr, nullptr);
			
			ASTNode* t = n->left;
			while (t) {
				VISIT(t);
				t = t->next;
			}
			
			pop_address(bytemap, 0, 0);
			push_byte(bytemap, ck_bytecodes::VSTATE_POP_SCOPE);
			
			break;
		}
		
		case EMPTY: {
			push_byte(bytemap, ck_bytecodes::NOP);
			break;
		}
		
		case WHILE: {
			vector<int> jmp_1; // <-- loop start replacement
			vector<int> jmp_2; // <-- loop end replacement
			
			int loop_start = relative_address(bytemap);
			
			VISIT(n->left);
			push_byte(bytemap, ck_bytecodes::JMP_IF_ZERO);
			
			// Expect loop end to be inserted later
			jmp_2.push_back(absolute_address(bytemap));
			push(bytemap, sizeof(int), &loop_start);
			
			// Start trackiing all placement templates
			push_address(BREAK_PLACEMENT_LOOP, loop_start, &jmp_1, &jmp_2);
			
			VISIT(n->right);
			push_byte(bytemap, ck_bytecodes::JMP);
			push(bytemap, sizeof(int), &loop_start);
			
			pop_address(bytemap, loop_start, relative_address(bytemap));
			
			break;
		}
		
		case THROW: {
			if (n->left->type == EMPTY)
				push_byte(bytemap, ck_bytecodes::THROW_NOARG);
			else {
				VISIT(n->left);
				push_byte(bytemap, ck_bytecodes::THROW);
			}
			
			break;
		}
		
		case BREAK: {
			// jmp_1 --> start
			// jmp_2 --> end
			
			// XXX: Support switch/case
			
			address_template& at = lookup_address(BREAK_PLACEMENT_LOOP);
			if (at.placement_type == BREAK_PLACEMENT_NONE) 
				push_raise(bytemap, L"break outside of the loop or case");
			else {
				// Clear all block's scopes
				int num_blocks = 0;
				for (int i = placement_address.size() - 1; i >= 0; --i)
					if (placement_address[i].placement_type == BREAK_PLACEMENT_BLOCK)
						++num_blocks;
					else
						break;
				
				if (num_blocks) 
					if (num_blocks == 1) 
						push_byte(bytemap, ck_bytecodes::VSTATE_POP_SCOPE);
					else {
						push_byte(bytemap, ck_bytecodes::VSTATE_POP_SCOPES);
						push(bytemap, sizeof(int), &num_blocks);
					}
			
				// Preserve address for jump to the end
				push_byte(bytemap, ck_bytecodes::JMP);
				at.jmp_2->push_back(absolute_address(bytemap));
				int dummy = 13;
				push(bytemap, sizeof(int), &dummy);
			}
			
			break;
		}
				
		case CONTINUE: {
			// jmp_1 --> start
			// jmp_2 --> end
			
			address_template& at = lookup_address(BREAK_PLACEMENT_LOOP);
			if (at.placement_type == BREAK_PLACEMENT_NONE) 
				push_raise(bytemap, L"continue outside of the loop or case");
			else {
				// Clear all block's scopes
				int num_blocks = 0;
				for (int i = placement_address.size() - 1; i >= 0; --i)
					if (placement_address[i].placement_type == BREAK_PLACEMENT_BLOCK)
						++num_blocks;
					else
						break;
				
				if (num_blocks) 
					if (num_blocks == 1) 
						push_byte(bytemap, ck_bytecodes::VSTATE_POP_SCOPE);
					else {
						push_byte(bytemap, ck_bytecodes::VSTATE_POP_SCOPES);
						push(bytemap, sizeof(int), &num_blocks);
					}
					
				// Preserve address for jump to the end
				push_byte(bytemap, ck_bytecodes::JMP);
				at.jmp_1->push_back(absolute_address(bytemap));
				int dummy = 13;
				push(bytemap, sizeof(int), &dummy);
			}
			
			break;
		}
		
		case RETURN: {
			// jmp_1 --> start
			// jmp_2 --> end
			
			address_template& at = lookup_address(BREAK_PLACEMENT_FUNCTION);
			if (at.placement_type == BREAK_PLACEMENT_NONE) 
				push_raise(bytemap, L"return outside of the function");
			else {
				if (n->left != nullptr)
					VISIT(n->left);
				else
					push_byte(bytemap, ck_bytecodes::PUSH_CONST_UNDEFINED);
				
				/*
				// Clear all block's scopes
				int num_blocks = 0;
				for (int i = placement_address.size() - 1; i >= 0; --i)
					if (placement_address[i].placement_type == BREAK_PLACEMENT_BLOCK)
						++num_blocks;
					else
						break;
				
				if (num_blocks) 
					if (num_blocks == 1) 
						push_byte(bytemap, ck_bytecodes::VSTATE_POP_SCOPE);
					else {
						push_byte(bytemap, ck_bytecodes::VSTATE_POP_SCOPES);
						push(bytemap, sizeof(int), &num_blocks);
					}
					*/
				
				push_byte(bytemap, ck_bytecodes::RETURN_VALUE);
			}
			
			break;
		}
		
		case SWITCH: {
			// PUSH condition
			// ...
			
			// | For each case type
			// v
			
			//  VSTACK_DUP <-- duplicate condition for check
			//  PUSH case i expression
			//  OPERATOR [==]
			//  JMP_IF_ZERO .case_i+1_test <-- matched condition, clear stack
			//  VSTACK_POP
			//  JMP .case_i
			// .case_i+1_test:
			
			// | If default exist, inserted after check
			// v
			
			// VSTACK_POP
			// JMP .default
			
			// | If default doesn't exist, inserted after check
			// v
			
			// VSTACK_POP
			// JMP .sw_end
			
			// .case_i:
			//  ~ BREAK .sw_end
			
			// ^
			// | Sequently, no jump out, in ast seqence
			// v
			
			// .default
			//  ~ BREAK .sw_end
			
			// | Inserted into end
			// v
			
			// .sw_end:
		}
		
		case FUNCTION: {
			// Push all arguments
			int argc = 0;
			ASTObjectList* list = n->objectlist;
			while (list) {
				list = list->next;
				++argc;
			}
			
			// BYTECODE:
			// PUSH_CONST_FUNCTION
			// [argc]
			// [arg names]
			// [size of block]
			// [block]
			
			push_byte(bytemap, ck_bytecodes::PUSH_CONST_FUNCTION);
			push(bytemap, sizeof(int), &argc);
			
			list = n->objectlist;
			while (list) {
				wstring* str = (wstring*) list->object;
				list = list->next;
				int size = str->size();
				push(bytemap, sizeof(int), &size);
				
				for (int i = 0; i < size; ++i) {
					wchar_t c = (*str)[i];
					push(bytemap, sizeof(wchar_t), &c);
				}	
			}
			
			int size_of_block = 13;
			int start_of_size = absolute_address(bytemap);
			push(bytemap, sizeof(int), &size_of_block);
			int start_of_block = relative_address(bytemap);
			
			push_address(BREAK_PLACEMENT_FUNCTION, 0, nullptr, nullptr);
			
			// Function is subscriptable and have to has it's own address range
			int function = relative_address(bytemap);
			jump_address_offset -= function;
			
			VISIT(n->left);
			
			// push_byte(bytemap, ck_bytecodes::PUSH_CONST_UNDEFINED);
			// push_byte(bytemap, ck_bytecodes::RETURN_VALUE);
			
			pop_address(bytemap, 0, 0);
			
			jump_address_offset += function;
			
			int end_of_block = relative_address(bytemap);
			size_of_block = end_of_block - start_of_block;
			
			for (int i = 0; i < sizeof(int); ++i) 
				bytemap[i + start_of_size] = ((unsigned char*) &size_of_block)[i];
			
			break;
		}
	
		case DO: {
			vector<int> jmp_1; // <-- loop start replacement
			vector<int> jmp_2; // <-- loop end replacement
			
			// do 
			// ...
			// while (...)
			// 
			// :I:
			//  JMP .loop_body
			// .loop_start:
			//  ... <-- condition
			// .loop_body:
			//  JMP_IF_ZERO .loop_end
			//  ...
			//  JMP .loop_start 
			// 
			// :II:
			// .loop_start:
			//  ...
			// .loop_condition: <-- here goes continue
			//  ... <-- condition
			//  JMP_IF_ZERO .loop_end
			//  JMP .loop_start
			// .loop_end:
		
			// :II:
			int loop_start = relative_address(bytemap);
			push_address(BREAK_PLACEMENT_LOOP, loop_start, &jmp_1, &jmp_2);
			VISIT(n->right);
			
			int loop_condition = relative_address(bytemap);
			VISIT(n->left); // <-- Expressions only, no break/continue/return are expected.
			
			int loop_end = 13;
			push_byte(bytemap, ck_bytecodes::JMP_IF_ZERO);
			jmp_2.push_back(absolute_address(bytemap));
			push(bytemap, sizeof(int), &loop_end);
			
			push_byte(bytemap, ck_bytecodes::JMP);
			push(bytemap, sizeof(int), &loop_start);
			
			loop_end = relative_address(bytemap);
			pop_address(bytemap, loop_condition, loop_end);
		
			/*
			// :I:
			int loop_body = relative_address(bytemap);
			push_byte(bytemap, ck_bytecodes::JMP);
			int loop_body_jmp_addr = relative_address(bytemap);
			push(bytemap, sizeof(int), &loop_body);
			
			int loop_start = relative_address(bytemap);
			
			VISIT(n->left);
			push_byte(bytemap, ck_bytecodes::JMP_IF_ZERO);
			
			// Expect loop end to be inserted later
			jmp_2.push_back(relative_address(bytemap));
			push(bytemap, sizeof(int), &loop_start);
			
			// Save JMP .loop_block
			loop_body = relative_address(bytemap);
			for (int i = 0; i < sizeof(int); ++i) 
				bytemap[i + loop_body_jmp_addr] = ((unsigned char*) &loop_body)[i];
			
			
			// Start trackiing all placement templates
			push_address(BREAK_PLACEMENT_LOOP, loop_start, &jmp_1, &jmp_2);
			
			VISIT(n->right);
			push_byte(bytemap, ck_bytecodes::JMP);
			push(bytemap, sizeof(int), &loop_start);
			
			pop_address(bytemap, loop_start, relative_address(bytemap));
			*/
			
			break;
		}
		
		case IF: {
			if (n->left->next->next->type != EMPTY) {
				// if ...
				// ...
				// else
				// ...
				// 
				// ... <-- condition
				// JMP_IF_ZERO .else_node
				// ...
				// JMP .end
				// .else_node:
				// ...
				// .end:
				
				VISIT(n->left);
				
				int else_node = 13;
				push_byte(bytemap, ck_bytecodes::JMP_IF_ZERO);
				int else_node_jump = absolute_address(bytemap);
				push(bytemap, sizeof(int), &else_node);
				
				VISIT(n->left->next);
				
				int end = 13;
				push_byte(bytemap, ck_bytecodes::JMP);
				int end_jump = absolute_address(bytemap);
				push(bytemap, sizeof(int), &end);
				
				else_node = relative_address(bytemap);
				for (int i = 0; i < sizeof(int); ++i) 
					bytemap[i + else_node_jump] = ((unsigned char*) &else_node)[i];
				
				VISIT(n->left->next->next);
				
				end = relative_address(bytemap);
				for (int i = 0; i < sizeof(int); ++i) 
					bytemap[i + end_jump] = ((unsigned char*) &end)[i];
			} else {
				// if ...
				// ...
				// 
				// ... <-- condition
				// JMP_IF_ZERO .end
				// ...
				// .end:
				
				VISIT(n->left);
				
				int end = 13;
				push_byte(bytemap, ck_bytecodes::JMP_IF_ZERO);
				int end_jump = absolute_address(bytemap);
				push(bytemap, sizeof(int), &end);
				
				VISIT(n->left->next);
				
				end = relative_address(bytemap);
				for (int i = 0; i < sizeof(int); ++i) 
					bytemap[i + end_jump] = ((unsigned char*) &end)[i];
			}
			
			break;
			
			
		}
	
		case FOR: {
			// XXX: Solve for all types of for structures
			
			// for (var i = 10; i < 10; ++i) statement
			// 
			// [outer scope]
			//  \
			//   [scope with i] <-- statement is executed in this scope
			
			// for (var i = 10; i < 10; ++i) block
			// 
			// [outer scope]
			//  \
			//   [scope with i] 
			//    \
			//     [scope of the block] <-- block is executed in this scope
			
			vector<int> jmp_1; // <-- loop start replacement
			vector<int> jmp_2; // <-- loop end replacement
			
			// for ($1; $2; $3)
			// ...
			// 
			// $1 - simple statement
			// $2 - expression
			// $3 - expression => VSTACK_POP
			//
			// :$$$:
			//
			//  $1
			// .loop_condition:
			//  $2
			//  JMP_IF_ZERO .end
			//  JMP .loop_block
			// .loop_increment:
			//  $3
			//  JMP .loop_condition
			// .loop_block:
			//  ...
			// JMP .loop_increment
			// .end:
			
			int loop_start = relative_address(bytemap);
			
			// Placement for loop
			push_address(BREAK_PLACEMENT_LOOP, loop_start, &jmp_1, &jmp_2);
			
			// Placement for block (scope)
			push_byte(bytemap, ck_bytecodes::VSTATE_PUSH_SCOPE);
			push_address(BREAK_PLACEMENT_BLOCK, 0, nullptr, nullptr);
			
			if (n->left->type != EMPTY)
				VISIT(n->left);
			else
				push_byte(bytemap, ck_bytecodes::NOP);
			
			int loop_condition = relative_address(bytemap);
			
			if (n->left->next->type != EMPTY)
				VISIT(n->left->next);
			else {
				push_byte(bytemap, ck_bytecodes::PUSH_CONST_BOOLEAN);
				bool TrUe = 1;
				push(bytemap, 1, &TrUe);
			}
				
			
			int end = 13;
			push_byte(bytemap, ck_bytecodes::JMP_IF_ZERO);
			int end_addr = absolute_address(bytemap);
			jmp_2.push_back(end_addr);
			push(bytemap, sizeof(int), &end);
			
			int loop_block = 13;
			push_byte(bytemap, ck_bytecodes::JMP);
			int loop_block_jump = absolute_address(bytemap);
			push(bytemap, sizeof(int), &loop_block);
			
			int loop_increment = relative_address(bytemap);
			
			if (n->left->next->next->type != EMPTY) {
				VISIT(n->left->next->next);
				
				// Pop expression value
				push_byte(bytemap, ck_bytecodes::VSTACK_POP);
			} else 
				push_byte(bytemap, ck_bytecodes::NOP);
			
			push_byte(bytemap, ck_bytecodes::JMP);
			push(bytemap, sizeof(int), &loop_condition);
			
			loop_block = relative_address(bytemap);
			for (int i = 0; i < sizeof(int); ++i) 
				bytemap[i + loop_block_jump] = ((unsigned char*) &loop_block)[i];
			
			/* if (n->left->next->next->next->type == BLOCK) {
				ASTNode* t = n->left->next->next->next->left;
				while (t) {
					VISIT(t);
					t = t->next;
				}
			} else */
			
			VISIT(n->left->next->next->next);
			
			push_byte(bytemap, ck_bytecodes::JMP);
			push(bytemap, sizeof(int), &loop_increment);
			
			end = relative_address(bytemap);			
			
			// Pop block
			pop_address(bytemap, 0, 0);
			push_byte(bytemap, ck_bytecodes::VSTATE_POP_SCOPE);
			
			// Pop loop
			pop_address(bytemap, loop_condition, end);
			
			
			// for ($1; true; $3)
			// ...
			// 
			// :$.$:
			//
			//  $1
			//  JMP_IF_ZERO .end
			//  JMP .loop_block
			// .loop_increment:
			//  $3
			//  JMP .loop_condition
			// .loop_block:
			//  ...
			// .end:
			
			// ...
			// XXX: FINISH HIM
			
			break;
		}
		
		case CONDITION: {
			// condition ? exp1 : exp2
			
			//  $condidion
			//  JMP_IF_ZERO .else_node
			//  $if_node
			//  JMP .end
			// .else_node:
			//  $else_node
			// .end:
			
			VISIT(n->left);
			
			int else_node = 13;
			push_byte(bytemap, ck_bytecodes::JMP_IF_ZERO);
			int else_node_jump = absolute_address(bytemap);
			push(bytemap, sizeof(int), &else_node);
			
			VISIT(n->left->next);
			
			int end = 13;
			push_byte(bytemap, ck_bytecodes::JMP);
			int end_jump = absolute_address(bytemap);
			push(bytemap, sizeof(int), &end);
			
			else_node = relative_address(bytemap);
			for (int i = 0; i < sizeof(int); ++i) 
				bytemap[i + else_node_jump] = ((unsigned char*) &else_node)[i];
			
			VISIT(n->left->next->next);
			
			end = relative_address(bytemap);
			for (int i = 0; i < sizeof(int); ++i) 
				bytemap[i + end_jump] = ((unsigned char*) &end)[i];
			
			break;
		}
		
		case TRY: {
			// VSTATE_PUSH_TRY [type] [try_node] [catch_node] [catch_name]
			// [type]: (unsigned char)
			//  TRY_NO_CATCH -> try {}              -> VSTATE_PUSH_TRY [type] [try_node] [exit]
			//  TRY_NO_ARG   -> try {} catch {}     -> VSTATE_PUSH_TRY [type] [try_node] [catch_node]
			//  TRY_WITH_ARG -> try {} catch (e) {} -> VSTATE_PUSH_TRY [type] [try_node] [catch_node] [catch_name] --> creates new scope
			
			if (n->right->type == EMPTY) {
				// try {
				//  ...
				// }
				
				//  VSTATE_PUSH_TRY [TRY_NO_CATCH] [try_node] [exit]
				//  ...
				//  VSTATE_POP_TRY
				// .exit:      <-- jumped due the exception
				
				push_byte(bytemap, ck_bytecodes::VSTATE_PUSH_TRY);
				
				unsigned char type = ck_bytecodes::TRY_NO_CATCH;
				push(bytemap, sizeof(unsigned char), &type);
				
				int try_node = 13;
				int try_node_jump = absolute_address(bytemap);
				push(bytemap, sizeof(int), &try_node);
				
				int end = 13;
				int end_jump = absolute_address(bytemap);
				push(bytemap, sizeof(int), &end);
				
				try_node = relative_address(bytemap);
				for (int i = 0; i < sizeof(int); ++i) 
					bytemap[i + try_node_jump] = ((unsigned char*) &try_node)[i];
				
				VISIT(n->left);
				
				push_byte(bytemap, ck_bytecodes::VSTATE_POP_TRY);
				
				end = relative_address(bytemap);
				for (int i = 0; i < sizeof(int); ++i) 
					bytemap[i + end_jump] = ((unsigned char*) &end)[i];
			} else if (n->objectlist == nullptr) {
				// try {
				//  ...
				// } catch {}
				
				//  VSTATE_PUSH_TRY [TRY_NO_ARG] [try_node] [catch_node]
				//  ...
				//  VSTATE_POP_TRY
				// 	JMP .end
				// .catch_node:
				//  ...
				// .end:
				
				push_byte(bytemap, ck_bytecodes::VSTATE_PUSH_TRY);
				
				unsigned char type = ck_bytecodes::TRY_NO_ARG;
				push(bytemap, sizeof(unsigned char), &type);
				
				int try_node = 13;
				int try_node_jump = absolute_address(bytemap);
				push(bytemap, sizeof(int), &try_node);
				
				int catch_node = 13;
				int catch_node_jump = absolute_address(bytemap);
				push(bytemap, sizeof(int), &catch_node);
				
				try_node = relative_address(bytemap);
				for (int i = 0; i < sizeof(int); ++i) 
					bytemap[i + try_node_jump] = ((unsigned char*) &try_node)[i];
				
				VISIT(n->left);
				
				push_byte(bytemap, ck_bytecodes::VSTATE_POP_TRY);
				
				int end = 13;
				push_byte(bytemap, ck_bytecodes::JMP);
				int end_jump = absolute_address(bytemap);
				push(bytemap, sizeof(int), &end);
				
				catch_node = relative_address(bytemap);
				for (int i = 0; i < sizeof(int); ++i) 
					bytemap[i + catch_node_jump] = ((unsigned char*) &catch_node)[i];
				
				VISIT(n->right);
				
				end = relative_address(bytemap);
				for (int i = 0; i < sizeof(int); ++i) 
					bytemap[i + end_jump] = ((unsigned char*) &end)[i];
			} else {
				// try {
				//  ...
				// } catch (e) {}
				
				//  VSTATE_PUSH_TRY [TRY_WITH_ARG] [try_node] [catch_node] [catch_name]
				//  ...
				//  VSTATE_POP_TRY
				// 	JMP .end
				// .catch_node:
				//  ...
				//  VSTATE_POP_SCOPE
				// .end:
				
				push_byte(bytemap, ck_bytecodes::VSTATE_PUSH_TRY);
				
				unsigned char type = ck_bytecodes::TRY_WITH_ARG;
				push(bytemap, sizeof(unsigned char), &type);
				
				int try_node = 13;
				int try_node_jump = absolute_address(bytemap);
				push(bytemap, sizeof(int), &try_node);
				
				int catch_node = 13;
				int catch_node_jump = absolute_address(bytemap);
				push(bytemap, sizeof(int), &catch_node);
				
				wstring& s = *(wstring*) n->objectlist->object;
				int size = s.size();
				
				push(bytemap, sizeof(int), &size);
				
				for (int i = 0; i < size; ++i) {
					wchar_t c = s[i];
					push(bytemap, sizeof(wchar_t), &c);
				}
				
				try_node = relative_address(bytemap);
				for (int i = 0; i < sizeof(int); ++i) 
					bytemap[i + try_node_jump] = ((unsigned char*) &try_node)[i];
				
				VISIT(n->left);
				
				push_byte(bytemap, ck_bytecodes::VSTATE_POP_TRY);
				
				int end = 13;
				push_byte(bytemap, ck_bytecodes::JMP);
				int end_jump = absolute_address(bytemap);
				push(bytemap, sizeof(int), &end);
				
				catch_node = relative_address(bytemap);
				for (int i = 0; i < sizeof(int); ++i) 
					bytemap[i + catch_node_jump] = ((unsigned char*) &catch_node)[i];
				
				if (n->right->type != BLOCK)
					VISIT(n->right);
				else {					
					ASTNode* t = n->right->left;
					while (t) {
						VISIT(t);
						t = t->next;
					}
				}
				
				push_byte(bytemap, ck_bytecodes::VSTATE_POP_SCOPE);
				
				end = relative_address(bytemap);
				for (int i = 0; i < sizeof(int); ++i) 
					bytemap[i + end_jump] = ((unsigned char*) &end)[i];
			}
			
			break;
		}
		
		case THIS: {
			push_byte(bytemap, ck_bytecodes::PUSH_THIS);
			break;
		}
	
		case IN: {
			// CONTAINS_KEY [name]
			
			VISIT(n->left);
			
			push_byte(bytemap, ck_bytecodes::CONTAINS_KEY);
			
			wstring& s = *(wstring*) n->objectlist->object;
			int size = s.size();
			
			push(bytemap, sizeof(int), &size);
			
			for (int i = 0; i < size; ++i) {
				wchar_t c = s[i];
				push(bytemap, sizeof(wchar_t), &c);
			}
		}
	}
};

void ck_translator::translate(ck_bytecode& bytecode, ASTNode* n) {
	translate(bytecode.bytemap, bytecode.lineno_table, n);
};

void ck_translator::translate(vector<unsigned char>& bytemap, vector<int>& lineno_table, ASTNode* n) {
	// lineno_table - Table of Line Numbers
	// Provides range of commands mapped to a single line number
	// [lineno, start_cmd]
	
	// bytemap - the resulting bytemap
	
	if (!(n && n->type != TERR))
		return;
	
	visit(bytemap, lineno_table, n);
	push_byte(bytemap, ck_bytecodes::BCEND);
	
	last_lineno = -1;
	lineno_table.push_back(last_lineno);
	last_lineno_addr = bytemap.size();
	lineno_table.push_back(last_lineno_addr);
};

void ck_translator::translate_function(ck_bytecode& bytecode, ASTNode* n) {
	translate_function(bytecode.bytemap, bytecode.lineno_table, n);
};

void ck_translator::translate_function(vector<unsigned char>& bytemap, vector<int>& lineno_table, ASTNode* n) {
	// lineno_table - Table of Line Numbers
	// Provides range of commands mapped to a single line number
	// [lineno, start_cmd]
	
	// bytemap - the resulting bytemap
	
	if (!(n && n->type != TERR))
		return;
	
	// Translate bytecode inside "fake" function body
	push_address(BREAK_PLACEMENT_FUNCTION, 0, nullptr, nullptr);
	visit(bytemap, lineno_table, n);
	pop_address(bytemap, 0, 0);
	
	last_lineno = -1;
	lineno_table.push_back(last_lineno);
	last_lineno_addr = bytemap.size();
	lineno_table.push_back(last_lineno_addr);
};

bool read(vector<unsigned char>& bytemap, int& index, int size, void* p) {
	if (index + size > bytemap.size())
		return 0;
	
	for (int i = 0; i < size; ++i) 
		((unsigned char*) p)[i] = bytemap[index + i];
	
	index += size;
	
	return 1;
};

void ck_translator::print(vector<unsigned char>& bytemap, int off, int offset, int length) {
	int int_offset = bytemap.size() == 0 ? 1 : 0;
	int num = bytemap.size();
	
	while (num) {
		++int_offset;
		num /= 10;
	}
	
	for (int k = (offset == -1) ? 0 : offset; k < ((length == -1) ? bytemap.size() : offset + length);) {
		wcout << '[' << setw(int_offset) << k-(offset == -1 ? 0 : offset) << setw(-1) << "] ";
		
		for (int i = 0; i < off; ++i)
			wcout << (wchar_t) U'>';
		
		switch(bytemap[k++]) {
			case ck_bytecodes::LINENO: {
				int lineno; 
				read(bytemap, k, sizeof(int), &lineno);
				wcout << "LINENO: " << lineno << endl;
				break;
			}
			
			case ck_bytecodes::NOP: {
				wcout << "> NOP" << endl;
				break;
			}
			
			case ck_bytecodes::PUSH_CONST_INT: {
				int64_t i; 
				read(bytemap, k, sizeof(int64_t), &i);
				wcout << "> PUSH_CONST[int]: " << i << endl;
				break;
			}
			
			case ck_bytecodes::PUSH_CONST_DOUBLE: {
				double i; 
				read(bytemap, k, sizeof(double), &i);
				wcout << "> PUSH_CONST[double]: " << i << endl;
				break;
			}
			
			case ck_bytecodes::PUSH_CONST_BOOLEAN: {
				bool i; 
				read(bytemap, k, sizeof(bool), &i);
				wcout << "> PUSH_CONST[boolean]: " << i << endl;
				break;
			}
			
			case ck_bytecodes::PUSH_CONST_NULL: {
				wcout << "> PUSH_CONST: null" << endl;
				break;
			}
			
			case ck_bytecodes::PUSH_CONST_UNDEFINED: {
				wcout << "> PUSH_CONST: undefined" << endl;
				break;
			}
			
			case ck_bytecodes::PUSH_CONST_STRING: {
				int size;
				read(bytemap, k, sizeof(int), &size);
				wchar_t cstr[size+1];
				read(bytemap, k, sizeof(wchar_t) * size, cstr);
				cstr[size] = 0;
				
				wcout << "> PUSH_CONST[string]: \"" << cstr << '"' << endl;
				break;
			}
			
			case ck_bytecodes::LOAD_VAR: {
				int size;
				read(bytemap, k, sizeof(int), &size);
				wchar_t cstr[size+1];
				read(bytemap, k, sizeof(wchar_t) * size, cstr);
				cstr[size] = 0;
				
				wcout << "> LOAD_VAR: " << cstr << endl;
				break;
			}
			
			case ck_bytecodes::VSTACK_POP: {
				wcout << "> VSTACK_POP" << endl;
				break;
			}
			
			case ck_bytecodes::PUSH_CONST_ARRAY: {
				int size; 
				read(bytemap, k, sizeof(int), &size);
				wcout << "> PUSH_CONST[array]: [" << size << ']' << endl;
				break;
			}
			
			case ck_bytecodes::PUSH_CONST_OBJECT: {
				int size; 
				read(bytemap, k, sizeof(int), &size);
				wcout << "> PUSH_CONST[object]: {";
				
				for (int i = 0; i < size; ++i) {
					int ssize;
					read(bytemap, k, sizeof(int), &ssize);
					wchar_t cstr[ssize+1];
					read(bytemap, k, sizeof(wchar_t) * ssize, cstr);
					cstr[ssize] = 0;
					wcout << cstr;
					if (i != size-1)
						wcout << ", ";
				}
				wcout << '}' << endl;
				break;
			}
			
			case ck_bytecodes::DEFINE_VAR: {
				int amount; 
				read(bytemap, k, sizeof(int), &amount);
				wcout << "> DEFINE_VAR: ";
				
				for (int i = 0; i < amount; ++i) {
					int ssize = 0;
					read(bytemap, k, sizeof(int), &ssize);
					
					wchar_t cstr[ssize+1];
					read(bytemap, k, sizeof(wchar_t) * ssize, cstr);
					cstr[ssize] = 0;
					wcout << cstr;
					
					unsigned char ops = 0;
					read(bytemap, k, sizeof(unsigned char), &ops);
					
					if ((ops & 0b1000) == 0)
						wcout << " = [undefined]";
					
					if (i != amount-1)
						wcout << ", ";
				}
				
				wcout << endl;
				break;
			}
		
			case ck_bytecodes::VSTACK_DUP: {
				wcout << "> VSTACK_DUP" << endl;
				break;
			}
			
			case ck_bytecodes::CALL: {
				int i; 
				read(bytemap, k, sizeof(int), &i);
				wcout << "> CALL [" << i << ']' << endl;
				break;
			}
			
			case ck_bytecodes::CALL_MEMBER: {
				int i; 
				read(bytemap, k, sizeof(int), &i);
				wcout << "> CALL_MEMBER [" << i << ']' << endl;
				break;
			}
			
			case ck_bytecodes::CALL_NAME: {
				int i; 
				read(bytemap, k, sizeof(int), &i);
				
				int size;
				read(bytemap, k, sizeof(int), &size);
				wchar_t cstr[size+1];
				read(bytemap, k, sizeof(wchar_t) * size, cstr);
				cstr[size] = 0;
				
				wcout << "> CALL_NAME [" << i << "] [" << cstr << "]" << endl;
				break;
			}
			
			case ck_bytecodes::CALL_FIELD: {
				int i; 
				read(bytemap, k, sizeof(int), &i);
				
				int size;
				read(bytemap, k, sizeof(int), &size);
				wchar_t cstr[size+1];
				read(bytemap, k, sizeof(wchar_t) * size, cstr);
				cstr[size] = 0;
				
				wcout << "> CALL_FIELD [" << i << "] [" << cstr << "]" << endl;
				break;
			}
			
			case ck_bytecodes::CONTAINS_KEY: {
				int size;
				read(bytemap, k, sizeof(int), &size);
				wchar_t cstr[size+1];
				read(bytemap, k, sizeof(wchar_t) * size, cstr);
				cstr[size] = 0;
				
				wcout << "> CONTAINS_KEY [" << cstr << "]" << endl;
				break;
			}
			
			case ck_bytecodes::OPERATOR: {
				unsigned char i; 
				read(bytemap, k, sizeof(unsigned char), &i);
				wcout << "> OPERATOR [" << (int) i << ']' << endl;
				break;
			}
			
			case ck_bytecodes::STORE_VAR: {
				int size;
				read(bytemap, k, sizeof(int), &size);
				wchar_t cstr[size+1];
				read(bytemap, k, sizeof(wchar_t) * size, cstr);
				cstr[size] = 0;
				
				wcout << "> STORE_VAR: " << cstr << endl;
				break;
			}
			
			case ck_bytecodes::STORE_FIELD: {
				int size;
				read(bytemap, k, sizeof(int), &size);
				wchar_t cstr[size+1];
				read(bytemap, k, sizeof(wchar_t) * size, cstr);
				cstr[size] = 0;
				
				wcout << "> STORE_FIELD: " << cstr << endl;
				break;
			}
			
			case ck_bytecodes::STORE_MEMBER: {
				wcout << "> STORE_MEMBER " << endl;
				break;
			}
			
			case ck_bytecodes::LOAD_FIELD: {
				int size;
				read(bytemap, k, sizeof(int), &size);
				wchar_t cstr[size+1];
				read(bytemap, k, sizeof(wchar_t) * size, cstr);
				cstr[size] = 0;
				
				wcout << "> LOAD_FIELD: " << cstr << endl;
				break;
			}
			
			case ck_bytecodes::LOAD_MEMBER: {
				wcout << "> LOAD_MEMBER " << endl;
				break;
			}
		
			case ck_bytecodes::UNARY_OPERATOR: {
				unsigned char i; 
				read(bytemap, k, sizeof(unsigned char), &i);
				wcout << "> UNARY_OPERATOR [" << (int) i << ']' << endl;
				break;
			}
			
			case ck_bytecodes::VSTACK_SWAP: {
				wcout << "> VSTACK_SWAP" << endl;
				break;
			}
			
			case ck_bytecodes::VSTACK_SWAP1: {
				wcout << "> VSTACK_SWAP1" << endl;
				break;
			}
			
			case ck_bytecodes::VSTACK_SWAP2: {
				wcout << "> VSTACK_SWAP2" << endl;
				break;
			}
			
			case ck_bytecodes::VSTATE_PUSH_SCOPE: {
				wcout << "> VSTATE_PUSH_SCOPE" << endl;
				break;
			}
			
			case ck_bytecodes::VSTATE_POP_SCOPE: {
				wcout << "> VSTATE_POP_SCOPE" << endl;
				break;
			}
			
			case ck_bytecodes::JMP_IF_ZERO: {
				int i; 
				read(bytemap, k, sizeof(int), &i);
				wcout << "> JMP_IF_ZERO [" << i << ']' << endl;
				break;
			}
			
			case ck_bytecodes::JMP_IF_NOT_ZERO: {
				int i; 
				read(bytemap, k, sizeof(int), &i);
				wcout << "> JMP_IF_NOT_ZERO [" << i << ']' << endl;
				break;
			}
			
			case ck_bytecodes::JMP: {
				int i; 
				read(bytemap, k, sizeof(int), &i);
				wcout << "> JMP [" << i << ']' << endl;
				break;
			}
			
			case ck_bytecodes::BCEND: {
				wcout << "> BCEND" << endl;
				break;
			}
			
			case ck_bytecodes::THROW_NOARG: {
				wcout << "> THROW_NOARG" << endl;
				break;
			}
			
			case ck_bytecodes::THROW: {
				wcout << "> THROW" << endl;
				break;
			}
			
			case ck_bytecodes::THROW_STRING: {
				int size;
				read(bytemap, k, sizeof(int), &size);
				wchar_t cstr[size+1];
				read(bytemap, k, sizeof(wchar_t) * size, cstr);
				cstr[size] = 0;
				
				wcout << "> THROW_STRING: \"" << cstr << '"' << endl;
				break;
			}
			
			case ck_bytecodes::VSTATE_POP_SCOPES: {
				int i; 
				read(bytemap, k, sizeof(int), &i);
				wcout << "> VSTATE_POP_SCOPES [" << i << ']' << endl;
				break;
			}
			
			case ck_bytecodes::RETURN_VALUE: {
				wcout << "> RETURN_VALUE" << endl;
				break;
			}
			
			case ck_bytecodes::PUSH_CONST_FUNCTION: {
				int argc; 
				read(bytemap, k, sizeof(int), &argc);
				wcout << "> PUSH_CONST_FUNCTION (" << argc << ") (";
				
				for (int i = 0; i < argc; ++i) {
					int ssize = 0;
					read(bytemap, k, sizeof(int), &ssize);
					
					wchar_t cstr[ssize+1];
					read(bytemap, k, sizeof(wchar_t) * ssize, cstr);
					cstr[ssize] = 0;
					wcout << cstr;
					
					if (i != argc-1)
						wcout << ", ";
				}
				
				int sizeof_block; 
				read(bytemap, k, sizeof(int), &sizeof_block);
				
				wcout << ") [" << sizeof_block << "]:" << endl;
				
				print(bytemap, off + 1, k, sizeof_block);
				k += sizeof_block;
				
				break;
			}
		
			case ck_bytecodes::VSTATE_POP_TRY: {
				wcout << "> VSTATE_POP_TRY" << endl;
				break;
			}

			case ck_bytecodes::VSTATE_PUSH_TRY: {
				unsigned char type;
				read(bytemap, k, sizeof(unsigned char), &type);
				
				if (type == ck_bytecodes::TRY_NO_CATCH) {
					int try_node;
					int exit;
					
					read(bytemap, k, sizeof(int), &try_node);
					read(bytemap, k, sizeof(int), &exit);
					
					wcout << "> VSTATE_PUSH_TRY [TRY_NO_CATCH] [" << try_node << "] [" << exit << ']' << endl;
				} else if (type == ck_bytecodes::TRY_NO_ARG) {
					int try_node;
					int catch_node;
					
					read(bytemap, k, sizeof(int), &try_node);
					read(bytemap, k, sizeof(int), &catch_node);
					
					wcout << "> VSTATE_PUSH_TRY [TRY_NO_ARG] [" << try_node << "] [" << catch_node << ']' << endl;
				} else {
					int try_node;
					int catch_node;
					int name_size;
					
					read(bytemap, k, sizeof(int), &try_node);
					read(bytemap, k, sizeof(int), &catch_node);
					read(bytemap, k, sizeof(int), &name_size);
										
					wchar_t cstr[name_size+1];
					read(bytemap, k, sizeof(wchar_t) * name_size, cstr);
					cstr[name_size] = 0;
					
					wcout << "> VSTATE_PUSH_TRY [TRY_WITH_ARG] (" << cstr << ") [" << try_node << "] [" << catch_node << ']' << endl;
				}
				
				break;
			}
		
			case ck_bytecodes::PUSH_THIS: {
				wcout << "> PUSH_THIS" << endl;
				break;
			}
		}
	}
};

void ck_translator::print_lineno_table(vector<int>& lineno_table) {
	for (int i = 0; i < lineno_table.size();) {
		int lineno = lineno_table[i++];
		int start  = lineno_table[i++];
		
		wcout << "lineno [" << lineno << "], start [" << start << ']' << endl;
	}
};


// Поздравляю, ты достиг дна

// ((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((/*,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,*/((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((#((###########%
// (((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((/*,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,*/((((((((((((((((((((((((((((((((((((((((((((((((((((((((((###########%
// (((((((((((((((((((((((((((/((((((((/((((((((((((((((((((//,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,*((((((((((((((((((((((((((((((((((((((((((((((((((((((#(#########
// ((((((((((((((((((((((((((((((((((((//((/((((((((((((/*,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,*((((((((((((((((((((((((((((((((((((((((((((((((((##########
// (((((((((((((((/(((((((((((((((/((/(/(////(((((((/*,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,/#%%,,,,,,,,,,,**,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,/(((((((((((((((((((((((((((((((((((((((((((((###########
// ((((((((((((((((((((((((((((((((((///(//((((//*,,,,,,,,,,,,,,,,,,,,,,,,,,,,*(%%,,,%&%/,,,,*&&%,,,,,,,,,,/&&&*&&&,,,,**,,,,,,,,,,,,,,,,,,,,,,,,,,/((((((((((((((((((((((((((((((((((((#(#((#((########
// (((((((((((((((((((((((((((/(((((((((/((/(/,,,,,,,,,,,,,,,,,,,,,,,,,,,,,/*#%&%,,,,#&&&%%*&&&*,,,,,,,,,#&&%/&&&,,,%&&/%&%*,,,,,,,,,,,,,,,,,,,,,,,,/((((((((((((((((((((((((((((((((((#(#((#(########
// (((((((((((((((((((((((((((/(//(((((////,,,,,,,,,,,,,,,,,,,,,,*,,,,,,,,,,,,&%%*,,,*&&&,&&&*%&,,,,,,,,,&&&(#&&%,,*&&&*&&&*,,#&%/,,,,,,,,,,,,,,,,,,,,,/((((((((((((((((((((((((((((((((((############
// ((((((((((((((((((((((((((((((///(///,,,,,,,,,,,,,,,,,,,,,,,#%%%,,,,,,,,,,,/%&%,,,,&&&/%&&(/&&%,,,,,,,,#&&&*&&&(,,%&&%#&&%,,(&&%*%&%,,,,,,,,,,,,,,,,,,,,,/(((((((((((((((((((((((((((((#((#(#########
// (((((((((((((((((((((((((((((((((/*,,,,,,,,,,,,,,,,,,,*#%%*,,/%%#,,,,,,,,,,,%%%*,,,*,,,,,,,,,,,,,,,,,,*%&%,,*&&&,,%&&*&&&*,*&&&*,*(%,,,%%*,,,,,,,,,,,,,,,,,*(((((((((((((((((((((((((((#(####((######
// (((((((((((((((((((((/(((((/////,,,,,,,,,,,,,,,,,,,/%%//%%%/,,,%%(,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,***,,&&&/(/*,,,,,*&&&%#,,,,,,,,,,,,,,,,,/((((((((((((((((((((((((#(#((#########
// ((((((((((((((((((((((((((////,,,,,,,,,,,,,,,,,*%%(*%%%#,%%%#,,*%%%,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,*#&&&&,,,,/&&&*,,,,(*,,,,,,,,,,,,,,,*(((((((((((((((((((((((#(#((########
// (((((((((((((((((((((/(((//*,,,,,,,,,,,,,,,,,,,,%%%%#%%%%,#%%(,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,/&&&*,,,,&&&&,,,,,,,,,,,,,,,,*(((((((((((((((((((((##(((########
// (((((((((((((((((((((//(/*,,,,,,,,,,,,,,,,/%%*,,,#%%%*(%%%%/,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,(&&*,,,*&&&%(&&&(,,,,,,,,,,,,,,*(((((((((((((((((((((((#########
// ((((((((((((((((((/((//*,,,,,,,,,,,,,,,,##*%%%%*,,/%%%(,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,%&&&&&&&&&,,,*,,,,,,,,,,,,,*((((((((((((((((((((##(#######
// (((((((((((((((((((///,,,,,,,,,,,,,,,,,%%%%,(%%%%,,,#*,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,**/##%%##(/***,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,*%&/(&&&(,,*&&&*,,,,,,,,,,,,,/(((((((((((((((((((#########
// (((((((((((((((((((/,,,,,,,,,,,,,,,*,,,,(%%%%%#%%%*,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,*(&&&&&&&&&&&&&&&&&&&&&%(*,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,*&&&*,*&&&&(*&/,,,,,,,,,,,,,((((((((((((((((((((#######
// (((((((((((((((((/*,,,,,,,,,,,,,,*%%%#*,,,,/%%#,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,*%&&&&&%%&&&&&&&&&&&&&&&&&&&&&&/,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,#&&&%,,,,,,,,,,,,,,,,*,,*((((((((((((((((#########
// ((((((((((((((((/,,,,,,,,,,,,,,,#%,#%%%%*,,,%/,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,(&&&&&%%##%&&&&&&&&&&&&&&&&&&&&&&&&%/,,,,,,,,,,,,,,,,,,,,,,,,,,,,,/&&*,,,,,,,,,,,,,,,,,,,*,*(((((((((((((((#(#######
// (((((((((((((((*,,,,,,,,,,,,,,*#%%%%,/%%%%(,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,*&&&&&&&&%%#%%%%%%%%&%&&&&&&&&&&&&&&&&&&&/,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,**(((((((((((#((((######
// (((((((((((((/.,,,,,,,,,,,,,,,,,,(%%%%(,#*,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,#%&&&&&&&&&%%%%%%%%%%%%%%&&&&&&&&&&&&&&&&&&&&&/,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,*(*,,,,,,,*,**(((((((((((((((######
// ((((((((((((/,,,,,,,,,,,,,,#%#,,,,,*%%%,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,**/&&&&&&&&%%%%%%%%%%%%%&&&&&&&&&&&&&&&&&&&&&&&&%*,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,/&&&&**,,,*,,**,,/(((((((((##(#######
// (((((((((((*,,,,,,,,,,,,,(#%%%%/#*,,*(,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,****&&&&&&%&&%%%%%%%%%&&&&&&&&&&&&&&&&&&&&&&@&&&&&/,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,#/&&&&&/***,,,,,,***/((((((((((########
// ((((((((((,,,,,,,,,,,,,,%%%#*%%%%%%*,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,*,****&&%%%&&&%%%%%&&&&&%#///////////#&&&&&&&&@&&&&&(,,,,,,,,,,,,,,,,,,,,,,,,,,,,,#&&&&%*%&&&,****,**,,,/(((((##(((#######
// (((((((((,,,,,,,,,,,,,,,*#%%%%#**%#,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,*******&&&&&&&&&&&&(///////////////////(&&&&&@@&&@&&&(,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,/&&&&&&*,,,,**,,,,,,*,*(((((((((#(#####
// ((((((((,,,,,,,,,,,,,,,,,,,*%%%%%/,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,*******&&&&&&%//////////////////////////&&&&&@&@@@&&&/,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,(&&&/**,**&&/***,,***,**#(((((((#######
// (((((((,,,,,,,,,,,,,(%#/*,,,,,*#,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,**,*****/(**/////////////////////////////%&&&&&@&@&&&&/,,,,,,,,,,,,,,,,,,,*,,,,,,,,*,/#***#&&@&&%**,,,,,,****((((((########
// ((((((.,,,,,,,,,,,,(%#%%%%%%%#/,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,**,,*********////(((###((///*****/*/////////&&&&&@&@@@&&&*,,,,,,,,,,,,,,,,,,,,,,,,*,,,,,,*/&&&&&/%&&(*,,,******/#(((##(###(##
// (((((,,,,,,,,,,,,,*%%%%/,##%%(,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,***////*****//(##/////(((#/*********////////&&&&&@@@@@@&&%,,,,,,,,,,,,,,,,,,,,,,,*,*,,,*,,,**/&&&&&&/*,,*,******(((#(((##(###
// ((((/,,,,,,,,,,,,,,,,#%%%%(,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,**////(((****/////(#######%**************///&&&&&&&@@@@@&&%,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,*&&**,,,***********###(((#(###(
// (((/,,,,,,,,,,,,,**,,,,,#%%/,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,*(,,,/#%%******#,,*/#&&/*/#/**************/&&&&&&&@&@@@&&&&/,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,*****/(%&/**********##((#######
// (((,,,,,,,,,,,,,%%%%%%,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,**,,/%&&%*,****/,,,(&&(///(/*************/&&%&&&&&&&&@@@@&&&,,,,,,,,,,,,,,,,,,,*,,,,,,,**,*,,,*&&&&@&&&&***********##(#(#####
// ((*,,,,,,,,,,,,#/,/#%%,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,******(((*,/****/*****//(((*******************//(&&@@@&@@@&&&*,,,,,,,,,*,,,,,,,,,*,*,,,,,***,,*//*&*&&&&&*,,*******(#((#((###
// (/,,,,,,,,,,,,(%%%%%%(,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,***/**//(***/(*****(###%%(/********************////&&@@@@@@@&&(,,,,,,,,,,,,,,,,,,,,,,,,,,,,***,**(&&&&&*,*,,,****,***(((###(##
// (,,,,,,,,,,,,,,*/%%%%%%%*,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,*****#(****/(((**************************///////////&@@@@@@@&&&**,,,,,,,,,,,,,*,,,,,*,*,*,***,*,**&%************,****(##(#(#(#
// /,,,,,,,,,,,,,,,,,,,,*((,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,*********//(((((*******************/*///(((#(*/////#&@@@@@@@@&&*,*,,,,,,,*,,,,,,,,,,,**,*,,**,*,*****%&&&**********####(###
// ,,,,,,,,,,,,*%%%%#/*****,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,**********/((((((((((**************/((((/(#(##***/////&@@@@@@@@@&%*,,,,,,,,,,,*,*,,,,**,,***,,*,,,,*%&/&%(*********,***(##((###
// ,,,,,,,,,,,,%%%%%%%%%%%%*,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,***,******/(((((((((((((((((((((((/////(((##%/*****////&@@@@@@@@@&,,,,,,*,,,,,,,,,,,*,***,***,**,*****,*,*******,******(######
// ,,,,,,,,,,,*%%#/*,**(%,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,*///((/(((/(((((((((((((((((((////////((((####(/*****/////%@@@@@@@@@@&&*,,,,,,,,,,*,,*,*,,***,,*,,*,**********,,************#######
// ,,,,,,,,,,,#%%%%%%%%%#,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,*///////((((((((((((((/////////////(#(/((####%(/*******/////#@@@@@@@@@@&&&,*,,,,,,*,,,,,,,*,******,*,*******,*****,***********(######
// ,,,,,,,,,,,,,,,,,*/%%%%,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,**///////(((((((((((/////////((((((((((######/*************///#@@@@@@@@@@@@*,,,,,,,,,,,*,,**,,****,*,,**,*,*,,*****************###(#(
// ,,,,,,,,,,,,,,,,,,,,,**,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,**//////((((((((((((###(((((((((#(#######(***************/**/*#&@@@@@@@@@@@@&**,,,*,,***,,***,,,****,****,,*****,**********,*****###(((
// ,,,,,,,,,,*%%%%*%%%%,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,///((#################################/*********************///&@@@@@@@@@@@@@&&*,,,,,,*,,,**,*,,**,***,****,**********************####(#
// ,,,,,,,,,,#%%%%%%%%%(,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,/((#########%########%##%%%#####(/********************/****///&@@@@@@@@@@@@@@**,,,,*,,*,,****,*,*******,***********************###((#
// ,,,,,,,,,,%%%#*%,#(%/,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,*/#####%%%#%%%%%%%%%%%%%#(/////*************************///#@@&@@@@@@@@@@@@@&******,****,**************************************(#####
// ,,,,,,,,,,#&%/,,*%&%*,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,*,,,,,,,,,***(#%%%%%%%%%%%%%((////////*************************////&&@@@@@@@@@@@@@@@&&****,,,*,,**,************************************(##(##
// ,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,******(#####((((((((((////////*/*********************/////@@@@@@@@@@@@@@@@@@************************************************(##(##
// ,,,,,,,,(,,***//(##/,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,*******///((((((((((////////////***************/*****//*//@@@@@@@@@@@@@@@@@@@&************************************************######
// ,,,,,,,,,,%&&&&&&&&&*,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,********/////(((((/(////////////***/*******/******///////#@@@@@@@@@@@@@@@@@@@@&***********************************************######
// ,,,,,,,,,,%(******/%(,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,**********/////////////////////////*****/*/******/*///////#@@@@@@@@@@@@@@@@@@@@&%**********************************************######
// ,,,,,,,,,,(&&&&&&&&,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,*************//////////////////////****/*/*/******////////%@@@@@@@@@@@@@@@@@@@@@&/*********************************************######
// ,,,,,,,,,,,(%#(//***,,,,,,,,,,,,,,,,,,,,,*,,,,,,,,,,,,,*,,,*,,,********************/////////////*/***************///*/////%@@@@@@@@@@@@@@@@@@@@@@&/********************************************######
// ,,,,,,,,,,,,,*,,***(%*,,,,,,,,,,,,,,,,,,,,,,,,,,,*,,,,,,,,,***,************************/////////***************/***///////&@@@@@@@@@@@@@@@@@@@@@@@&*******************************************(###(##
// ,,,,,,,,,,,%&&&&&&&&,,,,,,,,,,,,,,,,,,,,,,,,,,,,,*,*,,,,,*,,,***************************//////*************/**/*//*/////&@@@@@@@@@@@@@@@@@@@@@@@@&******************************************#######
// ,,,,,,,,,,,/&&&&%(****,,,,,,,,,,,,,,,,,,,,,,,*,**,,,,,,*,,****************************************/*****************/*////&@@@@@@@@@@@@@@@@@@@@@@@@@&*****************************************#######
// ,,,,,,,,,,,*%&&&&&&&/,,*,,,,,,*,,,,,,*,*,,**,*,,*,*,**,**,*,****************************************************/***////&@@@@@@@@@&&@@@@@@@@@@@@@@@&***************************************(#######
// ,,,,,,,,,,,*#&&&&&&%(**,,,,,,*,,,,,,,,,,,,,,*,****,,**,****,*********************************************************/**///@@@@@@@@@@@@@@@@@@@@@@@@@@@&**************************************######(#
// *,,,,,,,,,,,,**,,,,,*,,**,,**,,,,*,,,,*,,*,*,*,,**,**********,*******************************************************/*////&&@@@@@@@@@@@@@@@@@@@@@@@@@@%************************************/########
// (*,*,,,,,,*,,,,,,,,,,,,**,*,,,,*,****,,,,,*,,,*,*******************************************************************/****////&@@@@@@@@@@@@@@@@@@@@@@@@@@@/***********************************###(#####
// (**,,,,,,,*,*,,,***,*,*,,*,,**,,,,,**,,**,*,**,********,,*************************************************/************//////&@@@@@@@@@@@@@@@@@@@@@@@@@@&**********************************(#########
// ((*,,,,,,,,*,*******,,*,,*,**,*,,**,,*******************************************************************************/***/*////@@@@@@@@@@@@@@@@@@@@@@@@@@@&*********************************##########
// ((/**,,*,,,,,,**,*,*,*,**,,**,***,******,*,*****************************************************************************///////%@@@@@@@@@@@@@@@@@@@@@@@@@@&*******************************###########
// (((******,***,*************,***,,****,**********************************************************************************////////(%@@@@@@@@@@@@@@@@@@@@@@@@@@&/***************************/###########
// ((((*,**,,*,,*******,,,*,*,*,****,****,,**,**************************************************************************/****////////&@@@@@@@@@@@@@@@@@@@@@@@@@@@*************************############
// ((((/**,*,,,,,*************,*********************************************************************************************//////////#@@@@@@@@@@@@@@@@@@@@@@@@@@@@&&/*********************%############
// (((((/***,***********,,***,***,********,*******************************************************************************/**///////////@@@@@@@@@@@@@@@@@@@@@@@@@&&&@@&(******************%#############
// ((((((***********************************************************************************************************/*******/////////////%@@@@@@@@@@@@@@@@@@@@@@@@@&&@%&%///%************%##############
// (((((((***,******,***********************************************************************************************/********//////////////@@@@&&@@@@@@@@@@@@@@%(&@%///&((((#***********%###############
// ((((((((********,*,******************************************************************************************************////////////////(@@@@@@@@@@@@(///@///@@@(((%((((#(*********%################
// (((((((((****,*,**,****************************************************************************************************////////////////////(&&(/////@@#(((%((%@@@#########@@%******%%################
// ((((((((((/************************************************************************************************************//////////////(////(#///#&((&@@&(((####@@@####&#%@@@@#***%%%################
// ((((((((((((********************************************************************************************************/*////////(#/(///(/////((((#&@@@@@@###%###%@@%%%%@@@@@@@@@@@/&%##################
// (((((((((((#(********************************************************************************************************//(%(/////#//((((%%((&(####(&%##@@###%@%%%&@@@@@@@@@@@@@@@@&%%%#################
// ((((((((((((((**************************************************************************************************/##/**////(((/((//#((######(%###%%##%&@%%@@@@@@@@@@@@@@@@@@@@@@&%%###################
// (((((((((((((##/********************************************&***********************************************///////#/*/((((/(((((/%###%%###(#%%%(((#@@@@@@@@@@@@@@@@@@@@@&&%%####################
// ((((((((((((((###******************************************&&**************************************(%(///#*/(//(&(((#//#(###%####/%###%%%%%//(((((((((((((@@@@@@@@@@@@@@@@@@&%%%#####################
// ((((((((((((((((##(****************************************@&************************************///////(/*/#((((##(#//##&##&/(%&%(////((((((((((((((((@@@@@@@@@@@@@@@&&%%#######################
// ((((((((((((((((####**************************************&@&**************************%(//////**((((/((#%**%########%/%###%%%%(/////////////((((((((((((((#@@@@@@@@@@@@@&&%%########################
// ((#(((((((((##(#(((##(***********************************(@@&*******************#%/#*(/////((((/*%(((%#####/%###//####(&(////////////////////((((((((((((((((@@@@@@@&%@&&%%%%########################
// (((((((((((((#(((######(*********************************&&@%*************%(///%///(*/((((/((##(*%###&##&/%##%/*///***////////////////////(((((((((((((((((#@@@@@@@&&&%%%##%#######################
// (((((((((((((((((########(******************************(&&@%*************(///((((%***#(#######%*####%#%%%*******/**//*///////////////////////((((((((((((((##&@@@@@&&%%%%%%#########################
// ((((((((((((((((#((########(****************************@@@&%*************/((((((#%***%###%##&*/%%(************//*/*/*///////////////////(//(((((((((((((((##@@@&&&%%%%%%##########################
// ((((((((##((((((#####(########*************************(@@@@(**************####%###%**###%%%***********/********/*/*/////////////////////(/(((((((((((((((((&&&&&%%%%%#############################
// (((((((((((#(#####(#((##########/**********************&@@@@(**************%####%###%*************************/*//////////////////////////////(((((((((((((#&@&&&%%%%%%%%##%#########################
// (((((((((((((#(#((#######(#########*******************#@@@@@(**************##&*#****************************///////////////////////////////(/((((((((((%@&&&&%%%%%%%%%###%%%%######################
// ((((((#(((#(#(((#####################/****************&@@@@&/**************/*******************************//////////*////////////////////////((((((((#&&@&&&&%%%%%%%%#%%#%##%#%#####################
// (((((((##(((###((#(####(#(##############*************(@@@@@@/********************************************/*//////////////////////////////////(((((((&@@&&&&&%%%%%%%%%%%%#%%##%%%######%##############
// ##((#((#########(#######################%%%/*********@@@@@@@/****************************************/*/*//*/////////////////////////////////((((&@@@@&&&&%%%%%%%%%%%%%%#%%#%%%%%%%##################
// ((##((#####((#####((#####################%%%%%(*****(@@@@@@@**********************************/*/****//**/**///*////////////////////////////(#@@@@@@@&&&%%%%%%%%%%%%%%%%%%####################%#%#%%#
// #(##(#((#((#(#(((#########################%%%%%%%%#*@@@@@@@@*****************************************///*/*//////////////////////////////#&@@@@&@&&&&&%%%%%%%%%%%%%%%%%%%%%%####################%####
// (##(##((##(#(###############################%%%%%%%%&&&@@@@&***********************/*******************//////////////////////////////%&@@@@@&@@&&&&%%%%%%%%%%%%%%%%%%%%%%#%%###%####################%
// ((#(((#(#(#(((###(#########################%%%%%%%%%%%&&&&&&/************************************/******////////////////////////#&@@@@@@@@&&&&&&&%%%%%%%%%%%%%%%%%%%%%%%%##%%##%%####################
// ((#(((((##(##(#(#########################%#%%%%%%%%%%%%%&&&&&&&&%(*******************************//**/////////////////////%&@@@@@@@@@@&@@&&&&&%%%%%%%%%%%%%%%%%%%%%%%%%%##%%%##%####%###############%
// #((#((((##((##(#(########################%%%%%%%%%%%%%%%%%&%&&&&&&&&&&&/**************/***/*/*/*/*//////////////(%@@@@@@@@@@@@@@@@@@@&&&&%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%#%%#%#%#####%##%%##########%
// ((((((((###############(##############%###%%#%%#%%%%%%%%%%%&%&&&&&&&&&&&&&&&&&&&&&&&%#((////////////(#%%&@@@@@@@@@@@@@@@@@@@@@@@@@@@&&&&%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%#%#%###%%%####%%%%###%%
// ((((##(#(#(((((#########################%##%##%%%%%%%%%%%%%&&%&&&&&&&&&&&&&&&&&&&&&@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@&&&&&%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%##%%#%%##%%####%%%###%##%