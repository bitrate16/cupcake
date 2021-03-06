#include <cwchar>
#include <sstream>
#include <fstream>

#include "parser.h"
#include "ast.h"

#define TOKENIZER_ERROR(...) { messages.error(__VA_ARGS__); has_error = 1; return put(TERR); }
#define TOKENIZER_WARNING(...) messages.warning(__VA_ARGS__);

#define PARSER_ERROR_RETURN(...) { messages.error(__VA_ARGS__); error_ = 1; return NULL; }
#define PARSER_ERROR(...) { messages.error(__VA_ARGS__); error_ = 1; }
#define PARSER_WARNING(...) messages.warning(__VA_ARGS__);

using namespace ck_token;
using namespace ck_parser;
using namespace ck_ast;


std::wstring ck_parser::token_to_string(int token) {
	switch(token) {
		 case IF            : return L"if";
		 case ELSE          : return L"else";
		 case SWITCH        : return L"switch";
		 case CASE          : return L"case";
		 case DEFAULT       : return L"default";
		 case BREAK         : return L"break";
		 case CONTINUE      : return L"continue";
		 case RETURN        : return L"return";
		 case WHILE         : return L"while";
		 case DO            : return L"do";
		 case FOR           : return L"for";
		 case IN            : return L"in";
		 case FUNCTION      : return L"function";
		 case VAR           : return L"var";
		 case TRY           : return L"try";
		 case CATCH         : return L"catch";
		 case THROW         : return L"throw";
		 case CONST         : return L"const";
		 case SAFE          : return L"safe";
		 case LOCAL         : return L"local";
		 case TRUE          : return L"true";
		 case FALSE         : return L"false";
		 case SELF          : return L"sefl";
		 case THIS          : return L"this";
		 case WITH          : return L"with";
		 case TYPEOF        : return L"typeof";
		 case ISTYPEOF      : return L"istypeof";
		 case AS            : return L"as";
		 case ASSIGN        : return L"=";
		 case HOOK          : return L"?";
		 case COLON         : return L":";
		 case DOT           : return L".";
		 case COMMA         : return L",";
		 case SEMICOLON     : return L";";
		 case LP            : return L"(";
		 case RP            : return L")";
		 case LB            : return L"[";
		 case RB            : return L"]";
		 case LC            : return L"{";
		 case RC            : return L"}";
		 case EQ            : return L"==";
		 case NEQ           : return L"!=";
		 case LEQ           : return L"==";
		 case NLEQ          : return L"!=";
		 case OR            : return L"||";
		 case AND           : return L"&&";
		 case BITOR         : return L"|";
		 case BITAND        : return L"&";
		 case BITXOR        : return L"^";
		 case GT            : return L">";
		 case GE            : return L">=";
		 case LT            : return L"<";
		 case LE            : return L"<=";
		 case BITRSH        : return L">>";
		 case BITLSH        : return L"<<";
		 case BITURSH       : return L">>>";
		 case BITULSH       : return L"<<<";
		 case PLUS          : return L"+";
		 case MINUS         : return L"-";
		 case MUL           : return L"*";
		 case DIV           : return L"/";
		 case PATH          : return L"\\";
		 case DIR           : return L"\\\\";
		 case MOD           : return L"%";
		 case HASH          : return L"#";
		 case NOT           : return L"!";
		 case BITNOT        : return L"~";
		 case INC           : return L"++";
		 case DEC           : return L"--";
		 case ASSIGN_ADD    : return L"+=";
		 case ASSIGN_SUB    : return L"-=";
		 case ASSIGN_MUL    : return L"*=";
		 case ASSIGN_DIV    : return L"/=";
		 case ASSIGN_BITRSH : return L">>=";
		 case ASSIGN_BITLSH : return L"<<=";
		 case ASSIGN_BITURSH: return L">>>=";
		 case ASSIGN_BITULSH: return L"<<<=";
		 case ASSIGN_DIR    : return L"\\\\=";
		 case ASSIGN_PATH   : return L"\\=";
		 case ASSIGN_MOD    : return L"%=";
		 case ASSIGN_BITOR  : return L"|=";
		 case ASSIGN_BITAND : return L"&=";
		 case ASSIGN_BITXOR : return L"^=";
		 case PUSH          : return L"=>";
		 case ARROW         : return L"->";
		 case DOG           : return L"@";
		 case ASSIGN_HASH   : return L"#=";
		 case PRE_INC       : return L"++";
		 case PRE_DEC       : return L"--";
		 case POS_INC       : return L"++";
		 case POS_DEC       : return L"--";
		 case POS           : return L"-";
		 case NEG           : return L"+";
		 case TEOF          : return L"@EOF";
		 case TEOL          : return L"@EOL";
		 case TERR          : return L"@ERROR";
		 case NONE          : return L"@NONE";
		 case FIELD         : return L"@FIELD";
		 case MEMBER        : return L"@MEMBER";
		 case CALL          : return L"@CALL";
		 case EMPTY         : return L"@EMPTY";
		 case BLOCK         : return L"@BLOCK";
		 case DEFINE        : return L"@DEFINE";
		 case EXPRESSION    : return L"@EXPRESSION";
		 case ASTROOT       : return L"@ASTROOT";
		 case CONDITION     : return L"@CONDITION";		 
		 default: return L"";
	}
};


// S T R E A M _ W R A P P E R

// Returns next redden character.
int stream_wrapper::getc() {
	if (_eof == 1)
		return WEOF;
	
	int c;
	switch(source_type) {
		case IN_STRING:
			if (cursor >= string->size()) {
				_eof = 1;
				return WEOF;
			}
			return (*string)[cursor++];
		
		case IN_STDIN:
			c = fgetwc(stdin);
			if (c == WEOF)
				_eof = 1;
			
			return c;
			
		case IN_FILE: {
			wchar_t t;
			if (*file >> t)
				return t;
			
			c = WEOF;
			if (c == WEOF)
				_eof = 1;
			
			return c;
		}
		
		default:
			_eof = 1;
			return WEOF;
	};
};

// Returns true whatewer EOF is reached.
bool stream_wrapper::eof() {
	if (_eof == 1)
		return 1;
	
	int c;
	switch(source_type) {
		case IN_STRING:
			if (cursor >= string->size()) {
				_eof = 1;
				return 1;
			}
			return 0;
		
		case IN_STDIN:
			c = fgetwc(stdin);
			ungetwc(c, stdin);
			
			if (c == WEOF)
				_eof = 1;
			
			return _eof;
			
		case IN_FILE: {
			wchar_t t;
			_eof = (bool) (*file >> t);
			if (!_eof)
				file->unget();
			
			return _eof;
		}
		
		default:
			_eof = 1;
			return 1;
	};
};


// T O K E N I Z E R
// (It was written when i was drunk but it works unlike my liver..)

int to_upper(int c) {
	if ('a' <= c && c <= L'z')
		return c - L'a' + L'A';
	if (L'а' <= c && c <= L'я')
		return c - L'а' + L'А';
	return c;
};

