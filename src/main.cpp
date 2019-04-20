#include <csignal>
#include <fstream>
#include <codecvt>
#include <locale>

#include "stack_utils.h"

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

#include "objects/Cake.h"
#include "objects/Int.h"
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

// XXX: Use -DCK_SINGLETHREAD to defined singlethreaded usage and CK_SINGLETHREAD macro value

// make && valgrind --leak-check=full --track-origins=yes ./test

// About passing vector without reference: https://stackoverflow.com/questions/15120264/when-is-a-vector-copied-when-is-a-reference-passed

// Main-local fields
static vscope* root_scope;
static ck_script* main_script;
static GIL* gil_instance;

// Signal handling for main thread
static bool has_signaled;
static int signaled_number;

// SIGNALS + THREADS = POSIX
// https://gist.github.com/rtv/4989304
// https://habr.com/ru/post/141206/
// https://devarea.com/linux-handling-signals-in-a-multithreaded-application/#.XLoeW5gzbIU
// Signals setup for handling them during execution
static void signal_handler(int sig) {
	// Set up ignore signals during signal processing
	signal(SIGINT,  signal_handler); // <-- user interrupt
	signal(SIGKILL, signal_handler); // <-- kill signal
	signal(SIGTERM, signal_handler); // <-- Terminate request
	signal(SIGABRT, signal_handler); // <-- abortion is murder
	
	// Forcibly terminate the process
	if (sig == SIGTERM) {
		GIL::instance()->terminate();
		return;
	}
	
	// Warning: checking thread for alive state and then 
	//  processing signal with late_call_object().
	// If executer was finishing it's work while signal received, 
	//  no signal processing would be done.
	if (GIL::current_thread()->is_alive()) {
		// Thread is alive.
		// Using late_call_object() to execute __defsignalhandler() on the 
		//  next step of execute_bytecode().
		// If executer was finishing it's work and no more rutting execution loop, 
		//  signal is ignored.
		
		vobject* __defsignalhandler = root_scope->get(L"__defsignalhandler");
		if (__defsignalhandler == nullptr || __defsignalhandler->is_typeof<Undefined>() || __defsignalhandler->is_typeof<Null>()) {
			// No handler is found. Default action is terminate.
			GIL::instance()->terminate();
		} else if (GIL::executer_instance()->late_call_size() == 0)
			// Function should be appended only if there is no other events to prevent corruption.
			GIL::executer_instance()->late_call_object(__defsignalhandler, nullptr, { new Int(sig) }, L"__defsignalhandler", root_scope);
	} else {
		has_signaled = 1;
		// Clear blocking and mark thread alive
		GIL::current_thread()->clear_blocks();
		GIL::current_thread()->set_alive(1);
		signaled_number = sig;
	}
};

