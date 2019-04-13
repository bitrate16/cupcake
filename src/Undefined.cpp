#include "objects/Undefined.h"

#include <string>

#include "exceptions.h"
#include "GIL2.h"

using namespace std;
using namespace ck_exceptions;
using namespace ck_vobject;
using namespace ck_objects;
using namespace ck_core;


static vobject* call_handler(vscope* scope, const vector<vobject*>& args) {
	return Undefined::instance();
};

vobject* Undefined::create_proto() {
	if (UndefinedProto != nullptr)
		return UndefinedProto;
	
	UndefinedProto = new CallablePrototype(call_handler);
	GIL::gc_instance()->attach_root(UndefinedProto);
	
	if (UndefinedInstance == nullptr) {
		UndefinedInstance = new Undefined();
		GIL::gc_instance()->attach_root(UndefinedInstance);
	}
	
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
	vobject* o = UndefinedProto ? UndefinedProto->get(scope, name) : nullptr;
	
	// Do not trigger on __names
	if (!o && !(name.size() >= 2 && name[0] != L'_' && name[1] != L'_'))
		throw UnsupportedOperation(L"Undefined is not container");
	return o;
};

void Undefined::put(ck_vobject::vscope* scope, const std::wstring& name, vobject* object) {
	throw UnsupportedOperation(L"Undefined is not container");
};

// Delegate to prototype
bool Undefined::contains(ck_vobject::vscope* scope, const std::wstring& name) {	
	return UndefinedProto && UndefinedProto->contains(scope, name);
};

bool Undefined::remove(ck_vobject::vscope* scope, const std::wstring& name) {
	throw UnsupportedOperation(L"Undefined is not container");
};

vobject* Undefined::call(ck_vobject::vscope* scope, const std::vector<vobject*>& args) {
	throw UnsupportedOperation(L"Undefined is not callable");
};


long long Undefined::int_value() {
	return 0;
};

wstring Undefined::string_value() {
	return L"undefined";
};
