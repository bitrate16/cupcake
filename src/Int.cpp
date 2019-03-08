#include "objects/Int.h"

#include <string>

#include "exceptions.h"
#include "GIL2.h"

using namespace std;
using namespace ck_exceptions;
using namespace ck_vobject;
using namespace ck_objects;
using namespace ck_core;

vobject* Int::create_proto() {
	if (IntProto != nullptr)
		return IntProto;
	
	IntProto = new Object();
	GIL::gc_instance()->attach_root(IntProto);
	
	// ...
	
	return IntProto;
};


Int::Int(long long value) : val(value) {};

Int::~Int() {};

// Delegate to prototype
vobject* Int::get(ck_vobject::vscope* scope, const std::wstring& name) {
	return IntProto ? IntProto->get(scope, name) : nullptr;
};

void Int::put(ck_vobject::vscope* scope, const std::wstring& name, vobject* object) {
	throw ck_message(L"Int is not container", ck_message_type::CK_UNSUPPORTED_OPERATION);
};

// Delegate to prototype
bool Int::contains(ck_vobject::vscope* scope, const std::wstring& name) {	
	return IntProto && IntProto->contains(scope, name);
};

bool Int::remove(ck_vobject::vscope* scope, const std::wstring& name) {
	throw ck_message(L"Int is not container", ck_message_type::CK_UNSUPPORTED_OPERATION);
	return 0;
};

vobject* Int::call(ck_vobject::vscope* scope, std::vector<vobject*> args) {
	throw ck_message(L"Int is not callable", ck_message_type::CK_UNSUPPORTED_OPERATION);
};

void Int::gc_mark() {};

void Int::gc_finalize() {};


long long Int::int_value() {
	return val;
};

wstring Int::string_value() {
	return std::to_wstring(val);
};
