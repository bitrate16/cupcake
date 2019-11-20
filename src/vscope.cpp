#include "vscope.h"

#if defined(__BORLANDC__)
    typedef unsigned char uint8_t;
    typedef __int64 int64_t;
    typedef unsigned long uintptr_t;
#elif defined(_MSC_VER)
    typedef unsigned char uint8_t;
    typedef __int64 int64_t;
#else
    #include <stdint.h>
#endif

#include <typeinfo>
#include <string>

#include "GC.h"
#include "GIL2.h"
#include "exceptions.h"

#include "objects/Object.h"
#include "objects/String.h"
#include "objects/Null.h"
#include "objects/Undefined.h"
#include "objects/Bool.h"
#include "objects/NativeFunction.h"
#include "objects/Array.h"
#include "objects/String.h"

using namespace std;
using namespace ck_core;
using namespace ck_exceptions;
using namespace ck_vobject;
using namespace ck_objects;


// S C O P E		

// _ _ I N I T _ _

// I S C O P E
vobject* iscope::create_proto() {
	if (ScopeProto != nullptr)
		return ScopeProto;
	
	ScopeProto = new Object();
	GIL::gc_instance()->attach_root(ScopeProto);
	
	// __proto value has to be inserted after definition of all objects.
	//  So it can not be inserted to scope instance and has to be accessed 
	//   over constant check.
	ScopeProto->put(L"__typename", new String(L"Scope"));
	ScopeProto->put(L"parent", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			// Validate __this
			if (!scope) return Undefined::instance();
			vobject* __this = scope->get(L"__this", 1);
			if (!__this || !__this->as_type<iscope>())
				return Undefined::instance();
			
			vobject* parent = static_cast<iscope*>(__this)->parent;
			return parent ? parent : Undefined::instance();
		}));
	ScopeProto->put(L"root", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			// Validate __this
			if (!scope) return Undefined::instance();
			vobject* __this = scope->get(L"__this", 1);
			if (!__this || !__this->as_type<iscope>())
				return Undefined::instance();
			
			vobject* root = static_cast<iscope*>(__this)->get_root();
			return root ? root : Undefined::instance();
		}));
	ScopeProto->put(L"contains", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			// Validate __this
			if (!scope) return Undefined::instance();
			vobject* __this = scope->get(L"__this", 1);
			if (!__this || !__this->as_type<iscope>())
				return Undefined::instance();
			
			// Validate args
			bool con = 1;
			for (auto &i : args)
				if (!i || !static_cast<iscope*>(__this)->contains(i->string_value())) {
					con = 0;
					break;
				}
			
			if (con)
				return Bool::True();
			else
				return Bool::False();
		}));
	ScopeProto->put(L"remove", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			// Validate __this
			if (!scope) return Undefined::instance();
			vobject* __this = scope->get(L"__this", 1);
			if (!__this || !__this->as_type<iscope>())
				return Undefined::instance();
			
			// Validate args
			bool con = 1;
			for (auto &i : args)
				con = con && static_cast<iscope*>(__this)->remove(i->string_value());
			
			if (con)
				return Bool::True();
			else
				return Bool::False();
		}));
	ScopeProto->put(L"keys", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			// Validate __this
			if (!scope) return Undefined::instance();
			vobject* __this = scope->get(L"__this", 1);
			if (!__this || !__this->as_type<iscope>())
				return Undefined::instance();
			
			std::vector<vobject*> keys;
	
			for (const auto& any : static_cast<iscope*>(__this)->objects) 
				keys.push_back(new String(any.first));
			
			return new Array(keys);
		}));
	
	// __operator== can be used in other objects because it does not depend on type.
	ScopeProto->put(L"__operator==", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			return Bool::instance(args.size() && args[0] == args[1]);
		}));
	ScopeProto->put(L"__operator!=", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			return Bool::instance(!args.size() || args[0] != args[1]);
		}));
	
	return ScopeProto;
};

