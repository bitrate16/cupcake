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

int last_lineno = 0;
void visit(vector<unsigned char>& bytemap, ASTNode* n) {
	if (!n)
		return;
	
	// Each time the line number changes, it will be appended to the bytecode.
	// XXX: Use Line Number Table
	if (n->lineno != last_lineno) {
		last_lineno = n->lineno;
		push_byte(bytemap, ck_bytecodes::LINENO);
		push(bytemap, sizeof(int), &last_lineno);
	}
	
	switch(n->type) {
		case ASTROOT: {
			ASTNode* t = n->left;
			while (t) {
				visit(bytemap, t);
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
				visit(bytemap, t);
				t = t->next;
			}
			
			push_byte(bytemap, ck_bytecodes::VSTACK_POP);
			break;
		}
	
		case ARRAY: {
			// Initialize all objects into object stack than call PUSH_CONST_ARRAY
			
			int size = 0;
			ASTNode* t = n->left;
			while (t) {
				visit(bytemap, t);
				t = t->next;
				++size;
			}
			
			push_byte(bytemap, PUSH_CONST_ARRAY);
			push(bytemap, sizeof(int), &size);
		}
	};
};

vector<unsigned char> ck_translator::translate(ASTNode* n) {
	vector<unsigned char> bytemap;
	
	if (n && n->type != TERR)
		visit(bytemap, n);
	
	return bytemap;
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
			
			case ck_bytecodes::VSTACK_POP: {
				wcout << "> VSTACK_POP" << endl;
				break;
			}
		}
	}
};



