#pragma once 

#include <vector>

#include "ast.h"

namespace ck_bytecodes {
	const int LINENO               = 9; // Marks the lineno change
	const int NONE                 = 10;
	const int PUSH_CONST_INT       = 11;
	const int PUSH_CONST_DOUBLE    = 12;
	const int PUSH_CONST_BOOLEAN   = 13;
	const int PUSH_CONST_NULL      = 13;
	const int PUSH_CONST_UNDEFINED = 14;
	const int PUSH_CONST_STRING    = 15;
	const int LOAD_VAR             = 16;
};

namespace ck_translator {
	std::vector<unsigned char> translate(ck_ast::ASTNode* n);
};