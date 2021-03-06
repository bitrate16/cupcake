#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <cstdio>

#include "ast.h"
#include "token.h"

namespace ck_parser {
	std::wstring token_to_string(int token);
	
	class raw_token {
	public:	
		int token  = ck_token::NONE;
		int lineno = -1;
		int charno = -1;
		
		// integer value
		int64_t iv = 0;
		// byte value
		bool bv = 0;
		// double value
		double dv = 0;
		// string value
		std::wstring sv;
		
		raw_token() {};
		
		raw_token(raw_token& t) {	
			token = t.token;
			lineno = t.lineno;
			charno = t.charno;
			iv = t.iv;
			bv = t.bv;
			dv = t.dv;
			sv = std::wstring(t.sv);
		};
		
		~raw_token() {};
		
		friend std::wostream& operator<<(std::wostream& os, const raw_token& t) {
			switch(t.token) {
				case ck_token::IF : os << "if"; break;
				case ck_token::ELSE : os << "else"; break;
				case ck_token::SWITCH : os << "switch"; break;
				case ck_token::CASE : os << "case"; break;
				case ck_token::DEFAULT : os << "default"; break;
				case ck_token::BREAK : os << "break"; break;
				case ck_token::CONTINUE: os << "continue"; break;
				case ck_token::RETURN : os << "return"; break;
				case ck_token::WHILE : os << "while"; break;
				case ck_token::DO : os << "do"; break;
				case ck_token::FOR : os << "for"; break;
				case ck_token::IN : os << "in"; break;
				case ck_token::FUNCTION: os << "function"; break;
				case ck_token::VAR : os << "var"; break;
				case ck_token::TRY : os << "try"; break;
				case ck_token::CATCH : os << "catch"; break;
				case ck_token::THROW : os << "raise"; break;
				case ck_token::CONST : os << "const"; break;
				case ck_token::SAFE : os << "safe"; break;
				case ck_token::LOCAL : os << "local"; break;
				case ck_token::TRUE : os << "true"; break;
				case ck_token::FALSE : os << "false"; break;
				case ck_token::SELF : os << "self"; break;
				case ck_token::THIS : os << "this"; break;
				case ck_token::WITH : os << "with"; break;
				case ck_token::TYPEOF : os << "typeof"; break;
				case ck_token::AS : os << "as"; break;
				case ck_token::ISTYPEOF : os << "istypeof"; break;
				case ck_token::ASSIGN : os << "="; break;
				case ck_token::HOOK : os << "?"; break;
				case ck_token::COLON : os << ":"; break;
				case ck_token::DOT : os << "."; break;
				case ck_token::COMMA : os << ","; break;
				case ck_token::SEMICOLON : os << ";"; break;
				case ck_token::LP : os << "("; break;
				case ck_token::RP : os << ")"; break;
				case ck_token::LB : os << "["; break;
				case ck_token::RB : os << "]"; break;
				case ck_token::LC : os << "{"; break;
				case ck_token::RC : os << "}"; break;
				case ck_token::EQ : os << "=="; break;
				case ck_token::NEQ : os << "!="; break;
				case ck_token::OR : os << "||"; break;
				case ck_token::AND : os << "&"; break;
				case ck_token::BITOR : os << "|"; break;
				case ck_token::BITAND : os << "&"; break;
				case ck_token::BITXOR : os << "^"; break;
				case ck_token::GT : os << ">"; break;
				case ck_token::GE : os << ">="; break;
				case ck_token::LT : os << "<"; break;
				case ck_token::LE : os << "<="; break;
				case ck_token::BITRSH : os << "<<"; break;
				case ck_token::BITLSH : os << ">>"; break;
				case ck_token::BITURSH : os << "<<<"; break;
				case ck_token::PLUS : os << "+"; break;
				case ck_token::MINUS : os << "-" ; break;
				case ck_token::MUL : os << "*"; break;
				case ck_token::DIV : os << "/"; break;
				case ck_token::DIR : os << "\\\\" ; break;
				case ck_token::MOD : os << "%"; break;
				case ck_token::HASH : os << "#"; break;
				case ck_token::NOT : os << "!"; break;
				case ck_token::BITNOT : os << "~" ; break;
				case ck_token::INC : os << "++"; break;
				case ck_token::DEC : os << "--"; break;
				case ck_token::ASSIGN_ADD : os << "+=" ; break;
				case ck_token::ASSIGN_SUB : os << "-=" ; break;
				case ck_token::ASSIGN_MUL : os << "*=" ; break;
				case ck_token::ASSIGN_DIV : os << "/=" ; break;
				case ck_token::ASSIGN_BITRSH : os << ">>=" ; break;
				case ck_token::ASSIGN_BITLSH : os << "<<=" ; break;
				case ck_token::ASSIGN_BITURSH : os << "<<<="; break;
				case ck_token::ASSIGN_DIR : os << "\\\\="; break;
				case ck_token::ASSIGN_PATH : os << "\\="; break;
				case ck_token::ASSIGN_HASH : os << "#="; break;
				case ck_token::ASSIGN_MOD : os << "%="; break;
				case ck_token::ASSIGN_BITOR : os << "|=" ; break;
				case ck_token::ASSIGN_BITAND : os << "&=" ; break;
				case ck_token::ASSIGN_BITXOR : os << "^=" ; break;
				case ck_token::PATH : os << "\\" ; break;
				case ck_token::PUSH : os << "->" ; break;
				case ck_token::ARROW : os << "=>" ; break;
				case ck_token::DOG : os << "@x"; break;
				case ck_token::INTEGER: os << t.iv; break;
				case ck_token::DOUBLE: os << t.dv; break;
				case ck_token::BOOLEAN: os << t.bv; break;
				case ck_token::STRING: os << '"' << t.sv << '"'; break;
				case ck_token::NAME: os << t.sv; break;
				case ck_token::TNULL: os << "null"; break;
				case ck_token::UNDEFINED: os << "undefined"; break;
				default: os << '[' << t.token << ']'; break;
			};
		
			return os;
		};
	};
	