// X S C O P E
vobject* xscope::create_proto() {
	if (ProxyScopeProto != nullptr)
		return ProxyScopeProto;
	
	ProxyScopeProto = new Object();
	GIL::gc_instance()->attach_root(ProxyScopeProto);
	
	// __proto value has to be inserted after definition of all objects.
	//  So it can not be inserted to scope instance and has to be accessed 
	//   over constant check.
	ProxyScopeProto->put(L"__typename", new String(L"Scope"));
	ProxyScopeProto->put(L"__proxy",        Bool::True());
	ProxyScopeProto->put(L"parent", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			// Validate __this
			if (!scope) return Undefined::instance();
			vobject* __this = scope->get(L"__this", 1);
			if (!__this || !__this->as_type<xscope>())
				return Undefined::instance();
			
			vobject* parent = static_cast<xscope*>(__this)->parent;
			return parent ? parent : Undefined::instance();
		}));
	ProxyScopeProto->put(L"root", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			// Validate __this
			if (!scope) return Undefined::instance();
			vobject* __this = scope->get(L"__this", 1);
			if (!__this || !__this->as_type<xscope>())
				return Undefined::instance();
			
			vobject* root = static_cast<xscope*>(__this)->get_root();
			return root ? root : Undefined::instance();
		}));
	ProxyScopeProto->put(L"contains", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			// Validate __this
			if (!scope) return Undefined::instance();
			vobject* __this = scope->get(L"__this", 1);
			if (!__this || !__this->as_type<xscope>())
				return Undefined::instance();
			
			// Validate args
			bool con = 1;
			for (auto &i : args)
				if (!i || !static_cast<xscope*>(__this)->contains(i->string_value())) {
					con = 0;
					break;
				}
			
			if (con)
				return Bool::True();
			else
				return Bool::False();
		}));
	ProxyScopeProto->put(L"remove", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			// Validate __this
			if (!scope) return Undefined::instance();
			vobject* __this = scope->get(L"__this", 1);
			if (!__this || !__this->as_type<xscope>())
				return Undefined::instance();
			
			// Validate args
			bool con = 1;
			for (auto &i : args)
				con = con && static_cast<xscope*>(__this)->remove(i->string_value());
			
			if (con)
				return Bool::True();
			else
				return Bool::False();
		}));
	// Keys has undefined behaviour for proxies
	/*ProxyScopeProto->put(L"keys", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			// Validate __this
			if (!scope) return Undefined::instance();
			vobject* __this = scope->get(L"__this", 1);
			if (!__this || !__this->as_type<xscope>())
				return Undefined::instance();
			
			std::vector<vobject*> keys;
	
			for (const auto& any : static_cast<xscope*>(__this)->objects) 
				keys.push_back(new String(any.first));
			
			return new Array(keys);
		}));*/
	
	// __operator== can be used in other objects because it does not depend on type.
	ProxyScopeProto->put(L"__operator==", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			// Validate __this
			if (!scope) return Undefined::instance();
			vobject* __this = scope->get(L"__this", 1);
			if (!__this)
				return Undefined::instance();
			
			bool eq = args.size();
			
			for (int i = 0; i < args.size(); ++i)
				if (args[i] != __this)
					return Bool::False();
			
			return Bool::instance(eq);
		}));
	ProxyScopeProto->put(L"__operator!=", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			// Validate __this
			if (!scope) return Undefined::instance();
			vobject* __this = scope->get(L"__this", 1);
			if (!__this)
				return Undefined::instance();
			
			for (int i = 0; i < args.size(); ++i)
				if (args[i] == __this)
					return Bool::False();
			
			return Bool::True();
		}));
	
	return ProxyScopeProto;
};


iscope::iscope(vscope* parent) : vscope(parent) { 
	this->parent = parent; 
};

iscope::~iscope() {};


vobject* iscope::get(vscope* scope, const wstring& name) {
	// Wrap return of __proto
	if (name == L"__proto")
		return ScopeProto;
	
	vobject* ret = get(name, 1);
	if (!ret && ScopeProto)
		return ScopeProto->get(scope, name);
	return ret;
};

void iscope::put(vscope* scope, const wstring& name, vobject* object) {
	if (name == L"__proto")
		return;
	
	put(name, object);
};

