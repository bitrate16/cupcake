#include <string>
#include <vector>

#include "translator.h"
#include "parser.h"

using namespace std;
using namespace ck_token;
using namespace ck_ast;
using namespace ck_parser;

void push_byte(vector<unsigned char>& bytemap, unsigned char n) {
	bytemap.push_back(n);
};

void push(vector<unsigned char>& bytemap, int n, const void* p) {
	for (int i = 0; i < n; ++i)
		bytemap.push_back(((unsigned char*) p)[i]);
};

// Tracking line numbers change
int last_lineno = 0;
int last_lineno_addr = 0;

void visit(vector<unsigned char>& bytemap, vector<unsigned char>& lineno_table, ASTNode* n) {
	if (!n)
		return;
	
	// Each time the line number changes, it will be appended to the bytecode.
	// XXX: Use Line Number Table
	if (n->lineno != last_lineno) {
		last_lineno = n->lineno;
		
		push(lineno_table, sizeof(int), &last_lineno);        // Record: [lineno|start]
		push(lineno_table, sizeof(int), &last_lineno_addr);
		last_lineno_addr = bytemap.size();
		
		push_byte(bytemap, ck_bytecodes::LINENO);
		push(bytemap, sizeof(int), &last_lineno);
	}
	
	switch(n->type) {
		case ASTROOT: {
			ASTNode* t = n->left;
			while (t) {
				visit(bytemap, lineno_table, t);
				t = t->next;
			}
			break;
		}
		
		case INTEGER: {
			push_byte(bytemap, ck_bytecodes::PUSH_CONST_INT);
			push(bytemap, sizeof(long long), (n->objectlist->object));
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
				visit(bytemap, lineno_table, t);
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
				visit(bytemap, lineno_table, t);
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
				visit(bytemap, lineno_table, t);
				t = t->next;
				++size;
			}
			
			push_byte(bytemap, ck_bytecodes::PUSH_CONST_OBJECT);
			push(bytemap, sizeof(int), &size);
			
			ASTObjectList* list = n->objectlist;
			while (list) {
				wstring* str = (wstring*) list->object;
				size = str->size();
				push(bytemap, sizeof(int), &size);
				
				for (int i = 0; i < size; ++i) {
					wchar_t c = (*str)[i];
					push(bytemap, sizeof(wchar_t), &c);
					list = list->next;
				}	
			}
			break;
		}
	
		case DEFINE: { // [amount|name1|ops1|...|nameN|opsN]		
			// ops  = has_value | safe  | local | const
			//        1000/0000   0100    0010    0001
		
			ASTNode* t = n->left;
			while (t) {
				visit(bytemap, lineno_table, t);
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
				int size = str.size();
				
				push(bytemap, sizeof(int), &size);
				
				for (int i = 0; i < size; ++i) {
					wchar_t c = str[i];
					push(bytemap, sizeof(wchar_t), &c);
					list = list->next;
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
			// ref  <-- may be nothing
			// func <-- you are here
			
			int argc = 0;
			ASTNode* t = n->left->next;
			while (t) {
				visit(bytemap, lineno_table, t);
				t = t->next;
				++argc;
			}
			
			if (n->left->type == FIELD) { // REF_CALL_FIELD [name]
			
				visit(bytemap, lineno_table, n->left->left);  // -> ref
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
				
				push_byte(bytemap, ck_bytecodes::REF_CALL);
				
				// STACK:
				// return
			} else if (n->left->type == MEMBER) { // REF_CALL_MEMBER [name]
				
				visit(bytemap, lineno_table, n->left->left); // -> ref
				push_byte(bytemap, ck_bytecodes::VSTACK_DUP);
				
				// STACK:
				// ref
				// ref
				
				visit(bytemap, lineno_table, n->left->right);       // -> obj
				
				// STACK:
				// ref
				// ref
				// key
				
				push_byte(bytemap, ck_bytecodes::LOAD_MEMBER);
				
				// STACK:
				// ref
				// val
				
				push_byte(bytemap, ck_bytecodes::REF_CALL);
				
				// STACK:
				// return
			} else {
				
				visit(bytemap, lineno_table, n->left);
				push_byte(bytemap, ck_bytecodes::CALL);
			}
			
			push(bytemap, sizeof(int), &argc);
			break;
		}
		
		case ASSIGN: {
			if (n->left->type == FIELD) { // STORE_FIELD [name]
			
				visit(bytemap, lineno_table, n->left->left);
				
				// STACK:
				// ref
				
				visit(bytemap, lineno_table, n->right);
				
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
			
				visit(bytemap, lineno_table, n->left->left);
				visit(bytemap, lineno_table, n->left->right);
				
				// STACK:
				// ref
				// key
				
				visit(bytemap, lineno_table, n->right);
				
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
				
				visit(bytemap, lineno_table, n->right);
				
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
		case ASSIGN_BITNOT :
		case ASSIGN_DIR    :
		case ASSIGN_PATH   :
		case ASSIGN_MOD    :
		case ASSIGN_BITOR  :
		case ASSIGN_BITAND :
		case ASSIGN_BITXOR :
		case ASSIGN_HASH   : {			
			if (n->left->type == FIELD) { // STORE_FIELD [name]
			
				visit(bytemap, lineno_table, n->left->left);
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
				
				visit(bytemap, lineno_table, n->right);
				
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
					case ASSIGN_BITNOT:  push_byte(bytemap, ck_bytecodes::OPT_BITNOT ); break;
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
			
				visit(bytemap, lineno_table, n->left->left);
				push_byte(bytemap, ck_bytecodes::VSTACK_DUP);
				visit(bytemap, lineno_table, n->left->right);
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
				
				visit(bytemap, lineno_table, n->right);
				
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
					case ASSIGN_BITNOT:  push_byte(bytemap, ck_bytecodes::OPT_BITNOT ); break;
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
				
				visit(bytemap, lineno_table, n->right);
				
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
					case ASSIGN_BITNOT:  push_byte(bytemap, ck_bytecodes::OPT_BITNOT ); break;
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
			visit(bytemap, lineno_table, n->left);
			
			push_byte(bytemap, ck_bytecodes::LOAD_FIELD);
			
			wstring& s = *(wstring*) n->left->objectlist->object;
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
			
			visit(bytemap, lineno_table, n->left);
			visit(bytemap, lineno_table, n->right);
			
			push_byte(bytemap, ck_bytecodes::LOAD_MEMBER);
			break;
		}
	
		case PRE_INC:
		case PRE_DEC: {
			if (n->left->type == FIELD) { // STORE_FIELD [name]
			
				visit(bytemap, lineno_table, n->left->left);
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
			
				visit(bytemap, lineno_table, n->left->left);
				push_byte(bytemap, ck_bytecodes::VSTACK_DUP);
				visit(bytemap, lineno_table, n->left->right);
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
		
		case POS_INC:
		case POS_DEC: {
			if (n->left->type == FIELD) { // STORE_FIELD [name]
			
				visit(bytemap, lineno_table, n->left->left);
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
				if (n->type == POS_INC)
					push_byte(bytemap, ck_bytecodes::OPT_INC);
				else
					push_byte(bytemap, ck_bytecodes::OPT_DEC);
				
				// STACK:
				// val
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
			
				visit(bytemap, lineno_table, n->left->left);
				push_byte(bytemap, ck_bytecodes::VSTACK_DUP);
				visit(bytemap, lineno_table, n->left->right);
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
				if (n->type == POS_INC)
					push_byte(bytemap, ck_bytecodes::OPT_INC);
				else
					push_byte(bytemap, ck_bytecodes::OPT_DEC);
				
				// STACK:
				// ref
				// key
				// val
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
				if (n->type == POS_INC)
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
	
		case PLUS   : { visit(bytemap, lineno_table, n->left); visit(bytemap, lineno_table, n->right); push_byte(bytemap, ck_bytecodes::OPERATOR); push_byte(bytemap, ck_bytecodes::OPT_ADD    ); break; }
		case MINUS  : { visit(bytemap, lineno_table, n->left); visit(bytemap, lineno_table, n->right); push_byte(bytemap, ck_bytecodes::OPERATOR); push_byte(bytemap, ck_bytecodes::OPT_SUB    ); break; }
		case MUL    : { visit(bytemap, lineno_table, n->left); visit(bytemap, lineno_table, n->right); push_byte(bytemap, ck_bytecodes::OPERATOR); push_byte(bytemap, ck_bytecodes::OPT_MUL    ); break; }
		case DIV    : { visit(bytemap, lineno_table, n->left); visit(bytemap, lineno_table, n->right); push_byte(bytemap, ck_bytecodes::OPERATOR); push_byte(bytemap, ck_bytecodes::OPT_DIV    ); break; }
		case BITRSH : { visit(bytemap, lineno_table, n->left); visit(bytemap, lineno_table, n->right); push_byte(bytemap, ck_bytecodes::OPERATOR); push_byte(bytemap, ck_bytecodes::OPT_BITRSH ); break; }
		case BITLSH : { visit(bytemap, lineno_table, n->left); visit(bytemap, lineno_table, n->right); push_byte(bytemap, ck_bytecodes::OPERATOR); push_byte(bytemap, ck_bytecodes::OPT_BITLSH ); break; }
		case BITURSH: { visit(bytemap, lineno_table, n->left); visit(bytemap, lineno_table, n->right); push_byte(bytemap, ck_bytecodes::OPERATOR); push_byte(bytemap, ck_bytecodes::OPT_BITURSH); break; }
		case BITNOT : { visit(bytemap, lineno_table, n->left); visit(bytemap, lineno_table, n->right); push_byte(bytemap, ck_bytecodes::OPERATOR); push_byte(bytemap, ck_bytecodes::OPT_BITNOT ); break; }
		case DIR    : { visit(bytemap, lineno_table, n->left); visit(bytemap, lineno_table, n->right); push_byte(bytemap, ck_bytecodes::OPERATOR); push_byte(bytemap, ck_bytecodes::OPT_DIR    ); break; }
		case PATH   : { visit(bytemap, lineno_table, n->left); visit(bytemap, lineno_table, n->right); push_byte(bytemap, ck_bytecodes::OPERATOR); push_byte(bytemap, ck_bytecodes::OPT_PATH   ); break; }
		case MOD    : { visit(bytemap, lineno_table, n->left); visit(bytemap, lineno_table, n->right); push_byte(bytemap, ck_bytecodes::OPERATOR); push_byte(bytemap, ck_bytecodes::OPT_MOD    ); break; }
		case BITOR  : { visit(bytemap, lineno_table, n->left); visit(bytemap, lineno_table, n->right); push_byte(bytemap, ck_bytecodes::OPERATOR); push_byte(bytemap, ck_bytecodes::OPT_BITOR  ); break; }
		case BITAND : { visit(bytemap, lineno_table, n->left); visit(bytemap, lineno_table, n->right); push_byte(bytemap, ck_bytecodes::OPERATOR); push_byte(bytemap, ck_bytecodes::OPT_BITAND ); break; }
		case HASH   : { visit(bytemap, lineno_table, n->left); visit(bytemap, lineno_table, n->right); push_byte(bytemap, ck_bytecodes::OPERATOR); push_byte(bytemap, ck_bytecodes::OPT_HASH   ); break; }
		case EQ     : { visit(bytemap, lineno_table, n->left); visit(bytemap, lineno_table, n->right); push_byte(bytemap, ck_bytecodes::OPERATOR); push_byte(bytemap, ck_bytecodes::OPT_EQ     ); break; }
		case NEQ    : { visit(bytemap, lineno_table, n->left); visit(bytemap, lineno_table, n->right); push_byte(bytemap, ck_bytecodes::OPERATOR); push_byte(bytemap, ck_bytecodes::OPT_NEQ    ); break; }
		case OR     : { visit(bytemap, lineno_table, n->left); visit(bytemap, lineno_table, n->right); push_byte(bytemap, ck_bytecodes::OPERATOR); push_byte(bytemap, ck_bytecodes::OPT_OR     ); break; }
		case AND    : { visit(bytemap, lineno_table, n->left); visit(bytemap, lineno_table, n->right); push_byte(bytemap, ck_bytecodes::OPERATOR); push_byte(bytemap, ck_bytecodes::OPT_AND    ); break; }
		case GT     : { visit(bytemap, lineno_table, n->left); visit(bytemap, lineno_table, n->right); push_byte(bytemap, ck_bytecodes::OPERATOR); push_byte(bytemap, ck_bytecodes::OPT_GT     ); break; }
		case GE     : { visit(bytemap, lineno_table, n->left); visit(bytemap, lineno_table, n->right); push_byte(bytemap, ck_bytecodes::OPERATOR); push_byte(bytemap, ck_bytecodes::OPT_GE     ); break; }
		case LT     : { visit(bytemap, lineno_table, n->left); visit(bytemap, lineno_table, n->right); push_byte(bytemap, ck_bytecodes::OPERATOR); push_byte(bytemap, ck_bytecodes::OPT_LT     ); break; }
		case LE     : { visit(bytemap, lineno_table, n->left); visit(bytemap, lineno_table, n->right); push_byte(bytemap, ck_bytecodes::OPERATOR); push_byte(bytemap, ck_bytecodes::OPT_LE     ); break; }
		case PUSH   : { visit(bytemap, lineno_table, n->left); visit(bytemap, lineno_table, n->right); push_byte(bytemap, ck_bytecodes::OPERATOR); push_byte(bytemap, ck_bytecodes::OPT_PUSH   ); break; }
		case ARROW  : { visit(bytemap, lineno_table, n->left); visit(bytemap, lineno_table, n->right); push_byte(bytemap, ck_bytecodes::OPERATOR); push_byte(bytemap, ck_bytecodes::OPT_ARROW  ); break; }
	
		case DOG   : { visit(bytemap, lineno_table, n->left); push_byte(bytemap, ck_bytecodes::UNARY_OPERATOR); push_byte(bytemap, ck_bytecodes::OPT_DOG   ); break; }
		case NOT   : { visit(bytemap, lineno_table, n->left); push_byte(bytemap, ck_bytecodes::UNARY_OPERATOR); push_byte(bytemap, ck_bytecodes::OPT_NOT   ); break; }
		case BITXOR: { visit(bytemap, lineno_table, n->left); push_byte(bytemap, ck_bytecodes::UNARY_OPERATOR); push_byte(bytemap, ck_bytecodes::OPT_BITXOR); break; }
		case POS   : { visit(bytemap, lineno_table, n->left); push_byte(bytemap, ck_bytecodes::UNARY_OPERATOR); push_byte(bytemap, ck_bytecodes::OPT_POS   ); break; }
		case NEG   : { visit(bytemap, lineno_table, n->left); push_byte(bytemap, ck_bytecodes::UNARY_OPERATOR); push_byte(bytemap, ck_bytecodes::OPT_NEG   ); break; }
		
		case BLOCK: {
			push_byte(bytemap, VSTATE_PUSH_SCOPE);
			
			ASTNode* t = n->left;
			while (t) {
				visit(bytemap, lineno_table, t);
				t = t->next;
			}
			
			push_byte(bytemap, VSTATE_POP_SCOPE);
			
			break;
		}
		
		case EMPTY: {
			push_byte(bytemap, NOP);
			break;
		}
	};
};

void ck_translator::translate(vector<unsigned char>& bytemap, vector<unsigned char>& lineno_table, ASTNode* n) {
	// lineno_table - Table of Line Numbers
	// Provides range of commands mapped to a single line number
	// [lineno, start_cmd]
	
	// bytemap - the resulting bytemap
	
	if (n && n->type != TERR)
		visit(bytemap, lineno_table, n);
	
	last_lineno = -1;
	push(lineno_table, sizeof(int), &last_lineno);
	last_lineno_addr = bytemap.size();
	push(lineno_table, sizeof(int), &last_lineno_addr);
};

bool read(vector<unsigned char>& bytemap, int& index, int size, void* p) {
	if (index + size > bytemap.size())
		return 0;
	
	for (int i = 0; i < size; ++i) 
		((unsigned char*) p)[i] = bytemap[index + i];
	
	index += size;
	
	return 1;
};

void ck_translator::print(vector<unsigned char>& bytemap) {
	for (int k = 0; k < bytemap.size();) {
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
				long long i; 
				read(bytemap, k, sizeof(long long), &i);
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
			
			case ck_bytecodes::REF_CALL: {
				int i; 
				read(bytemap, k, sizeof(int), &i);
				wcout << "> REF_CALL [" << i << ']' << endl;
				break;
			}
			
			case ck_bytecodes::CALL: {
				int i; 
				read(bytemap, k, sizeof(int), &i);
				wcout << "> CALL [" << i << ']' << endl;
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
			
		}
	}
};

void ck_translator::print_lineno_table(vector<unsigned char>& lineno_table) {
	for (int i = 0; i < lineno_table.size();) {
		int lineno, start;
		read(lineno_table, i, sizeof(int), &lineno);
		read(lineno_table, i, sizeof(int), &start);
		
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