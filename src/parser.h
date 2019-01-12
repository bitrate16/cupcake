#pragma once

#include "ast"

#include <wstring>
#include <cstdio>

namespace ck_token {
	// Pre-defined type codes
	const int INTEGER	                 12;
	const int DOUBLE                     13;
	const int BOOLEAN                    14;
	const int STRING                     15;
	const int NAME                       16;
	const int FLAG                       17;
	const int LONG                       18;
	const int BYTE                       19;
	const int ARRAY                      20;
	const int OBJECT                     21;
	const int SCOPE                      22;
	const int STRING_PROTOTYPE           23;
	const int DOUBLE_PROTOTYPE           24;
	const int NULL_PROTOTYPE             25;
	const int OBJECT_PROTOTYPE           26;
	const int SCOPE_PROTOTYPE            27;
	const int PROXY_SCOPE                28;
	const int CODE_FUNCTION_PROTOTYPE    29;
	const int CODE_FUNCTION              30;
	const int NATIVE_FUNCTION_PROTOTYPE  31;
	const int NATIVE_FUNCTION            32;
	const int INTEGER_PROTOTYPE          33;
	const int BOOLEAN_PROTOTYPE          34;
	const int UNDEFINED_PROTOTYPE        35;
	const int TNULL                      36;
	const int UNDEFINED                  37;
	const int ARRAY_PROTOTYPE            38;
	const int ERROR                      39;
	const int ERROR_PROTOTYPE            40;

	// Cupcake keywords
	const int IF               47;
	const int ELSE             48;
	const int SWITCH           49;
	const int CASE             50;
	const int DEFAULT          74;
	const int BREAK            51;
	const int CONTINUE         52;
	const int RETURN           53;
	const int WHILE            54;
	const int DO               55;
	const int FOR              56;
	const int IN               57;
	const int FUNCTION         58;
	const int VAR              59;
	const int TRY              60;
	const int EXPECT           61;
	const int RAISE            62;
	const int PROTOTYPE        63;
	const int CONST            64;
	const int SAFE             65;
	const int NEW              66;
	const int LOCAL            67;
	const int TRUE             68;
	const int FALSE            69;
	const int SELF             72;
	const int THIS             73;
	
	// Cupcake operators
	const int ASSIGN           80;
	const int HOOK             81;
	const int COLON            82;
	const int DOT              83;
	const int COMMA            84;
	const int SEMICOLON        85;
	const int LP               86;
	const int RP               87;
	const int LB               88;
	const int RB               89;
	const int LC               90;
	const int RC               91;
	const int EQ               92;
	const int NEQ              93;
	const int OR               94;
	const int AND              95;
	const int BITOR            96;
	const int BITAND           97;
	const int BITXOR           98;
	const int GT               99;
	const int GE              100;
	const int LT              101;
	const int LE              102;
	const int BITRSH          103;
	const int BITLSH          104;
	const int BITURSH         105;
	const int PLUS            106;
	const int MINUS           107;
	const int MUL             108;
	const int DIV             109;
	const int MDIV            110;
	const int MOD             111;
	const int HASH            112;
	const int NOT             113;
	const int BITNOT          114;
	const int INC             115;
	const int DEC             116;
	const int ASSIGN_ADD      117;
	const int ASSIGN_SUB      118;
	const int ASSIGN_MUL      119;
	const int ASSIGN_DIV      120;
	const int ASSIGN_BITRSH   121;
	const int ASSIGN_BITLSH   122;
	const int ASSIGN_BITURSH  123;
	const int ASSIGN_BITNOT   124;
	const int ASSIGN_MDIV     125;
	const int ASSIGN_MOD      126;
	const int ASSIGN_BITOR    127;
	const int ASSIGN_BITAND   128;
	const int ASSIGN_BITXOR   129;
	const int PATH            130;
	const int PUSH            131;
	const int LAMBDA          132;
	const int PRE_INC         133;
	const int PRE_DEC         134;
	const int POS_INC         135;
	const int POS_DEC         136;
	const int POS             157;
	const int NEG             158;