	// Container for single message produced during parsing.
	class parser_message {
		parser_message(int type, int lineno, int charno, std::wstring message) 
						: type(type), lineno(lineno), charno(charno), message(message) {};
		
	public:
		static const int MSG_WARNING = 0;
		static const int MSG_ERROR   = 1;
		int type;
		int lineno;
		int charno;
		std::wstring message;
		
		parser_message() {};
		
		// Returns new error instance
		static inline parser_message error(std::wstring m, int lineno = -1, int charno = -1) {
			return parser_message(MSG_ERROR, lineno, charno, m);
		};
		
		// Returns new warning instance
		static inline parser_message warning(std::wstring m, int lineno = -1, int charno = -1) {
			return parser_message(MSG_WARNING, lineno, charno, m);
		};
	};
	
	// Class containing messages produced during parsing.
	// Also marks if errors during parsing occurred or shit hapenned.
	class parser_massages {
		int contains_error = 0;
		std::vector<parser_message> messages;
		std::wstring filename;
		
	public:
		parser_massages(const std::wstring& _filename) : filename(_filename) {};
	
		// Creates and adds new error instance to the container
		inline void error(std::wstring m, int lineno = -1, int charno = -1) {
			messages.push_back(parser_message::error(m, lineno, charno));
			++contains_error;
		};
		
		// Creates and adds new warning instance to the container
		inline void warning(std::wstring m, int lineno = -1, int charno = -1) {
			messages.push_back(parser_message::warning(m, lineno, charno));
		};
		
		// Returns number of errors
		int errors() { return contains_error; };
		
		inline const std::wstring& get_filename() { return filename; }
		
		// Returns messages
		inline const std::vector<parser_message> &get_messages() { return messages; };
		
		void print() {
			for (int i = 0; i < messages.size(); ++i) 
				if (messages[i].type == parser_message::MSG_WARNING) 
					std::wcout << "warning: " << messages[i].message << ", at <" << filename << ">:[" << messages[i].lineno << ':' << messages[i].charno << ']' << std::endl;
				else
					std::wcout << "error: " << messages[i].message << ", at <" << filename << ">:[" << messages[i].lineno << ':' << messages[i].charno << ']' << std::endl;
		};
	};
	
	// Wrapped for data input.
	// Allows passing input source from STDIN, 
	// FILE or even source string.
	// Provides basic methods of getting next character,
	// checking for eof or 
	class stream_wrapper {
		// Source type:
		// Reading code line by line from standard input
		static const int IN_STDIN  = 1;
		// Reading code from passed file
		static const int IN_FILE   = 2;
		// Reading code from passed string
		static const int IN_STRING = 3;
		
		std::wifstream* file;
		
		const std::wstring* string;
		int cursor = 0;
		
		int source_type = 0;
		
		int _eof = 0;
		
	public:
		
		stream_wrapper()                      : source_type(IN_STDIN)              { _eof = 0; };
		stream_wrapper(std::wifstream& f)     : source_type(IN_FILE),   file(&f)   { _eof = 0; };
		stream_wrapper(const std::wstring &s) : source_type(IN_STRING), string(&s) { _eof = 0; };
		
		// Returns next redden character.
		int getc();
		
		// Returns true whatewer EOF is reached.
		bool eof();
	};
	
	class parser;
	class tokenizer {
	public:
		// Wrapper for input stream. Created by parser, passed here.
		stream_wrapper& sw;
		
		parser_massages& messages;
		
		// Temporary token to be returned.
		raw_token *token = nullptr;
		
		// Buffer for redden codes
		int buffer[9] = { 0,0,0,0,0,0,0,0,0 };
		
		int has_error = 0;
		int lineno = 0;
		int charno = 0;
		int has_eof = 0;
		
		bool repl = 0;
		
		friend class parser;
		
