#include "objects/NativeFunction.h"

#include "exceptions.h"
#include "GIL2.h"

#include "objects/String.h"
#include "objects/Bool.h"

using namespace std;
using namespace ck_vobject;
using namespace ck_objects;
using namespace ck_core;
using namespace ck_exceptions;

vobject* NativeFunction::create_proto() {
	if (NativeFunctionProto != nullptr)
		return NativeFunctionProto;
	
	NativeFunctionProto = new Object();
	GIL::gc_instance()->attach_root(NativeFunctionProto);
	
	NativeFunctionProto->Object::put(L"__typename", new String(L"Function"));	
	NativeFunctionProto->Object::put(L"__native",       Bool::True());	
	
	return NativeFunctionProto;
};


NativeFunction::NativeFunction(ck_vobject::vobject* (*wrapper) (ck_vobject::vscope*, const std::vector<ck_vobject::vobject*>&)) {
	if (NativeFunctionProto == nullptr)
		NativeFunction::create_proto();
	
	call_wrapper = wrapper;
};

NativeFunction::~NativeFunction() {};


vobject* NativeFunction::get(ck_vobject::vscope* scope, const std::wstring& name) {
	if (name == L"__proto")
		return NativeFunctionProto;
	
	return NativeFunctionProto ? NativeFunctionProto->Object::get(scope, name) : nullptr;
};

void NativeFunction::put(ck_vobject::vscope* scope, const std::wstring& name, vobject* object) {
	throw UnsupportedOperation(L"NativeFunction is not container");
};

bool NativeFunction::contains(ck_vobject::vscope* scope, const std::wstring& name) {
	if (name == L"__proto")
		return 1;
	
	return NativeFunctionProto && NativeFunctionProto->Object::contains(scope, name);
};

bool NativeFunction::remove(ck_vobject::vscope* scope, const std::wstring& name) {
	throw UnsupportedOperation(L"NativeFunction is not container");
};

vobject* NativeFunction::call(ck_vobject::vscope* scope, const std::vector<vobject*>& args) {
	throw UnsupportedOperation(L"BytecodeFunction is not directly callable");
};

void NativeFunction::gc_mark() {
	Function::gc_mark();
};


std::wstring NativeFunction::string_value() { 
	return std::wstring(L"[NativeFunction ") + std::to_wstring((int) (intptr_t) call_wrapper) + std::wstring(L"]"); 
};

