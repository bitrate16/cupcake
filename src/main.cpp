#include <csignal>
#include <fstream>
#include <codecvt>
#include <locale>
#include <cstring>

#include "parser.h"
#include "translator.h"
#include "sfile.h"
#include "script.h"
#include "executer.h"
#include "exceptions.h"
#include "GIL2.h"
#include "exit_listener.h"
#include "ck_args.h"
#include "stack_locator.h"

#include "ASTPrinter.h"

#include "vscope.h"
#include "init_default.h"

#include "objects/Cake.h"
#include "objects/Int.h"
#include "objects/Undefined.h"
#include "objects/Null.h"
#include "objects/Array.h"
#include "objects/String.h"

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

// Main entry file.
// Performs setting up default encoding for input-output streams.
// Sets up environmental variables, composes arguments into __args array in global scope.
// Defines global root scope and fills it with default fields like Object types, default 
//  set of functions and constants.
// Performs parsing of input code from file or (WIP) REPL console.
// Performs translating input AST into bytecode and executed it on new instance of executer.
// Initializes GIL2, executer and thread instance pointer for this MAIN thread.
// Does waiting for other processes to finish their work and locks until they are done.
// Does acting on input system signals by calling __defsignalhandler or calling GIL2::exit() 
//  funciton to force stop the execution.
// Finally, performs GarbageCollection and disposes all threads and globals.
//
// Optional build flags: 
// -DCK_SINGLETHREAD - disable multithreading feature, disable locks / unlocks, block queue, 
//  GIL2 synchronization and runs in a single thread.
// -DDEBUG_OUTPUT - enables debug oytput of parsed AST, bytecode with captions and line 
//  numbers table.


// Main-local fields
// Root execution scope is stored here to be accessible 
//  from signal handler and main functions both.
static vscope* root_scope;
// Main script instance is stored here to be accessoble..
//  ..why is it here?
static ck_script* main_script;
// Gil instance is stored inside global to be accessible 
//  from signal handler and main both.
static GIL* gil_instance;
// Instance of executer of the main thread.
// Uses direct access during handling signal
static ck_executer* main_executer;
// Instance of thread for main.
// Uses direct access during handling signal
static gil_thread* main_thread;

// Signal state is stored in blobal to be accessible 
//  from main and signal handler.
static bool has_signaled = 0;
static int  signaled_number = 0;

// Exception processing state of main function.
bool exception_processing = 0;
// Instance of catched exception.
cake message;