bool iscope::contains(vscope* scope, const wstring& name) {
	if (name == L"__proto")
		return 1;
	
	return contains(name) || (ScopeProto && ScopeProto->contains(scope, name));
};

bool iscope::remove(vscope* scope, const wstring& name) {
	if (name == L"__proto")
		return 0;
	
	if (remove(name))
		return 1;
	if (ScopeProto && ScopeProto->remove(scope, name))
		return 1;
	return 0;
};


vobject* iscope::call(vscope* scope, const std::vector<vobject*>& args) { 
	throw UnsupportedOperation(L"Scope is not callable");
};


vscope* iscope::get_root() {
	vscope* t = this;
	while (t->parent)
		t = t->parent;
	return t;
};

vscope* iscope::get_parent() {
	return parent;
};


// Must return integer representation of an object
long long iscope::int_value() { 
	return (int) (intptr_t) this; 
};

// Must return string representation of an object
std::wstring iscope::string_value() { 
	return std::wstring(L"[scope ") + std::to_wstring((int) (intptr_t) this) + std::wstring(L"]"); 
};


void iscope::root() {
	GIL::gc_instance()->attach_root(this);
};

void iscope::unroot() {
	GIL::gc_instance()->deattach_root(this);
};

void iscope::gc_mark() {
	if (gc_reachable)
		return;
	
	gc_reach();
	
	for (const auto& any : objects) 
		if (any.second && !any.second->gc_reachable)
			any.second->gc_mark();
	
	if (parent != nullptr)
		parent->gc_mark();
};

void iscope::gc_finalize() {};


vobject* iscope::get(const std::wstring& name, bool parent_get, bool proto_get) {
	map<wstring, vobject*>::const_iterator pos;

	{
		#ifndef CK_SINGLETHREAD
			ck_pthread::mutex_lock lck(mutex());
		#endif
		
		pos = objects.find(name);
		if (pos != objects.end())
			return pos->second;
	}
	
	vobject* obj;
	if (parent_get && parent)
		obj = parent->get(name, 1);
	else
		obj = nullptr;
	
	if (!obj && proto_get && ScopeProto)
		return ScopeProto->get(name);
	
	return obj;	
};
		
bool iscope::put(const std::wstring& name, vobject* object, bool parent_put, bool create_new) {
	map<wstring, vobject*>::const_iterator pos;
	
	{
		#ifndef CK_SINGLETHREAD
			ck_pthread::mutex_lock lck(mutex());
		#endif
		
		pos = objects.find(name);
		if (pos != objects.end()) {
			objects[name] = object;
			return 1;
		}
	}
	
	if (parent_put && parent) {
		if (parent->put(name, object, 1, 0))
			return 1;
		else if (create_new) {
			#ifndef CK_SINGLETHREAD
				ck_pthread::mutex_lock lck(mutex());
			#endif
			
			objects[name] = object;
			return 1;
		}
	} else if (create_new) {
		#ifndef CK_SINGLETHREAD
			ck_pthread::mutex_lock lck(mutex());
		#endif
		
		objects[name] = object;
		return 1;
	}
	
	return 0;
};

bool iscope::contains(const std::wstring& name, bool parent_search) {
	map<wstring, vobject*>::const_iterator pos = objects.find(name);
	if (pos == objects.end())
		return parent_search && parent && parent->contains(name, 1);
	return 1;
};

bool iscope::remove(const std::wstring& name, bool parent_remove) {
	map<wstring, vobject*>::const_iterator pos;
	
	{
		#ifndef CK_SINGLETHREAD
			ck_pthread::mutex_lock lck(mutex());
		#endif
			
		pos = objects.find(name);
		if (pos != objects.end()) {
			objects.erase(pos);
			return 1;
		}
	}
	
	if (parent_remove && parent)
		return parent->remove(name, 1);
	else
		return 0;
};


// P R O X Y _ S C O P E


xscope::xscope(ck_vobject::vobject* proxy, vscope* parent) : vscope(parent) { 
	this->parent = parent; 
	this->proxy = proxy; 
	
	put(wstring(L"parent"), parent == nullptr ? (vobject*) Null::instance() : (vobject*) parent);
	put(wstring(L"proto"),  ProxyScopeProto);
};