int tokenizer::next_token() {
	token->lineno = lineno;
	token->charno = charno;
	
	if (has_error) 
		return put(TERR);
	if (eof()) 
		return put(TEOF);
	
	clear();

	int c  = get(0);
	int c1 = get(1);

	// Remove all whitescapes and comments
	while (white(c) || (c == L'/' && ((c1 = get(1)) == L'/' || c1 == L'*'))) {
		if (white(c)) {
			c = next();
			c1 = get(1);
		} else if (c == L'/' && c1 == L'/') {
			// Skip till newline || TEOF
			int c0 = c;
			while (c0 != TEOL && c != TEOF) {
				c0 = c;
				c = next();
				c1 = get(1);
			}
		} else if (c == L'/' && c1 == L'*') {
			// Skip all code till L'*' L'/' sequence
			int closed = false;
			while (c != TEOF) {
				c = next();
				c1 = get(1);

				if (c == L'*' && c1 == L'/') {
					closed = true;
					c = next();
					c = next();
					c1 = get(1);
					break;
				}
			}
			
			
			// Skip all code till L'*' L'/' sequence
			// Warning: L'*/' may be hidden in string. That must be ignored
			// int q0 = false; // L' - quote
			// int q1 = false; // " - quote
			// int closed = false;
			// // \/--- !(c == L'*' && c1 == L'/' && !q0 && !q1) &&
			// while (c != TEOF) {
			// 	c = next();
			// 	c1 = get(1);
			// 	if (c1 == L'\'' && !q1)
			// 		q0 = !q0;
			// 	if (c1 == L'\"' && !q0)
			// 		q1 = !q1;
			// 
			// 	if (c == L'*' && c1 == L'/' && !q0 && !q1) {
			// 		closed = true;
			// 		c = next();
			// 		c = next();
			// 		c1 = get(1);
			// 		break;
			// 	}
			// }

			if (!closed) TOKENIZER_ERROR(L"block comment expected to be closed", lineno, charno)
		}
	}

	if (get(0) == TEOF) {
		token->lineno = lineno;
		token->charno = charno;
		return put(TEOF);
	}

	token->lineno = lineno;
	token->charno = charno;
	
	/* K E Y W O R D S */ 
	if (alpha(c)) {
		std::wstring& svref = token->sv;
		do {
			svref += (wchar_t) c;
			c = next();
		} while((alpha(c) || digit(c)) && c != TEOF);

		if (svref == L"true")
			return put(TRUE);
		if (svref == L"false")
			return put(FALSE);
		if (svref == L"null")
			return put(TNULL);
		if (svref == L"undefined")
			return put(UNDEFINED);

		if (svref == L"this")
			return put(THIS);
		if (svref == L"self")
			return put(SELF);
		if (svref == L"try")
			return put(TRY);
		if (svref == L"catch")
			return put(CATCH);
		if (svref == L"throw")
			return put(THROW);
		if (svref == L"if")
			return put(IF);
		if (svref == L"else")
			return put(ELSE);
		if (svref == L"for")
			return put(FOR);
		if (svref == L"switch")
			return put(SWITCH);
		if (svref == L"case")
			return put(CASE);
		if (svref == L"default")
			return put(DEFAULT);
		if (svref == L"while")
			return put(WHILE);
		if (svref == L"do")
			return put(DO);
		if (svref == L"break")
			return put(BREAK);
		if (svref == L"continue")
			return put(CONTINUE);
		if (svref == L"return")
			return put(RETURN);
		if (svref == L"function")
			return put(FUNCTION);
		if (svref == L"var")
			return put(VAR);
		if (svref == L"const")
			return put(CONST);
		if (svref == L"safe")
			return put(SAFE);
		if (svref == L"local")
			return put(LOCAL);
		if (svref == L"with")
			return put(WITH);
		if (svref == L"as")
			return put(AS);
		if (svref == L"typeof")
			return put(TYPEOF);
		if (svref == L"istypeof")
			return put(ISTYPEOF);
		if (svref == L"in")
			return put(IN);

		return put(NAME);
	}

	/* S T R I N G S */ 
	if (c == L'\'' || c == L'\"') {
		int quote = c;
		std::wstring& svref = token->sv;

		c = next();
		while (c != TEOF) {
			if (c == TEOL || c == TEOF) TOKENIZER_ERROR(L"string expected to be closed", lineno, charno)
			if (c == quote)
				break;
			if (c == L'\\') {
				c1 = next();
				if (c1 == L't')
					svref += L'\t';
				else if (c1 == L'b')
					svref += L'\b';
				else if (c1 == L'n')
					svref += L'\n';
				else if (c1 == L'r')
					svref += L'\r';
				else if (c1 == L'f')
					svref += L'\f';
				else if (c1 == L'\\')
					svref += L'\\';
				else if (c1 == L'\'')
					svref += L'\'';
				else if (c1 == L'\"')
					svref += L'\"';
				else if (c1 == L'0')
					svref += L'\0';
				else if (c1 == L'u') {
					c1 = next();
					// Expect 8-digit hex number
					int chpoint = 0;
					int point   = 0;
					for (int i = 0; i < 8; i++) {
						c1 = strtlc(c1);
						int cp = -1;
						if (c1 >= L'a' && c1 <= L'f')
							cp = 10 + c1 - L'a';
						if (c1 >= L'0' && c1 <= L'9')
							cp = c1 - L'0';
						if (cp == -1 && !point) 
							TOKENIZER_ERROR(std::wstring(L"expected hexadecimal character code point ") + (wchar_t) cp + std::wstring(L" [") + (wchar_t) cp + std::wstring(L"]"), lineno, charno)
						if (cp == -1)
							break;
						++point;
						chpoint = chpoint * 16 + cp;
						c1 = next();
					}
				} else if (c1 == L'x') {
					c1 = next();
					// Expect 4-digit hex number
					int chpoint = 0;
					int point   = 0;
					for (int i = 0; i < 4; i++) {
						c1 = strtlc(c1);
						int cp = -1;
						if (c1 >= L'a' && c1 <= L'f')
							cp = 10 + c1 - L'a';
						if (c1 >= L'0' && c1 <= L'9')
							cp = c1 - L'0';
						if (cp == -1 && !point) 
							TOKENIZER_ERROR(std::wstring(L"expected hexadecimal character code point ") + (wchar_t) cp + std::wstring(L" [") + (wchar_t) cp + std::wstring(L"]"), lineno, charno)
						if (cp == -1)
							break;
						++point;
						chpoint = chpoint * 16 + cp;
						c1 = next();
					}
				} else
					TOKENIZER_ERROR(std::wstring(L"unexpected character after escape point ") + (wchar_t) c1 + std::wstring(L" [") + (wchar_t) c1 + std::wstring(L"]"), lineno, charno)
			} else
				svref += (wchar_t) c;
			c = next();
		}
		next();
		return put(STRING);
	}
	
	/* M U L T I L I N E _ S T R I N G S */ 
	if (c == L'`') {
		int quote = c;
		std::wstring& svref = token->sv;

		c = next();
		while (c != TEOF) {
			if (c == TEOF) { TOKENIZER_ERROR(L"string expected to be closed", lineno, charno); }
			else if (c == TEOL)
				svref += L'\n';
			else if (c == quote)
				break;
			else if (c == L'\\') {
				c1 = next();
				if (c1 == L't')
					svref += L'\t';
				else if (c1 == L'b')
					svref += L'\b';
				else if (c1 == L'n')
					svref += L'\n';
				else if (c1 == L'r')
					svref += L'\r';
				else if (c1 == L'f')
					svref += L'\f';
				else if (c1 == L'\\')
					svref += L'\\';
				else if (c1 == L'\'')
					svref += L'\'';
				else if (c1 == L'\"')
					svref += L'\"';
				else if (c1 == L'0')
					svref += L'\0';
				else if (c1 == L'u') {
					c1 = next();
					// Expect 8-digit hex number
					int chpoint = 0;
					int point   = 0;
					for (int i = 0; i < 8; i++) {
						c1 = strtlc(c1);
						int cp = -1;
						if (c1 >= L'a' && c1 <= L'f')
							cp = 10 + c1 - L'a';
						if (c1 >= L'0' && c1 <= L'9')
							cp = c1 - L'0';
						if (cp == -1 && !point) 
							TOKENIZER_ERROR(std::wstring(L"expected hexadecimal character code point ") + (wchar_t) cp + std::wstring(L" [") + (wchar_t) cp + std::wstring(L"]"), lineno, charno)
						if (cp == -1)
							break;
						++point;
						chpoint = chpoint * 16 + cp;
						c1 = next();
					}
				} else if (c1 == L'x') {
					c1 = next();
					// Expect 4-digit hex number
					int chpoint = 0;
					int point   = 0;
					for (int i = 0; i < 4; i++) {
						c1 = strtlc(c1);
						int cp = -1;
						if (c1 >= L'a' && c1 <= L'f')
							cp = 10 + c1 - L'a';
						if (c1 >= L'0' && c1 <= L'9')
							cp = c1 - L'0';
						if (cp == -1 && !point) 
							TOKENIZER_ERROR(std::wstring(L"expected hexadecimal character code point ") + (wchar_t) cp + std::wstring(L" [") + (wchar_t) cp + std::wstring(L"]"), lineno, charno)
						if (cp == -1)
							break;
						++point;
						chpoint = chpoint * 16 + cp;
						c1 = next();
					}
				} else
					TOKENIZER_ERROR(std::wstring(L"unexpected character after escape point ") + (wchar_t) c1 + std::wstring(L" [") + (wchar_t) c1 + std::wstring(L"]"), lineno, charno)
			} else
				svref += (wchar_t) c;
			
			c = next();
		}
		next();
		return put(STRING);
	}
	
	/* N U M B E R S */ 
	if (digit(c) || (c == L'.' && digit(c1))) {
		
		
		// XXX: Rewrite this code part to optimize/fix parser bugs
		
		
		int type = INTEGER;
		int hasPoint = false;
		int scientific = false;
		int base = 10;

		std::wstring& svref = token->sv;
		
		if (c == L'.') {
			type = DOUBLE;
			hasPoint = true;
		}

		svref += (wchar_t) c;

		c = next();
		c1 = get(1);
		while (c != TEOF) {
			if (scientific)
				TOKENIZER_ERROR(L"double can't be parsed after scientific notation", lineno, charno)
			else if (c == L'.') {
				if (hasPoint)
					TOKENIZER_ERROR(L"double can't have more than one point", lineno, charno)
				// if (base != 10)
				//	return tokenizer_error(lineno, "double can't be not decimal");
				if (!digit(c1))
					TOKENIZER_ERROR(L"unexpected fraction of double nomber", lineno, charno)
				type = DOUBLE;
				hasPoint = true;
				svref += L'.';
			} else if ((c == L'x' || c == L'X') && base == 10) {
				if (type == DOUBLE)
					TOKENIZER_ERROR(L"double can't be not decimal", lineno, charno)
				// if (base != 10)
				//	return tokenizer_error(lineno, "one token.iv can't have multiple numerical bases");
				if (svref != L"0")
					TOKENIZER_ERROR(L"base notation starts with 0", lineno, charno)
				svref = L"";
				base = 16;
			} else if ((c == L'o' || c == L'O') && base == 10) {
				if (type == DOUBLE)
					TOKENIZER_ERROR(L"double can't be not decimal", lineno, charno)
				// if (base != 10)
				//	return tokenizer_error(lineno, "one token.iv can't have multiple numerical bases");
				if (svref != L"0")
					TOKENIZER_ERROR(L"base notation starts with 0", lineno, charno)
				svref = L"";
				base = 8;
			} else if ((c == L'b' || c == L'B') && base == 10) {
				if (type == DOUBLE)
					TOKENIZER_ERROR(L"double can't be not decimal", lineno, charno)
				// if (base != 10)
				//	return tokenizer_error(lineno, "integer can't have multiple numerical bases");
				if (svref != L"0")
					TOKENIZER_ERROR(L"base notation starts with 0", lineno, charno)
				svref = L"";
				base = 2;
			} else if ((c == L'e' || c == L'E') && base != 16) {
				if (base != 10)
					TOKENIZER_ERROR(L"double can't be not decimal", lineno, charno)
				type = DOUBLE;
				// Start reading scientific notation
				// double + E + (+ or -) + integer
				svref += L'e';
				c = next();
				if ((c == L'-' || c == L'+')) {
					// 12.34E26 is the same as 12.34E+26
					svref += (wchar_t) c;
					c = next();
				}
				std::wstring integer;
				while (digit(c)) {
					integer += (wchar_t) c;
					c = next();
				}
				if (integer == L"")
					TOKENIZER_ERROR(L"expected exponent in scientific notation", lineno, charno)
				if (alpha(c))
					TOKENIZER_ERROR(L"unexpected character in scientific notation", lineno, charno)
				svref += integer;
				scientific = true;
				break;
			} /*else if ((c == L'l' || c == L'L' || (c == L'b' || c == L'B') && base != 16) && !digit(c1)) {
				if (c == L'l' || c == L'L')
					type = LONG;
				else
					type = BYTE;
				c = next();
				break;
			} */ else if (base == 16 && (('A' <= c && c <= L'F') || ('a' <= c && L'f' <= c) || digit(c)))
				svref += (wchar_t) c;
			else if (base == 8 && L'0' <= c && c <= L'7')
				svref += (wchar_t) c;
			else if (base == 2 && (c == L'0' || c == L'1'))
				svref += (wchar_t) c;
			else if (base == 10 && digit(c))
				svref += (wchar_t) c;
			else if (alpha(c) || (base == 8 && L'8' <= c && c <= L'9')
							  || (base == 2 && L'3' <= c && c <= L'9'))
				TOKENIZER_ERROR(std::wstring(L"unexpected character in number ") + (wchar_t) c + std::wstring(L" [") + (wchar_t) c + std::wstring(L"]"), lineno, charno)
			else
				break;

			c = next();
			c1 = get(1);
		}
		
		if (type == DOUBLE) {
			std::wstringstream ss(svref);
			ss >> token->dv;
			return put(DOUBLE);
		} else if (type == INTEGER) {
			int64_t mult = 1;
			for (int i = svref.size() - 1; i >= 0; --i) {
				token->iv += mult * ('A' <= to_upper(svref[i]) && to_upper(svref[i]) <= L'Z' ? to_upper(svref[i]) - L'A' + 10 : to_upper(svref[i]) - L'0');
				mult *= base;
			}
			return put(INTEGER);
		}
	}
	
	/* D E L I M I T E R S */ 
	if (match('>', L'>', L'>', L'='))
		return put(ASSIGN_BITURSH);
	if (match('<', L'<', L'<', L'='))
		return put(ASSIGN_BITULSH);
	if (match('>', L'>', L'>'))
		return put(BITURSH);
	if (match('<', L'<', L'<'))
		return put(BITULSH);
	if (match('>', L'>', L'='))
		return put(ASSIGN_BITRSH);
	if (match('<', L'<', L'='))
		return put(ASSIGN_BITLSH);
	if (match('\\', '\\', L'='))
		return put(ASSIGN_DIR);
	if (match('\\', L'='))
		return put(ASSIGN_PATH);
	if (match('>', L'>'))
		return put(BITRSH);
	if (match('<', L'<'))
		return put(BITLSH);
	if (match('>', L'='))
		return put(GE);
	if (match('<', L'='))
		return put(LE);
	if (match('&', L'&'))
		return put(AND);
	if (match('|', L'|'))
		return put(OR);
	if (match('=', L'=', L'='))
		return put(LEQ);
	if (match('!', L'=', L'='))
		return put(NLEQ);
	if (match('=', L'='))
		return put(EQ);
	if (match('!', L'='))
		return put(NEQ);
	if (match('-', L'>'))
		return put(PUSH);
	if (match('=', L'>'))
		return put(ARROW);
	if (match('\\', L'\\'))
		return put(DIR);
	if (match('+', L'+'))
		return put(INC);
	if (match('-', L'-'))
		return put(DEC);
	if (match('+', L'='))
		return put(ASSIGN_ADD);
	if (match('-', L'='))
		return put(ASSIGN_SUB);
	if (match('*', L'='))
		return put(ASSIGN_MUL);
	if (match('/', L'='))
		return put(ASSIGN_DIV);
	if (match('%', L'='))
		return put(ASSIGN_MOD);
	if (match('|', L'='))
		return put(ASSIGN_BITOR);
	if (match('&', L'='))
		return put(ASSIGN_BITAND);
	if (match('^', L'='))
		return put(ASSIGN_BITXOR);
	if (match('='))
		return put(ASSIGN);
	if (match('>'))
		return put(GT);
	if (match('<'))
		return put(LT);
	if (match('+'))
		return put(PLUS);
	if (match('-'))
		return put(MINUS);
	if (match('*'))
		return put(MUL);
	if (match('/'))
		return put(DIV);
	if (match('%'))
		return put(MOD);
	if (match('~'))
		return put(BITNOT);
	if (match('|'))
		return put(BITOR);
	if (match('&'))
		return put(BITAND);
	if (match('^'))
		return put(BITXOR);
	if (match('!'))
		return put(NOT);
	if (match('.'))
		return put(DOT);
	if (match('?'))
		return put(HOOK);
	if (match(':'))
		return put(COLON);
	if (match('\\'))
		return put(PATH);
	if (match('['))
		return put(LB);
	if (match(']'))
		return put(RB);
	if (match('{'))
		return put(LC);
	if (match('}'))
		return put(RC);
	if (match('('))
		return put(LP);
	if (match(')'))
		return put(RP);
	if (match(','))
		return put(COMMA);
	if (match(';'))
		return put(SEMICOLON);
	if (match('#'))
		return put(HASH);
	if (match('@'))
		return put(DOG);
	if (match('#', L'='))
		return put(ASSIGN_HASH);
	
	TOKENIZER_ERROR(std::wstring(L"unexpected character L'") + (wchar_t) c + std::wstring(L"' [") + std::to_wstring(c) + std::wstring(L"]"), lineno, charno)
};


