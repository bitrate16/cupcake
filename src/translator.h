#pragma once 

#include <vector>

#include "ast.h"

namespace ck_bytecodes {
	const int LINENO               = 9; // Marks the lineno change
	const int NONE                 = 10;
	const int PUSH_CONST_INT       = 11;
	const int PUSH_CONST_DOUBLE    = 12;
	const int PUSH_CONST_BOOLEAN   = 13;
	const int PUSH_CONST_NULL      = 14;
	const int PUSH_CONST_UNDEFINED = 15;
	const int PUSH_CONST_STRING    = 16;
	const int LOAD_VAR             = 17;
	const int VSTACK_POP           = 18; // Pop the valut from stack and do nothing with it
	const int PUSH_CONST_ARRAY     = 19;
	const int PUSH_CONST_OBJECT    = 20;
	const int DEFINE_VAR           = 21;
	const int REF_CALL             = 24; // a.b() or a['b']()
	const int VSTACK_DUP           = 25; // Duplicate top of the stack
	const int LOAD_MEMBER          = 26; // stack.pop().get_member(name)
	const int LOAD_FIELD           = 27; // stack.pop().get_field(name)
	const int CALL                 = 28; // var() or (<expression>)()
};

namespace ck_translator {
	void translate(std::vector<unsigned char>& lineno_table, std::vector<unsigned char>& bytemap, ck_ast::ASTNode* n);
	
	void print(std::vector<unsigned char> bytmap);
};