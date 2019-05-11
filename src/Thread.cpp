#include "objects/Thread.h"

#include <string>

#include "exceptions.h"
#include "GIL2.h"
#include "executer.h"

#include "objects/Object.h"
#include "objects/Int.h"
#include "objects/Array.h"
#include "objects/Bool.h"
#include "objects/NativeFunction.h"
#include "objects/Null.h"
#include "objects/Cake.h"
#include "objects/Undefined.h"
#include "objects/String.h"

using namespace std;
using namespace ck_exceptions;
using namespace ck_vobject;
using namespace ck_objects;
using namespace ck_core;


static vobject* call_handler(vscope* scope, const vector<vobject*>& args) {
	if (!args.size())
		return Undefined::instance();
	
	// Copy arguments
	vector<vobject*>* argv = new vector<vobject*>(args);
	// Copy backtrace from parent thread
	vector<BacktraceFrame>* backtrace = new vector<BacktraceFrame>(GIL::executer_instance()->collect_backtrace());
	
	Thread* t = new Thread(GIL::instance()->spawn_thread([scope, argv, backtrace]() -> void {
		// Create new scope for this thread
		vscope* nscope = new iscope(scope);
		
		// Unlock when entered this function call to be sure there 
		//  was no priority race and arguments and score are still relevant
		GIL::instance()->unlock();
		
		// Copy arguments
		std::vector<vobject*> argso(argv->size() ? argv->size() - 1 : 0);
		for (int i = 1; i < argv->size(); ++i)
			argso[i - 1] = (*argv)[i];
		
		// Append parent thread backtrae to this executer
		GIL::executer_instance()->append_backtrace(*backtrace);
		
		vobject* runnable = (*argv)[0];
		
		// dipose copy of backtrace
		delete backtrace;
		// Dispose copy of arguments
		delete argv;
		
		// Indicates if main returned an exception
		bool cake_started = 0;
		// Instance of catched cake
		cake message;
		vscope* root_scope = nscope->get_root();
		
		// E X E C U T E _ T H R E A D
		
		while (1) {
			try {
				if (!cake_started) {
					GIL::executer_instance()->call_object(runnable, nullptr, argso, L"<thread_runnable>", nscope);
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
					if (__defcakehandler == nullptr || __defcakehandler->is_typeof<Undefined>() || __defcakehandler->is_typeof<Null>()) {
						if (message.get_type_id() == cake_type::CK_OBJECT && message.get_object() != nullptr)
							if (message.get_object()->is_typeof<Cake>())
								((Cake*) message.get_object())->print_backtrace();
							else
								wcerr << "Unhandled cake: " << message << endl;
						else
							wcerr << "Unhandled cake: " << message << endl;
						
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
	}));
	
	t->Object::put(L"runnable", args[0]);
	
	return t;
};

vobject* Thread::create_proto() {
	if (ThreadProto != nullptr)
		return ThreadProto;
	
	ThreadProto = new CallableObject(call_handler);
	GIL::gc_instance()->attach_root(ThreadProto);
	
	ThreadProto->Object::put(L"__typename", new String(L"Thread"));
	ThreadProto->Object::put(L"__proto", ObjectProto);
	ThreadProto->Object::put(L"currentThread", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			return new Thread();
		}));
	ThreadProto->Object::put(L"isRunning", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			// Validate __this
			if (!scope) return Undefined::instance();
			vobject* __this = scope->get(L"__this", 1);
			if (!__this || !__this->is_typeof<Thread>())
				return Undefined::instance();
			
			Thread* t = static_cast<Thread*>(__this);
			
			// Acquire lock over threads array
			GIL::instance()->lock();
			
			ck_core::ckthread* th = GIL::instance()->thread_by_id(t->get_id());
			vobject* ret;
			if (th)
				ret = Bool::instance(th->is_running());
			else
				ret = Undefined::instance();
			
			GIL::instance()->unlock();
			// Is called after call on executer
			// GIL::instance()->accept_lock();
			
			return ret;
		}));
	ThreadProto->Object::put(L"isLocked", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			// Validate __this
			if (!scope) return Undefined::instance();
			vobject* __this = scope->get(L"__this", 1);
			if (!__this || !__this->is_typeof<Thread>())
				return Undefined::instance();
			
			Thread* t = static_cast<Thread*>(__this);
			
			// Acquire lock over threads array
			GIL::instance()->lock();
			
			ck_core::ckthread* th = GIL::instance()->thread_by_id(t->get_id());
			vobject* ret;
			if (th)
				ret = Bool::instance(th->is_locked());
			else
				ret = Undefined::instance();
			
			GIL::instance()->unlock();
			// Is called after call on executer
			// GIL::instance()->accept_lock();
			
			return ret;
		}));
	ThreadProto->Object::put(L"isBlocked", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			// Validate __this
			if (!scope) return Undefined::instance();
			vobject* __this = scope->get(L"__this", 1);
			if (!__this || !__this->is_typeof<Thread>())
				return Undefined::instance();
			
			Thread* t = static_cast<Thread*>(__this);
			
			// Acquire lock over threads array
			GIL::instance()->lock();
			
			ck_core::ckthread* th = GIL::instance()->thread_by_id(t->get_id());
			vobject* ret;
			if (th)
				ret = Bool::instance(th->is_blocked());
			else
				ret = Undefined::instance();
			
			GIL::instance()->unlock();
			// Is called after call on executer
			// GIL::instance()->accept_lock();
			
			return ret;
		}));
	ThreadProto->Object::put(L"getId", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			// Validate __this
			if (!scope) return Undefined::instance();
			vobject* __this = scope->get(L"__this", 1);
			if (!__this || !__this->is_typeof<Thread>())
				return Undefined::instance();
			
			Thread* t = static_cast<Thread*>(__this);
			
			return new Int(t->get_id());
		}));
	
	ThreadProto->Object::put(L"__operator==", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2)
				return Bool::False();
			
			if (!args[0] && !args[1])
				return Bool::True();
			
			if (args[0] && args[1] && args[0]->is_typeof<Thread>() && args[1]->is_typeof<Thread>())
				return Bool::instance(static_cast<Thread*>(args[0])->get_id() == static_cast<Thread*>(args[1])->get_id());
			
			return Bool::False();
		}));
	ThreadProto->Object::put(L"__operator!=", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2)
				return Bool::False();
			
			if (args[0] || args[1])
				return Bool::True();
			
			if (args[0] && args[1] && args[0]->is_typeof<Thread>() && args[1]->is_typeof<Thread>())
				return Bool::instance(static_cast<Thread*>(args[0])->get_id() != static_cast<Thread*>(args[1])->get_id());
			
			return Bool::True();
		}));
		
	
	return ThreadProto;
};


