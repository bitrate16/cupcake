#include "objects/BytecodeFunction.h"

#include <string>

#include "exceptions.h"
#include "GIL2.h"

#include "objects/Array.h"
#include "objects/String.h"

using namespace std;
using namespace ck_exceptions;
using namespace ck_vobject;
using namespace ck_objects;
using namespace ck_core;


static vobject* call_handler(vscope* scope, const vector<vobject*>& args) {
	throw UnsupportedOperation(L"BytecodeFunction construct uncomplete");
};

vobject* BytecodeFunction::create_proto() {
	if (BytecodeFunctionProto != nullptr)
		return BytecodeFunctionProto;
	
	BytecodeFunctionProto = new CallablePrototype(call_handler);
	GIL::gc_instance()->attach_root(BytecodeFunctionProto);
	
	BytecodeFunctionProto->Object::put(L"__typename", new String(L"Function"));	
	
	return BytecodeFunctionProto;
};


BytecodeFunction::BytecodeFunction(ck_vobject::vscope* definition_scope, ck_script* function_script, const std::vector<std::wstring>& argnames) : scope(definition_scope), script(function_script), argn(argnames) {};

BytecodeFunction::~BytecodeFunction() {
	delete script;
};

// Delegate to prototype
vobject* BytecodeFunction::get(ck_vobject::vscope* scope, const std::wstring& name) {
	if (name == L"__proto")
		return BytecodeFunctionProto;
	
	return BytecodeFunctionProto ? BytecodeFunctionProto->Object::get(name) : nullptr;
};

void BytecodeFunction::put(ck_vobject::vscope* scope, const std::wstring& name, vobject* object) {
	throw UnsupportedOperation(L"BytecodeFunction is not container");
};

// Delegate to prototype
bool BytecodeFunction::contains(ck_vobject::vscope* scope, const std::wstring& name) {	
	if (name == L"__proto")
		return 1;
	
	return BytecodeFunctionProto && BytecodeFunctionProto->Object::contains(name);
};

bool BytecodeFunction::remove(ck_vobject::vscope* scope, const std::wstring& name) {
	throw UnsupportedOperation(L"BytecodeFunction is not container");
};

vobject* BytecodeFunction::call(ck_vobject::vscope* scope, const std::vector<vobject*>& args) {
	throw UnsupportedOperation(L"BytecodeFunction is not directly callable");
};

vscope* BytecodeFunction::apply(ck_vobject::vobject* this_bind, const std::vector<ck_vobject::vobject*>& args, ck_vobject::vscope* caller_scope) {
	vscope* nscope = new iscope(scope);
	
	// Apply args
	Array* arguments = new Array(args);
	nscope->put(L"__args", arguments);
	
	int min = argn.size();
	min = min < args.size() ? min : args.size();
	
	for (int i = 0; i < min; ++i)
		nscope->put(argn[i], args[i], 0, 1);
	
	// Bind __this
	nscope->put(L"__this", this_bind);
	
	return nscope;
};

void BytecodeFunction::gc_mark() {
	if (gc_reachable)
		return;
	
	Function::gc_mark();
	
	if (scope && !scope->gc_reachable)
		scope->gc_mark();
};


wstring BytecodeFunction::string_value() {
	return std::wstring(L"[Function ") + std::to_wstring((int) (intptr_t) this) + std::wstring(L"]"); 
};

