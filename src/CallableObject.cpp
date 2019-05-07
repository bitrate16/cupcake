#include "objects/CallableObject.h"

using namespace std;
using namespace ck_vobject;
using namespace ck_objects;
using namespace ck_core;


CallableObject::CallableObject(ck_vobject::vobject* (*handler) (ck_vobject::vscope*, const std::vector<ck_vobject::vobject*>&)) {
	call_handler = handler;
};

vobject* CallableObject::call(ck_vobject::vscope* scope, const std::vector<vobject*>& args) {
	return call_handler(scope, args);
};

vobject* CallableObject::get(ck_vobject::vscope* scope, const std::wstring& name) {
	return Object::get(scope, name);
};

void CallableObject::put(ck_vobject::vscope* scope, const std::wstring& name, vobject* object) {
	Object::put(scope, name, object);
};

bool CallableObject::contains(ck_vobject::vscope* scope, const std::wstring& name) {	
	return Object::contains(scope, name);
};

bool CallableObject::remove(ck_vobject::vscope* scope, const std::wstring& name) {
	return remove(scope, name);
};