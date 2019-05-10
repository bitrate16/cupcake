#include "objects/Thread.h"

#include <string>

#include "exceptions.h"
#include "GIL2.h"

#include "objects/Object.h"
#include "objects/Int.h"
#include "objects/Array.h"
#include "objects/Bool.h"
#include "objects/NativeFunction.h"
#include "objects/Undefined.h"
#include "objects/String.h"

using namespace std;
using namespace ck_exceptions;
using namespace ck_vobject;
using namespace ck_objects;
using namespace ck_core;


static vobject* call_handler(vscope* scope, const vector<vobject*>& args) {
	// XXX: Make new thread
	return nullptr;
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
			
			ck_core::ckthread* th = t->get();
			
			if (th)
				return Bool::instance(th->is_running());
			else
				return Undefined::instance();
		}));
	ThreadProto->Object::put(L"isLocked", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			// Validate __this
			if (!scope) return Undefined::instance();
			vobject* __this = scope->get(L"__this", 1);
			if (!__this || !__this->is_typeof<Thread>())
				return Undefined::instance();
			
			Thread* t = static_cast<Thread*>(__this);
			
			ck_core::ckthread* th = t->get();
			
			if (th)
				return Bool::instance(th->is_locked());
			else
				return Undefined::instance();
		}));
	ThreadProto->Object::put(L"isBlocked", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			// Validate __this
			if (!scope) return Undefined::instance();
			vobject* __this = scope->get(L"__this", 1);
			if (!__this || !__this->is_typeof<Thread>())
				return Undefined::instance();
			
			Thread* t = static_cast<Thread*>(__this);
			
			ck_core::ckthread* th = t->get();
			
			if (th)
				return Bool::instance(th->is_blocked());
			else
				return Undefined::instance();
		}));
	ThreadProto->Object::put(L"getId", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			// Validate __this
			if (!scope) return Undefined::instance();
			vobject* __this = scope->get(L"__this", 1);
			if (!__this || !__this->is_typeof<Thread>())
				return Undefined::instance();
			
			Thread* t = static_cast<Thread*>(__this);
			
			ck_core::ckthread* th = t->get();
			
			if (th)
				return new Int(th->count_id());
			else
				return Undefined::instance();
		}));
		
	
	return ThreadProto;
};


Thread::Thread(ck_core::ckthread* th, vobject* runnable) : Object() {
	Object::put(L"runnable", runnable);
	bind(th);
};

Thread::Thread(vobject* runnable) {
	Object::put(L"runnable", runnable);
	bind(GIL::instance()->current_thread());
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
	if (get())
		return std::wstring(L"[Thread ") + std::to_wstring(get()->count()) + std::wstring(L"]"); 
	else
		return std::wstring(L"[Thread nullptr]"); 
};

