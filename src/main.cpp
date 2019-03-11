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

#include "objects/Error.h"
#include "objects/Undefined.h"
#include "objects/Null.h"

// #define DEBUG_OUTPUT

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
using namespace ck_objects;


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
	
#ifdef DEBUG_OUTPUT
	wcout << "AST:" << endl;
	printAST(n);
	
	wcout << "Bytecodes: " << endl;
	print(scr->bytecode.bytemap);
	wcout << endl;
				
	wcout << "Lineno Table: " << endl;
	print_lineno_table(scr->bytecode.lineno_table);
	wcout << endl;
#endif
	
	/*
	for (int i = 0; i < scr->bytecode.bytemap.size(); ++i)
		wcout << "[" << i << "] " << (int) scr->bytecode.bytemap[i] << endl;
	*/
	
	delete n;
	
	// Initialize GIL, GC and other root components
	GIL* gil = new GIL(); // <-- all is done inside
	
	vscope *scope = ck_objects::primary_init(); // ?? XXX: Use prototype object for all classes
	scope->root();
	
	// Indicates if main returned an exception
	bool exception_stated = 0;
	// Instance of handled exception
	ck_message message;
	
	while (1) {
		try {
			if (!exception_stated) {
				GIL::executer_instance()->execute(scr, scope);
				GIL::current_thread()->clear_blocks();
			
				// Finish execution loop on success
				break;
				
			} else {
				GIL::executer_instance()->clear();
				exception_stated = 0;
				
				// On exception caught, call stack, windows stack and try stack are empty.
				// Process exception by calling handler-function.
				// __defexceptionhandler(exception)
				
				vobject* __defexceptionhandler = scope->get(L"__defexceptionhandler");
				if (__defexceptionhandler == nullptr || __defexceptionhandler->is_typeof<Undefined>() || __defexceptionhandler->is_typeof<Null>())
					if (message.get_type() == ck_message_type::CK_OBJECT && message.get_object() != nullptr)
						wcerr << "Unhandled error: " << message.get_object()->string_value() << endl;
					else
						wcerr << "Unhandled error: " << message << endl;
				else
					GIL::executer_instance()->call_object(__defexceptionhandler, nullptr, { new Error(message) }, L"__defexceptionhandler", scope);
				
				// Clear thread blocks after each execution of side code to avoid fake blocking of thread.
				GIL::current_thread()->clear_blocks();
				
				// If reached this statement, then exception was processed correctly
				break;
			}
		} catch (const ck_message& msg) {
			GIL::current_thread()->clear_blocks();
			
			exception_stated = 1;
			message = msg;
		} catch (const std::exception& msg) {
			GIL::current_thread()->clear_blocks();
			
			exception_stated = 1;
			message = msg;
		} catch (...) {
			GIL::current_thread()->clear_blocks();
			
			exception_stated = 1;
			message = ck_message(ck_exceptions::ck_message_type::NATIVE_EXCEPTION);
		}
	}
	
	// Normally the next step is make this program wait for signals or other threads to die.
	
	// Free up heap
	delete gil;
	delete scr;
	
	// wcout << "MAIN EXITED" << endl;
	return 0;
};