xscope::~xscope() {};


vobject* xscope::get(vscope* scope, const wstring& name) {
	// Wrap return of __proto
	if (name == L"__proto")
		return ProxyScopeProto;
	
	vobject* ret = get(name, 1);
	if (!ret && ProxyScopeProto)
		return ProxyScopeProto->get(scope, name);
	return ret;
};

void xscope::put(vscope* scope, const wstring& name, vobject* object) {
	if (name == L"__proto")
		return;
	
	put(name, object);
};

bool xscope::contains(vscope* scope, const wstring& name) {
	if (name == L"__proto")
		return 1;
	
	return contains(name) || (ProxyScopeProto && ProxyScopeProto->contains(scope, name));
};

bool xscope::remove(vscope* scope, const wstring& name) {
	if (name == L"__proto")
		return 0;
	
	if (remove(name))
		return 1;
	if (ProxyScopeProto && ProxyScopeProto->remove(scope, name))
		return 1;
	return 0;
};


vobject* xscope::call(vscope* scope, const std::vector<vobject*>& args) { 
	throw UnsupportedOperation(L"Scope is not callable");
};


vscope* xscope::get_root() {
	vscope* t = this;
	while (t->parent)
		t = t->parent;
	return t;
};

vscope* xscope::get_parent() {
	return parent;
};


// Must return integer representation of an object
long long xscope::int_value() { 
	return (int) (intptr_t) this; 
};

// Must return string representation of an object
std::wstring xscope::string_value() { 
	return std::wstring(L"[xscope ") + std::to_wstring((int) (intptr_t) proxy) + std::wstring(L"]"); 
};


void xscope::root() {
	GIL::gc_instance()->attach_root(this);
};

void xscope::unroot() {
	GIL::gc_instance()->deattach_root(this);
};

void xscope::gc_mark() {
	if (gc_reachable)
		return;
	
	gc_reach();
	
	if (proxy && !proxy->gc_reachable)
		proxy->gc_mark();
	
	if (parent != nullptr)
		parent->gc_mark();
};

void xscope::gc_finalize() {};


vobject* xscope::get(const std::wstring& name, bool parent_get, bool proto_get) {
	if (name == L"__this")
		return __this;
	
	vobject* o = proxy ? proxy->get(this, name) : nullptr;
	
	if (!o) {
		vobject* obj;
		if (parent_get && parent)
			obj = parent->get(name, 1);
		else
			obj = nullptr;
		
		if (!obj && proto_get && ProxyScopeProto)
			return ScopeProto->get(name);
		
		return obj;	
	}
	return o;
};
		
bool xscope::put(const std::wstring& name, vobject* object, bool parent_put, bool create_new) {	
	bool pcontains = (name == L"__this" && __this) || (proxy ? proxy->contains(this, name) : 0);
	
	if (!pcontains)
		if (parent_put && parent) {
			if (parent->put(name, object, 1, 0))
				return 1;
			else if (create_new) {
				if (name == L"__this") __this = object;
				else if (proxy) proxy->put(this, name, object);
				
				return 1;
			}
		} else if (create_new) {
			if (name == L"__this") __this = object;
			else if (proxy) proxy->put(this, name, object);
			
			return 1;
		} else
			return 0;

	if (name == L"__this") __this = object;
	else if (proxy) proxy->put(this, name, object);
	
	return 1;
};

bool xscope::contains(const std::wstring& name, bool parent_search) {
	if (name == L"__this" && __this)
		return 1;
	
	bool pcontains = proxy ? proxy->contains(this, name) : 0;
	
	if (!pcontains)
		return parent_search && parent && parent->contains(name, 1);
	return 1;
};

bool xscope::remove(const std::wstring& name, bool parent_remove) {
	if (name == L"__this" && __this) 
		__this = nullptr;
	
	bool pcontains = proxy ? proxy->contains(this, name) : 0;
	if (!pcontains)
		if (parent_remove && parent)
			return parent->remove(name, 1);
		else
			return 0;
		
	proxy->remove(this, name);
	return 1;
};

