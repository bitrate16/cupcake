#include "primary_init.h"

#include <iostream>

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
#include "objects/Error.h"
#include "objects/NativeFunction.h"

using namespace std;
using namespace ck_objects;
using namespace ck_vobject;

static vobject* f_print(vscope* scope, const vector<vobject*>& args) {
	for (int i = 0; i < args.size(); ++i)
		if (args[i] != nullptr)
			wcout << args[i]->string_value() << ' ';
	
	return nullptr;
};


vscope* ck_objects::primary_init() {
	
	vscope* scope = new vscope();
	
	
	// Define root prototypes
	scope->put(L"Object",           Object          ::create_proto());
	scope->put(L"vscope",           vscope          ::create_proto());
	scope->put(L"String",           String          ::create_proto());
	scope->put(L"Array",            Array           ::create_proto());
	scope->put(L"Int",              Int             ::create_proto());
	scope->put(L"Double",           Double          ::create_proto());
	scope->put(L"Bool",             Bool            ::create_proto());
	scope->put(L"Null",             Null            ::create_proto());
	scope->put(L"Undefined",        Undefined       ::create_proto());
	scope->put(L"BytecodeFunction", BytecodeFunction::create_proto());
	scope->put(L"Error",            Error           ::create_proto());
	scope->put(L"NativeFunction",   NativeFunction  ::create_proto());
	
	// Define other objects and fields
	scope->put(L"print", new NativeFunction(f_print));
	
	return scope;
};