// P A R S E R
parser::~parser() {
	for (int i = 0; i < 7; i++)
		delete this->buffer[i];
};

raw_token *parser::get(int off) {
	if (off > 3 || off < -3) {
		wprintf(L"PAJOJDA OTDEBAJJE MENYA\n");
		return nullptr;
	}
	return buffer[3 + off];
};

raw_token *parser::next() {
	if (this->buffer[0] != NULL)
		delete this->buffer[0];
	
	for (int i = 1; i < 7; i++)
		this->buffer[i - 1] = this->buffer[i];
	
	this->source.next_token();
	//if (this->source->token->token == TERR)
	//	noline_parser_error("TS error");
	
	this->buffer[6] = new raw_token(*this->source.token);
	
	if (buffer[3] != NULL && buffer[3]->token == TEOF)
		eof_ = 1;
	
	return buffer[3];
};

bool parser::match(int token) {
	if (error_)
		return 0;
	
	if (get(0)->token != token)
		return 0;
	
	next();
	return 1;	
};

bool parser::match(int token0, int token1) {
	if (error_)
		return 0;
	
	if (get(0)->token != token0 || get(1)->token != token1)
		return 0;
	
	next();
	next();
	return 1;	
};

ASTNode *parser::primaryexp() {
	if (match(TEOF)) 
		PARSER_ERROR_RETURN(L"EOF Expression", get(0)->lineno, get(0)->charno);
	
	if (match(INTEGER)) {
		
		// OBJECTS:
		// value
		
		ASTNode *integerexp = new ASTNode(get(-1)->lineno, INTEGER);
		
		int64_t *n = new int64_t;
		*n = get(-1)->iv;
		integerexp->addLastObject(n);
		return integerexp;
	}
	
	if (match(BOOLEAN)) {
		
		// OBJECTS:
		// value
		ASTNode *booleanexp = new ASTNode(get(-1)->lineno, BOOLEAN);
		
		bool *n = new bool;
		*n = get(-1)->bv;
		booleanexp->addLastObject(n);
		
		return booleanexp;
	}
	
	if (match(DOUBLE)) {
		
		// OBJECTS:
		// value
		ASTNode *doubleexp = new ASTNode(get(-1)->lineno, DOUBLE);
		
		double *n = new double;
		*n = get(-1)->dv;
		doubleexp->addLastObject(n);
		
		return doubleexp;
	}
	
	if (match(NAME)) {
		
		// OBJECTS:
		// value
		ASTNode *nameexp = new ASTNode(get(-1)->lineno, NAME);
		
		nameexp->addLastObject(new std::wstring(get(-1)->sv));
		
		return nameexp;
	}
	
	if (match(STRING)) {
		
		// OBJECTS:
		// value
		ASTNode *stringexp = new ASTNode(get(-1)->lineno, STRING);
		
		stringexp->addLastObject(new std::wstring(get(-1)->sv));
		
		return stringexp;
	}
	
	if (match(THIS)) {
		ASTNode *thisexp = new ASTNode(get(-1)->lineno, THIS);
		
		return thisexp;
	}
	
	if (match(SELF)) {
		ASTNode *selfexp = new ASTNode(get(-1)->lineno, SELF);
		
		return selfexp;
	}
	
	if (match(TNULL)) {
		ASTNode *nullexp = new ASTNode(get(-1)->lineno, TNULL);
		
		return nullexp;
	}
	
	if (match(UNDEFINED)) {
		ASTNode *undefinedexp = new ASTNode(get(-1)->lineno, UNDEFINED);
		
		return undefinedexp;
	}
	
	if (match(LP)) {
		ASTNode *exp = expression();
		
		if (exp == NULL)
			return NULL;

		if (!match(RP)) {
			delete exp;
			PARSER_ERROR_RETURN(L"Expected )", get(0)->lineno, get(0)->charno);
		}
		
		return exp;
	}
	
	if (match(LB)) {
		// FRAME:
		// elem0
		// ...
		// elemN
		// OBJECTS:
		// element count
		//   _      _      _
        // >(.)__ <(.)__ =(.)__
        //  (___/  (___/  (___/  krya
		
		
		ASTNode *array = new ASTNode(get(-1)->lineno, ARRAY);
		int *length = new int;
		array->addLastObject(length);
		*length = 0;
		
		if (match(RB))
			return array;
		
		while (true) {
			ASTNode *elem = expression();
			if (checkNullExpression(elem))  {
				delete array;
				return NULL;
			}
			
			array->addLastChild(elem);
			++*length;
			
			if (match(TEOF)) {
				delete array;
				PARSER_ERROR_RETURN(L"Expected ]", get(0)->lineno, get(0)->charno);
			}
			if (!match(COMMA)) {
				if (!match(RB)) {
					delete array;
					PARSER_ERROR_RETURN(L"Expected ]", get(0)->lineno, get(0)->charno);
				} else
					break;
			}
		}
		
		return array;
	}
	
	if (match(LC)) {
		// FRAME:
		// object0
		// ...
		// objectN
		// OBJECTS:
		// name0
		// ...
		// nameN
		
		
		ASTNode *object = new ASTNode(get(-1)->lineno, OBJECT);
		
		if (match(RC))
			return object;
		
		while (true) {
		if (!(match(NAME) || match(STRING))) {
				delete object;
				PARSER_ERROR_RETURN(L"Expected name or string", get(0)->lineno, get(0)->charno);
			}
			
			std::wstring *name = new std::wstring(get(-1)->sv);
			
			if (!match(COLON)) {
				delete name;
				delete object;
				PARSER_ERROR_RETURN(L"Expected :", get(0)->lineno, get(0)->charno);
			}
			
			ASTNode *elem = expression();
			if (checkNullExpression(elem))  {
				delete name;
				delete object;
				return NULL;
			}
			
			object->addFirstObject(name);
			object->addChild(elem);
			
			if (match(TEOF)) {
				delete object;
				PARSER_ERROR_RETURN(L"Expected }", get(0)->lineno, get(0)->charno);
			}
			if (!match(COMMA)) {
				if (!match(RC)) {
					delete object;
					PARSER_ERROR_RETURN(L"Expected }", get(0)->lineno, get(0)->charno);
				} else
					break;
			}
		}
		
		return object;
	}
	
	if (match(FUNCTION)) {
		// FRAME:
		// block
		// OBJECTS:
		// arg0
		// ...
		// argN
		
		
		ASTNode *function = new ASTNode(get(-1)->lineno, FUNCTION);
		
		if (match(LP)) 
			if (!match(RP))
				while (true) {
					if (!match(NAME)) {
						delete function;
						PARSER_ERROR_RETURN(L"Expected name", get(0)->lineno, get(0)->charno);
					}
					
					function->addLastObject(new std::wstring(get(-1)->sv));
					
					if (match(TEOF)) {
						delete function;
						PARSER_ERROR_RETURN(L"Expected )", get(0)->lineno, get(0)->charno);
					}
					if (!match(COMMA)) {
						if (!match(RP)) {
							delete function;
							PARSER_ERROR_RETURN(L"Expected )", get(0)->lineno, get(0)->charno);
						} else
							break;
					}
				}
				
		function->addChild(statement());
		
		return function;
	}
	
	std::wstring error_str = L"unexpected token type " + std::to_wstring(get(0)->token) + L": ";
	switch (get(0)->token) {
		case INTEGER:
			error_str += std::to_wstring(get(0)->iv);
			break;
		case DOUBLE:
			error_str += std::to_wstring(get(0)->dv);
			break;
		case BOOLEAN:
			error_str += get(0)->bv ? L"true" : L"false";
			break;
		case STRING:
			error_str += L"\"" + get(0)->sv + L"\"";
			break;
		case NAME:
			error_str += get(0)->sv;
			break;
		case TNULL:
			error_str += L"null";
			break;
		case UNDEFINED:
			error_str += L"undefined";
			break;
		default:
			error_str += token_to_string(get(0)->token);
	}
	
	PARSER_ERROR_RETURN(error_str, get(0)->lineno, get(0)->charno)
};

