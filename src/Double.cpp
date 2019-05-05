#include "objects/Double.h"

#include <string>
#include <sstream>

#include "exceptions.h"
#include "GIL2.h"

#include "objects/String.h"
#include "objects/Int.h"
#include "objects/NativeFunction.h"
#include "objects/Undefined.h"
#include "objects/Bool.h"

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
	
	DoubleProto->Object::put(L"__typename", new String(L"Double"));
	DoubleProto->Object::put(L"parse", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (!args.size())
				return Undefined::instance();
			
			if (!args.size() || !args[0])
				return Undefined::instance();
			
			std::wstring str = args[0]->string_value();
			
			double dbl = 0.0;
			std::wistringstream num(str);

			num >> dbl;

			if(!num.fail() && num.eof())
				return new Double(dbl);
			else
				return Undefined::instance();
		})); 
	
	// Operators
	DoubleProto->Object::put(L"__operator==", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			Double *a, *b;
			
			a = dynamic_cast<Double*>(args[0]);
			b = dynamic_cast<Double*>(args[1]);
			
			double av = a ? a->value() : args[0]->int_value();
			double bv = b ? b->value() : args[1]->int_value();
			
			return Bool::instance(av == bv);
		}));
	DoubleProto->Object::put(L"__operator!=", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			Double *a, *b;
			
			a = dynamic_cast<Double*>(args[0]);
			b = dynamic_cast<Double*>(args[1]);
			
			double av = a ? a->value() : args[0]->int_value();
			double bv = b ? b->value() : args[1]->int_value();
			
			return Bool::instance(av != bv);
		}));
	
	
	return DoubleProto;
};


Double::Double(double value) : val(value) {};

Double::~Double() {};

// Delegate to prototype
vobject* Double::get(ck_vobject::vscope* scope, const std::wstring& name) {
	if (name == L"__proto")
		return IntProto;
	
	return DoubleProto ? DoubleProto->Object::get(scope, name) : nullptr;
};

void Double::put(ck_vobject::vscope* scope, const std::wstring& name, vobject* object) {
	throw UnsupportedOperation(L"Double is not container");
};

// Delegate to prototype
bool Double::contains(ck_vobject::vscope* scope, const std::wstring& name) {	
	if (name == L"__proto")
		return 1;
	
	return DoubleProto && DoubleProto->Object::contains(scope, name);
};

bool Double::remove(ck_vobject::vscope* scope, const std::wstring& name) {
	throw UnsupportedOperation(L"Double is not container");
};

vobject* Double::call(ck_vobject::vscope* scope, const std::vector<vobject*>& args) {
	throw UnsupportedOperation(L"Double is not callable");
};


long long Double::int_value() {
	return val;
};

wstring Double::string_value() {
	return std::to_wstring(val);
};
