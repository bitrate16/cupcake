#include "objects/Null.h"

#include <string>

#include "exceptions.h"
#include "GIL2.h"

#include "objects/String.h"
#include "objects/Double.h"
#include "objects/NativeFunction.h"
#include "objects/Bool.h"
#include "objects/Undefined.h"

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
	
	NullProto = new CallableObject(call_handler);
	GIL::gc_instance()->attach_root(NullProto);
	
	if (NullInstance == nullptr) {
		NullInstance = new Null();
		GIL::gc_instance()->attach_root(NullInstance);
	}
	
	NullProto->Object::put(L"__typename", new String(L"Null"));
	
	NullProto->Object::put(L"__operator==", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2)
				return Bool::False();
			
			if (!args[0] && !args[1])
				return Bool::True();
			
			if (args[0] && args[1] && dynamic_cast<Null*>(args[0]) && dynamic_cast<Null*>(args[1]))
				return Bool::True();
			
			return Bool::False();
		}));
	NullProto->Object::put(L"__operator!=", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2)
				return Bool::False();
			
			if (args[0] || args[1])
				return Bool::True();
			
			if (args[0] && args[1] && dynamic_cast<Null*>(args[0]) && dynamic_cast<Null*>(args[1]))
				return Bool::False();
			
			return Bool::True();
		}));
	
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
	vobject* o = NullProto ? NullProto->Object::get(scope, name) : nullptr;
	
	// Do not trigger on __names
	if (!o && !(name.size() >= 2 && name[0] != L'_' && name[1] != L'_'))
		throw UnsupportedOperation(L"Null is not container");
	return o;
};

void Null::put(ck_vobject::vscope* scope, const std::wstring& name, vobject* object) {
	throw UnsupportedOperation(L"Null is not container");
};

// Delegate to prototype
bool Null::contains(ck_vobject::vscope* scope, const std::wstring& name) {	
	return NullProto && NullProto->Object::contains(scope, name);
};

bool Null::remove(ck_vobject::vscope* scope, const std::wstring& name) {
	throw UnsupportedOperation(L"Null is not container");
};

vobject* Null::call(ck_vobject::vscope* scope, const std::vector<vobject*>& args) {
	throw UnsupportedOperation(L"Null is not callable");
};


int64_t Null::int_value() {
	return 0;
};

wstring Null::string_value() {
	return L"null";
};
