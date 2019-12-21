#include "objects/Object.h"

#include <string>

#include "exceptions.h"
#include "GIL2.h"

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

vobject* Object::create_proto() {
	if (ObjectProto != nullptr)
		return ObjectProto;
	
	ObjectProto = new Object();
	GIL::gc_instance()->attach_root(ObjectProto);
	
	// Object represents usable container with natively declared prototype.
	// Algorhytm of seaching for values is following:
	//  1. Try to find value in current instance of Object
	//  2. If self is not ObjectProto and ObjectProto is not null,
	//      search for value in ObjectProto.
	//
	// __proto value is inserted to Object prototype.
	// So, on being accessed it can not be found in Object instance.
	// Instance of __proto is contained in ObjectProto and accessed 
	//  over native prototype chain.
	ObjectProto->put(L"__typename", new String(L"Object"));
	ObjectProto->put(L"__proto", ObjectProto);
	ObjectProto->put(L"contains", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			// Validate __this
			if (!scope) return Undefined::instance();
			vobject* __this = scope->get(L"__this", 1);
			if (!__this || !__this->as_type<Object>())
				return Undefined::instance();
			
			// Validate args
			bool con = 1;
			for (auto &i : args)
				if (!i || !static_cast<Object*>(__this)->contains(i->string_value())) {
					con = 0;
					break;
				}
			
			if (con)
				return Bool::True();
			else
				return Bool::False();
		}));
	ObjectProto->put(L"remove", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			// Validate __this
			if (!scope) return Undefined::instance();
			vobject* __this = scope->get(L"__this", 1);
			if (!__this || !__this->as_type<Object>())
				return Undefined::instance();
			
			// Validate args
			bool con = 1;
			for (auto &i : args)
				con = con && static_cast<Object*>(__this)->remove(i->string_value());
			
			if (con)
				return Bool::True();
			else
				return Bool::False();
		}));
	ObjectProto->put(L"keys", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			// Validate __this
			if (!scope) return Undefined::instance();
			vobject* __this = scope->get(L"__this", 1);
			if (!__this || !__this->as_type<Object>())
				return Undefined::instance();
			
			std::vector<vobject*> keys;
	
			for (const auto& any : static_cast<Object*>(__this)->objects) 
				keys.push_back(new String(any.first));
			
			return new Array(keys);
		}));
	ObjectProto->Object::put(L"string", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			// Validate __this
			if (!scope) return Undefined::instance();
			vobject* __this = scope->get(L"__this", 1);
			if (!__this)
				return Undefined::instance();
			
			return new String(__this->string_value());
		}));
	ObjectProto->Object::put(L"int", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			// Validate __this
			if (!scope) return Undefined::instance();
			vobject* __this = scope->get(L"__this", 1);
			if (!__this)
				return Undefined::instance();
			
			return new Int(__this->int_value());
		}));
	
	// __operator== can be used in other objects because it does not depend on type.
	ObjectProto->put(L"__operator==", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			return Bool::instance(args.size() >= 2 && args[0] == args[1]);
		}));
	ObjectProto->put(L"__operator!=", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			return Bool::instance(!args.size() < 2 || args[0] != args[1]);
		}));
	ObjectProto->put(L"__roperator==", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			return Bool::instance(args.size() >= 2 && args[0] == args[1]);
		}));
	ObjectProto->put(L"__roperator!=", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			return Bool::instance(!args.size() < 2 || args[0] != args[1]);
		}));
	ObjectProto->put(L"__operator_typeof", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (!args.size() || !args[0])
				return Undefined::instance();
			
			return args[0]->get(scope, L"__typename");
		}));
	ObjectProto->put(L"__operator_istypeof", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[0])
				return Undefined::instance();
			
			vobject* __typename0 = args[0]->get(scope, L"__typename");
			vobject* __typename1 = args[1];
			
			return Bool::instance(__typename0 == nullptr && __typename1 == nullptr || __typename0 != nullptr && __typename1 != nullptr && __typename0->string_value() == __typename1->string_value());
		}));
	ObjectProto->put(L"__operator_as", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			vobject* __typename0 = args[0]->get(scope, L"__typename");
			vobject* __typename1 = args[1];
			
			if (__typename0 == nullptr && __typename1 == nullptr || __typename0 != nullptr && __typename1 != nullptr && __typename0->string_value() == __typename1->string_value()) {
			return args[0];}
			return Undefined::instance();
		}));
	
	return ObjectProto;
};


Object::Object(const std::map<std::wstring, ck_vobject::vobject*>& objec) : vsobject() {
	objects = objec;
};

Object::Object() {};
		
Object::~Object() {};
		
		
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
	return 0;
};

vobject* Object::call(vscope* scope, const vector<vobject*>& args) {
	// XXX: Construct object from input
	throw UnsupportedOperation(L"Object is not callable");
};


void Object::gc_mark() {
	if (gc_reachable)
		return;
	
	gc_reach();
	
	for (const auto& any : objects) 
		if (any.second && !any.second->gc_reachable)
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
	
	vsobject::vslock lk(this);
	
	objects[name] = object;
};

vobject* Object::get(const wstring& name) {
	
	vsobject::vslock lk(this);
	
	//wcout << this->string_value() << " getting " << name << endl;
	map<wstring, vobject*>::const_iterator pos = objects.find(name);
	if (pos == objects.end())
		return nullptr;
	return pos->second;
};

bool Object::contains(const wstring& name) {
	
	vsobject::vslock lk(this);
	
	map<wstring, vobject*>::const_iterator pos = objects.find(name);
	if (pos == objects.end())
		return 0;
	return 1;
};

bool Object::remove(const wstring& name) {
	
	vsobject::vslock lk(this);
	
	map<wstring, vobject*>::const_iterator pos = objects.find(name);
	if (pos == objects.end())
		return 0;
	objects.erase(pos);
	return 1;
};

// Must return integer representation of an object
int64_t Object::int_value() { 
	return (intptr_t) this; 
};

// Must return string representation of an object
std::wstring Object::string_value() { 
	return std::wstring(L"[Object ") + std::to_wstring((intptr_t) this) + std::wstring(L"]"); 
};

