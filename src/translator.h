#pragma once 

#include <vector>

#include "ast.h"

namespace ck_bytecodes {
	const int LINENO               = 9; // Marks the lineno change
	const int NOP                  = 10;
	const int PUSH_CONST_INT       = 11;
	const int PUSH_CONST_DOUBLE    = 12;
	const int PUSH_CONST_BOOLEAN   = 13;
	const int PUSH_CONST_NULL      = 14;
	const int PUSH_CONST_UNDEFINED = 15;
	const int PUSH_CONST_STRING    = 16;
	const int LOAD_VAR             = 17; // scope.get_var(name)
	const int VSTACK_POP           = 18; // Pop the valut from stack and do nothing with it
	const int PUSH_CONST_ARRAY     = 19;
	const int PUSH_CONST_OBJECT    = 20;
	const int DEFINE_VAR           = 21;
	const int REF_CALL             = 24; // a.b() or a['b']()
	const int VSTACK_DUP           = 25; // Duplicate top of the stack
	const int LOAD_MEMBER          = 26; // stack.next_to_top().get_member(stack.pop())
	const int LOAD_FIELD           = 27; // stack.pop().get_field(name)
	const int CALL                 = 28; // var() or (<expression>)()
	const int OPERATOR             = 29; // operate with b = stack.top and a = stack.next_to_top as a <opt> b
	const int STORE_VAR            = 30; // scope.store(name)
	const int STORE_FIELD          = 31; // top:[value, ref]
	const int STORE_MEMBER         = 32; // top:[value, key, ref]
	const int UNARY_OPERATOR       = 33;
	const int VSTACK_SWAP          = 34; // swap stack top and top-1
	const int VSTACK_SWAP1         = 35; // swap stack top-1 and top-2
	const int VSTACK_SWAP2         = 36; // swap stack top-2 and top-3
	const int VSTATE_PUSH_SCOPE    = 37; // Push scope to scope list
	const int VSTATE_POP_SCOPE     = 38; // Pop scope from scope list
	
	const int OPT_ADD     = 1;
	const int OPT_SUB     = 2;
	const int OPT_MUL     = 3;
	const int OPT_DIV     = 4;
	const int OPT_BITRSH  = 5;
	const int OPT_BITLSH  = 6;
	const int OPT_BITURSH = 7;
	const int OPT_BITNOT  = 8;
	const int OPT_DIR     = 9;
	const int OPT_PATH    = 10;
	const int OPT_MOD     = 11;
	const int OPT_BITOR   = 12;
	const int OPT_BITAND  = 13;
	const int OPT_HASH    = 15;	
	const int OPT_EQ      = 18;
	const int OPT_NEQ     = 19;
	const int OPT_OR      = 20;
	const int OPT_AND     = 21;
	const int OPT_GT      = 22;
	const int OPT_GE      = 23;
	const int OPT_LT      = 24;
	const int OPT_LE      = 25;
	const int OPT_PUSH    = 29;
	const int OPT_ARROW   = 30;
	
	const int OPT_DOG     = 31;
	const int OPT_NOT     = 28;
	const int OPT_BITXOR  = 14;
	const int OPT_POS     = 15;
	const int OPT_NEG     = 16;
	
	const int OPT_INC     = 16;	
	const int OPT_DEC     = 17;
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
};

namespace ck_translator {
	void translate(std::vector<unsigned char>& lineno_table, std::vector<unsigned char>& bytemap, ck_ast::ASTNode* n);
	
	void print(std::vector<unsigned char>& bytmap);
	
	void print_lineno_table(std::vector<unsigned char>& lineno_table);
};