#include "objects/Cake.h"

#include <vector>
#include <map>
#include <string>
#include <utility>

#include "exceptions.h"
#include "GIL2.h"
#include "executer.h"
#include "script.h"

#include "objects/String.h"
#include "objects/Array.h"
#include "objects/Int.h"
#include "objects/Object.h"
#include "objects/NativeFunction.h"
#include "objects/Undefined.h"

using namespace std;
using namespace ck_exceptions;
using namespace ck_vobject;
using namespace ck_objects;
using namespace ck_core;


static vobject* call_handler(vscope* scope, const vector<vobject*>& args) {
	Cake* err = new Cake();
	
	if (args.size() == 0)
		return err;
	if (args.size() == 1 && args[0]) {
		err->Object::put(L"message", args[0]);
		err->set_message(args[0]->string_value());
	}
	if (args.size() > 1 && args[0] && args[1]) {
		err->Object::put(L"type", args[0]);
		err->Object::put(L"message", args[1]);
		err->set_type(args[0]->string_value());
		err->set_message(args[1]->string_value());
	}
	
	return err;
};

vobject* Cake::create_proto() {
	if (CakeProto != nullptr)
		return CakeProto;
	
	CakeProto = new CallableObject(call_handler);
	GIL::gc_instance()->attach_root(CakeProto);
	
	CakeProto->Object::put(L"__typename", new String(L"Cake"));	
	CakeProto->Object::put(L"printBacktrace", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			// Validate __this
			if (!scope) return Undefined::instance();
			vobject* __this = scope->get(L"__this", 1);
			if (!__this || !__this->is_typeof<Cake>())
				return Undefined::instance();
			
			static_cast<Cake*>(__this)->print_backtrace();
			return Undefined::instance();
		}));
	CakeProto->Object::put(L"getBacktrace", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			// Validate __this
			if (!scope) return Undefined::instance();
			vobject* __this = scope->get(L"__this", 1);
			if (!__this || !__this->is_typeof<Cake>())
				return Undefined::instance();
			
			Cake* c = static_cast<Cake*>(__this);
			Array* a = new Array();
			for (int i = 0; i < c->get_backtrace().size(); ++i) {
				Object* sf = new Object();
				sf->put(L"lineno", new Int(c->get_backtrace()[i].lineno));
				sf->put(L"filename", new String(c->get_backtrace()[i].filename));
				sf->put(L"function", new String(c->get_backtrace()[i].function));
				
				a->items().push_back(sf);
			}
			
			return a;
		}));
	CakeProto->Object::put(L"getMessage", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			// Validate __this
			if (!scope) return Undefined::instance();
			vobject* __this = scope->get(L"__this", 1);
			if (!__this || !__this->is_typeof<Cake>())
				return Undefined::instance();
			
			Cake* c = static_cast<Cake*>(__this);
			
			return new String(c->get_message());
		}));
	CakeProto->Object::put(L"getType", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			// Validate __this
			if (!scope) return Undefined::instance();
			vobject* __this = scope->get(L"__this", 1);
			if (!__this || !__this->is_typeof<Cake>())
				return Undefined::instance();
			
			Cake* c = static_cast<Cake*>(__this);
			
			return new String(c->get_type());
		}));
	
	
	return CakeProto;
};


void Cake::collect_backtrace() {
	if (GIL::executer_instance() == nullptr)
		return;
	
	for (int i = 0; i < GIL::executer_instance()->call_stack.size(); ++i) {
		backtrace.push_back(BacktraceFrame());
		
		int pointer = GIL::executer_instance()->call_stack[i].pointer;
		int script_id = GIL::executer_instance()->call_stack[i].script_id;
		int lineno = 0;
		if (script_id >= GIL::executer_instance()->scripts.size())
			continue;
		
		ck_script* script = GIL::executer_instance()->scripts[script_id];
		
		if (script->bytecode.lineno_table.size() == 0)
			lineno = -1;
		
		if (pointer >= script->bytecode.bytemap.size()) 
			lineno = script->bytecode.lineno_table.rbegin()[1];
		
		for (int i = 0; i < script->bytecode.lineno_table.size() - 2; i += 2) {
			if (pointer >= script->bytecode.lineno_table[i + 1] && pointer < script->bytecode.lineno_table[i + 3]) {
				lineno = script->bytecode.lineno_table[i];
				break;
			}
		}
		
		backtrace.back().lineno = lineno;
		backtrace.back().filename = script->filename;
		backtrace.back().function = GIL::executer_instance()->call_stack[i].name;
	}
	
	// If no calls still, extract information from closest script
	if (GIL::executer_instance()->call_stack.size() == 0)
		if (GIL::executer_instance()->window_stack.size() != 0) {
			if (GIL::executer_instance()->scripts.size() == 0)
				return;
			
			ck_script* script = GIL::executer_instance()->scripts.back();
		
			int lineno = GIL::executer_instance()->lineno();
			
			backtrace.push_back(BacktraceFrame());
			backtrace.back().lineno = lineno;
			backtrace.back().filename = script->filename;
		}
};

