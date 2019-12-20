#include "objects/Undefined.h"

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
	return Undefined::instance();
};

vobject* Undefined::create_proto() {
	if (UndefinedProto != nullptr)
		return UndefinedProto;
	
	UndefinedProto = new CallableObject(call_handler);
	GIL::gc_instance()->attach_root(UndefinedProto);
	
	if (UndefinedInstance == nullptr) {
		UndefinedInstance = new Undefined();
		GIL::gc_instance()->attach_root(UndefinedInstance);
	}
	
	UndefinedProto->Object::put(L"__typename", new String(L"Undefined"));
	
	UndefinedProto->Object::put(L"__operator==", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2)
				return Bool::False();
			
			if (!args[0] && !args[1])
				return Bool::True();
			
			if (args[0] && args[1] && dynamic_cast<Undefined*>(args[0]) && dynamic_cast<Undefined*>(args[1]))
				return Bool::True();
			
			return Bool::False();
		}));
	UndefinedProto->Object::put(L"__operator!=", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2)
				return Bool::False();
			
			if (args[0] || args[1])
				return Bool::True();
			
			if (args[0] && args[1] && dynamic_cast<Undefined*>(args[0]) && dynamic_cast<Undefined*>(args[1]))
				return Bool::False();
			
			return Bool::True();
		}));
	
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
	vobject* o = UndefinedProto ? UndefinedProto->Object::get(scope, name) : nullptr;
	
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
	return UndefinedProto && UndefinedProto->Object::contains(scope, name);
};

bool Undefined::remove(ck_vobject::vscope* scope, const std::wstring& name) {
	throw UnsupportedOperation(L"Undefined is not container");
};

vobject* Undefined::call(ck_vobject::vscope* scope, const std::vector<vobject*>& args) {
	throw UnsupportedOperation(L"Undefined is not callable");
};


int64_t Undefined::int_value() {
	return 0;
};

wstring Undefined::string_value() {
	return L"undefined";
};
