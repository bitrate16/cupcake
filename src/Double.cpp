#include "objects/Double.h"

#include <string>

#include "exceptions.h"
#include "GIL2.h"

#include "objects/String.h"
#include "objects/Int.h"

using namespace std;
using namespace ck_exceptions;
using namespace ck_vobject;
using namespace ck_objects;
using namespace ck_core;


static vobject* call_handler(vscope* scope, const vector<vobject*>& args) {
	if (args.size() == 0)
		return new Double(0);
	
	if (args[0]->is_typeof<String>())
		try {
			return new Double(std::stod(((String*) args[0])->value()));
		} catch (...) {
			return new Double(0);
		}
	
	if (args[0]->is_typeof<Double>())
		return new Double(((Double*) args[0])->value());
	
	if (args[0]->is_typeof<Int>())
		return new Double(((Int*) args[0])->value());
		
	return new Double(args[0]->int_value());
};

vobject* Double::create_proto() {
	if (DoubleProto != nullptr)
		return DoubleProto;
	
	DoubleProto = new CallablePrototype(call_handler);
	GIL::gc_instance()->attach_root(DoubleProto);
	
	// ...
	
	return DoubleProto;
};


Double::Double(double value) : val(value) {};

Double::~Double() {};

// Delegate to prototype
vobject* Double::get(ck_vobject::vscope* scope, const std::wstring& name) {
	return DoubleProto ? DoubleProto->get(scope, name) : nullptr;
};

void Double::put(ck_vobject::vscope* scope, const std::wstring& name, vobject* object) {
	throw ck_message(L"Double is not container", ck_message_type::CK_UNSUPPORTED_OPERATION);
};

// Delegate to prototype
bool Double::contains(ck_vobject::vscope* scope, const std::wstring& name) {	
	return DoubleProto && DoubleProto->contains(scope, name);
};

bool Double::remove(ck_vobject::vscope* scope, const std::wstring& name) {
	throw ck_message(L"Double is not container", ck_message_type::CK_UNSUPPORTED_OPERATION);
	return 0;
};

vobject* Double::call(ck_vobject::vscope* scope, const std::vector<vobject*> args) {
	throw ck_message(L"Double is not callable", ck_message_type::CK_UNSUPPORTED_OPERATION);
};

void Double::gc_mark() {};

void Double::gc_finalize() {};


long long Double::int_value() {
	return val;
};

wstring Double::string_value() {
	return std::to_wstring(val);
};
