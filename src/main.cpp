#include "parser.h"
#include "executer.h"
#include "script.h"
#include "sfile.h"
#include "GIL2.h"

using namespace std;
using namespace ck_token;
using namespace ck_parser;
using namespace ck_ast;
using namespace ck_translator;
using namespace ck_bytecodes;
using namespace ck_core;


// make && valgrind --leak-check=full --track-origins=yes ./test

int main(int argc, const char** argv) {
	FILE *f = fopen("test.ck", "r");
	string mbfilename("test.ck");
	
	if (!f)
		return 1;
	
	// Convert input file to AST
	stream_wrapper sw(f);
	parser_massages pm(L"test.ck");
	parser p(pm, sw);
	ASTNode* n = p.parse();
	if (pm.errors()) {
		pm.print();
		delete n;
		return 1;
	}
	
	// Convert AST to bytecodes & initialize script instance
	ck_script* scr = new ck_script();
	scr->directory = ck_sfile::get_current_working_dir();
	scr->filename  = std::wstring(mbfilename.begin(), mbfilename.end());
	translate(scr->bytecode.bytemap, scr->bytecode.lineno_table, n);
	delete n;
	
	
	// Initialize GIL, GC and other root components
	GIL* gil = new GIL(); // <-- all is done inside
	
	GIL::executer_instance()->execute(scr);
	
	// Free up heap
	delete gil;
	delete scr;
	
	return 0;
};