ASTNode *parser::member_expression() {

	// Stack all <primary_exp>[]..[]()..()
	
	ASTNode *exp = primaryexp();
	if (checkNullExpression(exp)) 
		return NULL;
	
	while (match(LB) || match(LP) || match(DOT)) {
		int lineno = get(-1)->lineno;
		
		switch (get(-1)->token) {
			case LB: {
				// FRAME:
				// reference
				// index expression
				
				ASTNode *ind = expression();
				if (checkNullExpression(ind)) {
					delete exp;
					return NULL;
				}
				
				ASTNode *node = new ASTNode(lineno, MEMBER);
				node->addChild(exp);
				node->addChild(ind);
				exp = node;
				
				if (!match(RB)) {
					delete exp;
					PARSER_ERROR_RETURN(L"Expected ]", get(0)->lineno, get(0)->charno);
				}
				
				break;
			}
			
			case DOT: {
				// FRAME:
				// reference
				// OBJECTS:
				// field_name
				
				ASTNode *node = new ASTNode(lineno, FIELD);
				node->addChild(exp);
				exp = node;
				
				if (!match(NAME)) {
					delete exp;
					PARSER_ERROR_RETURN(L"Expected name", get(0)->lineno, get(0)->charno);
				}
				node->addLastObject(new std::wstring(get(-1)->sv));
				
				break;
			}
			
			case LP: {
				// FRAME:
				// reference
				// arg0
				// ...
				// argn
				
				ASTNode *node = new ASTNode(lineno, CALL);
				node->addChild(exp);
				exp = node;
				
				if (match(RP))
					break;
				
				while (true) {
					ASTNode *arg = expression();
					if (checkNullExpression(arg))  {
						delete exp;
						return NULL;
					}
					
					exp->addChild(arg);
					
					if (match(TEOF)) {
						delete exp;
						PARSER_ERROR_RETURN(L"Expected )", get(0)->lineno, get(0)->charno);
					}
					if (!match(COMMA)) {
						if (!match(RP)) {
							delete exp;
							PARSER_ERROR_RETURN(L"Expected )", get(0)->lineno, get(0)->charno);
						} else
							break;
					}
				}
				
				break;
			}
		}
	}
	
	return exp;
};