	// Technical tokens
	const int TEOF            137;
	const int TEOL            '\n';
	const int TERR            139;
	const int NONE            140;

	// Additional statements & expressions
	const int EMPTY           201;
	const int EXPRESSION      202;
	const int BLOCK           203;
	const int ASSIGN_DEFINE   204;
	const int DEFINE          205;
	const int ASTROOT         206;
	const int CONDITION       207;
	const int MEMBER          208;
	const int FIELD           209;
	const int CALL            210;
	const int NATIVE_CALL     211;
	const int IMPORTED_SCRIPT 212;
	const int FUNCTIONROOT    213;
};

namespace ck_parser {
	class raw_token {
	public:
		int token      = ck_token::NONE;
		int integerv   = -1;
		long longv     = -1;
		int bytev      = -1;
		bool booleanv  = 0;
		double doublev = 0;
		std::wstring stringv;
		int lineno     = -1;
		
		raw_token() {};
		
		~raw_token() : {};
		
		raw_token *copy() {
			raw_token *t = new raw_token();
			t->token     = this->token;
			t->integerv  = this->integerv;
			t->longv     = this->longv;
			t->bytev     = this->bytev;
			t->booleanv  = this->booleanv;
			// // GCC < 5 https://shaharmike.com/cpp/std-string/
			// // Copy on Write not working on less versions so adding support of "copy".
			// Copying in context of token stream.
			// After copy is done, old value is replaced by ""
			t->stringv   = this->stringv;
			t->lineno    = this->lineno;
			
			return t;
		};
	};
	
	// Container for single message produced during parsing.
	class parser_message {
		parser_message(int type, int lineno, std::wstring message) 
						: type(type), lineno(lineno), message(message) {};
		
	public:
		const int MSG_WARNING = 0;
		const int MSG_ERROR   = 1;
		int type;
		int lineno;
		std::wstring message;
		
		// Returns new error instance
		static inline parser_message error(std::wstring m, int lineno = -1) {
			return parser_message(MSG_ERROR, lineno, m);
		};
		
		// Returns new warning instance
		static inline parser_message warning(std::wstring m, int lineno = -1) {
			return parser_message(MSG_WARNING, lineno, m);
		};
	};
	
	// Class containing messages produced during parsing.
	// Also marks if errors during parsing occurred or shit hapenned.
	class parser_massages {
		int contains_error = 0;
		std::vector<parser_message> messages;
		
	public:
		// Creates and adds new error instance to the container
		void error(std::wstring m, int lineno = -1) {
			messages.push(parser_message.error(m, lineno));
			++contains_error;
		};
		
		// Creates and adds new warning instance to the container
		void warning(std::wstring m, int lineno = -1) {
			messages.push(parser_message.warning(m, lineno));
		};
		
		// Returns contains_error
		bool is_error() { return contains_error; };
		
		// Returns messages
		std::vector<parser_message> &get_messages() { return messages };
	};
	
	// Wrapped for data input.
	// Allows passing input source from STDIN, 
	// FILE or even source string.
	// Provides basic methods of getting next character,
	// checking for eof or 
	class stream_wrapper {
		// Source type:
		// Reading code line by line from standard input
		const int IN_STDIN  = 1;
		// Reading code from passed file
		const int IN_FILE   = 2;
		// Reading code from passed string
		const int IN_STRING = 3;
		
		FILE *file;
		
		const std::wstring &string;
		int cursor = 0;
		
		int source_type = 0;
		
		int eof = 0;
		
	public:
		
		stream_wrapper(FILE *f) : source_type(IN_FILE) { file = f; };
		stream_wrapper() : source_type(IN_STDIN) {};
		stream_wrapper(const std::wstring &s) : source_type(IN_STRING) { string = s; };
		
