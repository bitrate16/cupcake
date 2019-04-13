#include "objects/CallablePrototype.h"

using namespace std;
using namespace ck_vobject;
using namespace ck_objects;
using namespace ck_core;


CallablePrototype::CallablePrototype(ck_vobject::vobject* (*handler) (ck_vobject::vscope*, const std::vector<ck_vobject::vobject*>&)) {
	call_handler = handler;
};

CallablePrototype::~CallablePrototype() {
	
};

vobject* CallablePrototype::get(ck_vobject::vscope* scope, const std::wstring& name) {
	return Object::get(name);
};

void CallablePrototype::put(ck_vobject::vscope* scope, const std::wstring& name, vobject* object) {
	Object::put(name, object);
};

bool CallablePrototype::contains(ck_vobject::vscope* scope, const std::wstring& name) {
	return Object::contains(name);
};

bool CallablePrototype::remove(ck_vobject::vscope* scope, const std::wstring& name) {
	return Object::remove(name);
};

vobject* CallablePrototype::call(ck_vobject::vscope* scope, const std::vector<vobject*>& args) {
	return call_handler(scope, args);
};

void CallablePrototype::gc_mark() {
	Object::gc_mark();
};

void CallablePrototype::gc_finalize() {
	Object::gc_finalize();
};


long long CallablePrototype::int_value() {
	return (int) (intptr_t) this; 
};

std::wstring CallablePrototype::string_value() { 
	return std::wstring(L"[CallablePrototype ") + std::to_wstring((int) (intptr_t) this) + std::wstring(L"]"); 
};

