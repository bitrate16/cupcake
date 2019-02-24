#include "parser"
#include "exec_state"

#include <wchar>

#define TOKENIZER_ERROR(...) { messages.error(__VA_ARGS__); error_ = 1; return put(TERR); }
#define TOKENIZER_WARNING(...) messages.warning(__VA_ARGS__);

#define PARSER_ERROR_RETURN(...) { messages.error(__VA_ARGS__); error_ = 1; return NULL; }
#define PARSER_ERROR(...) { messages.error(__VA_ARGS__); error_ = 1; }
#define PARSER_WARNING(...) messages.warning(__VA_ARGS__);

using namespace ck_token;
using namespace ck_parser;
using namespace ck_ast;


// S T R E A M _ W R A P P E R

// Returns next redden character.
int stream_wrapper::getc() {
	if (eof)
		return WEOF;
	
	switch(source_type) {
		case IN_STRING:
			if (cursor >= string.size()) {
				eof = 1;
				return weof;
			}
			return string[cursor++];
		
		case IN_STDIN:
			int c = getwc(stdin);
			if (c == WEOF)
				eof = 1;
			
			return c;
			
		case IN_FILE:
			int c = getwc(file);
			if (c == WEOF)
				eof = 1;
			
			return c;
			
		default:
			eof = 1;
			return WEOF;
	};
};

// Returns true whatewer EOF is reached.
bool stream_wrapper::eof() {
	if (eof == 1)
		return 1;
	
	switch(source_type) {
		case IN_STRING:
			if (cursor >= string.size()) {
				eof = 1;
				return 1;
			}
			return 0;
		
		case IN_STDIN:
			int c = getwc(stdin);
			ungetwc(c, stdin);
			
			if (c == WEOF)
				eof = 1;
			
			return eof;
			
		case IN_FILE:
			int c = getwc(file);
			ungetwc(c, stdin);
			
			if (c == WEOF)
				eof = 1;
			
			return eof;
			
		default:
			eof = 1;
			return 1;
	};
};


// T O K E N I Z E R
// (It was written when i was drunk but it works unlike my liver..)

static int white(int c) {
	return c == '\n' || c == '\r' || c == '\t' || c == ' ' || c == 0x0A || c == 0x0D;
};

static int alpha(int c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= 'а' && c <= 'я') || (c >= 'А' && c <= 'Я') || c == '_';
};

static int digit(int c) {
	return c >= '0' && c <= '9';
};

static char strtlc(char c) {
	return (c >= 'a' && c <= 'z') ? c + 'A' - 'a' : c;
};

static int get(int off) {
	if (off > 4 || off < -4) {
		printf("PAJOJDA OTDEBAJJE MENYA\n");
		return -1;
	}
	return buffer[4 + off];
};

static int next() {
	for (int i = 1; i < 9; i++)
		buffer[i - 1] = buffer[i];
	
	if (buffer[7] == TEOF)
		buffer[8] = TEOF;
	else {
		int c = source->getc();
		
		if (c == WEOF)
			buffer[8] = TEOF;
		else
			buffer[8] = c;
	}
	
#ifdef PRINT_FILE
	if (buffer[8] != TEOF) {			
		putchar((char) buffer[8]);
	} else if (eof())
		putchar(10);
#endif
	
	if (buffer[4] == '\n') {
		++lineno;
		return TEOL;
	}
	
	return buffer[4];
};

static int put(int token) {
	this->token->token = token;
	
	// Modify subtype to final
	if (token == TRUE || token == FALSE) {
		this->token->booleanv = token == TRUE;
		this->token->token = BOOLEAN;
	}
	
	if (token == TEOF)
		return 0;
	
	return 1;
};

static int match(int charcode0) {
	if (get(0) != charcode0)
		return false;
	next();
	return true;
};

static int match(int charcode0, int charcode1) {
	if (get(0) != charcode0 || get(1) != charcode1)
		return false;
	next(); next();
	return true;
};

static int match(int charcode0, int charcode1, int charcode2) {
	if (get(0) != charcode0 || get(1) != charcode1 || get(2) != charcode2)
		return false;
	next(); next(); next();
	return true;
};

static int match(int charcode0, int charcode1, int charcode2, int charcode3) {
	if (get(0) != charcode0 || get(1) != charcode1 || get(2) != charcode2 || get(3) != charcode3)
		return false;
	next(); next(); next(); next();
	return true;
};

void tokenizer::clear() {
	token->token = NONE;
	token->iv = 0;
	token->dv = 0.0;
	token->bv = false;
	delete token->sv;
	token->sv = new string("");
};

