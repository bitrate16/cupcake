#include "objects/Error.h"

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

using namespace std;
using namespace ck_exceptions;
using namespace ck_vobject;
using namespace ck_objects;
using namespace ck_core;


static vobject* call_handler(vscope* scope, const vector<vobject*>& args) {
	Error* err = new Error();
	
	if (args.size() != 0)
		err->Object::put(L"message", args[0]);
	
	return err;
};

vobject* Error::create_proto() {
	if (ErrorProto != nullptr)
		return ErrorProto;
	
	ErrorProto = new CallablePrototype(call_handler);
	GIL::gc_instance()->attach_root(ErrorProto);
	
	// ...
	
	return ErrorProto;
};


void Error::collect_backtrace() {
	if (GIL::executer_instance() == nullptr)
		return;
	
	for (int i = 0; i < GIL::executer_instance()->call_stack.size(); ++i) {
		backtrace.push_back(Frame());
		
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
};

Error::Error() {
	if (ErrorProto == nullptr)
		Error::create_proto();
	
	collect_backtrace();
	
	Object::put(wstring(L"proto"), ErrorProto);
	
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
};

Error::Error(const wstring& str) : Error() {
	Object::put(wstring(L"message"), new String(str));
	
	message = str;
};

Error::Error(const ck_message& msg) : Error() {	
	switch (msg.get_type()) {
		case ck_message_type::CK_CMESSAGE: {
			string s = msg.get_message();
			message = wstring(s.begin(), s.end());
			break;
		}
		
		case ck_message_type::CK_WCMESSAGE: {
			message = msg.get_wmessage();
			break;
		}
		
		case ck_message_type::CK_STRING: {
			message = msg.get_string();
			break;
		}
		
		case ck_message_type::BAD_ALLOC: 
		case ck_message_type::BAD_ALLOC2: {
			message = L"bad alloc";
			break;
		}
		
		case ck_message_type::UNDEFINED_BEHAVIOUR: {
			message = L"unexpected exception catch(...)";
			break;
		}
		
		case ck_message_type::NATIVE_EXCEPTION: {
			string s = msg.get_exception().what();
			message = L"native exception: " + wstring(s.begin(), s.end());
			break;
		}
		
		case ck_message_type::CK_UNSUPPORTED_OPERATION: {
			message = L"unsupported operation: " + msg.get_string();
			break;
		}
		
		case ck_message_type::CK_RUNTIME_ERROR: {
			message = L"runtime error: " + msg.get_string();
			break;
		}
		
		case ck_message_type::CK_STACK_CORRUPTED: {
			message = L"stack corrupted error: " + msg.get_string();
			break;
		}
		
		case ck_message_type::CK_TYPE_ERROR: {
			message = L"type error: " + msg.get_string();
			break;
		}
		
		case ck_message_type::CK_INVALID_STATE: {
			message = L"invalid state error: " + msg.get_string();
			break;
		}
		
		case ck_message_type::CK_OBJECT: {
			message = msg.get_object() == nullptr ? wstring(L"null") : msg.get_object()->string_value();
			break;
		}
		
		case ck_message_type::CK_STACK_OVERFLOW: {
			message = L"stack overflow error: " + msg.get_string();
			break;
		}
		
		case ck_message_type::CK_EMPTY: {
			message = L"not an error";
			break;
		}
	}
	
	Object::put(wstring(L"message"), new String(message));
};

Error::~Error() {
	
};

vobject* Error::get(ck_vobject::vscope* scope, const std::wstring& name) {
	vobject* ret = Object::get(name);
	if (!ret && ErrorProto)
		return ErrorProto->get(scope, name);
	return ret;
};

void Error::put(ck_vobject::vscope* scope, const std::wstring& name, vobject* object) {
	Object::put(name, object);
};

bool Error::contains(ck_vobject::vscope* scope, const std::wstring& name) {
	return Object::contains(name) || (ErrorProto && ErrorProto->contains(scope, name));
};

bool Error::remove(ck_vobject::vscope* scope, const std::wstring& name) {
	if (Object::remove(name))
		return 1;
	if (ErrorProto && ErrorProto->remove(scope, name))
		return 1;
	return 0;
};

vobject* Error::call(ck_vobject::vscope* scope, const std::vector<vobject*> args) {
	throw ck_message(L"Error is not callable", ck_message_type::CK_UNSUPPORTED_OPERATION);
};

void Error::gc_mark() {
	Object::gc_mark();
};

void Error::gc_finalize() {
	Object::gc_finalize();
};


void Error::print_backtrace() {
	wcout << "Error: " << message << endl;
	for (int i = 0; i < backtrace.size(); ++i) {
		wcout << " at File <" << backtrace[i].filename << "> line " << backtrace[i].lineno;
		
		if (backtrace[i].function.size() != 0)
			wcout << " " << backtrace[i].function << "()";
		
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
			wcout << " + " << amount << " more" << endl;
		else
			wcout << endl;
	}
};

long long Error::int_value() {
	return (int) (intptr_t) this; 
};

wstring Error::string_value() {
	if (message.size() == 0)
		return L"[Error]";
	return L"[Error: " + message + L"]";
};