ASTNode *parser::unary_expression() {
	
	// FRAME:
	// exp
	
	if (match(NOT) || match(BITNOT) || match(PLUS) || match(MINUS) || match(DOG)) {
		// ! EXP | ~EXP | -EXP | +EXP | @EXP
		
		int token = get(-1)->token;
		int lineno = get(-1)->lineno;
		
		ASTNode *exp = unary_expression();
		if (checkNullExpression(exp)) 
			return NULL;
		
		ASTNode *expr = new ASTNode(lineno, token == PLUS ? POS : token == MINUS ? NEG : token);
		expr->addChild(exp);
		
		return expr;						
	} else if (match(TYPEOF)) { 
		// typeof EXP
		
		int token = get(-1)->token;
		int lineno = get(-1)->lineno;
		
		ASTNode *exp = expression();
		if (checkNullExpression(exp)) 
			return NULL;
		
		ASTNode *expr = new ASTNode(lineno, token);
		expr->addChild(exp);
		
		return expr;						
	} else if (match(INC) || match(DEC)) {
		// ++ EXP
		
		int token = get(-1)->token;
		int lineno = get(-1)->lineno;
		
		ASTNode *exp = member_expression();
		if (checkNullExpression(exp)) 
			return NULL;
		
		if (exp->type == NAME
			||
			exp->type == FIELD
			||
			exp->type == MEMBER) 
		{
			ASTNode *expr = new ASTNode(lineno, token == INC ? PRE_INC : PRE_DEC);
			expr->addChild(exp);
			
			return expr;			
		} else {
			delete exp;
			PARSER_ERROR_RETURN(L"Left side of the increment expected to be field", get(0)->lineno, get(0)->charno);
		}
	} else {
		// EXP | EXP ++
		
		ASTNode *exp = member_expression();
		if (checkNullExpression(exp)) 
			return NULL;
		
		if (match(INC) || match(DEC)) {
			
			if (exp->type == NAME
				||
				exp->type == FIELD
				||
				exp->type == MEMBER) 
			{
				ASTNode *expr = new ASTNode(get(-1)->lineno, get(-1)->token == INC ? POS_INC : POS_DEC);
				expr->addChild(exp);
				
				return expr;
			} else {
				delete exp;
				PARSER_ERROR_RETURN(L"Right side of the increment expected to be field", get(0)->lineno, get(0)->charno);
			}
		} return exp;
	}
};

ASTNode *parser::multiplication_expression() {
	
	// FRAME:
	// exp1
	// exp2
	
	// key in a
	if (match(STRING, IN) || match(NAME, IN)) {
		// objects: key
		// nodes: expr
		std::wstring str = get(-2)->sv;
		
		ASTNode *left = expression();
		if (checkNullExpression(left)) 
			return NULL;
		
		ASTNode *inexpr = new ASTNode(get(-1)->lineno, IN);
		inexpr->addLastObject(new std::wstring(str));
		inexpr->addChild(left);
		
		return inexpr;
	}
	
	// expr <op> expr
	ASTNode *left = unary_expression();
	if (checkNullExpression(left)) 
		return NULL;
	
	while (1) {
		if (match(MUL) || match(DIV) || match(HASH) || match(DIR) || match(MOD) || match(HASH) || match(PUSH) || match(AS) || match(ISTYPEOF)) {
			ASTNode *exp   = new ASTNode(get(-1)->lineno, get(-1)->token);
			exp->addChild(left);
			ASTNode *right = unary_expression();
			
			if (checkNullExpression(right)) {
				delete exp;
				return NULL;
			}
			
			exp->addChild(right);
			left = exp;
			continue;
		}
		break;
	}
	
	return left;
};

ASTNode *parser::addiction_expression() {
	
	// FRAME:
	// exp1
	// exp2
	
	ASTNode *left = multiplication_expression();
	if (checkNullExpression(left)) 
		return NULL;
	
	while (1) {
		if (match(PLUS) || match(MINUS) || match(PATH) || match(ARROW)) {
			ASTNode *exp   = new ASTNode(get(-1)->lineno, get(-1)->token);
			exp->addChild(left);
			ASTNode *right = multiplication_expression();
			
			if (checkNullExpression(right)) {
				delete exp;
				return NULL;
			}
			
			exp->addChild(right);
			left = exp;
			continue;
		}
		break;
	}
	
	return left;
};

ASTNode *parser::bitwise_shift_expression() {
	
	// FRAME:
	// exp1
	// exp2
	
	ASTNode *left = addiction_expression();
	if (checkNullExpression(left)) 
		return NULL;
	
	while (1) {
		if (match(BITRSH) || match(BITLSH) || match(BITURSH) || match(BITULSH)) {
			ASTNode *exp   = new ASTNode(get(-1)->lineno, get(-1)->token);
			exp->addChild(left);
			ASTNode *right = addiction_expression();
			
			if (checkNullExpression(right)) {
				delete exp;
				return NULL;
			}
			
			exp->addChild(right);
			left = exp;
			continue;
		}
		break;
	}
	
	return left;
};

ASTNode *parser::comparison_expression() {
	
	// FRAME:
	// exp1
	// exp2
	
	ASTNode *left = bitwise_shift_expression();
	if (checkNullExpression(left)) 
		return NULL;
	
	while (1) {
		if (match(GT) || match(GE) || match(LT) || match(LE)) {
			ASTNode *exp   = new ASTNode(get(-1)->lineno, get(-1)->token);
			exp->addChild(left);
			ASTNode *right = bitwise_shift_expression();
			
			if (checkNullExpression(right)) {
				delete exp;
				return NULL;
			}
			
			exp->addChild(right);
			left = exp;
			continue;
		}
		break;
	}
	
	return left;
};