int tokenizer::nextToken() {
	if (eof() || !_global_exec_state)
		return put(TEOF);
	
	clear();
	
	if (get(0) == TEOF) {
		token->lineno = lineno;
		this->eof_ = 1;
		return put(TEOF);
	}

	int c  = get(0);
	int c1 = get(1);

	// Remove all whitescapes and comments
	while (white(c) || (c == '/' && ((c1 = get(1)) == '/' || c1 == '*'))) {
		if (white(c)) {
			c = next();
			c1 = get(1);
		} else if (c == '/' && c1 == '/') {
			// Skip till newline || TEOF
			int c0 = c;
			while (c0 != TEOL && c != TEOF) {
				c0 = c;
				c = next();
				c1 = get(1);
			}
			//if(c0 == TEOL)
			//	token->lineno++;
		} else if (c == '/' && c1 == '*') {
			// Skip all code till '*' '/' sequence
			// Warning: '*/' may be hidden in string. That must be ignored
			int q0 = false; // ' - quote
			int q1 = false; // " - quote
			int closed = false;
			// \/--- !(c == '*' && c1 == '/' && !q0 && !q1) &&
			while (c != TEOF) {
				c = next();
				c1 = get(1);
				if (c1 == '\'' && !q1)
					q0 = !q0;
				if (c1 == '\"' && !q0)
					q1 = !q1;

				if (c == '*' && c1 == '/' && !q0 && !q1) {
					closed = true;
					c = next();
					c = next();
					c1 = get(1);
					break;
				}
			}

			if (!closed) TOKENIZER_ERROR("block comment expected to be closed", lineno)
		}
	}

	if (get(0) == TEOF) {
		token->lineno = lineno;
		this->eof_ = 1;
		return put(TEOF);
	}

	token->lineno = lineno;
	
	/* K E Y W O R D S */ {
		// Parse keywords|names
		if (alpha(c)) {
			string &stringvref = *token->stringv;
			do {
				stringvref += (char) c;
				c = next();
			} while((alpha(c) || digit(c)) && c != TEOF);

			if (stringvref == "true")
				return put(TRUE);
			if (stringvref == "false")
				return put(FALSE);
			if (stringvref == "null")
				return put(TNULL);
			if (stringvref == "undefined")
				return put(UNDEFINED);

			if (stringvref == "this")
				return put(THIS);
			if (stringvref == "self")
				return put(SELF);
			if (stringvref == "try")
				return put(TRY);
			if (stringvref == "expect")
				return put(EXPECT);
			if (stringvref == "raise")
				return put(RAISE);
			if (stringvref == "if")
				return put(IF);
			if (stringvref == "else")
				return put(ELSE);
			if (stringvref == "for")
				return put(FOR);
			if (stringvref == "switch")
				return put(SWITCH);
			if (stringvref == "case")
				return put(CASE);
			if (stringvref == "default")
				return put(DEFAULT);
			if (stringvref == "while")
				return put(WHILE);
			if (stringvref == "do")
				return put(DO);
			if (stringvref == "break")
				return put(BREAK);
			if (stringvref == "continue")
				return put(CONTINUE);
			if (stringvref == "return")
				return put(RETURN);
			if (stringvref == "function")
				return put(FUNCTION);
			if (stringvref == "prototype")
				return put(PROTOTYPE);
			if (stringvref == "var")
				return put(VAR);
			if (stringvref == "const")
				return put(CONST);
			if (stringvref == "safe")
				return put(SAFE);
			if (stringvref == "local")
				return put(LOCAL);
			if (stringvref == "new")
				return put(NEW);

			return put(NAME);
		}
	}
	
	/* S T R I N G S */ {
		// Parse string ("|')
		if (c == '\'' || c == '\"') {
			int quote = c;
			string &stringvref = *token->stringv;

			c = next();
			while (c != TEOF) {
				if (c == TEOL || c == TEOF) TOKENIZER_ERROR("string expected to be closed", lineno)
				if (c == quote)
					break;
				if (c == '\\') {
					c1 = next();
					if (c1 == 't')
						stringvref += '\t';
					else if (c1 == 'b')
						stringvref += '\b';
					else if (c1 == 'n')
						stringvref += '\n';
					else if (c1 == 'r')
						stringvref += '\r';
					else if (c1 == 'f')
						stringvref += '\f';
					else if (c1 == '\\')
						stringvref += '\\';
					else if (c1 == '\'')
						stringvref += '\'';
					else if (c1 == '\"')
						stringvref += '\"';
					else if (c1 == '0')
						stringvref += '\0';
					else if (c1 == 'u') {
						c1 = next();
						// Expect 8-digit hex number
						int chpoint = 0;
						int point   = 0;
						for (int i = 0; i < 8; i++) {
							c1 = strtlc(c1);
							int cp = -1;
							if (c1 >= 'a' && c1 <= 'f')
								cp = 10 + c1 - 'a';
							if (c1 >= '0' && c1 <= '9')
								cp = c1 - '0';
							if (cp == -1 && !point) 
								TOKENIZER_ERROR(std::wstring("expected hexadecimal character code point ") + cp + std::wstring(" [") + (wchar_t) cp + std::wstring("]"), lineno)
							if (cp == -1)
								break;
							++point;
							chpoint = chpoint * 16 + cp;
							c1 = next();
						}
					} else if (c1 == 'x') {
						c1 = next();
						// Expect 4-digit hex number
						int chpoint = 0;
						int point   = 0;
						for (int i = 0; i < 4; i++) {
							c1 = strtlc(c1);
							int cp = -1;
							if (c1 >= 'a' && c1 <= 'f')
								cp = 10 + c1 - 'a';
							if (c1 >= '0' && c1 <= '9')
								cp = c1 - '0';
							if (cp == -1 && !point) 
								TOKENIZER_ERROR(std::wstring("expected hexadecimal character code point ") + cp + std::wstring(" [") + (wchar_t) cp + std::wstring("]"), lineno)
							if (cp == -1)
								break;
							++point;
							chpoint = chpoint * 16 + cp;
							c1 = next();
						}
					} else
						TOKENIZER_ERROR(std::wstring("unexpected character after escape point ") + cp + std::wstring(" [") + (wchar_t) cp + std::wstring("]"), lineno)
				} else
					stringvref += (wchar_t) c;
				c = next();
			}
			next();
			return put(STRING);
		}
	}
	
	/* N U M B E R S */ {
		if (digit(c) || (c == '.' && digit(c1))) {
			
			
			// XXX: Rewrite this code part to optimize/fix parser bugs
			
			
			int type = INTEGER;
			int hasPoint = false;
			int scientific = false;
			int base = 10;

			string &stringvref = token->stringv;
			
			if (c == '.') {
				type = DOUBLE;
				hasPoint = true;
			}

			stringvref += (char) c;

			c = next();
			c1 = get(1);
			while (c != TEOF) {
				if (scientific)
					TOKENIZER_ERROR("double can't be parsed after scientific notation", lineno)
				else if (c == '.') {
					if (hasPoint)
						TOKENIZER_ERROR("double can't have more than one point", lineno)
					// if (base != 10)
					//	return tokenizer_error(lineno, "double can't be not decimal");
					if (!digit(c1))
						TOKENIZER_ERROR("unexpected fraction of double nomber", lineno)
					type = DOUBLE;
					hasPoint = true;
					stringvref += '.';
				} else if ((c == 'x' || c == 'X') && base == 10) {
					if (type == DOUBLE)
						TOKENIZER_ERROR("double can't be not decimal", lineno)
					// if (base != 10)
					//	return tokenizer_error(lineno, "one token.integerv can't have multiple numerical bases");
					if (stringvref != "0")
						TOKENIZER_ERROR("base notation starts with 0", lineno)
					stringvref = "";
					base = 16;
				} else if ((c == 'o' || c == 'O') && base == 10) {
					if (type == DOUBLE)
						TOKENIZER_ERROR("double can't be not decimal", lineno)
					// if (base != 10)
					//	return tokenizer_error(lineno, "one token.integerv can't have multiple numerical bases");
					if (stringvref != "0")
						TOKENIZER_ERROR("base notation starts with 0", lineno)
					stringvref = "";
					base = 8;
				} else if ((c == 'b' || c == 'B') && base == 10) {
					if (type == DOUBLE)
						TOKENIZER_ERROR("double can't be not decimal", lineno)
					// if (base != 10)
					//	return tokenizer_error(lineno, "integer can't have multiple numerical bases");
					if (stringvref != "0")
						TOKENIZER_ERROR("base notation starts with 0", lineno)
					stringvref = "";
					base = 2;
				} else if ((c == 'e' || c == 'E') && base != 16) {
					if (base != 10)
						TOKENIZER_ERROR("double can't be not decimal", lineno)
					type = DOUBLE;
					// Start reading scientific notation
					// double + E + (+ or -) + integer
					stringvref += 'e';
					c = next();
					if ((c == '-' || c == '+')) {
						// 12.34E26 is the same as 12.34E+26
						stringvref += (wchar_t) c;
						c = next();
					}
					string *integer = new string();
					while (digit(c)) {
						*integer += (wchar_t) c;
						c = next();
					}
					if (*integer == "")
						TOKENIZER_ERROR("expected exponent in scientific notation", lineno)
					if (alpha(c))
						TOKENIZER_ERROR("unexpected character in scientific notation", lineno)
					stringvref += *integer;
					delete integer;
					scientific = true;
					break;
				} else if ((c == 'l' || c == 'L' || (c == 'b' || c == 'B') && base != 16) && !digit(c1)) {
					if (c == 'l' || c == 'L')
						type = LONG;
					else
						type = BYTE;
					c = next();
					break;
				} else if (base == 16 && (('A' <= c && c <= 'F') || ('a' <= c && 'f' <= c) || digit(c)))
					stringvref += (wchar_t) c;
				else if (base == 8 && '0' <= c && c <= '7')
					stringvref += (wchar_t) c;
				else if (base == 2 && (c == '0' || c == '1'))
					stringvref += (wchar_t) c;
				else if (base == 10 && digit(c))
					stringvref += (wchar_t) c;
				else if (alpha(c) || (base == 8 && '8' <= c && c <= '9')
							      || (base == 2 && '3' <= c && c <= '9'))
					TOKENIZER_ERROR(std::wstring("unexpected character in number ") + c + std::wstring(" [") + (wchar_t) c + std::wstring("]"), lineno)
				else
					break;

				c = next();
				c1 = get(1);
			}
			
			if (type == DOUBLE) {
				token->doublev = stringvref.toDouble(0);
				return put(DOUBLE);
			}
			if (type == INTEGER) {
				token->integerv = (int) stringvref.toInt(base, 0);
				return put(INTEGER);
			}
			if (type == BYTE) {
				token->longv = (unsigned char) stringvref.toInt(base, 0);
				return put(BYTE);
			}
			if (type == LONG) {
				token->longv =  (long) stringvref.toInt(base, 0);
				return put(LONG);
			}
		}
	}
	
	/* D E L I M I T E R S */ {
		// Parse delimiters
		if (match('>', '>', '>', '='))
			return put(ASSIGN_BITURSH);
		if (match('>', '>', '>'))
			return put(BITURSH);
		if (match('>', '>', '='))
			return put(ASSIGN_BITRSH);
		if (match('<', '<', '='))
			return put(ASSIGN_BITLSH);
		if (match('>', '>'))
			return put(BITRSH);
		if (match('<', '<'))
			return put(BITLSH);
		if (match('>', '='))
			return put(GE);
		if (match('<', '='))
			return put(LE);
		if (match('&', '&'))
			return put(AND);
		if (match('|', '|'))
			return put(OR);
		if (match('=', '='))
			return put(EQ);
		if (match('!', '='))
			return put(NEQ);
		if (match('-', '>'))
			return put(PUSH);
		if (match('=', '>'))
			return put(LAMBDA);
		if (match('/', '/'))
			return put(MDIV);
		if (match('+', '+'))
			return put(INC);
		if (match('-', '-'))
			return put(DEC);
		if (match('+', '='))
			return put(ASSIGN_ADD);
		if (match('-', '='))
			return put(ASSIGN_SUB);
		if (match('*', '='))
			return put(ASSIGN_MUL);
		if (match('/', '='))
			return put(ASSIGN_DIV);
		if (match('%', '='))
			return put(ASSIGN_MOD);
		if (match('|', '='))
			return put(ASSIGN_BITOR);
		if (match('&', '='))
			return put(ASSIGN_BITAND);
		if (match('^', '='))
			return put(ASSIGN_BITXOR);
		if (match('~', '='))
			return put(ASSIGN_BITNOT);
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
		if (match('$'))
			return put(SELF);

		if (c == '@' && alpha(c1)) {
			// Read flag token
			c = next();
			while (alpha(c) || digit(c) || c == '_') {
				token->stringv += (char) c;
				c = next();
			}
			return put(NAME);
		}
	}
	
	this->error_ = 1;
	
	// printf("%c %c %c %c %c\n", c, get(-2), get(-1), get(0), get(1));
	
	TOKENIZER_ERROR(std::wstring("unexpected character ") + c + std::wstring(" [") + (wchar_t) c + std::wstring("]"), lineno)
};


