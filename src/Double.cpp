#include "objects/Double.h"

#include <string>
#include <sstream>
#include <cfloat>
#include <climits>
#include <cmath>

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
	
	if (args[0]->as_type<String>())
		try {
			return new Double(std::stod(((String*) args[0])->value()));
		} catch (...) {
			return new Double(0);
		}
	
	if (args[0]->as_type<Double>())
		return new Double(((Double*) args[0])->value());
	
	if (args[0]->as_type<Int>())
		return new Double(((Int*) args[0])->value());
		
	return new Double(args[0]->int_value());
};

vobject* Double::create_proto() {
	if (DoubleProto != nullptr)
		return DoubleProto;
	
	DoubleProto = new CallableObject(call_handler);
	GIL::gc_instance()->attach_root(DoubleProto);
	
	DoubleProto->Object::put(L"__typename", new String(L"Double"));
	DoubleProto->Object::put(L"MAX_VALUE", new Double(DBL_MAX));
	DoubleProto->Object::put(L"MIN_VALUE", new Double(DBL_MIN));
	DoubleProto->Object::put(L"expsilon", new Double(DBL_EPSILON));
	DoubleProto->Object::put(L"infinity", new Double(std::numeric_limits<double>::infinity()));
	DoubleProto->Object::put(L"SIZEOF", new Int(sizeof(double)));
	DoubleProto->Object::put(L"NaN", Double_NAN = new Double(nan("")));
	GIL::gc_instance()->attach_root(Double_NAN);
	
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
	DoubleProto->Object::put(L"isNaN", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (!args.size())
				return Undefined::instance();
			
			if (!args.size() || !args[0])
				return Undefined::instance();
			
			Double* d = dynamic_cast<Double*>(args[0]);
			
			return Bool::instance(isnan(d->value()));
		}));
	DoubleProto->Object::put(L"isInfinity", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (!args.size())
				return Undefined::instance();
			
			if (!args.size() || !args[0])
				return Undefined::instance();
			
			Double* d = dynamic_cast<Double*>(args[0]);
			
			return Bool::instance(isinf(d->value()));
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
	DoubleProto->Object::put(L"__operator>", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			if (Int* i = dynamic_cast<Int*>(args[0]); i) {
				if (Double* d = dynamic_cast<Double*>(args[1]); d)
					return Bool::instance(i->value() > d->value());
				return Bool::instance(i->value() > args[1]->int_value());
			}
			return Undefined::instance();
		}));
	DoubleProto->Object::put(L"__operator>=", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			if (Double* i = dynamic_cast<Double*>(args[0]); i) {
				if (Double* d = dynamic_cast<Double*>(args[1]); d)
					return Bool::instance(i->value() >= d->value());
				return Bool::instance(i->value() >= args[1]->int_value());
			}
			return Undefined::instance();
		}));
	DoubleProto->Object::put(L"__operator<", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			if (Double* i = dynamic_cast<Double*>(args[0]); i) {
				if (Double* d = dynamic_cast<Double*>(args[1]); d)
					return Bool::instance(i->value() < d->value());
				return Bool::instance(i->value() < args[1]->int_value());
			}
			return Undefined::instance();
		}));
	DoubleProto->Object::put(L"__operator<=", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			if (Double* i = dynamic_cast<Double*>(args[0]); i) {
				if (Double* d = dynamic_cast<Double*>(args[1]); d)
					return Bool::instance(i->value() <= d->value());
				return Bool::instance(i->value() <= args[1]->int_value());
			}
			return Undefined::instance();
		}));
	
	DoubleProto->Object::put(L"__operator+", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			if (Double* i = dynamic_cast<Double*>(args[0]); i) {
				if (String* s = dynamic_cast<String*>(args[1]); s) 
					return new String(i->string_value() + s->value());
				if (Double* d = dynamic_cast<Double*>(args[1]); d)
					return new Double(i->value() + d->value());
				return new Double(i->value() + args[1]->int_value());
			}
			return Undefined::instance();
		}));
	DoubleProto->Object::put(L"__operator-", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			if (Double* i = dynamic_cast<Double*>(args[0]); i) {
				if (Double* d = dynamic_cast<Double*>(args[1]); d)
					return new Double(i->value() - d->value());
				return new Double(i->value() - args[1]->int_value());
			}
			return Undefined::instance();
		}));
	DoubleProto->Object::put(L"__operator*", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			if (Double* i = dynamic_cast<Double*>(args[0]); i) {
				if (Double* d = dynamic_cast<Double*>(args[1]); d)
					return new Double(i->value() * d->value());
				return new Double(i->value() * args[1]->int_value());
			}
			return Undefined::instance();
		}));
	DoubleProto->Object::put(L"__operator/", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			if (Double* i = dynamic_cast<Double*>(args[0]); i) {
				if (Double* d = dynamic_cast<Double*>(args[1]); d)
					return new Double(i->value() / d->value());
				
				if (long long b = args[1]->int_value(); b)
					return new Double(i->value() / static_cast<double>(b));
				else
					return Double::NaN();
			}
			return Undefined::instance();
		}));
	// Integral division
	DoubleProto->Object::put(L"__operator#", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			if (Double* i = dynamic_cast<Double*>(args[0]); i) {
				if (long long b = args[1]->int_value(); b)
					return new Int(static_cast<long long>(round(i->value() / b)));
				else
					return Undefined::instance();
			}
			return Undefined::instance();
		}));
	DoubleProto->Object::put(L"__operator%", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			if (Double* i = dynamic_cast<Double*>(args[0]); i) {
				if (long long b = args[1]->int_value(); b)
					return new Double(fmod(i->value(), b));
				else
					return Undefined::instance();
			}
			return Undefined::instance();
		}));
		
	DoubleProto->Object::put(L"__operator&", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			if (Double* i = dynamic_cast<Double*>(args[0]); i) {
				long long b = args[1]->int_value();
				return new Int(static_cast<long long>(round(i->value())) & b);
			}
			return Undefined::instance();
		}));
	DoubleProto->Object::put(L"__operator|", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			if (Double* i = dynamic_cast<Double*>(args[0]); i) {
				long long b = args[1]->int_value();
				return new Int(static_cast<long long>(round(i->value())) | b);
			}
			return Undefined::instance();
		}));
	DoubleProto->Object::put(L"__operator^", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			if (Double* i = dynamic_cast<Double*>(args[0]); i) {
				long long b = args[1]->int_value();
				return new Int(static_cast<long long>(round(i->value())) ^ b);
			}
			return Undefined::instance();
		}));
	DoubleProto->Object::put(L"__operator<<", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			if (Double* i = dynamic_cast<Double*>(args[0]); i) {
				long long b = args[1]->int_value();
				return new Int(static_cast<long long>(round(i->value())) << b);
			}
			return Undefined::instance();
		}));
	DoubleProto->Object::put(L"__operator>>", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			if (Double* i = dynamic_cast<Double*>(args[0]); i) {
				long long b = args[1]->int_value();
				return new Int(static_cast<long long>(round(i->value())) >> b);
			}
			return Undefined::instance();
		}));
	DoubleProto->Object::put(L"__operator>>>", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			if (Double* i = dynamic_cast<Double*>(args[0]); i) {
				unsigned long long a = static_cast<long long>(round(i->value()));
				unsigned long long b = args[1]->int_value();
				return new Int(a >> b);
			}
			return Undefined::instance();
		}));
	
	DoubleProto->Object::put(L"__operator&&", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			if (Double* i = dynamic_cast<Double*>(args[0]); i) {
				long long b = args[1]->int_value();
				return Bool::instance(i->value() != 0 && b);
			}
			return Undefined::instance();
		}));
	DoubleProto->Object::put(L"__operator||", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			if (Double* i = dynamic_cast<Double*>(args[0]); i) {
				long long b = args[1]->int_value();
				return Bool::instance(i->value() != 0 || b);
			}
			return Undefined::instance();
		}));
	
	DoubleProto->Object::put(L"__operator-x", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 1 || !args[0])
				return Undefined::instance();
			
			if (Double* i = dynamic_cast<Double*>(args[0]); i) 
				return new Double(-i->value());
			
			return Undefined::instance();
		}));
	DoubleProto->Object::put(L"__operator+x", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 1 || !args[0])
				return Undefined::instance();
			
			if (Double* i = dynamic_cast<Double*>(args[0]); i) 
				return i;
			
			return Undefined::instance();
		}));
	DoubleProto->Object::put(L"__operator++", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 1 || !args[0])
				return Undefined::instance();
			
			if (Double* i = dynamic_cast<Double*>(args[0]); i) 
				return new Double(i->value() + 1.0);
			
			return Undefined::instance();
		}));
	DoubleProto->Object::put(L"__operator--", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 1 || !args[0])
				return Undefined::instance();
			
			if (Double* i = dynamic_cast<Double*>(args[0]); i) 
				return new Double(i->value() - 1.0);
			
			return Undefined::instance();
		}));
	DoubleProto->Object::put(L"__operator!x", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 1 || !args[0])
				return Undefined::instance();
			
			if (Double* i = dynamic_cast<Double*>(args[0]); i) 
				return new Double(i->value() == 0.0);
			
			return Undefined::instance();
		}));
	DoubleProto->Object::put(L"__operator~x", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 1 || !args[0])
				return Undefined::instance();
			
			if (Double* i = dynamic_cast<Double*>(args[0]); i) 
				return new Int(~static_cast<long long>(round(i->value())));
			
			return Undefined::instance();
		}));
	
	
	return DoubleProto;
};


Double::Double(double value) : val(value) {};

Double::~Double() {};

// Delegate to prototype
vobject* Double::get(ck_vobject::vscope* scope, const std::wstring& name) {
	if (name == L"__proto")
		return DoubleProto;
	
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

Double* Double::NaN() {
	if (Double_NAN)
		return Double_NAN;
	
	Double_NAN = new Double(nan(""));
	GIL::gc_instance()->attach_root(Double_NAN);
	return Double_NAN;
}

long long Double::int_value() {
	return static_cast<long long>(round(val));
};

wstring Double::string_value() {
	return std::to_wstring(val);
};