/*
// https://devarea.com/linux-handling-signals-in-a-multithreaded-application/#.XLsn95gzbIU
// 
// XXX:
//      Handling signals in any of existing threads
//      but after signal receiver, handler thread should 
//      save signum in GIL and notify all threads about it.
//      any thread will ignore the signal but main thread will woke 
//      up if it was waiting or check GIL signal flag
//      and process __defsighandler in a new call_object
//      subcall. As the result, any thread should hangle a signal as 
//      a normal signal handle but only main thread should process the signal.

// XXX:
//      Handling kill on a thread.
//      When thread changes it's execution state from alive & !dead to !alive & dead
//      it should stup current execution state as it is and perform thread-die 
//      operations in __defexithandler. 
//      There are two ways:
//       1. On thread being killed, executer handles flag change on the next step 
//          and fully ternimates it's work.
//       2. On thread being kille, executer keeps working in any case and only 
//          after the execution __defexithandler is being called.
//      As the result the most preferred bahaviour is 1.
//      On alive state changed executer should handle the state and exit.
//      To indicate the finalization ... what?
// XXX: 
//      Still expected solvation for determining whenever thread was killed.
// SOL: 
//      Instead of using any type of flags, disable usage of alive, dead
//      and use only running flag.

// XXX: 
//      On thread changing it's state to !running executer simple stops and exits.

// The default signal behaviour:
//     Only main thread receives signals from outer world.
//      When main receives a signal it causes following spicious wakeup and 
//      thread checks local condition for a signal being received.
//     If thread was not waiting on the condition, it calls late_object_call
//      and process signum on current stack state.
//     During the signal processing thread ignores all signals and after the
//      processing finished it returns handler back.

// SIGNALS + THREADS = POSIX
// https://gist.github.com/rtv/4989304
// https://habr.com/ru/post/141206/
// https://devarea.com/linux-handling-signals-in-a-multithreaded-application/#.XLoeW5gzbIU
// Signals setup for handling them during execution
*/
static void signal_handler(int sig) {
#if defined(LINUX)
	// Set up ignore signals during signal processing
	// $ kill -l
	//  1) SIGHUP       2) SIGINT       3) SIGQUIT      4) SIGILL
	//  5) SIGTRAP      6) SIGABRT      7) SIGBUS       8) SIGFPE
	//  9) SIGKILL     10) SIGUSR1     11) SIGSEGV     12) SIGUSR2
	// 13) SIGPIPE     14) SIGALRM     15) SIGTERM     17) SIGCHLD
	// 18) SIGCONT     19) SIGSTOP     20) SIGTSTP     21) SIGTTIN
	// 22) SIGTTOU     23) SIGURG      24) SIGXCPU     25) SIGXFSZ
	// 26) SIGVTALRM   27) SIGPROF     28) SIGWINCH    29) SIGIO
	// 30) SIGPWR      31) SIGSYS      34) SIGRTMIN    35) SIGRTMIN+1
	// 36) SIGRTMIN+2  37) SIGRTMIN+3  38) SIGRTMIN+4  39) SIGRTMIN+5
	// 40) SIGRTMIN+6  41) SIGRTMIN+7  42) SIGRTMIN+8  43) SIGRTMIN+9
	// 44) SIGRTMIN+10 45) SIGRTMIN+11 46) SIGRTMIN+12 47) SIGRTMIN+13
	// 48) SIGRTMIN+14 49) SIGRTMIN+15 50) SIGRTMAX-14 51) SIGRTMAX-13
	// 52) SIGRTMAX-12 53) SIGRTMAX-11 54) SIGRTMAX-10 55) SIGRTMAX-9
	// 56) SIGRTMAX-8  57) SIGRTMAX-7  58) SIGRTMAX-6  59) SIGRTMAX-5
	// 60) SIGRTMAX-4  61) SIGRTMAX-3  62) SIGRTMAX-2  63) SIGRTMAX-1
	// 64) SIGRTMAX
	
	// Critical
	signal(SIGTERM, signal_handler);
	signal(SIGSEGV, signal_handler);
	signal(SIGILL,  signal_handler);
	signal(SIGQUIT, signal_handler);
	signal(SIGABRT, signal_handler);
	signal(SIGSTOP, signal_handler);
	signal(SIGKILL, signal_handler);
	
	// Handleable with care 
	signal(SIGINT, signal_handler);
	
	// Just handleable
	signal(SIGUSR1, signal_handler);
	signal(SIGUSR2, signal_handler);
	
	// Deprecated: libc uses this signals to communicate between several threads.
	//             POSIX ONLY.
	// for (int i = SIGRTMIN; i < SIGRTMAX; ++i)
	// 	signal(i,  signal_handler);
	
	// Forcibly terminate the process
	if (   sig == SIGTERM 
		|| sig == SIGQUIT
		|| sig == SIGABRT
		|| sig == SIGSTOP
		|| sig == SIGKILL
		|| sig == SIGILL) {
		GIL::instance()->stop();
		return;
	}
	
	// Segmentation fault on program
	if (sig == SIGSEGV) {
		std::wcerr << "Segmentation fault" << std::endl;
		exit(1);
	}
#elif defined(WINDOWS)
	// SIGABRT  Abnormal termination
	// SIGFPE   Floating-point error
	// SIGILL   Illegal instruction
	// SIGINT   CTRL+C signal
	// SIGSEGV  Illegal storage access
	// SIGTERM  Termination request
	
	// Critical
	signal(SIGTERM, signal_handler);
	signal(SIGABRT, signal_handler);
	signal(SIGILL, signal_handler);
	signal(SIGSEGV, signal_handler);
	
	// Handleable with care
	signal(SIGINT, signal_handler);
	
	if (   sig == SIGTERM 
		|| sig == SIGABRT
		|| sig == SIGILL) {
		GIL::instance()->stop();
		return;
	}
	
	// Segmentation fault on program
	if (sig == SIGSEGV) {
		std::wcerr << "Segmentation fault" << std::endl;
		exit(1);
	}
#endif
	
	// In WIN32 there is other behaviour.
	// Signal handler starts in the new thread and it is not recorded by the GIL.
#if defined(WIN32)
	if (main_thread->is_running()) {		
		vobject* __defsignalhandler = root_scope->get(L"__defsignalhandler");
		if (__defsignalhandler == nullptr || __defsignalhandler->as_type<Undefined>() || __defsignalhandler->as_type<Null>()) {
			if (sig == SIGINT)
				GIL::instance()->stop();
		} else if (main_executer->late_call_size() == 0) 
			main_executer->late_call_object(__defsignalhandler, nullptr, { new Int(sig) }, L"__defsignalhandler", root_scope);
	} else {
		// No handler to put into late call.
		// Main thread may accidentally wake and say: 
		//  "Look, there was a signal, let's process it"
		// Then it will process the sgnal and wait for other threads 
		//  to stop doing what they are doing.
		has_signaled    = 1;
		signaled_number = sig;
		main_thread->clear_blocks();
		main_thread->restate();
	}
#else
	// Warning: checking thread for alive state and then 
	//  processing signal with late_call_object().
	// If executer was finishing it's work while signal received, 
	//  no signal processing would be done.
	if (main_thread->is_running()) {
		// Thread is alive.
		// Using late_call_object() to execute __defsignalhandler() on the 
		//  next step of execute_bytecode().
		// If executer was finishing it's work and no more rutting execution loop, 
		//  signal is ignored.
		
		vobject* __defsignalhandler = root_scope->get(L"__defsignalhandler");
		if (__defsignalhandler == nullptr || __defsignalhandler->as_type<Undefined>() || __defsignalhandler->as_type<Null>()) {
			// No handler is found. 
			// Default action for SIGINT is terminate.
			// Default action for [SIGRTMIN, SIGRTMAX], SIGUSR1, SIGUSR2 is nothing.
#if defined(LINUX)
			if (sig == SIGINT)
				GIL::instance()->stop();
#elif defined(WINDOWS)
			if (sig == SIGINT)
				GIL::instance()->stop();
#endif
		} else if (main_executer->late_call_size() == 0) 
			main_executer->late_call_object(__defsignalhandler, nullptr, { new Int(sig) }, L"__defsignalhandler", root_scope);
	} else {
		has_signaled    = 1;
		signaled_number = sig;
		// Clear blocking and mark thread alive
		main_thread->clear_blocks();
		main_thread->restate();
	}
#endif
	
	// GIL::instance()->notify();
};