		explicit tokenizer(parser_massages& pm, stream_wrapper& _sw) : messages(pm), sw(_sw), repl(0) {
			this->token = new raw_token();
			
			this->token->lineno = 1;
			this->token->charno = 1;
			this->lineno = 1;
			this->charno = 1;
			
			next(); next(); next(); next(); next(); 
		};
		
		~tokenizer() { delete token; };
		
		inline int white(int c) {
			return c == ck_token::TEOL || c == U'\n' || c == U'\r' || c == U'\t' || c == U' ' || c == 0x0A || c == 0x0D;
		};

		inline int alpha(int c) {
			return (c >= U'a' && c <= U'z') || (c >= U'A' && c <= U'Z') || (c >= U'а' && c <= U'я') || (c >= U'А' && c <= U'Я') || c == '_';
		};

		inline int digit(int c) {
			return c >= '0' && c <= '9';
		};

		inline char strtlc(char c) {
			return (c >= U'a' && c <= U'z') ? c + U'A' - U'a' : c;
		};

		inline int get(int off) {
			if (off > 4 || off < -4) {
				printf("PAJOJDA OTDEBAJJE MENYA\n");
				return -1;
			}
			return buffer[4 + off];
		};

		inline int next() {	
			if (buffer[4] != ck_token::TEOF)
				++charno;
			
			for (int i = 1; i < 9; i++)
				buffer[i - 1] = buffer[i];
			
			if (buffer[7] == ck_token::TEOF)
				buffer[8] = ck_token::TEOF;
			else {
				int c = sw.getc();
				
				if (c == WEOF)
					buffer[8] = ck_token::TEOF;
				else
					buffer[8] = c;
			}
			
			if (buffer[4] == U'\n') {
				++lineno;
				charno = 0;
				return ck_token::TEOL;
			}
			
			if (buffer[4] == ck_token::TEOF)
				has_eof = 1;
			
			return buffer[4];
		};

		inline int put(int token) {
			this->token->token = token;
			
			if (token == ck_token::TRUE || token == ck_token::FALSE) {
				this->token->bv = token == ck_token::TRUE;
				this->token->token = ck_token::BOOLEAN;
			}
			
			if (token == ck_token::TEOF)
				return 0;
			
			return 1;
		};

		inline int match(int charcode0) {
			if (get(0) != charcode0)
				return false;
			next();
			return true;
		};

		inline int match(int charcode0, int charcode1) {
			if (get(0) != charcode0 || get(1) != charcode1)
				return false;
			next(); next();
			return true;
		};

		inline int match(int charcode0, int charcode1, int charcode2) {
			if (get(0) != charcode0 || get(1) != charcode1 || get(2) != charcode2)
				return false;
			next(); next(); next();
			return true;
		};

		inline int match(int charcode0, int charcode1, int charcode2, int charcode3) {
			if (get(0) != charcode0 || get(1) != charcode1 || get(2) != charcode2 || get(3) != charcode3)
				return false;
			next(); next(); next(); next();
			return true;
		};

		inline void clear() {
			token->token = ck_token::NONE;
			token->iv = 0;
			token->dv = 0.0;
			token->bv = false;
			token->sv = L"";
		};

		inline int eof() { 
			return has_eof && sw.eof(); 
		};
		
		int next_token();
		
		inline void set_repl(bool _repl) { 
			repl = _repl; 
		};
	};
	
	class parser {
		// Container for all messages produced on parsing.
		parser_massages& messages;
		
		// Input token stream.
		tokenizer source;
		
		// Wrapper for an input stream
		stream_wrapper& swrapper;
		
		int   eof_ = 0;
		int error_ = 0;
		
		bool repl = 0;
		
		raw_token* buffer[7] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL };
		
	public:
		/*parser(parser_massages& msg, const std::wstring &str) : repl(0), messages(msg), source(msg, swrapper = stream_wrapper(str)) { // <-- input from string
			//swrapper = stream_wrapper(str);
			//source = tokenizer(messages, swrapper);
			next(); next(); next(); next();
		};
		*/
		parser(parser_massages& msg, stream_wrapper& sw) : repl(0), messages(msg), swrapper(sw), source(msg, sw) {
			next(); next(); next(); next();
		};
	
		~parser();
		
		raw_token *get(int off);
		
		raw_token *next();
		
		bool match(int token);
		
		bool match(int token0, int token1);
		
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
		
		bool checkNullExpression(ck_ast::ASTNode *exp);
		
		ck_ast::ASTNode *initializerstatement();
		
		bool peekStatementWithoutSemicolon();
		
		ck_ast::ASTNode *statement();
		
		ck_ast::ASTNode *statement_with_semicolons();
		
		// Persorms parsing of the entire input as a source.
		ck_ast::ASTNode *parse();
		
		// Performs parsing of a single statement as an expression.
		// Used in REPL console.
		// Input is terminated when statement is completed.
		ck_ast::ASTNode *parse_single_statement();
		
		int lineno();
		
		int charno();
		
		int eof();
		
		void set_repl(bool _repl) { repl = _repl; source.set_repl(1); };
	};
};

