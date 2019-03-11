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
#include "objects/Null.h"

using namespace std;
using namespace ck_core;
using namespace ck_exceptions;
using namespace ck_vobject;
using namespace ck_objects;
		

vobject* vscope::create_proto() {
	if (ScopeProto != nullptr)
		return ScopeProto;
	
	ScopeProto = new Object();
	GIL::gc_instance()->attach_root(ScopeProto);
	
	// ...
	
	return ScopeProto;
};


// S C O P E

vscope::vscope(vscope* parent) : Object::Object() { 
	this->parent = parent; 
	
	put(wstring(L"parent"), parent == nullptr ? (vobject*) Null::instance() : (vobject*) parent);
	put(wstring(L"proto"),  ScopeProto);
};

vscope::~vscope() {};


vobject* vscope::get(vscope* scope, const wstring& name) {
	vobject* ret = get(name, 1);
	if (!ret && ScopeProto != this && ScopeProto)
		return ScopeProto->get(scope, name);
	return ret;
};

void vscope::put(vscope* scope, const wstring& name, vobject* object) {
	put(name, object);
};

bool vscope::contains(vscope* scope, const wstring& name) {
	return contains(name) || (ScopeProto != this && ScopeProto && ScopeProto->contains(scope, name));
};

bool vscope::remove(vscope* scope, const wstring& name) {
	if (remove(name))
		return 1;
	if (ScopeProto != this && ScopeProto && ScopeProto->remove(scope, name))
		return 1;
	return 0;
};


vobject* vscope::call(vscope* scope, const std::vector<vobject*> args) { 
	throw ck_message(L"Scope is not callable", ck_message_type::CK_UNSUPPORTED_OPERATION);
};


vscope* vscope::get_root() {
	vscope* t = this;
	while (t->parent)
		t = t->parent;
	return t;
};

vscope* vscope::get_parent() {
	return parent;
};


// Must return integer representation of an object
long long vscope::int_value() { 
	return (int) (intptr_t) this; 
};

// Must return string representation of an object
std::wstring vscope::string_value() { 
	return std::wstring(L"[Scope ") + std::to_wstring((int) (intptr_t) this) + std::wstring(L"]"); 
};


void vscope::root() {
	GIL::gc_instance()->attach_root(this);
};

void vscope::unroot() {
	GIL::gc_instance()->deattach_root(this);
};

void vscope::gc_mark() {
	if (gc_reachable)
		return;
	
	gc_reach();
	
	for (const auto& any : objects) 
		if (any.second && !any.second->gc_reachable)
			any.second->gc_mark();
	
	if (parent != nullptr)
		parent->gc_mark();
};

void vscope::gc_finalize() {};


vobject* vscope::get(const std::wstring& name, bool parent_get) {
	map<wstring, vobject*>::const_iterator pos = objects.find(name);
	if (pos == objects.end())
		if (parent_get && parent)
			return parent->get(name, 1);
		else
			return nullptr;
	return pos->second;
};
		
bool vscope::put(const std::wstring& name, vobject* object, bool parent_put, bool create_new) {
	map<wstring, vobject*>::const_iterator pos = objects.find(name);
	
	if (pos == objects.end())
		if (parent_put && parent)
			if (parent->put(name, object, 1, 0))
				return 1;
			else if (create_new) {
				objects[name] = object;
				return 1;
			}
		else if (create_new) {
			objects[name] = object;
			return 1;
		} else
			return 0;

	objects[name] = object;
	return 1;
};

bool vscope::contains(const std::wstring& name, bool parent_search) {
	map<wstring, vobject*>::const_iterator pos = objects.find(name);
	if (pos == objects.end())
		return parent_search && parent && parent->contains(name, 1);
	return 1;
};

bool vscope::remove(const std::wstring& name, bool parent_remove) {
	map<wstring, vobject*>::const_iterator pos = objects.find(name);
	if (pos == objects.end())
		if (parent_remove && parent)
			return parent->remove(name, 1);
		else
			return 0;
	objects.erase(pos);
	return 1;
};

