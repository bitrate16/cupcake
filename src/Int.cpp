#include "objects/Int.h"

#include <string>
#include <limits>
#include <sstream>
#include <climits>
#include <cmath>

#include "exceptions.h"
#include "GIL2.h"

#include "objects/String.h"
#include "objects/Double.h"
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
		return new Int(0);
	
	if (args[0]->as_type<String>())
		try {
			return new Int(std::stoi(((String*) args[0])->value()));
		} catch (...) {
			return new Double(0);
		}
	
	if (args[0]->as_type<Double>())
		return new Int(((Double*) args[0])->value());
	
	if (args[0]->as_type<Int>())
		return new Int(((Int*) args[0])->value());
		
	return new Int(args[0]->int_value());
};

vobject* Int::create_proto() {
	if (IntProto != nullptr)
		return IntProto;
	
	IntProto = new CallableObject(call_handler);
	GIL::gc_instance()->attach_root(IntProto);
	
	IntProto->Object::put(L"__typename", new String(L"Int"));
	IntProto->Object::put(L"MAX_VALUE", new Int(std::numeric_limits<int64_t>::max()));
	IntProto->Object::put(L"MIN_VALUE", new Int(std::numeric_limits<int64_t>::min()));
	IntProto->Object::put(L"SIZEOF", new Int(sizeof(long long)));
	IntProto->Object::put(L"parse", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (!args.size())
				return Undefined::instance();
			
			if (!args.size() || !args[0])
				return Undefined::instance();
			
			std::wstring str = args[0]->string_value();
			
			int64_t dbl = 0.0;
			std::wistringstream num(str);

			num >> dbl;

			if(!num.fail() && num.eof())
				return new Int(dbl);
			else
				return Undefined::instance();
		})); 
	
	// Operators
	IntProto->Object::put(L"__operator==", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			return Bool::instance(args[0]->int_value() == args[1]->int_value());
		}));
	IntProto->Object::put(L"__operator!=", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			return Bool::instance(args[0]->int_value() != args[1]->int_value());
		}));
	IntProto->Object::put(L"__operator>", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			if (Int* i = dynamic_cast<Int*>(args[0]); i) {
				if (Double* d = dynamic_cast<Double*>(args[1]); d)
					return Bool::instance(static_cast<double>(i->value()) > d->value());
				return Bool::instance(i->value() > args[1]->int_value());
			}
			return Undefined::instance();
		}));
	IntProto->Object::put(L"__operator>=", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			if (Int* i = dynamic_cast<Int*>(args[0]); i) {
				if (Double* d = dynamic_cast<Double*>(args[1]); d)
					return Bool::instance(static_cast<double>(i->value()) >= d->value());
				return Bool::instance(i->value() >= args[1]->int_value());
			}
			return Undefined::instance();
		}));
	IntProto->Object::put(L"__operator<", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			if (Int* i = dynamic_cast<Int*>(args[0]); i) {
				if (Double* d = dynamic_cast<Double*>(args[1]); d)
					return Bool::instance(static_cast<double>(i->value()) < d->value());
				return Bool::instance(i->value() <args[1]->int_value());
			}
			return Undefined::instance();
		}));
	IntProto->Object::put(L"__operator<=", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			if (Int* i = dynamic_cast<Int*>(args[0]); i) {
				if (Double* d = dynamic_cast<Double*>(args[1]); d)
					return Bool::instance(static_cast<double>(i->value()) <= d->value());
				return Bool::instance(i->value() <= args[1]->int_value());
			}
			return Undefined::instance();
		}));
	
	IntProto->Object::put(L"__operator+", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			if (Int* i = dynamic_cast<Int*>(args[0]); i) {
				if (String* s = dynamic_cast<String*>(args[1]); s) 
					return new String(i->string_value() + s->value());
				if (Double* d = dynamic_cast<Double*>(args[1]); d)
					return new Double(i->value() + d->value());
				return new Int(i->value() + args[1]->int_value());
			}
			return Undefined::instance();
		}));
	IntProto->Object::put(L"__operator-", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			if (Int* i = dynamic_cast<Int*>(args[0]); i) {
				if (Double* d = dynamic_cast<Double*>(args[1]); d)
					return new Double(i->value() - d->value());
				return new Int(i->value() - args[1]->int_value());
			}
			return Undefined::instance();
		}));
	IntProto->Object::put(L"__operator*", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			if (Int* i = dynamic_cast<Int*>(args[0]); i) {
				if (Double* d = dynamic_cast<Double*>(args[1]); d)
					return new Double(i->value() * d->value());
				return new Int(i->value() * args[1]->int_value());
			}
			return Undefined::instance();
		}));
	IntProto->Object::put(L"__operator/", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			if (Int* i = dynamic_cast<Int*>(args[0]); i) {
				if (Double* d = dynamic_cast<Double*>(args[1]); d)
					return new Double(i->value() / d->value());
				
				if (int64_t b = args[1]->int_value(); b)
					return new Int(i->value() / static_cast<double>(b));
				else
					return Undefined::instance();
			}
			return Undefined::instance();
		}));
	// Integral division
	IntProto->Object::put(L"__operator#", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			if (Int* i = dynamic_cast<Int*>(args[0]); i) {
				if (int64_t b = args[1]->int_value(); b)
					return new Int(static_cast<int64_t>(round(i->value() / b)));
				else
					return Undefined::instance();
			}
			return Undefined::instance();
		}));
	IntProto->Object::put(L"__operator%", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			if (Int* i = dynamic_cast<Int*>(args[0]); i) {
				if (int64_t b = args[1]->int_value(); b)
					return new Int(i->value() % b);
				else
					return Undefined::instance();
			}
			return Undefined::instance();
		}));
		
	IntProto->Object::put(L"__operator&", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			if (Int* i = dynamic_cast<Int*>(args[0]); i) {
				int64_t b = args[1]->int_value();
				return new Int(i->value() & b);
			}
			return Undefined::instance();
		}));
	IntProto->Object::put(L"__operator|", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			if (Int* i = dynamic_cast<Int*>(args[0]); i) {
				int64_t b = args[1]->int_value();
				return new Int(i->value() | b);
			}
			return Undefined::instance();
		}));
	IntProto->Object::put(L"__operator^", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			if (Int* i = dynamic_cast<Int*>(args[0]); i) {
				int64_t b = args[1]->int_value();
				return new Int(i->value() ^ b);
			}
			return Undefined::instance();
		}));
	IntProto->Object::put(L"__operator<<", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			if (Int* i = dynamic_cast<Int*>(args[0]); i) {
				int64_t b = args[1]->int_value();
				return new Int(i->value() << b);
			}
			return Undefined::instance();
		}));
	IntProto->Object::put(L"__operator>>", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			if (Int* i = dynamic_cast<Int*>(args[0]); i) {
				int64_t b = args[1]->int_value();
				return new Int(i->value() >> b);
			}
			return Undefined::instance();
		}));
	IntProto->Object::put(L"__operator>>>", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			if (Int* i = dynamic_cast<Int*>(args[0]); i) {
				uint64_t a = i->value();
				uint64_t b = args[1]->int_value();
				return new Int(a >> b);
			}
			return Undefined::instance();
		}));
	
	IntProto->Object::put(L"__operator&&", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			if (Int* i = dynamic_cast<Int*>(args[0]); i) {
				int64_t a = i->value();
				int64_t b = args[1]->int_value();
				return Bool::instance(a && b);
			}
			return Undefined::instance();
		}));
	IntProto->Object::put(L"__operator||", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			if (Int* i = dynamic_cast<Int*>(args[0]); i) {
				int64_t a = i->value();
				int64_t b = args[1]->int_value();
				return Bool::instance(a || b);
			}
			return Undefined::instance();
		}));
	
	IntProto->Object::put(L"__operator-x", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 1 || !args[0])
				return Undefined::instance();
			
			if (Int* i = dynamic_cast<Int*>(args[0]); i) 
				return new Int(-i->value());
			
			return Undefined::instance();
		}));
	IntProto->Object::put(L"__operator+x", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 1 || !args[0])
				return Undefined::instance();
			
			if (Int* i = dynamic_cast<Int*>(args[0]); i) 
				return i;
			
			return Undefined::instance();
		}));
	IntProto->Object::put(L"__operator++", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 1 || !args[0])
				return Undefined::instance();
			
			if (Int* i = dynamic_cast<Int*>(args[0]); i) 
				return new Int(i->value() + 1);
			
			return Undefined::instance();
		}));
	IntProto->Object::put(L"__operator--", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 1 || !args[0])
				return Undefined::instance();
			
			if (Int* i = dynamic_cast<Int*>(args[0]); i) 
				return new Int(i->value() - 1);
			
			return Undefined::instance();
		}));
	IntProto->Object::put(L"__operator!x", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 1 || !args[0])
				return Undefined::instance();
			
			if (Int* i = dynamic_cast<Int*>(args[0]); i) 
				return new Int(!i->value());
			
			return Undefined::instance();
		}));
	IntProto->Object::put(L"__operator~x", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 1 || !args[0])
				return Undefined::instance();
			
			if (Int* i = dynamic_cast<Int*>(args[0]); i) 
				return new Int(~i->value());
			
			return Undefined::instance();
		}));
	
	return IntProto;
};


Int::Int(int64_t value) : val(value) {};

Int::~Int() {};

// Delegate to prototype
vobject* Int::get(ck_vobject::vscope* scope, const std::wstring& name) {
	if (name == L"__proto")
		return IntProto;
	
	return IntProto ? IntProto->Object::get(scope, name) : nullptr;
};

void Int::put(ck_vobject::vscope* scope, const std::wstring& name, vobject* object) {
	throw UnsupportedOperation(L"Int is not container");
};

// Delegate to prototype
bool Int::contains(ck_vobject::vscope* scope, const std::wstring& name) {	
	if (name == L"__proto")
		return 1;
	
	return IntProto && IntProto->Object::contains(scope, name);
};

bool Int::remove(ck_vobject::vscope* scope, const std::wstring& name) {
	throw UnsupportedOperation(L"Int is not container");
};

vobject* Int::call(ck_vobject::vscope* scope, const std::vector<vobject*>& args) {
	throw UnsupportedOperation(L"Int is not callable");
};


int64_t Int::int_value() {
	return val;
};

wstring Int::string_value() {
	return std::to_wstring(val);
};