ASTNode *parser::equality_expression() {
	
	// FRAME:
	// exp1
	// exp2
	
	ASTNode *left = comparison_expression();
	if (checkNullExpression(left)) 
		return NULL;
	
	while (1) {
		if (match(EQ) || match(NEQ) || match(LEQ) || match(NLEQ)) {
			ASTNode *exp   = new ASTNode(get(-1)->lineno, get(-1)->token);
			exp->addChild(left);
			ASTNode *right = comparison_expression();
			
			if (checkNullExpression(right)) {
				delete exp;
				return NULL;
			}
			
			exp->addChild(right);
			left = exp;
			continue;
		}
		break;
	}
	
	return left;
};

ASTNode *parser::bitwise_and_expression() {
	
	// FRAME:
	// exp1
	// exp2
	
	ASTNode *left = equality_expression();
	if (checkNullExpression(left)) 
		return NULL;
	
	while (1) {
		if (match(BITAND)) {
			ASTNode *exp   = new ASTNode(get(-1)->lineno, get(-1)->token);
			exp->addChild(left);
			ASTNode *right = equality_expression();
			
			if (checkNullExpression(right)) {
				delete exp;
				return NULL;
			}
			
			exp->addChild(right);
			left = exp;
			continue;
		}
		break;
	}
	
	return left;
};

ASTNode *parser::bitwise_xor_exppression() {
	
	// FRAME:
	// exp1
	// exp2
	
	ASTNode *left = bitwise_and_expression();
	if (checkNullExpression(left)) 
		return NULL;
	
	while (1) {
		if (match(BITXOR)) {
			ASTNode *exp   = new ASTNode(get(-1)->lineno, get(-1)->token);
			exp->addChild(left);
			ASTNode *right = bitwise_and_expression();
			
			if (checkNullExpression(right)) {
				delete exp;
				return NULL;
			}
			
			exp->addChild(right);
			left = exp;
			continue;
		}
		break;
	}
	
	return left;
};

ASTNode *parser::bitwise_or_expression() {
	
	// FRAME:
	// exp1
	// exp2
	
	ASTNode *left = bitwise_xor_exppression();
	if (checkNullExpression(left)) 
		return NULL;
	
	while (1) {
		if (match(BITOR)) {
			ASTNode *exp   = new ASTNode(get(-1)->lineno, get(-1)->token);
			exp->addChild(left);
			ASTNode *right = bitwise_xor_exppression();
			
			if (checkNullExpression(right)) {
				delete exp;
				return NULL;
			}
			
			exp->addChild(right);
			left = exp;
			continue;
		}
		break;
	}
	
	return left;
};

ASTNode *parser::and_expression() {
	
	
	// FRAME:
	// exp1
	// exp2
	
	ASTNode *left = bitwise_or_expression();
	if (checkNullExpression(left)) 
		return NULL;
	
	while (1) {
		if (match(AND)) {
			ASTNode *exp   = new ASTNode(get(-1)->lineno, get(-1)->token);
			exp->addChild(left);
			ASTNode *right = bitwise_or_expression();
			
			if (checkNullExpression(right)) {
				delete exp;
				return NULL;
			}
			
			exp->addChild(right);
			left = exp;
			continue;
		}
		break;
	}
	
	return left;
};

ASTNode *parser::or_expression() {
	
	
	// FRAME:
	// exp1
	// exp2
	
	ASTNode *left = and_expression();
	if (checkNullExpression(left)) 
		return NULL;
	
	while (1) {
		if (match(OR)) {
			ASTNode *exp   = new ASTNode(get(-1)->lineno, get(-1)->token);
			exp->addChild(left);
			ASTNode *right = and_expression();
			
			if (checkNullExpression(right)) {
				delete exp;
				return NULL;
			}
			
			exp->addChild(right);
			left = exp;
			continue;
		}
		break;
	}
	
	return left;
};

ASTNode *parser::condition_expression() {
	
	
	// FRAME:
	// condition
	// true-exp
	// false-exp
	
	ASTNode *or_exp = or_expression();
	if (checkNullExpression(or_exp)) 
		return NULL;
	
	ASTNode *condition_exp;
	
	if (match(HOOK))
		condition_exp = new ASTNode(get(-1)->lineno, CONDITION);
	else 
		return or_exp;
	
	condition_exp->addChild(or_exp);
	
	ASTNode *true_exp = assign_expression();
	if (checkNullExpression(true_exp)) {
		delete condition_exp;
		return NULL;
	}
	
	condition_exp->addChild(true_exp);
	
	if (!match(COLON)) {
		delete condition_exp;
		PARSER_ERROR_RETURN(L"Expected :", get(0)->lineno, get(0)->charno);
	}
	
	ASTNode *false_exp = assign_expression();
	if (checkNullExpression(false_exp)) {
		delete condition_exp;
		return NULL;
	}
	
	condition_exp->addChild(false_exp);
	
	return condition_exp;
};

ASTNode *parser::assign_expression() {
	
	
	// FRAME:
	// exp1
	// exp2
	
	ASTNode *condition_exp = condition_expression();
	if (checkNullExpression(condition_exp)) 
		return NULL;
	
	ASTNode *assign_exp;
	
	if (match(ASSIGN)
		||
		match(ASSIGN_ADD)
		||
		match(ASSIGN_SUB)
		||
		match(ASSIGN_MUL)
		||
		match(ASSIGN_DIV)
		||
		match(ASSIGN_BITRSH)
		||
		match(ASSIGN_BITLSH)
		||
		match(ASSIGN_BITURSH)
		||
		match(ASSIGN_BITULSH)
		||
		match(ASSIGN_HASH)
		||
		match(ASSIGN_PATH)
		||
		match(ASSIGN_DIR)
		||
		match(ASSIGN_MOD)
		||
		match(ASSIGN_BITOR)
		||
		match(ASSIGN_BITAND)
		||
		match(ASSIGN_BITXOR)) 
	{
		if (condition_exp->type == NAME
			||
			condition_exp->type == FIELD
			||
			condition_exp->type == MEMBER)
			assign_exp = new ASTNode(get(-1)->lineno, get(-1)->token);
		else {
			delete condition_exp;
			PARSER_ERROR_RETURN(L"Left side of the assignment expected to be field", get(0)->lineno, get(0)->charno);
		}
	} else 
		return condition_exp;
	
	assign_exp->addChild(condition_exp);
	
	ASTNode *assign_exp1 = assign_expression();
	if (checkNullExpression(assign_exp1)) {
		delete assign_exp;
		return NULL;
	}
	
	assign_exp->addChild(assign_exp1);
	
	return assign_exp;
};

ASTNode *parser::expression() {
	return assign_expression();
};

ASTNode *parser::checkNotNullExpression() {
	ASTNode *n = expression();
	if (n == NULL && !error_)
		PARSER_ERROR_RETURN(L"NULL-pointer expression", get(0)->lineno, get(0)->charno);
	return n;
};

bool parser::checkNullExpression(ASTNode *exp) {
	if (exp == NULL && !error_) {
		PARSER_ERROR_RETURN(L"NULL-pointer expression", get(0)->lineno, get(0)->charno);
		return 1;
	} else if (exp == NULL) // silent
		return 1;
	return 0;
};