// P A R S E R
Parser::~Parser() {
	for (int i = 0; i < 7; i++)
		delete this->buffer[i];
};

raw_token *Parser::get(int off) {
	if (off > 3 || off < -3) {
		printf("PAJOJDA OTDEBAJJE MENYA\n");
		return -1;
	}
	return buffer[3 + off];
};

raw_token *Parser::next() {
	if (this->buffer[0] != NULL)
		delete this->buffer[0];
	
	for (int i = 1; i < 7; i++)
		this->buffer[i - 1] = this->buffer[i];
	
	this->source->nextToken();
	//if (this->source->token->token == TERR)
	//	noline_parser_error("TS Error");
	
	this->buffer[6] = this->source->token->copy();
	
	if (buffer[3] != NULL && buffer[3]->token == TEOF)
		eof_ = 1;
	
	return buffer[3];
};

bool Parser::match(int token) {
	if (error_)
		return 0;
	
	if (get(0)->token != token)
		return 0;
	
	next();
	return 1;	
};

/*ASTNode *Parser::noline_parser_error(const char *msg) {
	chighred;
	printf("Parser error : %s\n", msg);
	cwhite;
	
	error_ = true;
	return NULL;
};

ASTNode *Parser::parser_error(const char *msg) {
	chighred;
	printf("Parser error at %d : %s\n", get(0)->lineno, msg);
	creset;
	
	error_ = true;
	return NULL;
};*/

