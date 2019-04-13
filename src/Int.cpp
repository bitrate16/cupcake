#include "objects/Int.h"

#include <string>

#include "exceptions.h"
#include "GIL2.h"

#include "objects/String.h"
#include "objects/Double.h"

using namespace std;
using namespace ck_exceptions;
using namespace ck_vobject;
using namespace ck_objects;
using namespace ck_core;


static vobject* call_handler(vscope* scope, const vector<vobject*>& args) {
	if (args.size() == 0)
		return new Int(0);
	
	if (args[0]->is_typeof<String>())
		try {
			return new Int(std::stoi(((String*) args[0])->value()));
		} catch (...) {
			return new Double(0);
		}
	
	if (args[0]->is_typeof<Double>())
		return new Int(((Double*) args[0])->value());
	
	if (args[0]->is_typeof<Int>())
		return new Int(((Int*) args[0])->value());
		
	return new Int(args[0]->int_value());
};

vobject* Int::create_proto() {
	if (IntProto != nullptr)
		return IntProto;
	
	IntProto = new CallablePrototype(call_handler);
	GIL::gc_instance()->attach_root(IntProto);
	
	// ...
	
	return IntProto;
};


Int::Int(long long value) : val(value) {};

Int::~Int() {};

// Delegate to prototype
vobject* Int::get(ck_vobject::vscope* scope, const std::wstring& name) {
	if (name == L"proto")
		return IntProto;
	
	return IntProto ? IntProto->Object::get(scope, name) : nullptr;
};

void Int::put(ck_vobject::vscope* scope, const std::wstring& name, vobject* object) {
	throw UnsupportedOperation(L"Int is not container");
};

// Delegate to prototype
bool Int::contains(ck_vobject::vscope* scope, const std::wstring& name) {	
	if (name == L"proto")
		return 1;
	
	return IntProto && IntProto->Object::contains(scope, name);
};

bool Int::remove(ck_vobject::vscope* scope, const std::wstring& name) {
	throw UnsupportedOperation(L"Int is not container");
};

vobject* Int::call(ck_vobject::vscope* scope, const std::vector<vobject*>& args) {
	throw UnsupportedOperation(L"Int is not callable");
};


long long Int::int_value() {
	return val;
};

wstring Int::string_value() {
	return std::to_wstring(val);
};
