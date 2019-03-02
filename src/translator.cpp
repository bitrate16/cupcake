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
int last_lineno = -1;
int last_lineno_addr = 0;

void visit(vector<unsigned char>& bytemap, vector<unsigned char>& lineno_table, ASTNode* n) {
	if (!n)
		return;
	
	// Each time the line number changes, it will be appended to the bytecode.
	// XXX: Use Line Number Table
	if (n->lineno != last_lineno) {
		last_lineno = n->lineno;
		push_byte(bytemap, ck_bytecodes::LINENO);
		push(bytemap, sizeof(int), &last_lineno);
		
		lineno_table.push_back(last_lineno);        // Record: [lineno|start]
		lineno_table.push_back(last_lineno_addr);
		last_lineno_addr = bytemap.size();
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
				// ...             --> VSTACK += ref
				// VSTACK_DUP
				// LOAD_FIELD name
				// REF_CALL
			
				visit(bytemap, lineno_table, n->left->left);
				
				push_byte(bytemap, ck_bytecodes::VSTACK_DUP);
				push_byte(bytemap, ck_bytecodes::LOAD_FIELD);
				
				wstring& s = *(wstring*) n->left->objectlist->object;
				int size = s.size();
				
				push(bytemap, sizeof(int), &size);
				
				for (int i = 0; i < size; ++i) {
					wchar_t c = s[i];
					push(bytemap, sizeof(wchar_t), &c);
				}
				
				push_byte(bytemap, ck_bytecodes::REF_CALL);
			} else if (n->left->type == MEMBER) { // REF_CALL_MEMBER [name]
				// ...              --> VSTACK += ref
				// VSTACK_DUP
				// LOAD_MEMBER name
				// REF_CALL
				
				visit(bytemap, lineno_table, n->left->left);
				
				push_byte(bytemap, ck_bytecodes::VSTACK_DUP);
				push_byte(bytemap, ck_bytecodes::LOAD_MEMBER);
				
				wstring& s = *(wstring*) n->left->objectlist->object;
				int size = s.size();
				
				push(bytemap, sizeof(int), &size);
				
				for (int i = 0; i < size; ++i) {
					wchar_t c = s[i];
					push(bytemap, sizeof(wchar_t), &c);
				}
				
				push_byte(bytemap, ck_bytecodes::REF_CALL);
			} else {
				// ...  --> VSTACK += val
				// CALL
				
				visit(bytemap, lineno_table, n->left);
				push_byte(bytemap, CALL);
			}
			
			push(bytemap, sizeof(int), &argc);
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
};

bool read(vector<unsigned char>& bytemap, int& index, int size, void* p) {
	if (index + size > bytemap.size())
		return 0;
	
	for (int i = 0; i < size; ++i) 
		((unsigned char*) p)[i] = bytemap[index + i];
	
	index += size;
	
	return 1;
};

void ck_translator::print(vector<unsigned char> bytemap) {
	for (int k = 0; k < bytemap.size();) {
		switch(bytemap[k++]) {
			case ck_bytecodes::LINENO: {
				int lineno; 
				read(bytemap, k, sizeof(int), &lineno);
				wcout << "LINENO: " << lineno << endl;
				break;
			}
			
			case ck_bytecodes::NONE: {
				wcout << ">" << endl;
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
		}
	}
};



