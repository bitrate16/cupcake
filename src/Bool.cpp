#include "objects/Bool.h"

#include <string>

#include "exceptions.h"
#include "GIL2.h"

#include "objects/Object.h"
#include "objects/Array.h"
#include "objects/Bool.h"
#include "objects/NativeFunction.h"
#include "objects/Undefined.h"
#include "objects/String.h"
#include "objects/Double.h"
#include "objects/Int.h"

using namespace std;
using namespace ck_exceptions;
using namespace ck_vobject;
using namespace ck_objects;
using namespace ck_core;


static vobject* call_handler(vscope* scope, const vector<vobject*>& args) {
	if (args.size() == 0)
		return new Bool(0);
	
	if (args[0]->is_typeof<String>())
		if (((String*) args[0])->value() == L"false")
			return new Bool(0);
		else
			return new Bool(1);
		
	return new Bool(args[0]->int_value());
};

vobject* Bool::create_proto() {
	if (BoolProto != nullptr)
		return BoolProto;
	
	BoolProto = new CallableObject(call_handler);
	GIL::gc_instance()->attach_root(BoolProto);
	
	BoolProto->Object::put(L"__typename", new String(L"Bool"));
	BoolProto->Object::put(L"parse", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (!args.size())
				return Undefined::instance();
			
			if (!args.size() || !args[0])
				return Undefined::instance();
			
			std::wstring str = args[0]->string_value();
			
			if (str == L"true")
				return Bool::True();
			if (str == L"false")
				return Bool::False();
			
			return Undefined::instance();
		}));
	
	// Operators
	BoolProto->Object::put(L"__operator==", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			return Bool::instance(args[0]->int_value() != 0 == args[1]->int_value() != 0);
		}));
	BoolProto->Object::put(L"__operator!=", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			return Bool::instance(args[0]->int_value() != 0 != args[1]->int_value() != 0);
		}));
	BoolProto->Object::put(L"__operator>", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			if (Bool* i = dynamic_cast<Bool*>(args[0]); i) 
				return Bool::instance(i->value() && !args[1]->int_value());
			return Undefined::instance();
		}));
	BoolProto->Object::put(L"__operator>=", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			if (Bool* i = dynamic_cast<Bool*>(args[0]); i) 
				return Bool::instance(i->value());
			return Undefined::instance();
		}));
	BoolProto->Object::put(L"__operator<", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			if (Bool* i = dynamic_cast<Bool*>(args[0]); i) 
				return Bool::instance(!i->value() && args[1]->int_value());
			return Undefined::instance();
		}));
	BoolProto->Object::put(L"__operator<=", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			if (Bool* i = dynamic_cast<Bool*>(args[0]); i) 
				return Bool::instance(!i->value());
			return Undefined::instance();
		}));
	
	BoolProto->Object::put(L"__operator+", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			if (Bool* i = dynamic_cast<Bool*>(args[0]); i) {
				if (String* s = dynamic_cast<String*>(args[1]); s) 
					return new String(i->string_value() + s->value());
				if (Double* d = dynamic_cast<Double*>(args[1]); d)
					return new Double(i->value() + d->value());
				return Bool::instance(i->value() + args[1]->int_value());
			}
			return Undefined::instance();
		}));
	BoolProto->Object::put(L"__operator-", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			if (Bool* i = dynamic_cast<Bool*>(args[0]); i) {
				if (Double* d = dynamic_cast<Double*>(args[1]); d)
					return new Double(i->value() - d->value());
				return new Int(i->value() - args[1]->int_value());
			}
			return Undefined::instance();
		}));
	BoolProto->Object::put(L"__operator*", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			if (Bool* i = dynamic_cast<Bool*>(args[0]); i) 
				return Bool::instance(i->value() && args[1]->int_value() != 0);
			return Undefined::instance();
		}));
		
	BoolProto->Object::put(L"__operator&", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			if (Bool* i = dynamic_cast<Bool*>(args[0]); i) {
				long long b = args[1]->int_value();
				return new Int(i->value() & b);
			}
			return Undefined::instance();
		}));
	BoolProto->Object::put(L"__operator|", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			if (Bool* i = dynamic_cast<Bool*>(args[0]); i) {
				long long b = args[1]->int_value();
				return new Int(i->value() | b);
			}
			return Undefined::instance();
		}));
	BoolProto->Object::put(L"__operator^", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			if (Bool* i = dynamic_cast<Bool*>(args[0]); i) {
				long long b = args[1]->int_value();
				return Bool::instance(i->value() ^ (b != 0));
			}
			return Undefined::instance();
		}));
	BoolProto->Object::put(L"__operator<<", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			if (Bool* i = dynamic_cast<Bool*>(args[0]); i) {
				long long b = args[1]->int_value();
				return new Int(i->value() << b);
			}
			return Undefined::instance();
		}));
	BoolProto->Object::put(L"__operator>>", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			if (Bool* i = dynamic_cast<Bool*>(args[0]); i) {
				long long b = args[1]->int_value();
				return new Int(i->value() >> b);
			}
			return Undefined::instance();
		}));
	BoolProto->Object::put(L"__operator>>>", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			if (Bool* i = dynamic_cast<Bool*>(args[0]); i) {
				unsigned long long a = i->value();
				unsigned long long b = args[1]->int_value();
				return new Int(a >> b);
			}
			return Undefined::instance();
		}));
	
	BoolProto->Object::put(L"__operator&&", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			if (Bool* i = dynamic_cast<Bool*>(args[0]); i) {
				long long a = i->value();
				long long b = args[1]->int_value();
				return Bool::instance(a && b);
			}
			return Undefined::instance();
		}));
	BoolProto->Object::put(L"__operator||", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			if (Bool* i = dynamic_cast<Bool*>(args[0]); i) {
				long long a = i->value();
				long long b = args[1]->int_value();
				return Bool::instance(a || b);
			}
			return Undefined::instance();
		}));
	
	BoolProto->Object::put(L"__operator-x", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 1 || !args[0])
				return Undefined::instance();
			
			if (Bool* i = dynamic_cast<Bool*>(args[0]); i) 
				return Bool::instance(!i->value());
			
			return Undefined::instance();
		}));
	BoolProto->Object::put(L"__operator+x", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 1 || !args[0])
				return Undefined::instance();
			
			if (Bool* i = dynamic_cast<Bool*>(args[0]); i) 
				return i;
			
			return Undefined::instance();
		}));
	BoolProto->Object::put(L"__operator++", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 1 || !args[0])
				return Undefined::instance();
			
			if (Bool* i = dynamic_cast<Bool*>(args[0]); i) 
				return Bool::instance(!i->value());
			
			return Undefined::instance();
		}));
	BoolProto->Object::put(L"__operator--", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 1 || !args[0])
				return Undefined::instance();
			
			if (Bool* i = dynamic_cast<Bool*>(args[0]); i) 
				return Bool::instance(!i->value());
			
			return Undefined::instance();
		}));
	BoolProto->Object::put(L"__operator!x", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 1 || !args[0])
				return Undefined::instance();
			
			if (Bool* i = dynamic_cast<Bool*>(args[0]); i) 
				return Bool::instance(!i->value());
			
			return Undefined::instance();
		}));
	BoolProto->Object::put(L"__operator~x", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 1 || !args[0])
				return Undefined::instance();
			
			if (Int* i = dynamic_cast<Int*>(args[0]); i) 
				return Bool::instance(!i->value());
			
			return Undefined::instance();
		}));
	
	
	return BoolProto;
};

