#include "init_default.h"

#include <cstdlib>
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
#include "objects/Thread.h"

using namespace std;
using namespace ck_core;
using namespace ck_objects;
using namespace ck_vobject;

static vobject* f_print(vscope* scope, const vector<vobject*>& args) {
	for (int i = 0; i < args.size(); ++i)
		if (args[i] != nullptr)
			if (args[i]->as_type<Cake>())
				((Cake*) args[i])->print_backtrace();
			else {
				if (i == args.size() - 1)
					wcout << args[i]->string_value();
				else
					wcout << args[i]->string_value() << ' ';
			}
		else
			if (i == args.size() - 1)
				wcout << "null";
			else
				wcout << "null ";
		
	wcout.flush();
		
	return Undefined::instance();
};

static vobject* f_println(vscope* scope, const vector<vobject*>& args) {
	for (int i = 0; i < args.size(); ++i)
		if (args[i] != nullptr)
			if (args[i]->as_type<Cake>())
				((Cake*) args[i])->print_backtrace();
			else {
				if (i == args.size() - 1)
					wcout << args[i]->string_value();
				else
					wcout << args[i]->string_value() << ' ';
			}
		else
			if (i == args.size() - 1)
				wcout << "null";
			else
				wcout << "null ";
	
	wcout << endl;
		
	wcout.flush();
		
	return Undefined::instance();
};

static vobject* f_system(vscope* scope, const vector<vobject*>& args) {
	if (args.size() == 0)
		return Undefined::instance();
	
	std::wstring arg = args[0]->string_value();
	std::string  exe(arg.begin(), arg.end());
	
	int status = system(exe.c_str());
		
	return new Int(status);
};

static vobject* f_exit(vscope* scope, const vector<vobject*>& args) {
	GIL::instance()->stop();
	return Undefined::instance();
};

static vobject* f_gc_objects(vscope* scope, const vector<vobject*>& args) {
	return new Int(GIL::gc_instance()->count());
};


vscope* ck_objects::init_default() {
	
	vscope* scope = new iscope();
	
	
	// Define root prototypes in new root scope.
	//  By default these values are not restroyed till interpreter finishes it's work.
	
	scope->put(L"Object",           Object          ::create_proto());
	scope->put(L"NativeFunction",   NativeFunction  ::create_proto());
	scope->put(L"String",           String          ::create_proto());
	scope->put(L"Scope",            iscope          ::create_proto());
	scope->put(L"XScope",           xscope          ::create_proto());
	scope->put(L"Array",            Array           ::create_proto());
	scope->put(L"Int",              Int             ::create_proto());
	scope->put(L"Double",           Double          ::create_proto());
	scope->put(L"Bool",             Bool            ::create_proto());
	scope->put(L"Null",             Null            ::create_proto());
	scope->put(L"Undefined",        Undefined       ::create_proto());
	scope->put(L"BytecodeFunction", BytecodeFunction::create_proto());
	scope->put(L"Cake",             Cake            ::create_proto());
	scope->put(L"Thread",           Thread          ::create_proto());
	
	// Define other objects and fields
	scope->put(L"print",   new NativeFunction(f_print));
	scope->put(L"println", new NativeFunction(f_println));
	// Execute native interpreter script command
	scope->put(L"system",  new NativeFunction(f_system));
	scope->put(L"exit",    new NativeFunction(f_exit));
	
	return scope;
};