int main(int argc, const char** argv) {
	// Request maximal stack for this process.
	ck_util::maximize_stack_size();

	// Set up locales
	setlocale(LC_ALL, "");
	
	// Create UTF-8 locale
	std::locale empty_locale;
	auto codecvt = new std::codecvt_utf8<wchar_t>();
	std::locale utf8_locale(empty_locale, codecvt); 
	
	// Apply locale on stdin
	wcin.imbue(utf8_locale);
	
	// Apply locale on stdout
	wcout.imbue(utf8_locale);
	wcerr.imbue(utf8_locale);
	
	// Set up input stream
	std::wifstream file("test.ck");
	// Preserve whitespaces for parser
	file >> std::noskipws;
	// Apply encoding UTF-8
	file.imbue(utf8_locale);
	
	// Preserve file name
	wstring wfilename(L"test.ck");
	
	if (file.fail())
		return 1;
	
	// Convert input file to AST
	stream_wrapper sw(file);
	parser_massages pm(wfilename);
	parser* p = new parser(pm, sw);
	ASTNode* n = p->parse();
	delete p;
	
	file.close();
	
	if (pm.errors()) {
		pm.print();
		delete n;
		return 1;
	}
	
	// Convert AST to bytecodes & initialize script instance
	main_script = new ck_script();
	main_script->directory = get_current_working_dir();
	main_script->filename  = wfilename;
	translate(main_script->bytecode.bytemap, main_script->bytecode.lineno_table, n);
	
#ifdef DEBUG_OUTPUT
	wcout << "AST:" << endl;
	printAST(n);
	
	wcout << "Bytecodes: " << endl;
	print(main_script->bytecode.bytemap);
	wcout << endl;
				
	wcout << "Lineno Table: " << endl;
	print_lineno_table(main_script->bytecode.lineno_table);
	wcout << endl;
#endif
	
	/*
	for (int i = 0; i < main_script->bytecode.bytemap.size(); ++i)
		wcout << "[" << i << "] " << (int) main_script->bytecode.bytemap[i] << endl;
	*/
	
	delete n;
	
	// Initialize GIL, GC and other root components
	gil_instance = new GIL(); // <-- all is done inside
	
	root_scope = ck_objects::primary_init(); // ?? XXX: Use prototype object for all classes
	root_scope->root();
	
	// Set up signals handling
	signal(SIGINT,  signal_handler); // <-- user interrupt
	signal(SIGKILL, signal_handler); // <-- kill signal
	signal(SIGTERM, signal_handler); // <-- Terminate request
	signal(SIGABRT, signal_handler); // <-- abortion is murder
	
	// Indicates if main returned an exception
	bool cake_started = 0;
	// Instance of catched cake
	cake message;
	
	while (1) {
		try {
			if (!cake_started) {
				GIL::executer_instance()->execute(main_script, root_scope);
				GIL::current_thread()->clear_blocks();
			
				// Finish execution loop on success
				break;
				
			} else {
				GIL::executer_instance()->clear();
				cake_started = 0;
				
				// On cake caught, call stack, windows stack and try stack are empty.
				// Process cake by calling handler-function.
				// __defcakehandler(exception)
				// The default behaviour is calling thread cake handler and then finish thread work.
				
				vobject* __defcakehandler = root_scope->get(L"__defcakehandler");
				if (__defcakehandler == nullptr || __defcakehandler->is_typeof<Undefined>() || __defcakehandler->is_typeof<Null>())
					if (message.get_type_id() == cake_type::CK_OBJECT && message.get_object() != nullptr)
						if (message.get_object()->is_typeof<Cake>())
							((Cake*) message.get_object())->print_backtrace();
						else
							wcerr << "Unhandled cake: 	" << message << endl;
					else
						wcerr << "Unhandled cake: " << message << endl;
				else
					GIL::executer_instance()->call_object(__defcakehandler, nullptr, { 
							message.get_type_id() == cake_type::CK_OBJECT ? message.get_object() : new Cake(message)
						}, L"__defcakehandler", root_scope);
				
				// Clear thread blocks after each execution of side code to avoid fake blocking of thread.
				GIL::current_thread()->clear_blocks();
				
				// If reached this statement, then exception was processed correctly
				break;
			}
		} catch (const cake& msg) {
			GIL::current_thread()->clear_blocks();
			
			cake_started = 1;
			message = msg;
		} catch (const std::exception& msg) {
			GIL::current_thread()->clear_blocks();
			
			cake_started = 1;
			message = msg;
		} catch (...) {
			GIL::current_thread()->clear_blocks();
			
			cake_started = 1;
			message = UnknownException();
		}
	}
	
	// Normally the next step is make this program wait for signals or other threads to die.
	/*
		conditional_variable.wait() => {
			if (has_signaled) {
				if (!GIL::lock_required ?)
					unblock for processing dignal
			} else if (threads_are_dead)
				goto end of main
		}
		vobject* __defsignalhandler = root_scope->get(L"__defsignalhandler");
		if (__defsignalhandler == nullptr || __defsignalhandler->is_typeof<Undefined>() || __defsignalhandler->is_typeof<Null>()) {
			// No handler is found. Default action is terminate.
			GIL::terminate();
		} else
			GIL::executer_instance()->late_call_object(__defsignalhandler, nullptr, { new Int(sig) }, L"__defsignalhandler", root_scope);
	*/
	
	// Free up heap
	delete gil_instance;
	delete main_script;
	
	delete codecvt;
	
	// wcout << "MAIN EXITED" << endl;
	return 0;
};

