#include "objects/Object.h"

#include <string>

#include "exceptions.h"
#include "GIL2.h"

using namespace std;
using namespace ck_exceptions;
using namespace ck_vobject;
using namespace ck_objects;
using namespace ck_core;

vobject* Object::create_proto() {
	if (ObjectProto != nullptr)
		return ObjectProto;
	
	ObjectProto = new Object();
	GIL::gc_instance()->attach_root(ObjectProto);
	
	// ...
	
	return ObjectProto;
};


Object::Object(std::map<std::wstring, ck_vobject::vobject*>& objec) {
	objects = objec;
	
	put(wstring(L"proto"), ObjectProto);
};

Object::Object() {
	put(wstring(L"proto"), ObjectProto);
};
		
Object::~Object() {
	
};
		
		
vobject* Object::get(vscope* scope, const wstring& name) {
	vobject* ret = get(name);
	if (!ret && ObjectProto != this && ObjectProto)
		return ObjectProto->get(scope, name);
	return ret;
};

void Object::put(vscope* scope, const wstring& name, vobject* object) {
	put(name, object);
};

bool Object::contains(vscope* scope, const wstring& name) {
	return contains(name) || (ObjectProto != this && ObjectProto && ObjectProto->contains(scope, name));
};

bool Object::remove(vscope* scope, const wstring& name) {
	if (remove(name))
		return 1;
	if (ObjectProto != this && ObjectProto && ObjectProto->remove(scope, name))
		return 1;
	return 0;
};

vobject* Object::call(vscope* scope, const vector<vobject*> args) {
	// XXX: Construct object from input
	throw ck_message(L"Object is not callable", ck_message_type::CK_UNSUPPORTED_OPERATION);
};


void Object::gc_mark() {
	if (gc_reachable)
		return;
	
	gc_reach();
	
	for (const auto& any : objects) 
		if (any.second && any.second->gc_reachable)
			any.second->gc_mark();
};

void Object::gc_finalize() {};
		
// Object functions only

void Object::append(Object* obj) {
	if (!obj)
		return;
	
	objects.insert(obj->objects.begin(), obj->objects.end());
};

vector<wstring> Object::keys() {
	vector<wstring> keys;
	
	for (const auto& any : objects) 
		keys.push_back(any.first);
	
	return keys;
};

// Scope-independent getter-setter-checker.
void Object::put(const wstring& name, vobject* object) {
	objects[name] = object;
};

vobject* Object::get(const wstring& name) {
	map<wstring, vobject*>::const_iterator pos = objects.find(name);
	if (pos == objects.end())
		return nullptr;
	return pos->second;
};

bool Object::contains(const wstring& name) {
	map<wstring, vobject*>::const_iterator pos = objects.find(name);
	if (pos == objects.end())
		return 0;
	return 1;
};

bool Object::remove(const wstring& name) {
	map<wstring, vobject*>::const_iterator pos = objects.find(name);
	if (pos == objects.end())
		return 0;
	objects.erase(pos);
	return 1;
};

// Must return integer representation of an object
long long Object::int_value() { 
	return (int) (intptr_t) this; 
};

// Must return string representation of an object
std::wstring Object::string_value() { 
	return std::wstring(L"[Object ") + std::to_wstring((int) (intptr_t) this) + std::wstring(L"]"); 
};