		~stream_wrapper() {
			switch(source_type) {
				case IN_STDIN:
				case IN_STRING:
				default:
					break;
				case IN_FILE:
					fclose(file);
			};
		};
		
		// Returns next redden character.
		int getc();
		
		// Returns true whatewer EOF is reached.
		bool eof();
	}
	
	class parser;
	class tokenizer {
		// Wrapper for input stream. Created by parser, passed here.
		stream_wrapper &sw;
		
		// Container for all messages. Created by Parser, passed here.
		parser_massages &messages;
		
		// Temporary token to be returned.
		raw_token    *token = nullptr;
		
		// Buffer for redden codes
		int      buffer[9] = { 0,0,0,0,0,0,0,0,0 };
		
		int   eof_ = 0;
		int error_ = 0;
		int lineno = 0;
		
		friend class parser;
		
		tokenizer(parser_massages &pm) : messages(pm) {};
		
		void set_stream(stream_wrapper &sw_) {
			this->sw = sw_;
			
			if (this->token)
				delete this->token;
			
			this->token = new raw_token();
			
			this->token->lineno = 1;
			this->lineno = 1;
			
			next(); next(); next(); next(); next(); 
		};
		
	public:
		~tokenizer() { delete token; };
		
		int get(int off);
		
		int next();
		
		void clear();
		
		int put(int token);
		
		int match(int charcode0);
		
		int match(int charcode0, int charcode1);
		
		int match(int charcode0, int charcode1, int charcode2);
		
		int match(int charcode0, int charcode1, int charcode2, int charcode3);
		
		int eof() { return eof = sw.eof() || eof; };
		
		int nextToken();
	};
	
	class parser {
		// Container for all messages produced on parsing.
		parser_massages messages;
		
		// Input token stream.
		tokenizer source(messages);
		
		// Wrapper for an input stream
		stream_wrapper swrapper;
		
		int           eof_ = 0;
		int         error_ = 0;
		
		raw_token *buffer[7] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL };
		
	public:
		Parser(const std::wstring &str) { // <-- input from string
			swrapper = stream_wrapper(str);
			this->source.set_stream(swrapper);
		};
		
		Parser(FILE *file) {              // <-- input from file
			swrapper = stream_wrapper(file);
			this->source.set_stream(swrapper);
		};
		
		Parser() {                        // <-- input from stdin
			this->source.set_stream(swrapper);
		};
	
	private:
		~Parser();
		
		raw_token *get(int off);
		
		raw_token *next();
		
		bool match(int token);
		
		ck_ast::ASTNode *primaryexp();
		
		ck_ast::ASTNode *member_expression();
		
		ck_ast::ASTNode *unary_expression();
		
		ck_ast::ASTNode *multiplication_expression();
		
		ck_ast::ASTNode *addiction_expression();
		
		ck_ast::ASTNode *bitwise_shift_expression();
		
		ck_ast::ASTNode *comparison_expression();
		
		ck_ast::ASTNode *equality_expression();
		
		ck_ast::ASTNode *bitwise_and_expression();
		
		ck_ast::ASTNode *bitwise_xor_exppression();
		
		ck_ast::ASTNode *bitwise_or_expression();
		
		ck_ast::ASTNode *and_expression();
		
		ck_ast::ASTNode *or_expression();
		
		ck_ast::ASTNode *condition_expression();
		
		ck_ast::ASTNode *assign_expression();
		
		ck_ast::ASTNode *expression();
		
		ck_ast::ASTNode *checkNotNullExpression();
		
		bool checkNullExpression(ASTNode *exp);
		
		ck_ast::ASTNode *initializerstatement();
		
		bool peekStatementWithoutSemicolon();
		
		ck_ast::ASTNode *statement();
		
		ck_ast::ASTNode *statement_with_semicolons();
		
		ck_ast::ASTNode *parse();
		
		int lineno();
		
		int eof();
	};
};