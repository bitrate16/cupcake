#include "objects/BytecodeFunction.h"

#include <string>

#include "exceptions.h"
#include "GIL2.h"

#include "objects/Array.h"

using namespace std;
using namespace ck_exceptions;
using namespace ck_vobject;
using namespace ck_objects;
using namespace ck_core;

vobject* BytecodeFunction::create_proto() {
	if (BytecodeFunctionProto != nullptr)
		return BytecodeFunctionProto;
	
	BytecodeFunctionProto = new Object();
	GIL::gc_instance()->attach_root(BytecodeFunctionProto);
	
	// ...
	
	return BytecodeFunctionProto;
};


BytecodeFunction::BytecodeFunction(ck_vobject::vscope* definition_scope, ck_script* function_script, const std::vector<std::wstring>& argnames) : scope(definition_scope), script(function_script), argn(argnames) {};

BytecodeFunction::~BytecodeFunction() {
	delete script;
};

// Delegate to prototype
vobject* BytecodeFunction::get(ck_vobject::vscope* scope, const std::wstring& name) {
	return BytecodeFunctionProto ? BytecodeFunctionProto->get(scope, name) : nullptr;
};

void BytecodeFunction::put(ck_vobject::vscope* scope, const std::wstring& name, vobject* object) {
	throw ck_message(L"BytecodeFunction is not container", ck_message_type::CK_UNSUPPORTED_OPERATION);
};

// Delegate to prototype
bool BytecodeFunction::contains(ck_vobject::vscope* scope, const std::wstring& name) {	
	return BytecodeFunctionProto && BytecodeFunctionProto->contains(scope, name);
};

bool BytecodeFunction::remove(ck_vobject::vscope* scope, const std::wstring& name) {
	throw ck_message(L"BytecodeFunction is not container", ck_message_type::CK_UNSUPPORTED_OPERATION);
	return 0;
};

vobject* BytecodeFunction::call(ck_vobject::vscope* scope, const std::vector<vobject*> args) {
	throw ck_message(L"BytecodeFunction is not directly callable", ck_message_type::CK_UNSUPPORTED_OPERATION);
};

vscope* BytecodeFunction::apply(const std::vector<ck_vobject::vobject*>& args) {
	vscope* nscope = new vscope(scope);
	Array* arguments = new Array(args);
	nscope->put(L"__arguments", arguments);
	return nscope;
};

void BytecodeFunction::gc_mark() {
	if (gc_reachable)
		return;
	
	gc_reach();
	
	if (scope && !scope->gc_reachable)
		scope->gc_mark();
};

void BytecodeFunction::gc_finalize() {};


long long BytecodeFunction::int_value() {
	return (int) (intptr_t) this;
};

wstring BytecodeFunction::string_value() {
	return std::wstring(L"[Function ") + std::to_wstring((int) (intptr_t) this) + std::wstring(L"]"); 
};

