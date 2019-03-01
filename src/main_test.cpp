#include "parser.h"

using namespace ck_token;
using namespace ck_parser;
using namespace std;

int main() {
	FILE *f = fopen("test.ck", "r");
	
	if (!f)
		return 1;
	
	parser_massages pm(L"test.ck");
	stream_wrapper sw(f);
	tokenizer tok(pm, sw);
	int lineno = 0;
	wcout << "[0] ";
	
	while (tok.next_token()) {
		if (tok.token->token == TERR)
			break;
		
		if (lineno != tok.token->lineno) {
			lineno = tok.token->lineno;
			wcout << endl << '[' << lineno << "] ";
		}
		wcout << *tok.token << ' ';
	}
	wcout << endl;
	
	pm.print();
};