Thread::Thread(uint64_t tid) : Object() {
	bind_id(tid);
};

Thread::Thread() {
	bind_id(GIL::instance()->current_thread()->get_id());
};
		
Thread::~Thread() {};
		
		
vobject* Thread::get(vscope* scope, const wstring& name) {
	vobject* ret = Object::get(name);

	if (!ret && ThreadProto)
		return ThreadProto->get(scope, name);
	return ret;
};

void Thread::put(vscope* scope, const wstring& name, vobject* object) {
	Object::put(name, object);
};

bool Thread::contains(vscope* scope, const wstring& name) {
	return Object::contains(name) || (ThreadProto && ThreadProto->contains(scope, name));
};

bool Thread::remove(vscope* scope, const wstring& name) {
	if (Object::remove(name))
		return 1;
	return 0;
};

vobject* Thread::call(vscope* scope, const vector<vobject*>& args) {
	throw UnsupportedOperation(L"Thread is not callable");
};


void Thread::gc_mark() {
	if (gc_reachable)
		return;
	
	Object::gc_mark();
};

void Thread::gc_finalize() {};

// Must return integer representation of an object
long long Thread::int_value() { 
	return (int) (intptr_t) this; 
};

// Must return string representation of an object
std::wstring Thread::string_value() { 
	return std::wstring(L"[Thread ") + std::to_wstring(get_id()) + std::wstring(L"]"); 
};

