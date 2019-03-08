#include "objects/Undefined.h"

#include <string>

#include "exceptions.h"
#include "GIL2.h"

using namespace std;
using namespace ck_exceptions;
using namespace ck_vobject;
using namespace ck_objects;
using namespace ck_core;

vobject* Undefined::create_proto() {
	if (UndefinedProto != nullptr)
		return UndefinedProto;
	
	UndefinedProto = new Object();
	GIL::gc_instance()->attach_root(UndefinedProto);
	
	// ...
	
	return UndefinedProto;
};

Undefined* Undefined::instance() {
	if (UndefinedInstance == nullptr) {
		UndefinedInstance = new Undefined();
		GIL::gc_instance()->attach_root(UndefinedInstance);
	}
	return UndefinedInstance;
};


Undefined::Undefined() {};

Undefined::~Undefined() {};

// Delegate to prototype
vobject* Undefined::get(ck_vobject::vscope* scope, const std::wstring& name) {
	return UndefinedProto ? UndefinedProto->get(scope, name) : nullptr;
};

void Undefined::put(ck_vobject::vscope* scope, const std::wstring& name, vobject* object) {
	throw ck_message(L"Undefined is not container", ck_message_type::CK_UNSUPPORTED_OPERATION);
};

// Delegate to prototype
bool Undefined::contains(ck_vobject::vscope* scope, const std::wstring& name) {	
	return UndefinedProto && UndefinedProto->contains(scope, name);
};

bool Undefined::remove(ck_vobject::vscope* scope, const std::wstring& name) {
	throw ck_message(L"Undefined is not container", ck_message_type::CK_UNSUPPORTED_OPERATION);
	return 0;
};

vobject* Undefined::call(ck_vobject::vscope* scope, std::vector<vobject*> args) {
	throw ck_message(L"Undefined is not callable", ck_message_type::CK_UNSUPPORTED_OPERATION);
};

void Undefined::gc_mark() {};

void Undefined::gc_finalize() {};


long long Undefined::int_value() {
	return 0;
};

wstring Undefined::string_value() {
	return L"null";
};