Cake::Cake(const wstring& type, const wstring& message) {
	if (CakeProto == nullptr)
		Cake::create_proto();
	
	collect_backtrace();
	
	Object::put(wstring(L"proto"), CakeProto);
	
	// Construct array of backtrace
	vector<vobject*> backtrace_array;
	for (int i = 0; i < backtrace.size(); ++i) {
		map<wstring, vobject*> obj;
		obj[L"lineno"] = new Int(backtrace[i].lineno);
		obj[L"filename"] = new String(backtrace[i].filename);
		obj[L"function"] = new String(backtrace[i].function);
		backtrace_array.push_back(new Object(obj));
	}
	Object::put(wstring(L"backtrace"), new Array(backtrace_array));
	
	if (type != L"")
		Object::put(wstring(L"type"), new String(type));
	if (message != L"")
		Object::put(wstring(L"message"), new String(message));
	
	this->type = type;
	this->message = message;
};

Cake::Cake(const cake& c) {	
	// Assign prototype
	Object::put(wstring(L"proto"), CakeProto);
	
	// Get the backtrace
	if (c.has_backtrace())
		backtrace = c.get_backtrace();
	else
		collect_backtrace();
	
	// Construct array of backtrace
	vector<vobject*> backtrace_array;
	for (int i = 0; i < backtrace.size(); ++i) {
		map<wstring, vobject*> obj;
		obj[L"lineno"] = new Int(backtrace[i].lineno);
		obj[L"filename"] = new String(backtrace[i].filename);
		obj[L"function"] = new String(backtrace[i].function);
		backtrace_array.push_back(new Object(obj));
	}
	Object::put(wstring(L"backtrace"), new Array(backtrace_array));

	// Determine type
	switch (c.get_type_id()) {
		case cake_type::CK_EMPTY: {
			type = L"EmptyCake";
			break;
		}
		
		case cake_type::CK_CAKE: {
			type = c.get_type();
			message = c.get_message();
			break;
		}
		
		case cake_type::CK_OBJECT: {
			message = c.get_object() == nullptr ? L"nullptr" : c.get_object()->string_value();
			break;
		}
		
		case cake_type::CK_NATIVE_EXCEPTION: {
			type = L"NativeException";
			string what = c.get_exception().what();
			message = wstring(what.begin(), what.end());
			break;
		}
		
		case cake_type::CK_UNKNOWN_EXCEPTION: {
			type = L"UnknownException";
			break;
		}
	}
	
	// Assign info
	Object::put(wstring(L"type"), new String(type));
	Object::put(wstring(L"message"), new String(message));
};

vobject* Cake::get(ck_vobject::vscope* scope, const std::wstring& name) {
	vobject* ret = Object::get(name);
	if (!ret && CakeProto)
		return CakeProto->Object::get(scope, name);
	return ret;
};

void Cake::put(ck_vobject::vscope* scope, const std::wstring& name, vobject* object) {
	Object::put(name, object);
};

bool Cake::contains(ck_vobject::vscope* scope, const std::wstring& name) {
	return Object::contains(name) || (CakeProto && CakeProto->Object::contains(scope, name));
};

bool Cake::remove(ck_vobject::vscope* scope, const std::wstring& name) {
	if (Object::remove(name))
		return 1;
	return 0;
};

vobject* Cake::call(ck_vobject::vscope* scope, const std::vector<vobject*>& args) {
	throw UnsupportedOperation(L"Cake is not callable");
};

void Cake::gc_mark() {
	Object::gc_mark();
};

void Cake::gc_finalize() {
	Object::gc_finalize();
};


void Cake::print_backtrace() {
	if (type == L"" && message != L"")
		wcerr << "Cake: " << message << endl;
	else if (type != L"" && message == L"")
		wcerr << type << endl;
	else if (type != L"" && message != L"")
		wcerr << type << ": " << message << endl;
	else
		wcerr << "Cake thrown" << endl;
	
	for (int i = 0; i < backtrace.size(); ++i) {
		wcerr << " at File <" << backtrace[i].filename << "> line " << backtrace[i].lineno;
		
		if (backtrace[i].function.size() != 0)
			wcerr << " from " << backtrace[i].function << "()";
		
		int amount = 0;
		while (i + amount + 1 < backtrace.size()) {
			if (backtrace[i].function == backtrace[i + amount + 1].function
				&&
				backtrace[i].lineno   == backtrace[i + amount + 1].lineno
				&&
				backtrace[i].filename == backtrace[i + amount + 1].filename)
				++amount;
			else
				break;
		}
		i += amount;
		
		if (amount) 
			wcerr << " + " << amount << " more" << endl;
		else
			wcerr << endl;
	}
};

long long Cake::int_value() {
	return (int) (intptr_t) this; 
};

wstring Cake::string_value() {
	if (message.size() == 0)
		return L"[Cake]";
	if (type == L"")
		return L"[Cake: " + message + L"]";
	else
		return L"[" + type + L": " + message + L"]";
};

