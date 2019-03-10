#include "parser.h"
#include "executer.h"
#include "exceptions.h"
#include "translator.h"
#include "script.h"
#include "sfile.h"
#include "GIL2.h"
#include "primary_init.h"
#include "vscope.h"
#include "ASTPrinter.h"

using namespace std;
using namespace ck_token;
using namespace ck_parser;
using namespace ck_ast;
using namespace ck_translator;
using namespace ck_bytecodes;
using namespace ck_sfile;
using namespace ck_exceptions;
using namespace ck_core;
using namespace ck_vobject;


// make && valgrind --leak-check=full --track-origins=yes ./test

// About passing vector without reference: https://stackoverflow.com/questions/15120264/when-is-a-vector-copied-when-is-a-reference-passed

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
	scr->directory = get_current_working_dir();
	scr->filename  = wstring(mbfilename.begin(), mbfilename.end());
	translate(scr->bytecode.bytemap, scr->bytecode.lineno_table, n);
	
	wcout << "AST:" << endl;
	printAST(n);
	
	wcout << "Bytecodes: " << endl;
	print(scr->bytecode.bytemap);
	wcout << endl;
				
	wcout << "Lineno Table: " << endl;
	print_lineno_table(scr->bytecode.lineno_table);
	wcout << endl;
	
	/*
	for (int i = 0; i < scr->bytecode.bytemap.size(); ++i)
		wcout << "[" << i << "] " << (int) scr->bytecode.bytemap[i] << endl;
	*/
	
	delete n;
	
	
	// Initialize GIL, GC and other root components
	GIL* gil = new GIL(); // <-- all is done inside
	
	vscope *scope = ck_objects::primary_init(); // ?? XXX: Use prototype object for all classes
	scope->root();
	
	try {
		GIL::executer_instance()->execute(scr, scope);
	} catch (const ck_message& msg) {
		wcout << "Unhandled error: " << msg << endl;
		// XXX: High level elevated exception. Should exit, or process it via system.defexceptionhandler()
	}
	
	// Normally the next step is make this program wait for signals or other threads to die.
	
	// Free up heap
	delete gil;
	delete scr;
	
	// wcout << "MAIN EXITED" << endl;
	return 0;
};