// Main wrapper.
// Called after setting up locales, parsing arguments and environment, initializing 
void wrap_main(int argc, void** argv) {
	// E X E C U T E _ P R O G R A M
	
	while (1) {
		try {
			if (!exception_processing) {
				// Normal execution, no exception processing
				GIL::executer_instance()->execute(main_script, root_scope);
				GIL::executer_instance()->restore_all();
				GIL::current_thread()->clear_blocks();
			
				// Finish execution loop on success
				break;
				
			} else {
				// Processing exception
				GIL::executer_instance()->restore_all();
				exception_processing = 0;
				
				// On cake caught, call stack, windows stack and try stack are empty.
				// Process cake by calling handler-function.
				// __defcakehandler(exception)
				// The default behaviour is calling thread cake handler and then finish thread work.
				// XXX: Disable processing stack overflow and out of memory
				// XXX: Add parameter to ck_cake to mark it as uncatchable.
				// XXX: GIL::is_stopped atomic to prevent exception print twice.
				//      Check & update before print.
				
				vobject* __defcakehandler = root_scope->get(L"__defcakehandler");
				if (__defcakehandler == nullptr || __defcakehandler->as_type<Undefined>() || __defcakehandler->as_type<Null>() || !message.is_handleable()) {
					if (message.get_type_id() == cake_type::CK_OBJECT && message.get_object() != nullptr)
						if (message.get_object()->as_type<Cake>()) {
							std::wcout << "Unhandled cake in Thread main: ";
							((Cake*) message.get_object())->print_backtrace();
						} else
							wcerr << "Unhandled cake in Thread main: " << message << endl;
					else
						wcerr << "Unhandled cake in Thread main: " << message << endl;
					
					// Unhandled exception -> terminate
					GIL::instance()->stop();
				} else
					GIL::executer_instance()->call_object(__defcakehandler, nullptr, { 
							message.get_type_id() == cake_type::CK_OBJECT ? message.get_object() : new Cake(message)
						}, L"__defcakehandler", root_scope);
				
				// Clear thread blocks after each execution of side code to avoid fake blocking of thread.
				GIL::current_thread()->clear_blocks();
				
				// If reached this statement, then exception was processed correctly
				break;
			}
		} catch (const cake& msg) {
			exception_processing = 1;
			message = msg;
			message.collect_backtrace();
			
			// Catch ck message exception
			GIL::executer_instance()->restore_all();
			GIL::current_thread()->clear_blocks();
		} catch (const std::exception& msg) {
			exception_processing = 1;
			message = msg;
			message.collect_backtrace();
			
			// Catch native message exception
			GIL::executer_instance()->restore_all();
			GIL::current_thread()->clear_blocks();
		} catch (...) {
			exception_processing = 1;
			message = UnknownException();
			message.collect_backtrace();
			
			// Catch something unresolved
			GIL::executer_instance()->restore_all();
			GIL::current_thread()->clear_blocks();
		}
	}
	
	// W A I T _ F O R _ T H R E A D S _ O R _ S I G N A L S
	
	while (1) {
		// Main thread locks the GIL
		GIL::instance()->lock();
		
		// Mark main thread as stopped
		GIL::current_thread()->set_running(0);
		
		// Main thread starts waiting for other threads to die from he years
		GIL::instance()->sync_var().wait(GIL::instance()->sync_mtx(), []() -> bool {
			
			// Priority race:
			//  Racing Thread:
			//   1. locks the GIL::lock()
			//   2. shuts down, sets running flag to 0
			//   3. sends signal
			//   4. unlocks GIL::unlock()
			//  Main Thread: 
			//   1. locks the GIL::lock()
			//   2. checks all threads running state
			//   3. joins GIL::sync_var.wait and unlocks the GIL::unlock().
			
			// Waiting for thermal death of the universe
		
			std::unique_lock<std::recursive_mutex> lk(GIL::instance()->threads_mtx());
			
			bool universe_is_dead = 1;
			for (int i = 1; i < GIL::instance()->get_threads().size() && universe_is_dead; ++i) // loop from 1 because main thread index is 0
				if (GIL::instance()->get_threads()[i]->is_running()) {
					universe_is_dead = 0;
					break;
				}
			
			return universe_is_dead || has_signaled;
		});
		
		// Universe is dead..
		// This thread is the last thread in the entire world.
		// Latest species in the whole deep and dark cold world.
		// The world that will never see the daylight or feel a nearby star heat.
		// The entropy starts decaying and the latest atoms are disposing into
		// straignt missing energy.
		// Darkness and cold covers everything and even supermassive black holes
		// are starting to evaporate and disappear.
		// Nothing, entirely empty and without echo clack space.
		// .. or a regular signal wakes it up before the supermassive dark matter collapse.
		
		GIL::instance()->unlock();
		
		if (!has_signaled)
			break;
		
		vobject* __defsignalhandler = root_scope->get(L"__defsignalhandler");
		if (__defsignalhandler == nullptr || __defsignalhandler->as_type<Undefined>() || __defsignalhandler->as_type<Null>()) {
			// No default signal handler
			GIL::instance()->stop();
			break;
		}
		
		exception_processing = 0;
		
		// Reset blocks keys
		GIL::current_thread()->set_running(1);
		
		while (1) {
			try {
				if (!exception_processing) {
					// Run as regular function
					// Clear executer state and remove all late call instances
					GIL::executer_instance()->clear();
					GIL::executer_instance()->call_object(__defsignalhandler, nullptr, { new Int(signaled_number) }, L"__defsignalhandler", root_scope);
					GIL::current_thread()->clear_blocks();
				
					// Finish execution loop on success
					break;
					
				} else {
					// Run exception processing
					GIL::executer_instance()->clear();
					exception_processing = 0;
					
					// On cake caught, call stack, windows stack and try stack are empty.
					// Process cake by calling handler-function.
					// __defcakehandler(exception)
					// The default behaviour is calling thread cake handler and then finish thread work.
					
					vobject* __defcakehandler = root_scope->get(L"__defcakehandler");
					if (__defcakehandler == nullptr || __defcakehandler->as_type<Undefined>() || __defcakehandler->as_type<Null>() || !message.is_handleable())
						if (message.get_type_id() == cake_type::CK_OBJECT && message.get_object() != nullptr)
							if (message.get_object()->as_type<Cake>())
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
				// Catch ck message exception
				GIL::current_thread()->clear_blocks();
				
				exception_processing = 1;
				message = msg;
			} catch (const std::exception& msg) {
				// Catch native message exception
				GIL::current_thread()->clear_blocks();
				
				exception_processing = 1;
				message = msg;
			} catch (...) {
				// Catch something unresolved
				GIL::current_thread()->clear_blocks();
				
				exception_processing = 1;
				message = UnknownException();
			}
		}
	}
	
	// W A I T _ F O R _ T E R M I N A T E
	
	// Main thread locks the GIL
	GIL::instance()->lock();
	
	// Finally wait for all threads to finish if process was terminated via GIL::terminate()
	GIL::current_thread()->set_running(0);
	
	// Main thread starts waiting for other threads to die from the years
	GIL::instance()->sync_var().wait(GIL::instance()->sync_mtx(), []() -> bool {
		
		// Waiting for thermal death of the universe
		
		std::unique_lock<std::recursive_mutex> lk(GIL::instance()->threads_mtx());
		
		bool universe_is_dead = 1;
		for (int i = 1; i < GIL::instance()->get_threads().size() && universe_is_dead; ++i) // loop from 1 because main thread index is 0
			if (!GIL::instance()->get_threads()[i]->is_running()) {
				universe_is_dead = 0;
				break;
			}
		
		return universe_is_dead;
	});
	
	GIL::instance()->unlock();
}

int main(int argc, const char** argv, const char** envp) {
	
	// S E T _ U P _ L O C A L E S
	
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
	
	// Apply locale on stderr
	wcerr.imbue(utf8_locale);
	
	// Create converter between UTF8 and UTF16 characters.
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	
	// Preserve file name
	wstring wfilename;
	std::wifstream file;
	
	// P A R S E _ A R G U M E N T S
	
	ck_core::ck_args::parse(argc, argv);
	
	// Print help on command line options
	if (argc > 1 && std::strcmp("-h", argv[1]) == 0) {
		std::wcout << "Usage:" << std::endl;
		std::wcout << "ck -h Display help message." << std::endl;
		std::wcout << "ck <filename> Execute input file as cupcake script." << std::endl;
		std::wcout << "ck <filename> { --CK::<key>=<value> or --CK::<key> } For passing execution options." << std::endl;
		std::wcout << "ck <filename> { --CK::<key>=<value> or --CK::<key> or any other argument } Full execution syntax." << std::endl;
		std::wcout << std::endl;
		std::wcout << "List of execution options:" << std::endl;
		std::wcout << "--CK::STACK_SIZE=<size> Specify new stack size in bytes (> 8Mb)" << std::endl;
		std::wcout << "--CK::THREAD_STACK_SIZE=<size> Specify new stack size for threads in bytes (> 8Mb)" << std::endl;
		std::wcout << "--CK::MAX_HEAP_SIZE=<size> Limit heap size per current process, default is 512Mb, minimal is 8Mb" << std::endl;
		std::wcout << "--CK::MIN_GC_INTERVAL=<amount> Minimal amount of objects to be created before gc call, default is 64" << std::endl;
		std::wcout << "--CK::PRINT_BYTECODE Debug output bytecode" << std::endl;
		std::wcout << "--CK::PRINT_AST Debug output AST for input script" << std::endl;
		std::wcout << std::endl;
		std::wcout << "Example: ck cake.ck --CK::STACK_SIZE=1000000" << std::endl;
		return 0;
	}
	
	// Get new stack size as command line parameter
	// XXX: implement parsing of size notations, like mb, Mb, GB, Kb, etc..
	int ck_new_stack_size = 8 * 1024 * 1024; // By default, is >= 16Mb
	
	if (ck_core::ck_args::has_option(L"STACK_SIZE")) try { 
		int try_new_stack_size = std::stoi(ck_core::ck_args::get_option(L"STACK_SIZE"));
		
		if (try_new_stack_size > ck_new_stack_size)
			ck_new_stack_size = try_new_stack_size;
	} catch (...) {
		std::wcout << "Invalid value for option --CK::STACK_SIZE (" << ck_core::ck_args::get_option(L"STACK_SIZE") << std::endl;
		return 0;
	}
	
	if (ck_core::ck_args::has_option(L"THREAD_STACK_SIZE")) try { 
		// Check for valid integer
		std::stoi(ck_core::ck_args::get_option(L"THREAD_STACK_SIZE"));
	} catch (...) {
		std::wcout << "Invalid value for option --CK::THREAD_STACK_SIZE (" << ck_core::ck_args::get_option(L"THREAD_STACK_SIZE") << std::endl;
		return 0;
	}
	
	if (ck_core::ck_args::has_option(L"MAX_HEAP_SIZE")) try { 
		// Check for valid integer
		int new_heap_size = std::stol(ck_core::ck_args::get_option(L"MAX_HEAP_SIZE"));
		
		GC::MAX_HEAP_SIZE = new_heap_size < 8 * 1024 * 1024 ? 8 * 1024 * 1024 : new_heap_size;
	} catch (...) {
		std::wcout << "Invalid value for option --CK::MAX_HEAP_SIZE (" << ck_core::ck_args::get_option(L"MAX_HEAP_SIZE") << std::endl;
		return 0;
	}
	
	if (ck_core::ck_args::has_option(L"MIN_GC_INTERVAL")) try { 
		// Check for valid integer
		int new_gc_interval = std::stoi(ck_core::ck_args::get_option(L"MIN_GC_INTERVAL"));
		
		GC::MIN_GC_INTERVAL = new_gc_interval < 0 ? 0 : new_gc_interval;
	} catch (...) {
		std::wcout << "Invalid value for option --CK::MIN_GC_INTERVAL (" << ck_core::ck_args::get_option(L"MIN_GC_INTERVAL") << std::endl;
		return 0;
	}
	
	// P A R S E _ I N P U T
	
	// Process filename
	if (argc < 2) {
		// Default file is cake.ck
		file = std::wifstream("cake.ck");
		wfilename = L"cake.ck";
	} else {
		file = std::wifstream(argv[1]);
		wfilename = converter.from_bytes(argv[1]);
	}
	
	// Preserve whitespaces for parser
	file >> std::noskipws;
	
	// Apply encoding UTF-8
	file.imbue(utf8_locale);
	
	if (file.fail()) {
		if (argc < 2)
			std::wcerr << "No input file" << std::endl;
		else
			std::wcerr << "File " << wfilename << " not found" << std::endl;
		return 1;
	}
	
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
	
// #ifdef DEBUG_OUTPUT
	if (ck_core::ck_args::has_option(L"PRINT_AST")) {
		wcout << "AST:" << endl;
		printAST(n);
	}
		
	if (ck_core::ck_args::has_option(L"PRINT_BYTECODE")) {
		wcout << "Bytecodes: " << endl;
		print(main_script->bytecode.bytemap);
		wcout << endl;
					
		// wcout << "Lineno Table: " << endl;
		// print_lineno_table(main_script->bytecode.lineno_table);
		// wcout << endl;
	}
//#endif
	
	// Free up memory
	delete n;
	
	// I N I T _ G I L
	
	// Initialize GIL, GC and other root components
	gil_instance = new GIL(); // <-- all is done inside
	
	root_scope = ck_objects::init_default(); // ?? XXX: Use prototype object for all classes
	root_scope->root();
	
	// C O L L E C T _ A R G U M E N T S
	
	// Collect arguments of the program & pass them as __args
	Array* __args = new Array();
	for (int i = 0; i < ck_core::ck_args::args_size(); ++i)
		__args->items().push_back(new String(ck_core::ck_args::get_args()[i]));
	
	root_scope->put(L"__args", __args);
	
	// Collect environment values & pass them as __env
	Array* __env = new Array();
	for (int i = 0; envp[i]; ++i) 
		__env->items().push_back(new String(converter.from_bytes(envp[i])));
	
	root_scope->put(L"__env", __env);
	
	// Copy instance of executer
	main_executer = GIL::executer_instance();
	// Copy instance of thread
	main_thread   = GIL::current_thread();
	
	// Set up signals handling
#if defined(LINUX)
	// Set up ignore signals during signal processing
	// $ kill -l
	//  1) SIGHUP       2) SIGINT       3) SIGQUIT      4) SIGILL
	//  5) SIGTRAP      6) SIGABRT      7) SIGBUS       8) SIGFPE
	//  9) SIGKILL     10) SIGUSR1     11) SIGSEGV     12) SIGUSR2
	// 13) SIGPIPE     14) SIGALRM     15) SIGTERM     17) SIGCHLD
	// 18) SIGCONT     19) SIGSTOP     20) SIGTSTP     21) SIGTTIN
	// 22) SIGTTOU     23) SIGURG      24) SIGXCPU     25) SIGXFSZ
	// 26) SIGVTALRM   27) SIGPROF     28) SIGWINCH    29) SIGIO
	// 30) SIGPWR      31) SIGSYS      34) SIGRTMIN    35) SIGRTMIN+1
	// 36) SIGRTMIN+2  37) SIGRTMIN+3  38) SIGRTMIN+4  39) SIGRTMIN+5
	// 40) SIGRTMIN+6  41) SIGRTMIN+7  42) SIGRTMIN+8  43) SIGRTMIN+9
	// 44) SIGRTMIN+10 45) SIGRTMIN+11 46) SIGRTMIN+12 47) SIGRTMIN+13
	// 48) SIGRTMIN+14 49) SIGRTMIN+15 50) SIGRTMAX-14 51) SIGRTMAX-13
	// 52) SIGRTMAX-12 53) SIGRTMAX-11 54) SIGRTMAX-10 55) SIGRTMAX-9
	// 56) SIGRTMAX-8  57) SIGRTMAX-7  58) SIGRTMAX-6  59) SIGRTMAX-5
	// 60) SIGRTMAX-4  61) SIGRTMAX-3  62) SIGRTMAX-2  63) SIGRTMAX-1
	// 64) SIGRTMAX
	
	// Critical
	signal(SIGTERM, signal_handler);
	signal(SIGSEGV, signal_handler);
	signal(SIGILL,  signal_handler);
	signal(SIGQUIT, signal_handler);
	signal(SIGABRT, signal_handler);
	signal(SIGSTOP, signal_handler);
	signal(SIGKILL, signal_handler);
	
	// Handleable with care 
	signal(SIGINT, signal_handler);
	
	// Just handleable
	signal(SIGUSR1, signal_handler);
	signal(SIGUSR2, signal_handler);
	
	// for (int i = SIGRTMIN; i < SIGRTMAX; ++i)
	// 	signal(i,  signal_handler);
#elif defined(WINDOWS)
	// SIGABRT  Abnormal termination
	// SIGFPE   Floating-point error
	// SIGILL   Illegal instruction
	// SIGINT   CTRL+C signal
	// SIGSEGV  Illegal storage access
	// SIGTERM  Termination request
	
	// Critical
	signal(SIGTERM, signal_handler);
	signal(SIGABRT, signal_handler);
	signal(SIGILL, signal_handler);
	signal(SIGSEGV, signal_handler);
	
	// Handleable with care
	signal(SIGINT, signal_handler);
#endif
	
	// R U N _ C O D E
	
	// Perform execution on the new stack
	if (!ck_core::stack_locator::call_replace_stack(0, nullptr, wrap_main, ck_new_stack_size))
		std::wcout << "Failed to allocate stack for current operation" << std::endl;
	
	// D I S P O S E
	
	// Free up heap
	delete gil_instance;
	delete main_script;
	
	// Dispose locale
	delete codecvt;
	
	// D E T A C H _ T H R E A D S
	
	// To allow all detached threads to finish clearing
	pthread_exit(0);
	
	// Fire all exit handlers.
	// For example, exit handler is used in native loader to unload 
	//  natives after program disposal.
	ck_core::ck_exit_listener::strike();
	
	// std::return_0;
	return 0;
};

