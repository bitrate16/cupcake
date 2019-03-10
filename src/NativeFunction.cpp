#include "objects/NativeFunction.h"

#include "GIL2.h"

using namespace std;
using namespace ck_vobject;
using namespace ck_objects;
using namespace ck_core;

vobject* NativeFunction::create_proto() {
	if (NativeFunctionProto != nullptr)
		return NativeFunctionProto;
	
	NativeFunctionProto = new Object();
	GIL::gc_instance()->attach_root(NativeFunctionProto);
	
	// ...
	
	return NativeFunctionProto;
};


NativeFunction::NativeFunction(ck_vobject::vobject* (*wrapper) (ck_vobject::vscope*, const std::vector<ck_vobject::vobject*>&)) {
	if (NativeFunctionProto == nullptr)
		NativeFunction::create_proto();
	
	Object::put(wstring(L"proto"), NativeFunctionProto);
	
	call_wrapper = wrapper;
};

NativeFunction::~NativeFunction() {};


vobject* NativeFunction::get(ck_vobject::vscope* scope, const std::wstring& name) {
	vobject* ret = Object::get(name);
	if (!ret && NativeFunctionProto)
		return NativeFunctionProto->get(scope, name);
	return ret;
};

void NativeFunction::put(ck_vobject::vscope* scope, const std::wstring& name, vobject* object) {
	Object::put(name, object);
};

bool NativeFunction::contains(ck_vobject::vscope* scope, const std::wstring& name) {
	return Object::contains(name) || (NativeFunctionProto && NativeFunctionProto->contains(scope, name));
};

bool NativeFunction::remove(ck_vobject::vscope* scope, const std::wstring& name) {
	if (Object::remove(name))
		return 1;
	if (NativeFunctionProto && NativeFunctionProto->remove(scope, name))
		return 1;
	return 0;
};

vobject* NativeFunction::call(ck_vobject::vscope* scope, const std::vector<vobject*> args) {
	return call_wrapper(scope, args);
};

void NativeFunction::gc_mark() {
	Object::gc_mark();
};

void NativeFunction::gc_finalize() {
	Object::gc_finalize();
};


long long NativeFunction::int_value() {
	return (int) (intptr_t) call_wrapper; 
};

std::wstring NativeFunction::string_value() { 
	return std::wstring(L"[NativeFunction ") + std::to_wstring((int) (intptr_t) call_wrapper) + std::wstring(L"]"); 
};