ASTNode *parser::initializerstatement() {
	
	// Initilizer statement used by FOR initializer
	
	if (match(TEOF))
		PARSER_ERROR_RETURN(L"EOF Statement", get(0)->lineno, get(0)->charno);
	
	if (error_)
		return NULL;
	
	if (match(VAR) || match(SAFE) || match(LOCAL) || match(CONST)) {
		
		// DEFINE statement
		
		// FRAME:
		// value1
		// value2
		// ...
		// OBJECTS:
		// ...
		// type3 (4bit integer)
		// name3 (string)
		// type2 (4bit integer)
		// name2 (string)
		// type1 (4bit integer)
		// name1 (string)
		
		// type = has_value | safe  | local | const
		//        1000/0000   0100    0010    0001
		
		// if (varN has_value = 0) => valueN miss
		
		// FRAME example
		// 10
		// "hello"
		// OBJECTS example
		// 1100
		// my_number                     -> 10
		// 0100
		// var_with_no_initial_value
		// 1100
		// my_string                     -> "hello"
		
		// const safe local var a = 10;
		//            ^ not used
		
		ASTNode *definenode = new ASTNode(get(-1)->lineno, DEFINE);
		
		// Modifiers
		bool _var   = (get(-1)->token == VAR);
		bool _safe  = (get(-1)->token == SAFE);
		bool _local = (get(-1)->token == LOCAL);
		bool _const = (get(-1)->token == CONST);
		
		while (match(VAR) || match(SAFE) || match(LOCAL) || match(CONST)) {
			_var   |= (get(-1)->token == VAR);
			_safe  |= (get(-1)->token == SAFE);
			_local |= (get(-1)->token == LOCAL);
			_const |= (get(-1)->token == CONST);			
		}
		
		while (true) {
			// if (match(SEMICOLON))
			// 	break;
			
			if (!match(NAME)) {
				delete definenode;
				PARSER_ERROR_RETURN(L"Expected name", get(0)->lineno, get(0)->charno);
			}
			
			std::wstring *name = new std::wstring(get(-1)->sv);
			
			if (match(ASSIGN)) {
				_var = 1;
				
				ASTNode *exp = checkNotNullExpression();
				
				// Check for NULL expressin and ommit memory leak
				if (error_) {
					delete name;
					delete definenode;
					return NULL;
				}
				definenode->addChild(exp);
			} else 
				_var = 0;
			
				
			int *type = new int;
			*type = (_var << 3) | (_safe << 2) | (_local << 1) | _const;
			definenode->addLastObject(type);
			definenode->addLastObject(name);
			
			if (!match(COMMA))
				break;
		}
		
		return definenode;
	}
	
	else {
		// Expression statement
		
		// FRAME:
		// expression
		
		ASTNode *expst = new ASTNode(get(0)->lineno, EXPRESSION);
		
		// Check for NULL expressin and ommit memory leak
		ASTNode *exp = checkNotNullExpression();
		if (error_) {
			delete expst;
			return NULL;
		}
		
		expst->addChild(exp);
		
		return expst;
	};
};

bool parser::peekStatementWithoutSemicolon() {
	
	// Used to check if next node if statement
	
	return 
			get(0)->token == LC
			||
			get(0)->token == IF
			||
			get(0)->token == SWITCH
			||
			get(0)->token == WHILE
			||
			get(0)->token == DO
			||
			get(0)->token == FOR
			||
			get(0)->token == BREAK
			||
			get(0)->token == CONTINUE
			||
			get(0)->token == RETURN
			||
			get(0)->token == THROW
			||
			get(0)->token == TRY
			||
			get(0)->token == VAR
			||
			get(0)->token == SAFE
			||
			get(0)->token == CONST
			||
			get(0)->token == LOCAL;
};

ASTNode *parser::statement() {
	
	// Parse statement & consume all semicolons after it
	
	if (match(TEOF))
		PARSER_ERROR_RETURN(L"EOF Statement", get(0)->lineno, get(0)->charno);
	
	if (error_)
		return NULL;
	
	ASTNode *s = statement_with_semicolons();
	
	while (match(SEMICOLON));
	
	return s;
};