Bool* Bool::True() {
	if (TrueInstance == nullptr) {
		TrueInstance = new Bool(1);
		GIL::gc_instance()->attach_root(TrueInstance);
	}
	return TrueInstance;
};

Bool* Bool::False() {
	if (FalseInstance == nullptr) {
		FalseInstance = new Bool(0);
		GIL::gc_instance()->attach_root(FalseInstance);
	}
	return FalseInstance;
};


Bool::Bool(bool value) : val(value) {};

Bool::~Bool() {};

// Delegate to prototype
vobject* Bool::get(ck_vobject::vscope* scope, const std::wstring& name) {
	if (name == L"__proto")
		return BoolProto;
	
	return BoolProto ? BoolProto->Object::get(name) : nullptr;
};

void Bool::put(ck_vobject::vscope* scope, const std::wstring& name, vobject* object) {
	throw UnsupportedOperation(L"Bool is not container");
};

// Delegate to prototype
bool Bool::contains(ck_vobject::vscope* scope, const std::wstring& name) {	
	if (name == L"__proto")
		return 1;
	
	return BoolProto && BoolProto->Object::contains(name);
};

bool Bool::remove(ck_vobject::vscope* scope, const std::wstring& name) {
	throw UnsupportedOperation(L"Bool is not container");
};

vobject* Bool::call(ck_vobject::vscope* scope, const std::vector<vobject*>& args) {
	throw UnsupportedOperation(L"Bool is not callable");
};


long long Bool::int_value() {
	return val;
};

wstring Bool::string_value() {
	return val ? L"true" : L"false";
};
