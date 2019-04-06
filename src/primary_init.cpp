#include "primary_init.h"

#include <iostream>

#include "GIL2.h"
#include "executer.h"
#include "exceptions.h"

#include "vscope.h"
#include "objects/Object.h"
#include "objects/Array.h"
#include "objects/String.h"
#include "objects/Int.h"
#include "objects/Double.h"
#include "objects/Bool.h"
#include "objects/Null.h"
#include "objects/Undefined.h"
#include "objects/BytecodeFunction.h"
#include "objects/Cake.h"
#include "objects/NativeFunction.h"

using namespace std;
using namespace ck_objects;
using namespace ck_vobject;

static vobject* f_print(vscope* scope, const vector<vobject*>& args) {
	for (int i = 0; i < args.size(); ++i)
		if (args[i] != nullptr)
			if (args[i]->is_typeof<Cake>())
				((Cake*) args[i])->print_backtrace();
			else
				wcout << args[i]->string_value() << ' ';
	//throw ck_exceptions::ck_message(wstring(L"ololo call to ") , ck_exceptions::ck_message_type::CK_TYPE_ERROR);
	return nullptr;
};


vscope* ck_objects::primary_init() {
	
	vscope* scope = new iscope();
	
	
	// Define root prototypes
	
	// XXX: Use call-wrapper-functions to initialize objects by call.
	
	scope->put(L"Object",           Object          ::create_proto());
	scope->put(L"Scope",            iscope          ::create_proto());
	scope->put(L"OScope",           xscope          ::create_proto());
	scope->put(L"String",           String          ::create_proto());
	scope->put(L"Array",            Array           ::create_proto());
	scope->put(L"Int",              Int             ::create_proto());
	scope->put(L"Double",           Double          ::create_proto());
	scope->put(L"Bool",             Bool            ::create_proto());
	scope->put(L"Null",             Null            ::create_proto());
	scope->put(L"Undefined",        Undefined       ::create_proto());
	scope->put(L"BytecodeFunction", BytecodeFunction::create_proto());
	scope->put(L"Cake",             Cake            ::create_proto());
	scope->put(L"NativeFunction",   NativeFunction  ::create_proto());
	
	// Define other objects and fields
	scope->put(L"print", new NativeFunction(f_print));
	
	return scope;
};