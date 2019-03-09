#include "objects/Bool.h"

#include <string>

#include "exceptions.h"
#include "GIL2.h"

using namespace std;
using namespace ck_exceptions;
using namespace ck_vobject;
using namespace ck_objects;
using namespace ck_core;

vobject* Bool::create_proto() {
	if (BoolProto != nullptr)
		return BoolProto;
	
	BoolProto = new Object();
	GIL::gc_instance()->attach_root(BoolProto);
	
	// ...
	
	return BoolProto;
};


Bool::Bool(bool value) : val(value) {};

Bool::~Bool() {};

// Delegate to prototype
vobject* Bool::get(ck_vobject::vscope* scope, const std::wstring& name) {
	return BoolProto ? BoolProto->get(scope, name) : nullptr;
};

void Bool::put(ck_vobject::vscope* scope, const std::wstring& name, vobject* object) {
	throw ck_message(L"Bool is not container", ck_message_type::CK_UNSUPPORTED_OPERATION);
};

// Delegate to prototype
bool Bool::contains(ck_vobject::vscope* scope, const std::wstring& name) {	
	return BoolProto && BoolProto->contains(scope, name);
};

bool Bool::remove(ck_vobject::vscope* scope, const std::wstring& name) {
	throw ck_message(L"Bool is not container", ck_message_type::CK_UNSUPPORTED_OPERATION);
	return 0;
};

vobject* Bool::call(ck_vobject::vscope* scope, const std::vector<vobject*> args) {
	throw ck_message(L"Bool is not callable", ck_message_type::CK_UNSUPPORTED_OPERATION);
};

void Bool::gc_mark() {};

void Bool::gc_finalize() {};


long long Bool::int_value() {
	return val;
};

wstring Bool::string_value() {
	return val ? L"true" : L"false";
};