ASTNode *Parser::primaryexp() {
	if (match(TEOF)) 
		PARSER_ERROR_RETURN("EOF Expression", get(0)->lineno);
	
	if (match(INTEGER)) {
		
		// OBJECTS:
		// value
		
		DEBUG("INTEGER expression")
		ASTNode *integerexp = new ASTNode(get(-1)->lineno, INTEGER);
		
		int *n = new int;
		*n = get(-1)->integerv;
		integerexp->addLastObject(n);
		return integerexp;
	}
	
	if (match(LONG)) {
		
		// OBJECTS:
		// value
		DEBUG("LONG expression")
		ASTNode *longexp = new ASTNode(get(-1)->lineno, LONG);
		
		long *n = new long;
		*n = get(-1)->longv;
		longexp->addLastObject(n);
		
		return longexp;
	}
	
	if (match(BYTE)) {
		
		// OBJECTS:
		// value
		DEBUG("BYTE expression")
		ASTNode *byteexp = new ASTNode(get(-1)->lineno, BYTE);
		
		char *n = new char;
		*n = get(-1)->bytev;
		byteexp->addLastObject(n);
		
		return byteexp;
	}
	
	if (match(BOOLEAN)) {
		
		// OBJECTS:
		// value
		DEBUG("BOOLEAN expression")
		ASTNode *booleanexp = new ASTNode(get(-1)->lineno, BOOLEAN);
		
		bool *n = new bool;
		*n = get(-1)->booleanv;
		booleanexp->addLastObject(n);
		
		return booleanexp;
	}
	
	if (match(DOUBLE)) {
		
		// OBJECTS:
		// value
		DEBUG("DOUBLE expression")
		ASTNode *doubleexp = new ASTNode(get(-1)->lineno, DOUBLE);
		
		double *n = new double;
		*n = get(-1)->doublev;
		doubleexp->addLastObject(n);
		
		return doubleexp;
	}
	
	if (match(NAME)) {
		
		// OBJECTS:
		// value
		DEBUG("NAME expression")
		ASTNode *nameexp = new ASTNode(get(-1)->lineno, NAME);
		
		nameexp->addLastObject(get(-1)->stringv->copy());
		
		return nameexp;
	}
	
	if (match(STRING)) {
		
		// OBJECTS:
		// value
		DEBUG("STRING expression")
		ASTNode *stringexp = new ASTNode(get(-1)->lineno, STRING);
		
		stringexp->addLastObject(get(-1)->stringv->copy());
		
		return stringexp;
	}
	
	if (match(THIS)) {
		DEBUG("THIS expression")
		ASTNode *thisexp = new ASTNode(get(-1)->lineno, THIS);
		
		return thisexp;
	}
	
	if (match(SELF)) {
		DEBUG("SELF expression")
		ASTNode *selfexp = new ASTNode(get(-1)->lineno, SELF);
		
		return selfexp;
	}
	
	if (match(TNULL)) {
		DEBUG("NULL expression")
		ASTNode *nullexp = new ASTNode(get(-1)->lineno, TNULL);
		
		return nullexp;
	}
	
	if (match(UNDEFINED)) {
		DEBUG("NULL expression")
		ASTNode *undefinedexp = new ASTNode(get(-1)->lineno, UNDEFINED);
		
		return undefinedexp;
	}
	
	if (match(LP)) {
		DEBUG("(<exp>)")
		ASTNode *exp = expression();
		
		if (exp == NULL)
			return NULL;

		if (!match(RP)) {
			delete exp;
			PARSER_ERROR_RETURN("Expected )", get(0)->lineno);
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
		
		DEBUG("ARRAY expression")
		
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
				PARSER_ERROR_RETURN("Expected ]", get(0)->lineno);
			}
			if (!match(COMMA)) {
				if (!match(RB)) {
					delete array;
					PARSER_ERROR_RETURN("Expected ]", get(0)->lineno);
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
		// nameN
		// ...
		// name0
		
		DEBUG("OBJECT expression")
		
		ASTNode *object = new ASTNode(get(-1)->lineno, OBJECT);
		
		if (match(RC))
			return object;
		
		while (true) {
		if (!(match(NAME) || match(STRING))) {
				delete object;
				PARSER_ERROR_RETURN("Expected name or string", get(0)->lineno);
			}
			
			string *name = get(-1)->stringv->copy();
			
			if (!match(COLON)) {
				delete name;
				delete object;
				PARSER_ERROR_RETURN("Expected :", get(0)->lineno);
			}
			
			ASTNode *elem = expression();
			if (checkNullExpression(elem))  {
				delete name;
				delete object;
				return NULL;
			}
			
			object->addLastObject(name);
			object->addChild(elem);
			
			if (match(TEOF)) {
				delete object;
				PARSER_ERROR_RETURN("Expected }", get(0)->lineno);
			}
			if (!match(COMMA)) {
				if (!match(RC)) {
					delete object;
					PARSER_ERROR_RETURN("Expected }", get(0)->lineno);
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
		
		DEBUG("FUNCTION node")
		
		ASTNode *function = new ASTNode(get(-1)->lineno, FUNCTION);
		
		if (match(LP)) 
			if (!match(RP))
				while (true) {
					if (!match(NAME)) {
						delete function;
						PARSER_ERROR_RETURN("Expected name", get(0)->lineno);
					}
					
					function->addLastObject(get(-1)->stringv->copy());
					
					if (match(TEOF)) {
						delete function;
						PARSER_ERROR_RETURN("Expected )", get(0)->lineno);
					}
					if (!match(COMMA)) {
						if (!match(RP)) {
							delete function;
							PARSER_ERROR_RETURN("Expected )", get(0)->lineno);
						} else
							break;
					}
				}
		
		function->addChild(statement());
		
		return function;
	}
	
	printf("Parser error at %d : Unexpected token %d (%s)\n", get(0)->lineno, get(0)->token, tokenToString(get(0)->token));
	error_ = true;
	return NULL;
};

ASTNode *Parser::member_expression() {
	if (match(NEW)) {
		
		// DEPRECATED
		
		// FRAME
		// ARGUMENTS_NODE
		// \ arg0
		// | arg1
		// | ...
		// | argn
		// INITIALIZER_OBJECT
		
		PARSER_ERROR_RETURN("New is unsupported", get(0)->lineno);
	}

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
					PARSER_ERROR_RETURN("Expected ]", get(0)->lineno);
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
					PARSER_ERROR_RETURN("Expected name", get(0)->lineno);
				}
				node->addLastObject(get(-1)->stringv->copy());
				
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
						PARSER_ERROR_RETURN("Expected )", get(0)->lineno);
					}
					if (!match(COMMA)) {
						if (!match(RP)) {
							delete exp;
							PARSER_ERROR_RETURN("Expected )", get(0)->lineno);
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

ASTNode *Parser::unary_expression() {
	
	DEBUG("UNARY expression")
	
	// FRAME:
	// exp
	
	if (match(NOT) || match(BITNOT) || match(PLUS) || match(MINUS)) {
		// ! EXP | ~EXP | -EXP | +EXP
		
		int token = get(-1)->token;
		int lineno = get(-1)->lineno;
		
		ASTNode *exp = unary_expression();
		if (checkNullExpression(exp)) 
			return NULL;
		
		ASTNode *expr = new ASTNode(lineno, token == PLUS ? POS : token == MINUS ? NEG : token);
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
			PARSER_ERROR_RETURN("Left side of the increment expected to be field", get(0)->lineno);
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
				PARSER_ERROR_RETURN("Right side of the increment expected to be field", get(0)->lineno);
			}
		} return exp;
	}
};

ASTNode *Parser::multiplication_expression() {
	
	DEBUG("MULTIPLICATION expression")
	
	// FRAME:
	// exp1
	// exp2
	
	ASTNode *left = unary_expression();
	if (checkNullExpression(left)) 
		return NULL;
	
	while (1) {
		if (match(MUL) || match(DIV) || match(MDIV) || match(MOD) || match(HASH)) {
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

ASTNode *Parser::addiction_expression() {
	
	DEBUG("ADDICTION expression")
	
	// FRAME:
	// exp1
	// exp2
	
	ASTNode *left = multiplication_expression();
	if (checkNullExpression(left)) 
		return NULL;
	
	while (1) {
		if (match(PLUS) || match(MINUS)) {
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

ASTNode *Parser::bitwise_shift_expression() {
	
	DEBUG("BITWISE SHIFT expression")
	
	// FRAME:
	// exp1
	// exp2
	
	ASTNode *left = addiction_expression();
	if (checkNullExpression(left)) 
		return NULL;
	
	while (1) {
		if (match(BITRSH) || match(BITLSH) || match(BITURSH)) {
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

ASTNode *Parser::comparison_expression() {
	
	DEBUG("COMPARISON expression")
	
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

ASTNode *Parser::equality_expression() {
	
	DEBUG("EQUALITY expression")
	
	// FRAME:
	// exp1
	// exp2
	
	ASTNode *left = comparison_expression();
	if (checkNullExpression(left)) 
		return NULL;
	
	while (1) {
		if (match(EQ) || match(NEQ)) {
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

ASTNode *Parser::bitwise_and_expression() {
	
	DEBUG("BITWISE AND expression")
	
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

ASTNode *Parser::bitwise_xor_exppression() {
	
	DEBUG("BITWISE XOR expression")
	
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

ASTNode *Parser::bitwise_or_expression() {
	
	DEBUG("BITWISE OR expression")
	
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

ASTNode *Parser::and_expression() {
	
	DEBUG("AND expression")
	
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

ASTNode *Parser::or_expression() {
	
	DEBUG("OR expression")
	
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

ASTNode *Parser::condition_expression() {
	
	DEBUG("CONDITIONAL expression")
	
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
		PARSER_ERROR_RETURN("Expected :", get(0)->lineno);
	}
	
	ASTNode *false_exp = assign_expression();
	if (checkNullExpression(false_exp)) {
		delete condition_exp;
		return NULL;
	}
	
	condition_exp->addChild(false_exp);
	
	return condition_exp;
};

ASTNode *Parser::assign_expression() {
	
	DEBUG("ASSIGN expression")
	
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
		match(ASSIGN_BITNOT)
		||
		match(ASSIGN_MDIV)
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
			PARSER_ERROR_RETURN("Left side of the assignment expected to be field", get(0)->lineno);
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

ASTNode *Parser::expression() {
	return assign_expression();
};

ASTNode *Parser::checkNotNullExpression() {
	ASTNode *n = expression();
	if (n == NULL && !error_)
		PARSER_ERROR_RETURN("NULL-pointer expression", get(0)->lineno);
	return n;
};

bool Parser::checkNullExpression(ASTNode *exp) {
	if (exp == NULL && !error_) {
		PARSER_ERROR_RETURN("NULL-pointer expression", get(0)->lineno);
		return 1;
	}
	return 0;
};

ASTNode *Parser::initializerstatement() {
	
	// Initilizer statement used by FOR initializer
	
	if (match(TEOF))
		PARSER_ERROR_RETURN("EOF Statement", get(0)->lineno);
	
	if (error_)
		return NULL;
	
	if (match(VAR) || match(SAFE) || match(LOCAL) || match(CONST)) {
		DEBUG("DEFINE statement")
		
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
				PARSER_ERROR_RETURN("Expected name", get(0)->lineno);
			}
			
			string *name = get(-1)->stringv->copy();
			
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
			definenode->addFirstObject(name);
			definenode->addFirstObject(type);
			
			if (!match(COMMA))
				break;
		}
		
		return definenode;
	}
	
	else {
		// Expression statement
		
		// FRAME:
		// expression
		
		DEBUG("\\/ EXPRESSION STATEMENT node")
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

bool Parser::peekStatementWithoutSemicolon() {
	
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
			get(0)->token == RAISE
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

ASTNode *Parser::statement() {
	
	// Parse statement & consume all semicolons after it
	
	if (match(TEOF))
		PARSER_ERROR_RETURN("EOF Statement", get(0)->lineno);
	
	if (error_)
		return NULL;
	
	ASTNode *s = statement_with_semicolons();
	
	while (match(SEMICOLON));
	
	return s;
};

ASTNode *Parser::statement_with_semicolons() {
	
	if (match(TEOF))
		PARSER_ERROR_RETURN("EOF Statement", get(0)->lineno);
	
	if (error_)
		return NULL;
	
	else if (match(SEMICOLON)) {
		// printf("____ %d %d %d\n", get(-1)->token, get(0)->token, get(1)->token);
		DEBUG("EMPTY node")
		ASTNode *empty = new ASTNode(get(-1)->lineno, EMPTY);
		
		return empty;
	}
	
	else if (match(IF)) {
		// FRAME:
		// condition
		// IF node
		// ELSE node
		
		DEBUG("IF node")
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
			PARSER_ERROR_RETURN("Expected )", get(0)->lineno);
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
		
		DEBUG("SWITCH node")
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
			PARSER_ERROR_RETURN("Expected )", get(0)->lineno);
		}
		
		if (match(SEMICOLON));
		else {
			if (!match(LC)) {
				delete switchcase;
				PARSER_ERROR_RETURN("Expected {", get(0)->lineno);
			}
			
			bool matched_default = 0;
			
			while (match(CASE) || match(DEFAULT)) {
				int lineno = get(-1)->lineno;
				
				if (get(-1)->token == CASE) { 
					// CASE node
					
					// FRAME:
					// condition
					// node
				
					DEBUG("CASE node")
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
						PARSER_ERROR_RETURN("Expected :", get(0)->lineno);
					}
					
					while (true) {
						if (error_)  {
							delete switchcase;
							return NULL;
						}
						if (match(TEOF)) {
							delete switchcase;
							PARSER_ERROR_RETURN("Expected }", get(0)->lineno);
						}
						if (get(0)->token == CASE || get(0)->token == DEFAULT || get(0)->token == RC)
							break;
						casenode->addChild(statement());
					}
					
				} else { 
					// DEFAULT node
					
					// FRAME:
					// node
				
					DEBUG("DEFAULT node")
					if (matched_default) {
						delete switchcase;
						PARSER_ERROR_RETURN("Duplicate default case", get(0)->lineno);
					}
					matched_default = 1;
					
					if (!match(COLON)) {
						delete switchcase;
						PARSER_ERROR_RETURN("Expected :", get(0)->lineno);
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
							PARSER_ERROR_RETURN("Expected }", get(0)->lineno);
						}
						if (get(0)->token == CASE || get(0)->token == DEFAULT || get(0)->token == RC)
							break;
						defaultnode->addChild(statement());
					}
				}
			}
			
			if (!match(RC)) {
				delete switchcase;
				PARSER_ERROR_RETURN("Expected }", get(0)->lineno);
			}
		}
		
		/*
		// Insert default after condition
		if (defaultnode == NULL) {
			defaultnode = new ASTNode(-1, EMPTY);
		
			// Add visitor function for this node
			defaultnode->node_visit = NULL; // TODO: NODE_VISITOR
		}
		ASTNode *tmp = switchcase->left->next;
		switchcase->left = defaultnode;
		defaultnode->next = tmp;
		*/
		
		return switchcase;
	}
	
	else if (match(WHILE)) {
		// FRAME:
		// condition
		// BODY node
		
		DEBUG("WHILE node")
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
			PARSER_ERROR_RETURN("Expected )", get(0)->lineno);
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
		
		DEBUG("DO WHILE node")
		ASTNode *doloop = new ASTNode(get(-1)->lineno, DO);
		ASTNode *body   = statement();
		
		if (!match(WHILE)) {
			delete doloop;
			delete body;
			PARSER_ERROR_RETURN("Expected while", get(0)->lineno);
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
			PARSER_ERROR_RETURN("Expected )", get(0)->lineno);
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
		
		DEBUG("FOR node")
		ASTNode *forloop = new ASTNode(get(-1)->lineno, FOR);
		
		if (!match(LP)) {
			delete forloop;
			PARSER_ERROR_RETURN("Expected )", get(0)->lineno);
		}
		
		if (!match(SEMICOLON)) {
			ASTNode *initialization = initializerstatement();
			forloop->addChild(initialization);
			
			if (!match(SEMICOLON)) {
				delete forloop;
				PARSER_ERROR_RETURN("Expected ;", get(0)->lineno);
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
				PARSER_ERROR_RETURN("Expected ;", get(0)->lineno);
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
				PARSER_ERROR_RETURN("Expected )", get(0)->lineno);
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
				PARSER_ERROR_RETURN("Expected }", get(0)->lineno);
			}
			if (match(RC))
				break;
			
			blocknode->addChild(statement());
		}
		
		return blocknode;
	}
	
	else if (match(BREAK)) {
		
		DEBUG("BREAK node");
		ASTNode *breaknode = new ASTNode(get(-1)->lineno, BREAK);
		
		return breaknode;
	}
	
	else if (match(CONTINUE)) {
		
		DEBUG("CONTINUE node");
		ASTNode *coontinuenode = new ASTNode(get(-1)->lineno, CONTINUE);
		
		return coontinuenode;
	}
	
	else if (match(RETURN)) {
		
		DEBUG("RETURN node");
		// FREAME:
		// value / EMPTY
		
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
	
	else if (match(RAISE)) {
		
		DEBUG("RAISE node");
		// FREAME:
		// value / EMPTY
		
		ASTNode *raisenode = new ASTNode(get(-1)->lineno, RAISE);
		
		if (peekStatementWithoutSemicolon()) {
			ASTNode *empty = new ASTNode(-1, EMPTY);
			raisenode->addChild(empty);
		} else {
			ASTNode *exp = checkNotNullExpression();
			
			// Check for NULL expressin and ommit memory leak
			if (error_) {
				delete raisenode;
				return NULL;
			}
			raisenode->addChild(exp);
		}
		
		return raisenode;
	}
	
	else if (match(TRY)) {
		// FRAME
		// try node
		// expect node
		// OBJECTS:
		// handler name / null
		
		DEBUG("TRY node")
		ASTNode *tryexpect = new ASTNode(get(-1)->lineno, TRY);
		tryexpect->addChild(statement());
		
		if (match(EXPECT)) {
			if (match(LP)) {
				if (!match(NAME)) {
					delete tryexpect;
					PARSER_ERROR_RETURN("Expected name", get(0)->lineno);
				}
				
				string *name = get(-1)->stringv->copy();
				
				if (!match(RP)) {
					delete name;
					delete tryexpect;
					PARSER_ERROR_RETURN("Expected )", get(0)->lineno);
				}
				
				tryexpect->addLastObject(name);
			}
			
			tryexpect->addChild(statement());
		} else {
			ASTNode *empty = new ASTNode(-1, EMPTY);
			tryexpect->addChild(empty);
		}
		
		return tryexpect;
	}
	
	else
		return initializerstatement();
};

ASTNode *Parser::parse() {
	ASTNode *root = NULL;
	
	if (!repl)
		root = new ASTNode(0, ASTROOT);
	
	if (error_ || !_global_exec_state || eof_)
		return NULL;
	
	while (!eof()) {
		ASTNode *node = statement();
		
		if (error_ || !_global_exec_state)
			break;
		
		if (node != NULL)
			root->addChild(node);
		else {
			eof_ = 1;
		}
		
		if (repl)
			return node;
	}
	
	if (error_) {
		delete root;
		PARSER_ERROR_RETURN("Parser Error", get(0)->lineno);
		return NULL;
	}
	
	return root;
};

int Parser::lineno() {
	return get(0)->lineno;
};

int Parser::eof() {
	return // source->eof() || 
		eof_ || error_;
};


