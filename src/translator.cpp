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

void push(vector<unsigned char>& bytemap, int n, const unsigned char* p) {
	for (int i = 0; i < n; ++i)
		bytemap.push_back(p[i]);
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
		push(bytemap, sizeof(int), (unsigned char*) &last_lineno);
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
			push(bytemap, sizeof(long long), (unsigned char*) (n->objectlist->object));
			break;
		}
		
		case DOUBLE: {
			push_byte(bytemap, ck_bytecodes::PUSH_CONST_DOUBLE);
			push(bytemap, sizeof(double), (unsigned char*) (n->objectlist->object));
			break;
		}
		
		case BOOLEAN: {
			push_byte(bytemap, ck_bytecodes::PUSH_CONST_BOOLEAN);
			push(bytemap, 1, (unsigned char*) (n->objectlist->object));
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
			
			push(bytemap, sizeof(int), (unsigned char*) &size);
			
			for (int i = 0; i < size; ++i) {
				wchar_t c = s[i];
				push(bytemap, sizeof(wchar_t), (unsigned char*) &c);
			}
			break;
		}
		
		case NAME: { // [size|name]
			push_byte(bytemap, ck_bytecodes::LOAD_VAR);
			
			wstring& s = *(wstring*) n->objectlist->object;
			int size = s.size();
			
			push(bytemap, sizeof(int), (unsigned char*) &size);
			
			for (int i = 0; i < size; ++i) {
				wchar_t c = s[i];
				push(bytemap, sizeof(wchar_t), (unsigned char*) &c);
			}
			break;
		}
			
	};
};

vector<unsigned char> ck_translator::translate(ASTNode* n) {
	vector<unsigned char> bytemap;
	
	if (n && n->type != TERR)
		visit(bytemap, n);
	
	return bytemap;
};