ASTNode *parser::statement_with_semicolons() {
	
	if (match(TEOF))
		PARSER_ERROR_RETURN(L"EOF Statement", get(0)->lineno, get(0)->charno);
	
	if (error_)
		return NULL;
	
	else if (match(SEMICOLON)) {
		// printf("____ %d %d %d\n", get(-1)->token, get(0)->token, get(1)->token);
		ASTNode *empty = new ASTNode(get(-1)->lineno, EMPTY);
		
		return empty;
	}
	
	else if (match(IF)) {
		// FRAME:
		// condition
		// IF node
		// ELSE node
		
		ASTNode *ifelse = new ASTNode(get(-1)->lineno, IF);
		// printf("lo lineno: %d %d\n", get(-1)->lineno);
		int lp = match(LP);
		
		// Check for NULL expressin and ommit memory leak
		ASTNode *condition = checkNotNullExpression();
		if (error_) {
			delete ifelse;
			return NULL;
		}
		
		ifelse->addChild(condition);
		
		if (lp && !match(RP)) {
			delete ifelse;
			PARSER_ERROR_RETURN(L"Expected )", get(0)->lineno, get(0)->charno);
		}
		
		// Parse if & else nodes
		ASTNode *ifnode   = statement();
		ASTNode *elsenode = NULL;
		
		if (match(ELSE))
			elsenode = statement();
		else {
			elsenode = new ASTNode(-1, EMPTY);
		}
		
		// Add nodes
		ifelse->addChild(ifnode);
		ifelse->addChild(elsenode);
		
		return ifelse;
	}
	
	else if (match(SWITCH)) {
		// FRAME:
		// condition
		// case1
		// ...
		// caseN
		
		ASTNode *switchcase = new ASTNode(get(-1)->lineno, SWITCH);
		int lp = match(LP);
		
		// Check for NULL expressin and ommit memory leak
		ASTNode *condition = checkNotNullExpression();
		if (error_) {
			delete switchcase;
			return NULL;
		}
		
		switchcase->addChild(condition);
		
		if (lp && !match(RP)) {
			delete switchcase;
			PARSER_ERROR_RETURN(L"Expected )", get(0)->lineno, get(0)->charno);
		}
		
		if (match(SEMICOLON));
		else {
			if (!match(LC)) {
				delete switchcase;
				PARSER_ERROR_RETURN(L"Expected {", get(0)->lineno, get(0)->charno);
			}
			
			bool matched_default = 0;
			
			while (match(CASE) || match(DEFAULT)) {
				int lineno = get(-1)->lineno;
				
				if (get(-1)->token == CASE) { 
					// CASE node
					
					// FRAME:
					// condition
					// node
				
					// Check for NULL expressin and ommit memory leak
					ASTNode *condition = checkNotNullExpression();
					if (error_) {
						delete switchcase;
						return NULL;
					}
					
					ASTNode *casenode = new ASTNode(lineno, CASE);
					casenode->addChild(condition);
					switchcase->addChild(casenode);
					
					if (!match(COLON)) {
						delete switchcase;
						PARSER_ERROR_RETURN(L"Expected :", get(0)->lineno, get(0)->charno);
					}
					
					while (true) {
						if (error_)  {
							delete switchcase;
							return NULL;
						}
						if (match(TEOF)) {
							delete switchcase;
							PARSER_ERROR_RETURN(L"Expected }", get(0)->lineno, get(0)->charno);
						}
						if (get(0)->token == CASE || get(0)->token == DEFAULT || get(0)->token == RC)
							break;
						casenode->addChild(statement());
					}
					
				} else { 
					// DEFAULT node
					
					// FRAME:
					// node
				
					if (matched_default) {
						delete switchcase;
						PARSER_ERROR_RETURN(L"Duplicate default case", get(0)->lineno, get(0)->charno);
					}
					matched_default = 1;
					
					if (!match(COLON)) {
						delete switchcase;
						PARSER_ERROR_RETURN(L"Expected :", get(0)->lineno, get(0)->charno);
					}
					
					ASTNode *defaultnode = new ASTNode(lineno, DEFAULT);
					switchcase->addChild(defaultnode);
					
					while (true) {
						if (error_)  {
							delete switchcase;
							return NULL;
						}
						if (match(TEOF)) {
							delete switchcase;
							PARSER_ERROR_RETURN(L"Expected }", get(0)->lineno, get(0)->charno);
						}
						if (get(0)->token == CASE || get(0)->token == DEFAULT || get(0)->token == RC)
							break;
						defaultnode->addChild(statement());
					}
				}
			}
			
			if (!match(RC)) {
				delete switchcase;
				PARSER_ERROR_RETURN(L"Expected }", get(0)->lineno, get(0)->charno);
			}
		}
		
		return switchcase;
	}
	
	else if (match(WHILE)) {
		// FRAME:
		// condition
		// BODY node
		
		ASTNode *whileloop = new ASTNode(get(-1)->lineno, WHILE);
		int lp = match(LP);
		
		// Check for NULL expressin and ommit memory leak
		ASTNode *condition = checkNotNullExpression();
		if (error_) {
			delete whileloop;
			return NULL;
		}
		
		whileloop->addChild(condition);
		
		if (lp && !match(RP)) {
			delete whileloop;
			PARSER_ERROR_RETURN(L"Expected )", get(0)->lineno, get(0)->charno);
		}
		
		// Parse if & else nodes
		ASTNode *bodynode   = statement();
		
		// Add nodes
		whileloop->addChild(bodynode);
		
		return whileloop;
	}
	
	else if (match(DO)) {
		// FRAME:
		// condition
		// BODY node
		
		ASTNode *doloop = new ASTNode(get(-1)->lineno, DO);
		ASTNode *body   = statement();
		
		if (!match(WHILE)) {
			delete doloop;
			delete body;
			PARSER_ERROR_RETURN(L"Expected while", get(0)->lineno, get(0)->charno);
		}
		
		int lp = match(LP);
		
		// Check for NULL expressin and ommit memory leak
		ASTNode *condition = checkNotNullExpression();
		if (error_) {
			delete doloop;
			delete body;
			return NULL;
		}
		
		doloop->addChild(condition);
		doloop->addChild(body);
		
		if (lp && !match(RP)) {
			delete doloop;
			PARSER_ERROR_RETURN(L"Expected )", get(0)->lineno, get(0)->charno);
		}
		
		return doloop;
	}
	
	else if (match(FOR)) {
		// () expected for (;;)
		
		// FRAME:
		// INITIALIZATION node
		// CONDITION node
		// INCREMENT node
		// BODY node
		
		ASTNode *forloop = new ASTNode(get(-1)->lineno, FOR);
		
		if (!match(LP)) {
			delete forloop;
			PARSER_ERROR_RETURN(L"Expected )", get(0)->lineno, get(0)->charno);
		}
		
		if (!match(SEMICOLON)) {
			ASTNode *initialization = initializerstatement();
			forloop->addChild(initialization);
			
			if (!match(SEMICOLON)) {
				delete forloop;
				PARSER_ERROR_RETURN(L"Expected ;", get(0)->lineno, get(0)->charno);
			}
		} else {
			ASTNode *empty = new ASTNode(-1, EMPTY);
			forloop->addChild(empty);
		}
		
		if (!match(SEMICOLON)) {
			// Check for NULL expressin and ommit memory leak
			ASTNode *condition = checkNotNullExpression();
			if (error_) {
				delete forloop;
				return NULL;
			}
			forloop->addChild(condition);
			
			if (!match(SEMICOLON)) {
				delete forloop;
				PARSER_ERROR_RETURN(L"Expected ;", get(0)->lineno, get(0)->charno);
			}
		} else {
			ASTNode *empty = new ASTNode(-1, EMPTY);
			forloop->addChild(empty);
		}
		
		if (!match(RP)) {
			// Check for NULL expressin and ommit memory leak
			ASTNode *increment = checkNotNullExpression();
			if (error_) {
				delete forloop;
				return NULL;
			}
			forloop->addChild(increment);

			if (!match(RP)) {
				delete forloop;
				PARSER_ERROR_RETURN(L"Expected )", get(0)->lineno, get(0)->charno);
			}
		} else {
			ASTNode *empty = new ASTNode(-1, EMPTY);
			forloop->addChild(empty);
		}
		
		// Parse if & else nodes
		ASTNode *bodynode   = statement();
		
		// Add nodes
		forloop->addChild(bodynode);
		
		return forloop;
	}
	
	else if (match(LC)) { 
		ASTNode *blocknode = new ASTNode(get(-1)->lineno, BLOCK);
		
		while (true) {
			if (error_) {
				delete blocknode;
				return NULL;
			}
			if (match(TEOF)) {
				delete blocknode;
				PARSER_ERROR_RETURN(L"Expected }", get(0)->lineno, get(0)->charno);
			}
			if (match(RC))
				break;
			
			blocknode->addChild(statement());
		}
		
		return blocknode;
	}
	
	else if (match(BREAK)) {
		
		ASTNode *breaknode = new ASTNode(get(-1)->lineno, BREAK);
		
		return breaknode;
	}
	
	else if (match(CONTINUE)) {
		
		ASTNode *coontinuenode = new ASTNode(get(-1)->lineno, CONTINUE);
		
		return coontinuenode;
	}
	
	else if (match(RETURN)) {
		
		// FREAME:
		// value / NULL
		
		ASTNode *returnnode = new ASTNode(get(-1)->lineno, RETURN);
		
		if (!peekStatementWithoutSemicolon() && get(0)->token != SEMICOLON) {
			ASTNode *exp = checkNotNullExpression();
			
			// Check for NULL expressin and ommit memory leak
			if (!exp) {
				delete returnnode;
				return NULL;
			}
			returnnode->addChild(exp);
		}
		
		return returnnode;
	}
	
	else if (match(THROW)) {
		
		// FREAME:
		// value / EMPTY
		
		ASTNode *thrownode = new ASTNode(get(-1)->lineno, THROW);
		
		if (!peekStatementWithoutSemicolon() && get(0)->token != SEMICOLON) {
			ASTNode *exp = checkNotNullExpression();
			
			// Check for NULL expressin and ommit memory leak
			if (error_) {
				delete thrownode;
				return NULL;
			}
			thrownode->addChild(exp);
		} else {
			ASTNode *empty = new ASTNode(-1, EMPTY);
			thrownode->addChild(empty);
		}
		return thrownode;
	}
	
	else if (match(TRY)) {
		// FRAME
		// try node
		// expect node
		// OBJECTS:
		// handler name / null
		
		ASTNode *tryexpect = new ASTNode(get(-1)->lineno, TRY);
		tryexpect->addChild(statement());
		
		if (match(CATCH)) {
			if (match(LP)) {
				if (!match(NAME)) {
					delete tryexpect;
					PARSER_ERROR_RETURN(L"Expected name", get(0)->lineno, get(0)->charno);
				}
				
				std::wstring *name = new std::wstring(get(-1)->sv);
				
				if (!match(RP)) {
					delete name;
					delete tryexpect;
					PARSER_ERROR_RETURN(L"Expected )", get(0)->lineno, get(0)->charno);
				}
				
				tryexpect->addLastObject(name);
			}
			
			ASTNode *catch_node = statement();
			if (!catch_node) {
				delete tryexpect;
				return NULL;
			}
			
			tryexpect->addChild(catch_node);
		} else {
			ASTNode *empty = new ASTNode(-1, EMPTY);
			tryexpect->addChild(empty);
		}
		
		return tryexpect;
	}
	
	else
		return initializerstatement();
};

ASTNode *parser::parse() {
	ASTNode *root = NULL;
	
	if (error_ || eof_)
		return NULL;
	
	if (!repl)
		root = new ASTNode(0, ASTROOT);
	
	while (!eof()) {
		ASTNode *node = statement();
		
		if (error_)
			break;
		
		if (node != NULL)
			if (repl)
				return node;
			else
				root->addChild(node);
		else {
			eof_ = 1;
		}
		
		if (repl)
			return node;
	}
	
	if (error_) {
		delete root;
		PARSER_ERROR_RETURN(L"parsing error", get(0)->lineno, get(0)->charno);
		return NULL;
	}
	
	return root;
};

ASTNode *parser::parse_single_statement() {
	ASTNode *root = NULL;
	
	if (error_ || eof_)
		return nullptr;
	
	if (!eof()) {
		ASTNode *node = statement();
		
		// Info about error
		if (error_) {
			PARSER_ERROR_RETURN(L"parsing error", get(0)->lineno, get(0)->charno);
			return nullptr;
		}
		
		// Return value
		if (node != nullptr) {
			ASTNode* root = new ASTNode(0, ASTROOT);
			root->addChild(node);
			return root;
		} else {
			eof_ = 1;
		}
	}
	
	return nullptr;
};

int parser::lineno() {
	return get(0)->lineno;
};

int parser::charno() {
	return get(0)->charno;
};

int parser::eof() {
	return // source->eof() || 
		eof_ || error_;
};

//      /(|
//     (  :
//    __\  \  
//  (____)  `|\
// (____)|   | \
//  (____).__|  \
//   (___)__.|   \_____
//
// I'LL BE BACK...

