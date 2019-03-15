#include "objects/Null.h"

#include <string>

#include "exceptions.h"
#include "GIL2.h"

using namespace std;
using namespace ck_exceptions;
using namespace ck_vobject;
using namespace ck_objects;
using namespace ck_core;


static vobject* call_handler(vscope* scope, const vector<vobject*>& args) {
	return Null::instance();
};

vobject* Null::create_proto() {
	if (NullProto != nullptr)
		return NullProto;
	
	NullProto = new CallablePrototype(call_handler);
	GIL::gc_instance()->attach_root(NullProto);
	
	if (NullInstance == nullptr) {
		NullInstance = new Null();
		GIL::gc_instance()->attach_root(NullInstance);
	}
	
	// ...
	
	return NullProto;
};

Null* Null::instance() {
	if (NullInstance == nullptr) {
		NullInstance = new Null();
		GIL::gc_instance()->attach_root(NullInstance);
	}
	return NullInstance;
};


Null::Null() {};

Null::~Null() {};

// Delegate to prototype
vobject* Null::get(ck_vobject::vscope* scope, const std::wstring& name) {
	return NullProto ? NullProto->get(scope, name) : nullptr;
};

void Null::put(ck_vobject::vscope* scope, const std::wstring& name, vobject* object) {
	throw UnsupportedOperation(L"Null is not container");
};

// Delegate to prototype
bool Null::contains(ck_vobject::vscope* scope, const std::wstring& name) {	
	return NullProto && NullProto->contains(scope, name);
};

bool Null::remove(ck_vobject::vscope* scope, const std::wstring& name) {
	throw UnsupportedOperation(L"Null is not container");
	return 0;
};

vobject* Null::call(ck_vobject::vscope* scope, const std::vector<vobject*> args) {
	throw UnsupportedOperation(L"Null is not callable");
};


long long Null::int_value() {
	return 0;
};

wstring Null::string_value() {
	return L"null";
};
