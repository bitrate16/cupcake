#include "objects/Object.h"

#include <string>

#include "exceptions.h"

using namespace std;
using namespace ck_exceptions;
using namespace ck_vobject;
using namespace ck_objects;

static void create_proto() {
	ObjectProto = new Object();
	
	// ...
};


Object::Object() {
	if (ObjectProto == nullptr) 
		create_proto();
	
	put(wstring(L"proto"), ObjectProto);
};
		
Object::~Object() {
	
};
		
		
vobject* Object::get(vscope* scope, const wstring& name) {
	vobject* ret = get(name);
	if (!ret && ObjectProto != this)
		return ObjectProto->get(scope, name);
	return ret;
};

void Object::put(vscope* scope, const wstring& name, vobject* object) {
	if (contains(name))
		put(name, object);
	else if(ObjectProto != this)
		ObjectProto->put(scope, name, object);
};

bool Object::contains(vscope* scope, const wstring& name) {
	return contains(name) || (ObjectProto != this && ObjectProto->contains(scope, name));
};

bool Object::remove(vscope* scope, const wstring& name) {
	if (remove(name))
		return 1;
	if (ObjectProto != this && ObjectProto->remove(scope, name))
		return 1;
	return 0;
};

vobject* Object::call(vscope* scope, vector<vobject*> args) {
	// XXX: Construct object from input
	throw ck_message("Object is not callable");
};

void Object::gc_mark() {
	if (gc_reachable)
		return;
	
	gc_reach();
	
	for (const auto& any : objects) 
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
