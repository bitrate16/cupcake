#include "objects/Bool.h"

#include <string>

#include "exceptions.h"
#include "GIL2.h"

#include "objects/String.h"

using namespace std;
using namespace ck_exceptions;
using namespace ck_vobject;
using namespace ck_objects;
using namespace ck_core;


static vobject* call_handler(vscope* scope, const vector<vobject*>& args) {
	if (args.size() == 0)
		return new Bool(0);
	
	if (args[0]->is_typeof<String>())
		if (((String*) args[0])->value() == L"false")
			return new Bool(0);
		else
			return new Bool(1);
		
	return new Bool(args[0]->int_value());
};

vobject* Bool::create_proto() {
	if (BoolProto != nullptr)
		return BoolProto;
	
	BoolProto = new CallablePrototype(call_handler);
	GIL::gc_instance()->attach_root(BoolProto);
	
	// ...
	
	return BoolProto;
};


Bool::Bool(bool value) : val(value) {};

Bool::~Bool() {};

// Delegate to prototype
vobject* Bool::get(ck_vobject::vscope* scope, const std::wstring& name) {
	if (name == L"proto")
		return BoolProto;
	
	return BoolProto ? BoolProto->Object::get(scope, name) : nullptr;
};

void Bool::put(ck_vobject::vscope* scope, const std::wstring& name, vobject* object) {
	throw UnsupportedOperation(L"Bool is not container");
};

// Delegate to prototype
bool Bool::contains(ck_vobject::vscope* scope, const std::wstring& name) {	
	if (name == L"proto")
		return 1;
	
	return BoolProto && BoolProto->Object::contains(scope, name);
};

bool Bool::remove(ck_vobject::vscope* scope, const std::wstring& name) {
	throw UnsupportedOperation(L"Bool is not container");
};

vobject* Bool::call(ck_vobject::vscope* scope, const std::vector<vobject*> args) {
	throw UnsupportedOperation(L"Bool is not callable");
};


long long Bool::int_value() {
	return val;
};

wstring Bool::string_value() {
	return val ? L"true" : L"false";
};
