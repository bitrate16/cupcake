#include "executer.h"

#include <vector>
#include <map>

#include "GIL2.h"
#include "vobject.h"
#include "script.h"
#include "translator.h"

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


using namespace std;
using namespace ck_core;
using namespace ck_vobject;
using namespace ck_objects;
using namespace ck_exceptions;


// C K _ E X E C U T E R _ G C _ O B J E C T

ck_executer_gc_object::ck_executer_gc_object(ck_executer* exec_instance) {
	this->exec_instance = exec_instance;
	GIL::gc_instance()->attach_root(this);
};

ck_executer_gc_object::~ck_executer_gc_object() {};

void ck_executer_gc_object::gc_mark() {
	if (!exec_instance)
		return;
	
	for (int i = 0; i < exec_instance->scopes.size(); ++i)
		if (exec_instance->scopes[i] && !exec_instance->scopes[i]->gc_reachable)
			exec_instance->scopes[i]->gc_mark();
	for (int i = 0; i < exec_instance->objects.size(); ++i)
		if (exec_instance->objects[i] && !exec_instance->objects[i]->gc_reachable)
			exec_instance->objects[i]->gc_mark();
};

void ck_executer_gc_object::gc_finalize() {};


// C K _ E X E C U T E R

ck_executer::ck_executer() {
	// Will be disposed by GC.
	gc_marker = new ck_executer_gc_object(this);
};

ck_executer::~ck_executer() {
	
};


bool ck_executer::read(int size, void* buf) {
	if (!scripts.back())
		return 0;
	
	if (pointer + size > scripts.back()->bytecode.bytemap.size())
		return 0;
	
	for (int i = 0; i < size; ++i)
		((unsigned char*) buf)[i] = scripts.back()->bytecode.bytemap[pointer++];
	
	return 1;
};

inline bool ck_executer::is_eof() {
	return !scripts.back() || pointer >= scripts.back()->bytecode.bytemap.size() || scripts.back()->bytecode.bytemap[pointer] == ck_bytecodes::HALT || pointer == -1;
};

ck_vobject::vobject* ck_executer::vpop() {
	if (objects.size() == 0)
		throw ck_message(L"objects stack corrupted", ck_message_type::CK_STACK_CORRUPTED);
	
	vobject* t = objects.back();
	objects.pop_back();
	
	return t;
};

ck_vobject::vobject* ck_executer::vpeek() {
	if (objects.size() == 0)
		throw ck_message(L"objects stack corrupted", ck_message_type::CK_STACK_CORRUPTED);
	
	return objects.back();
};

void ck_executer::vpush(ck_vobject::vobject* o) {
	objects.push_back(o);
};

void ck_executer::vswap() {
	if (objects.size() < 2)
		throw ck_message(L"objects stack corrupted", ck_message_type::CK_STACK_CORRUPTED);
	
	std::iter_swap(objects.end(), objects.end() - 1);
};

void ck_executer::vswap1() {
	if (objects.size() < 3)
		throw ck_message(L"objects stack corrupted", ck_message_type::CK_STACK_CORRUPTED);
	
	std::iter_swap(objects.end() - 1, objects.end() - 2);
};

void ck_executer::vswap2() {
	if (objects.size() < 4)
		throw ck_message(L"objects stack corrupted", ck_message_type::CK_STACK_CORRUPTED);
	
	std::iter_swap(objects.end() - 2, objects.end() - 3);
};

int ck_executer::lineno() {
	if (scripts.back()->bytecode.lineno_table.size() == 0)
		return -1;
	
	if (pointer >= scripts.back()->bytecode.bytemap.size()) 
		return scripts.back()->bytecode.lineno_table.rbegin()[1];
	
	for (int i = 0; i < scripts.back()->bytecode.lineno_table.size() - 2; i += 2) {
		if (i >= scripts.back()->bytecode.lineno_table[i + 1] && i < scripts.back()->bytecode.lineno_table[i + 3])
			return scripts.back()->bytecode.lineno_table[i];
	}
	
	return -1;
};
		
void ck_executer::store_frame(std::vector<stack_frame>& stack, int stack_id, const std::wstring& name, bool own_scope) {
	
	if (stack_id == call_stack_id && stack.size() == call_stack_limit)
		throw ck_message(L"call stack overflow", ck_message_type::CK_STACK_OVERFLOW);
	else if (stack_id == try_stack_id && stack.size() == try_stack_limit)
		throw ck_message(L"try stack overflow", ck_message_type::CK_STACK_OVERFLOW);
	else if (stack_id == window_stack_id && stack.size() == window_stack_limit)
		throw ck_message(L"window stack overflow", ck_message_type::CK_STACK_OVERFLOW);
		
	// Create stack_window and save executer state
	stack_frame frame;
	frame.window_id = window_stack.size() - 1;
	frame.try_id    = try_stack.size()    - 1;
	frame.call_id   = call_stack.size()   - 1;
	frame.script_id = scripts.size()      - 1;
	frame.scope_id  = scopes.size()       - 1;
	frame.object_id = objects.size()      - 1;
	frame.own_scope = own_scope;
	frame.name      = name;
	frame.pointer   = pointer;
	
	/*if (stack_id == call_stack_id)
		++frame.call_id;
	else if (stack_id == try_stack_id)
		++frame.try_id;
	else if (stack_id == window_stack_id)
		++frame.window_id;*/
	
	stack.push_back(frame);
};

void ck_executer::restore_frame(std::vector<stack_frame>& stack, int stack_id, int restored_frame_id) {
	
	/*if (stack_id == call_stack_id)
		wcout << "RESTORE CALL" << endl;
	else if (stack_id == try_stack_id)
		wcout << "RESTORE TRY" << endl;
	else if (stack_id == window_stack_id)
		wcout << "RESTORE WINDOW" << endl;
	*/
	;//wcout << "Check for needence: restored_frame_id = " << restored_frame_id << ", stack.size() - 1 = " << stack.size() - 1 << endl; 
	if (restored_frame_id > (int) stack.size() - 1)
		return;
	;//wcout << "-------------> call   stack id current " << (int)call_stack.size()-1 << endl;
	;//wcout << "-------------> try    stack id current " << (int)try_stack.size() -1<< endl;
	;//wcout << "-------------> window stack id current " << (int)window_stack.size()-1 << endl;
	;//wcout << "-------------> scripts      id current " << (int)scripts.size()-1 << endl;
	;//wcout << "-------------> scopes       id current " << (int)scopes.size()-1 << endl;
	
	if (stack_id == call_stack_id && stack.size() == 0)
		throw ck_message(L"call stack corrupted", ck_message_type::CK_STACK_CORRUPTED);
	else if (stack_id == try_stack_id && stack.size() == 0)
		throw ck_message(L"try stack corrupted", ck_message_type::CK_STACK_CORRUPTED);
	else if (stack_id == window_stack_id && stack.size() == 0)
		throw ck_message(L"window stack corrupted", ck_message_type::CK_STACK_CORRUPTED);
	
	int window_id = stack.back().window_id;
	int try_id    = stack.back().try_id;
	int call_id   = stack.back().call_id;
	int script_id = stack.back().script_id;;//wcout << "------------------------ SCRIPT " << script_id << ", val = " << scripts.size() << endl;
	int scope_id  = stack.back().scope_id;
	int object_id = stack.back().object_id;
	int pointer_v = stack.back().pointer;
	/*
	if (stack_id == call_stack_id)
		call_id = restored_frame_id;
	else if (stack_id == try_stack_id)
		try_id = restored_frame_id;
	else if (stack_id == window_stack_id)
		window_id = restored_frame_id;
	*/
	;//wcout << "-------------> call   stack id expected " << call_id   << endl;
	;//wcout << "-------------> try    stack id expected " << try_id    << endl;
	;//wcout << "-------------> window stack id expected " << window_id << endl;
	;//wcout << "-------------> scripts      id expected " << script_id << endl;
	;//wcout << "-------------> scopes       id expected " << scope_id << endl;
	
	// Restore windows
	for (int i = window_stack.size() - 1; i > window_id; --i) {
		// Pop scopes that are used by windows
		if (window_stack.back().scope_id >= 0 && window_stack.back().scope_id < scopes.size()) 
			if (scopes[window_stack.back().scope_id]) {
				if (window_stack.back().own_scope)
						scopes[window_stack.back().scope_id]->unroot();
				
				scopes[window_stack.back().scope_id] = nullptr;
			}
		
		window_stack.pop_back();
	}
	
	// Restore try frames
	// Logically, try does not own any scopes
	// Only catch block owns a scope, so it has to be popped out later as the rest.
	for (int i = try_stack.size() - 1; i > try_id; --i) 
		try_stack.pop_back();
	
	
	// Restore call frames
	for (int i = call_stack.size() - 1; i > call_id; --i) {
		// Pop scopes that are used by call frames
		if (call_stack.back().scope_id >= 0 && call_stack.back().scope_id < scopes.size()) 
			if (scopes[call_stack.back().scope_id]) {
				if (call_stack.back().own_scope)
						scopes[call_stack.back().scope_id]->unroot();
				
				scopes[call_stack.back().scope_id] = nullptr;
			}
		
		call_stack.pop_back();
	}
	
	// Restore scripts
	for (int i = scripts.size() - 1; i > script_id; --i) 
		scripts.pop_back();
	
	// Restore scopes
	for (int i = scopes.size() - 1; i > scope_id; --i) {		
		// Only catch blocks scopes are left. Remove then
		if (scopes[i])
			scopes[i]->unroot();
		
		scopes.pop_back();
	}
	
	// Restore objects stack
	for (int i = objects.size() - 1; i > object_id; --i) 
		objects.pop_back();
	
	// Restore address pointer
	pointer = pointer_v;
	
	//stack.pop_back();
	
	;//wcout << "-------------> AFTER" << endl;
	;//wcout << "-------------> call   stack id result " << (int)call_stack.size()-1 << endl;
	;//wcout << "-------------> try    stack id result " << (int)try_stack.size() -1<< endl;
	;//wcout << "-------------> window stack id result " << (int)window_stack.size()-1 << endl;
	;//wcout << "-------------> scripts      id result " << (int)scripts.size()-1 << endl;
	;//wcout << "-------------> scopes       id result " << (int)scopes.size()-1 << endl;
};

void ck_executer::follow_exception(const ck_message& msg) {
	if (try_stack.size() == 0)
		throw msg;	
	
	int type          = try_stack.back().try_type;
	int catch_address = try_stack.back().catch_node; ;//wcout << "catch_address = " << catch_address << endl;
	int window_id     = try_stack.back().window_id;
	int call_id       = try_stack.back().call_id;
	int try_id        = try_stack.back().try_id;
	wstring handler   = try_stack.back().name; ;//wcout << "HANDLER NAME GOING TO EXTECTED TO BE THIS: " << handler << endl;
	;//wcout << "---CURRENT---> call   stack id current  " << (int)call_stack.size()-1   << endl;
	;//wcout << "---CURRENT---> try    stack id current  " << (int)try_stack.size() -1    << endl;
	;//wcout << "---CURRENT---> win    stack id current  " << (int)window_stack.size() -1    << endl;
	;//wcout << "=============> call   stack id expected " << call_id   << endl;
	;//wcout << "=============> try    stack id expected " << try_id    << endl;
	;//wcout << "=============> win    stack id expected " << window_id    << endl;
	;//wcout << "===AAAAAAAAA=> call_stack_try id        " << call_stack.back().try_id    << endl;
	;//wcout << "====AAAAAA===> window_stack_try id      " << window_stack.back().try_id    << endl;
	;//wcout << "==BBBBBBBAAA=> call_stack_call id       " << call_stack.back().call_id    << endl;
	;//wcout << "====AABBBBB==> window_stack_call id     " << window_stack.back().call_id    << endl  << endl;
	;//wcout << "call: call_id = " << call_id << ", call_stack.size() - 1 = " << call_stack.size() - 1 << endl;
	// Check for valid call
	if (call_stack.size() != 0 && call_stack.back().try_id == try_stack.size()-1) { ;//wcout << "exit by call exit" << endl;
		;//wcout << "CALL ROLLBACK" << endl;
		restore_frame(call_stack, call_stack_id, call_id - 1);
		throw msg;
	}
	;//wcout << "window: window_id = " << window_id << ", window_stack.size() - 1 = " << window_stack.size() - 1 << endl;
	// Check for valid script
	if (window_stack.size() != 0 && window_stack.back().try_id == try_stack.size()-1) { ;//wcout << "exit by window exit" << endl;
		;//wcout << "WINDOW ROLLBACK" << endl;
		restore_frame(window_stack, window_stack_id, window_id - 1);
		throw msg;
	}
	
	// The default behaviour is to pop try_frame out when handling exception.
	// Normally try_trame should be popped by POP_TRY when try block finishes work without error.
	restore_frame(try_stack, try_stack_id, try_stack.size() - 1);
	;//wcout << "catch_address = " << catch_address << endl;
	
	switch(type) {
		case ck_bytecodes::TRY_NO_ARG:
		case ck_bytecodes::TRY_NO_CATCH:
			goto_address(catch_address); 
			break;
			
		case ck_bytecodes::TRY_WITH_ARG: {;//wcout << "HANDLER NAME EXPECTED TO BE THIS: " << handler << endl;
			vscope* scope = new vscope(scopes.size() == 0 ? nullptr : scopes.back());
			scope->root();
			scope->put(handler, msg.get_type() == ck_message_type::CK_OBJECT ? msg.get_object() : new Error(msg), 0, 1);
			scopes.push_back(scope);
			goto_address(catch_address); 
			break;
		}
	}
	;//wcout << "pointer = " << pointer << endl;
};


void ck_executer::validate_scope() {
	if (scopes.size() == 0 || scopes.back() == nullptr)
		throw ck_message(L"scopes stack corrupted", ck_message_type::CK_STACK_CORRUPTED);		
};

void ck_executer::exec_bytecode() {
	while (!is_eof()) {
		;//wcout << "[" << pointer << "] ";
		if (pointer == 67) {
			;//wcout << " - - - - - - - - - - - - - - call_size = " << call_stack.size() << endl;
			;//wcout << " - - - - - - - - - - - - - - scripts_size = " << scripts.size() << endl;
		}
		switch(scripts.back()->bytecode.bytemap[pointer++]) {
			case ck_bytecodes::LINENO: {
				int lineno; 
				read(sizeof(int), &lineno);
				;//wcout << "LINENO: " << lineno << endl;
				break;
			}
			
			case ck_bytecodes::NOP: {
				;//wcout << "> NOP" << endl;
				break;
			}
			
			case ck_bytecodes::PUSH_CONST_INT: {
				long long i; 
				read(sizeof(long long), &i);
				;//wcout << "> PUSH_CONST[int]: " << i << endl;
				vpush(new Int(i));
				break;
			}
			
			case ck_bytecodes::PUSH_CONST_DOUBLE: {
				double i; 
				read(sizeof(double), &i);
				;//wcout << "> PUSH_CONST[double]: " << i << endl;
				vpush(new Double(i));
				break;
			}
			
			case ck_bytecodes::PUSH_CONST_BOOLEAN: {
				bool i; 
				read(sizeof(bool), &i);
				;//wcout << "> PUSH_CONST[boolean]: " << i << endl;
				vpush(new Bool(i));
				break;
			}
			
			case ck_bytecodes::PUSH_CONST_NULL: {
				;//wcout << "> PUSH_CONST: null" << endl;
				vpush(Null::instance());
				break;
			}
			
			case ck_bytecodes::PUSH_CONST_UNDEFINED: {
				;//wcout << "> PUSH_CONST: undefined" << endl;
				vpush(Undefined::instance());
				break;
			}
			
			case ck_bytecodes::PUSH_CONST_STRING: {
				int size;
				read(sizeof(int), &size);
				wchar_t cstr[size+1];
				read(sizeof(wchar_t) * size, cstr);
				cstr[size] = 0;
				;//wcout << "> PUSH_CONST[string]: \"" << cstr << '"' << endl;
				
				vpush(new String(cstr));
				break;
			}
			
			case ck_bytecodes::LOAD_VAR: {
				int size;
				read(sizeof(int), &size);
				wchar_t cstr[size+1];
				read(sizeof(wchar_t) * size, cstr);
				cstr[size] = 0;
				;//wcout << "> LOAD_VAR: " << cstr << endl;
				
				// Check for valid scope
				validate_scope();
				
				// Scope should return nullptr if value does not exist.
				vobject* o = scopes.back()->get(cstr, 1);
				if (o == nullptr)
					throw ck_message(wstring(L"undefined reference to ") + cstr, ck_message_type::CK_TYPE_ERROR);
				
				vpush(o);
				break;
			}
			
			case ck_bytecodes::VSTACK_POP: {
				;//wcout << "> VSTACK_POP" << endl;
				vpop();
				break;
			}
			
			case ck_bytecodes::PUSH_CONST_ARRAY: {
				int size; 
				read(sizeof(int), &size);
				
				;//wcout << "> PUSH_CONST[array]: [" << size << ']' << endl;
				
				vector<vobject*> array;
				for (int i = 0; i < size; ++i)
					array.push_back(vpop());
				vpush(new Array(array));
				break;
			}
			
			case ck_bytecodes::PUSH_CONST_OBJECT: {
				int size; 
				read(sizeof(int), &size);
				;//wcout << "> PUSH_CONST[object]: {";
				
				map<wstring, vobject*> objects;
				
				for (int i = 0; i < size; ++i) {
					int ssize;
					read(sizeof(int), &ssize);
					wchar_t cstr[ssize+1];
					read(sizeof(wchar_t) * ssize, cstr);
					cstr[ssize] = 0;
					
					objects[cstr] = vpop();
					
					;//wcout << cstr;
					if (i != size-1)
						;//wcout << ", ";
				}
				
				;//wcout << '}' << endl;
				
				vpush(new Object(objects));
				break;
			}
			
			case ck_bytecodes::DEFINE_VAR: {
				int amount; 
				read(sizeof(int), &amount);
				;//wcout << "> DEFINE_VAR: ";
				
				for (int i = 0; i < amount; ++i) {
					int ssize = 0;
					read(sizeof(int), &ssize);
					
					wchar_t cstr[ssize+1];
					read(sizeof(wchar_t) * ssize, cstr);
					cstr[ssize] = 0;
					;//wcout << cstr;
					
					unsigned char ops = 0;
					read(sizeof(unsigned char), &ops);
					
					// Check for valid scope
					validate_scope();			
					
					if ((ops & 0b1000) == 0) {
						scopes.back()->put(cstr, Undefined::instance(), 0, 1);
						;//wcout << " = [undefined]";
					} else
						scopes.back()->put(cstr, vpop(), 0, 1);
					
					if (i != amount-1)
						;//wcout << ", ";
				}
				
				;//wcout << endl;
				break;
			}
		
			case ck_bytecodes::VSTACK_DUP: {
				;//wcout << "> VSTACK_DUP" << endl;
				vpush(vpeek());
				break;
			}
			
			case ck_bytecodes::CALL: {
				// bytecode: CALL [argc]
				// stack: argN..arg0 fun
				
				int argc; 
				read(sizeof(int), &argc);
				
				;//wcout << "> CALL [" << argc << ']' << endl;
				
				if (objects.size() < argc + 1)
					throw ck_message(L"objects stack corrupted", ck_message_type::CK_INVALID_STATE); 
				
				// Copy args
				vector<vobject*> args;
				for (int k = 0; k < argc; ++k)
					args.push_back(objects.rbegin()[k + 1]);
				
				vobject* obj = call_object(objects.rbegin()[0], nullptr, args, L"");
				for (int k = 0; k < argc + 1; ++k)
					objects.pop_back();
				vpush(obj);
				
				break;
			}
			
			case ck_bytecodes::CALL_FIELD: {
				// bytecode: CALL [argc] [name]
				// stack: argN..arg0 ref
				
				int argc; 
				read(sizeof(int), &argc);
				
				int size;
				read(sizeof(int), &size);
				wchar_t cstr[size+1];
				read(sizeof(wchar_t) * size, cstr);
				cstr[size] = 0;
				
				;//wcout << "> CALL_FIELD [" << argc << "] [" << cstr << ']' << endl;
				
				if (objects.size() < argc + 1)
					throw ck_message(L"objects stack corrupted", ck_message_type::CK_INVALID_STATE); 
				
				if (objects.rbegin()[0] == nullptr)
					throw ck_message(wstring(L"undefined reference to ") + cstr, ck_message_type::CK_TYPE_ERROR);
					
				validate_scope();
					
				vpush(objects.rbegin()[0]->get(scopes.back(), cstr));
				// stack: argN..arg0 ref fun
				
				// Copy args
				vector<vobject*> args;
				for (int k = 0; k < argc; ++k)
					args.push_back(objects.rbegin()[k + 2]);
				
				vobject* obj = call_object(objects.rbegin()[0], objects.rbegin()[1], args, cstr);
				for (int k = 0; k < argc + 2; ++k)
					objects.pop_back();
				vpush(obj);
				
				break;
			}
			
			case ck_bytecodes::CALL_NAME: {
				// bytecode: CALL [argc] [name]
				// stack: argN..arg0
				
				int argc; 
				read(sizeof(int), &argc);
				
				int size;
				read(sizeof(int), &size);
				wchar_t cstr[size+1];
				read(sizeof(wchar_t) * size, cstr);
				cstr[size] = 0;
				
				;//wcout << "> CALL_NAME [" << argc << "] [" << cstr << ']' << endl;
				
				if (objects.size() < argc)
					throw ck_message(L"objects stack corrupted", ck_message_type::CK_INVALID_STATE); 
					
				validate_scope();
					
				vpush(scopes.back()->get(scopes.back(), cstr));
				// stack: argN..arg0 fun
				
				// Copy args
				vector<vobject*> args;
				for (int k = 0; k < argc; ++k)
					args.push_back(objects.rbegin()[k + 1]);
				
				vobject* obj = call_object(objects.rbegin()[0], nullptr, args, cstr);
				for (int k = 0; k < argc + 1; ++k)
					objects.pop_back();
				vpush(obj);
				;//wcout << "scipts_size = " << scripts.size() << endl;
				break;
			}
			
			case ck_bytecodes::CALL_MEMBER: {
				// bytecode: CALL [argc] [name]
				// stack: argN..arg0 ref key
				
				int argc; 
				read(sizeof(int), &argc);
				
				if (objects.size() < argc + 2)
					throw ck_message(L"objects stack corrupted", ck_message_type::CK_INVALID_STATE); 
				
				if (objects.rbegin()[0] == nullptr)
					throw ck_message(wstring(L"undefined reference to member"), ck_message_type::CK_TYPE_ERROR);
				
				wstring key = objects.rbegin()[0]->string_value();
				
				;//wcout << "> CALL_MEMBER [" << argc << "] [" << key << ']' << endl;
				
				if (objects.rbegin()[1] == nullptr)
					throw ck_message(wstring(L"undefined reference to ") + key, ck_message_type::CK_TYPE_ERROR);
					
				validate_scope();
					
				vpush(objects.rbegin()[1]->get(scopes.back(), key));
				// stack: argN..arg0 ref key fun
				
				// Copy args
				vector<vobject*> args;
				for (int k = 0; k < argc; ++k)
					args.push_back(objects.rbegin()[k + 3]);
				
				vobject* obj = call_object(objects.rbegin()[0], objects.rbegin()[2], args, L"[" + key + L"]");
				for (int k = 0; k < argc + 3; ++k)
					objects.pop_back();
				vpush(obj);
				
				break;
			}
			
			case ck_bytecodes::OPERATOR: {
				unsigned char i; 
				read(sizeof(unsigned char), &i);
				
				validate_scope();
				
				if (objects.size() < 2)
					throw ck_message(L"objects stack corrupted", ck_message_type::CK_STACK_CORRUPTED);
				
				vobject *ref = objects.rbegin()[1];
				vobject *fun = nullptr;
				
				if (ref == nullptr)
					throw ck_message(wstring(L"undefined reference to operator"), ck_message_type::CK_TYPE_ERROR);
				
				wstring fun_name;
				
				// lvalue operator
				switch (i) {
					case ck_bytecodes::OPT_ADD    : fun_name = (L"+"); break;
					case ck_bytecodes::OPT_SUB    : fun_name = (L"-"); break;
					case ck_bytecodes::OPT_MUL    : fun_name = (L"*"); break;
					case ck_bytecodes::OPT_DIV    : fun_name = (L"/"); break;
					case ck_bytecodes::OPT_BITRSH : fun_name = (L">>"); break;
					case ck_bytecodes::OPT_BITLSH : fun_name = (L"<<"); break;
					case ck_bytecodes::OPT_BITURSH: fun_name = (L">>>"); break;
					case ck_bytecodes::OPT_DIR    : fun_name = (L"\\"); break;
					case ck_bytecodes::OPT_PATH   : fun_name = (L"\\\\"); break;
					case ck_bytecodes::OPT_MOD    : fun_name = (L"%"); break;
					case ck_bytecodes::OPT_BITOR  : fun_name = (L"|"); break;
					case ck_bytecodes::OPT_BITAND : fun_name = (L"&"); break;
					case ck_bytecodes::OPT_HASH   : fun_name = (L"#"); break;
					case ck_bytecodes::OPT_EQ     : fun_name = (L"=="); break;
					case ck_bytecodes::OPT_NEQ    : fun_name = (L"!="); break;
					case ck_bytecodes::OPT_OR     : fun_name = (L"||"); break;
					case ck_bytecodes::OPT_AND    : fun_name = (L"&&"); break;
					case ck_bytecodes::OPT_GT     : fun_name = (L">"); break;
					case ck_bytecodes::OPT_GE     : fun_name = (L">="); break;
					case ck_bytecodes::OPT_LT     : fun_name = (L"<"); break;
					case ck_bytecodes::OPT_LE     : fun_name = (L"<="); break;
					case ck_bytecodes::OPT_PUSH   : fun_name = (L"=>"); break;
					case ck_bytecodes::OPT_BITXOR : fun_name = (L"^"); break;
				}
				
				;//wcout << "> OPERATOR [" << fun_name << ']' << endl;
				
				fun = ref->get(scopes.back(), L"__operator" + fun_name);
				
				// rvalue operator
				if (fun == nullptr || fun->is_typeof<Undefined>() || fun->is_typeof<Null>()) 
					fun = ref->get(scopes.back(), L"__roperator" + fun_name);
				else
					fun_name = L"__operator" + fun_name;
				
				fun = ref->get(scopes.back(), L"__operator" + fun_name);				
					
				// no operator
				if (fun == nullptr || fun->is_typeof<Undefined>() || fun->is_typeof<Null>()) 
					throw ck_message(wstring(L"undefined reference to operator ") + fun_name, ck_message_type::CK_TYPE_ERROR);
				else
					fun_name = L"__roperator" + fun_name;
				
				vobject* res = call_object(fun, ref, { ref, objects.rbegin()[0] }, fun_name);
				
				vpop(); 
				vpop();
				vpush(res);
				
				break;
			}
			
			case ck_bytecodes::STORE_VAR: {
				int size;
				read(sizeof(int), &size);
				wchar_t cstr[size+1];
				read(sizeof(wchar_t) * size, cstr);
				cstr[size] = 0;
				
				// Check for scope
				validate_scope();		
				
				scopes.back()->put(cstr, vpop(), 0, 1);
				
				;//wcout << "> STORE_VAR: " << cstr << endl;
				break;
			}
			
			case ck_bytecodes::STORE_FIELD: {
				int size;
				read(sizeof(int), &size);
				wchar_t cstr[size+1];
				read(sizeof(wchar_t) * size, cstr);
				cstr[size] = 0;
				
				vobject* val = vpop();
				vobject* ref = vpop();
				
				if (ref == nullptr)
					throw ck_message(wstring(L"undefined reference to ") + cstr, ck_message_type::CK_TYPE_ERROR);
				
				// Check for scope
				validate_scope();		
				
				ref->put(scopes.back(), cstr, val);
				
				;//wcout << "> STORE_FIELD: " << cstr << endl;
				break;
			}
			
			case ck_bytecodes::STORE_MEMBER: {
				vobject* val = vpop();
				vobject* key = vpop();
				vobject* ref = vpop();
				
				if (ref == nullptr)
					throw ck_message(wstring(L"undefined reference"), ck_message_type::CK_TYPE_ERROR);
				
				if (key == nullptr)
					throw ck_message(wstring(L"undefined reference to member"), ck_message_type::CK_TYPE_ERROR);
				
				// Check for scope
				validate_scope();		
				
				ref->put(scopes.back(), key->string_value(), val);
				
				;//wcout << "> STORE_MEMBER " << endl;
				break;
			}
			
			case ck_bytecodes::LOAD_FIELD: {
				int size;
				read(sizeof(int), &size);
				wchar_t cstr[size+1];
				read(sizeof(wchar_t) * size, cstr);
				cstr[size] = 0;
				
				vobject* ref = vpop();
				
				if (ref == nullptr)
					throw ck_message(wstring(L"undefined reference to ") + cstr, ck_message_type::CK_TYPE_ERROR);
				
				// Check for scope
				validate_scope();		
				
				vpush(ref->get(scopes.back(), cstr));
				
				;//wcout << "> LOAD_FIELD: " << cstr << endl;
				break;
			}
			
			case ck_bytecodes::LOAD_MEMBER: {
				vobject* key = vpop();
				vobject* ref = vpop();
				
				if (ref == nullptr)
					throw ck_message(wstring(L"undefined reference"), ck_message_type::CK_TYPE_ERROR);
				
				if (key == nullptr)
					throw ck_message(wstring(L"undefined reference to member"), ck_message_type::CK_TYPE_ERROR);
				
				// Check for scope
				validate_scope();		
				
				vpush(ref->get(scopes.back(), key->string_value()));
				
				;//wcout << "> LOAD_MEMBER " << endl;
				break;
			}
		
			case ck_bytecodes::UNARY_OPERATOR: {				
				unsigned char i; 
				read(sizeof(unsigned char), &i);
				
				validate_scope();
				
				if (objects.size() == 0)
					throw ck_message(L"objects stack corrupted", ck_message_type::CK_STACK_CORRUPTED);
				
				vobject *ref = objects.rbegin()[0];
				vobject *fun = nullptr;
				
				if (ref == nullptr)
					throw ck_message(wstring(L"undefined reference to operator"), ck_message_type::CK_TYPE_ERROR);
				
				wstring fun_name;
				
				// lvalue operator
				switch (i) {
					case ck_bytecodes::OPT_DOG   : fun_name = (L"@x"); break;
					case ck_bytecodes::OPT_NOT   : fun_name = (L"!x"); break;
					case ck_bytecodes::OPT_BITNOT: fun_name = (L"~x"); break;
					case ck_bytecodes::OPT_POS   : fun_name = (L"+x"); break;
					case ck_bytecodes::OPT_NEG   : fun_name = (L"-x"); break;
					case ck_bytecodes::OPT_INC   : fun_name = (L"++x"); break;
					case ck_bytecodes::OPT_DEC   : fun_name = (L"--x"); break;
				}
				
				;//wcout << "> OPERATOR [" << fun_name << ']' << endl;
				
				fun = ref->get(scopes.back(), L"__operator" + fun_name);
				
				// rvalue operator
				if (fun == nullptr || fun->is_typeof<Undefined>() || fun->is_typeof<Null>()) 
					fun = ref->get(scopes.back(), L"__roperator" + fun_name);
				else
					fun_name = L"__operator" + fun_name;
				
				fun = ref->get(scopes.back(), L"__operator" + fun_name);				
					
				// no operator
				if (fun == nullptr || fun->is_typeof<Undefined>() || fun->is_typeof<Null>()) 
					throw ck_message(wstring(L"undefined reference to operator ") + fun_name, ck_message_type::CK_TYPE_ERROR);
				else
					fun_name = L"__roperator" + fun_name;
				
				vobject* res = call_object(fun, ref, { ref }, fun_name);
				
				vpop(); 
				vpush(res);
				
				break;
			}
			
			case ck_bytecodes::VSTACK_SWAP: {
				;//wcout << "> VSTACK_SWAP" << endl;
				vswap();
				break;
			}
			
			case ck_bytecodes::VSTACK_SWAP1: {
				;//wcout << "> VSTACK_SWAP1" << endl;
				vswap1();
				break;
			}
			
			case ck_bytecodes::VSTACK_SWAP2: {
				;//wcout << "> VSTACK_SWAP2" << endl;
				vswap2();
				break;
			}
			
			case ck_bytecodes::VSTATE_PUSH_SCOPE: {
				;//wcout << "> VSTATE_PUSH_SCOPE" << endl;
				vscope* s = new vscope(scopes.size() == 0 ? nullptr : scopes.back());
				GIL::gc_instance()->attach_root(s);
				scopes.push_back(s);
				break;
			}
			
			case ck_bytecodes::VSTATE_POP_SCOPE: {
				;//wcout << "> VSTATE_POP_SCOPE" << endl;
				
				// Check for scope
				validate_scope();	
				
				vscope* s = scopes.back();
				GIL::gc_instance()->deattach_root(s);
				scopes.pop_back();
				break;
			}
			
			case ck_bytecodes::JMP_IF_ZERO: {
				int i; 
				read(sizeof(int), &i);
				
				;//wcout << "> JMP_IF_ZERO [" << i << ']' << endl;
				
				vobject* o = vpop();
				if (o == nullptr || o->int_value() == 0)
					goto_address(i);
				break;
			}
			
			case ck_bytecodes::JMP: {
				int i; 
				read(sizeof(int), &i);
				
				;//wcout << "> JMP [" << i << ']' << endl;
				
				goto_address(i);
				break;
			}
			
			case ck_bytecodes::HALT: {
				;//wcout << "> HALT" << endl;
				
				--pointer;
				return;
				
				break;
			}
			
			case ck_bytecodes::THROW_NOARG: {
				;//wcout << "> THROW_NOARG" << endl;
				
				throw ck_message((vobject*) nullptr);
				
				break;
			}
			
			case ck_bytecodes::THROW: {				
				;//wcout << "> THROW" << endl;
				
				throw ck_message(vpop());
				
				break;
			}
			
			case ck_bytecodes::THROW_STRING: {
				int size;
				read(sizeof(int), &size);
				wchar_t cstr[size+1];
				read(sizeof(wchar_t) * size, cstr);
				cstr[size] = 0;
				
				;//wcout << "> THROW_STRING: \"" << cstr << '"' << endl;
				
				throw ck_message(new String(cstr));
				
				break;
			}
			
			case ck_bytecodes::VSTATE_POP_SCOPES: {
				int i; 
				read(sizeof(int), &i);
				;//wcout << "> VSTATE_POP_SCOPES [" << i << ']' << endl;
				
				if (scopes.size() < i)
					throw ck_message(L"scopes stack corrupted", ck_message_type::CK_STACK_CORRUPTED);
				
				for (int k = 0; k < i; ++k)
					scopes.pop_back();
				
				break;
			}
			
			case ck_bytecodes::RETURN_VALUE: {
				;//wcout << "> RETURN_VALUE" << endl;
				
				throw ck_message(vpop(), ck_message_type::CK_RETURN);
				
				break;
			}
			
			case ck_bytecodes::PUSH_CONST_FUNCTION: {
				
				// Check for scope
				validate_scope();		
				
				int argc; 
				read(sizeof(int), &argc);
				;//wcout << "> PUSH_CONST_FUNCTION (" << argc << ") (";
				
				vector<wstring> argn;
				
				for (int i = 0; i < argc; ++i) {
					int ssize = 0;
					read(sizeof(int), &ssize);
					
					wchar_t cstr[ssize+1];
					read(sizeof(wchar_t) * ssize, cstr);
					cstr[ssize] = 0;
					;//wcout << cstr;
					
					argn.push_back(cstr);
					
					if (i != argc-1)
						;//wcout << ", ";
				}
				
				int sizeof_block; 
				read(sizeof(int), &sizeof_block);
				
				;//wcout << ") [" << sizeof_block << "]" << endl;
				
				ck_script* script = new ck_script();
				script->directory = scripts.back()->directory;
				script->filename  = scripts.back()->filename;
				
				// Copy range of bytecodes
				
				script->bytecode.bytemap = vector<unsigned char>(scripts.back()->bytecode.bytemap.begin() + pointer, scripts.back()->bytecode.bytemap.begin() + pointer + sizeof_block);
				
				// int lineno_a = lineno();
				// int lineno_b = lineno();
				
				// Copy range of lineno table
				
				for (int i = 0; i < scripts.back()->bytecode.lineno_table.size();) {
					int lineno = scripts.back()->bytecode.lineno_table[i++];
					int byteof = scripts.back()->bytecode.lineno_table[i++];
					
					if (pointer >= byteof || pointer + sizeof_block >= byteof) {
						script->bytecode.lineno_table.push_back(lineno);
						script->bytecode.lineno_table.push_back((byteof - pointer) < 0 ? 0 : byteof - pointer); // Offset roll back
					}
				}
				
				// Append last marker
				script->bytecode.lineno_table.push_back(-1);
				script->bytecode.lineno_table.push_back(sizeof_block);
				
				/*
				;//wcout << "Function Bytecode: " << endl;
				ck_translator::print(script->bytecode.bytemap);
				;//wcout << endl;
				
				;//wcout << "Function Lineno Table: " << endl;
				ck_translator::print_lineno_table(script->bytecode.lineno_table);
				;//wcout << endl;
				*/				
				
				pointer += sizeof_block;
				
				vpush(new BytecodeFunction(scopes.back(), script, argn));
				
				break;
			}
		
			case ck_bytecodes::VSTATE_POP_TRY: {
				if (try_stack.size() == 0)
					throw ck_message(L"try stack corrupted", ck_message_type::CK_STACK_CORRUPTED);
				
				// No explicit restoration. 
				// Everything expected to be fine.
				// Simply pop the frame
				int pointer_tmp = pointer;
				restore_frame(try_stack, try_stack_id, try_stack.size() - 1);
				pointer = pointer_tmp;
				
				;//wcout << "> VSTATE_POP_TRY" << endl;
				break;
			}

			case ck_bytecodes::VSTATE_PUSH_TRY: {	
		
				if (try_stack.size() == try_stack_limit)
					throw ck_message(L"try-catch stack overflow", ck_message_type::CK_STACK_OVERFLOW);
				
				int try_node = 0;
				int catch_node = 0;
				wstring handler_name;
				unsigned char type;
				read(sizeof(unsigned char), &type);
				
				if (type == ck_bytecodes::TRY_NO_CATCH) {					
					read(sizeof(int), &try_node);
					read(sizeof(int), &catch_node);
					
					;//wcout << "> VSTATE_PUSH_TRY [TRY_NO_CATCH] [" << try_node << "] [" << catch_node << ']' << endl;
				} else if (type == ck_bytecodes::TRY_NO_ARG) {			
					read(sizeof(int), &try_node);
					read(sizeof(int), &catch_node);
					
					;//wcout << "> VSTATE_PUSH_TRY [TRY_NO_ARG] [" << try_node << "] [" << catch_node << ']' << endl;
				} else {
					int name_size;
					
					read(sizeof(int), &try_node);
					read(sizeof(int), &catch_node);
					read(sizeof(int), &name_size);
										
					wchar_t cstr[name_size+1];
					read(sizeof(wchar_t) * name_size, cstr);
					cstr[name_size] = 0;
					
					handler_name = cstr;
					
					;//wcout << "> VSTATE_PUSH_TRY [TRY_WITH_ARG] (" << cstr << ") [" << try_node << "] [" << catch_node << ']' << endl;
				}
				;//wcout << "TRY AHNDLER NAME -----------------> " << handler_name << endl;
				store_frame(try_stack, try_stack_id, handler_name, 0);
				try_stack.back().try_type = type;
				try_stack.back().catch_node = catch_node;
				
				break;
			}
			
			default: throw ck_message(L"invalid bytecode [" + to_wstring(scripts.back()->bytecode.bytemap[pointer-1]) + L"]", ck_message_type::CK_INVALID_STATE);
		}
		
		// Respond to GIL requests
		GIL::instance()->accept_lock();
		
		// Perform GC collection ?
		GIL::gc_instance()->collect();
	}
};


void ck_executer::execute(ck_core::ck_script* scr, ck_vobject::vscope* scope, std::vector<std::wstring>* argn, std::vector<ck_vobject::vobject*>* argv) {
	
	if (window_stack.size() == window_stack_limit)
		throw ck_message(L"executer stack overflow", ck_message_type::CK_STACK_OVERFLOW);
	
	// Apply new scope
	if (scope == nullptr) {
		scope = new vscope();
		scope->root();
	}
		
	if (argn != nullptr && argv != nullptr) {
		int argc = argn->size() < argv->size() ? argn->size() : argv->size();
		for (int i = 0; i < argc; ++i)
			scope->put((*argn)[i], (*argv)[i]);
	}
	
	// Push scope
	scopes.push_back(scope);
	
	// Push call frame and mark own scope
	store_frame(window_stack, window_stack_id, L"", 1);
	
	// Save expected call id
	int window_id = window_stack.size() - 1;
	
	// Push script instance to the bottom
	scripts.push_back(scr);
	
	// Reset pointer to 0 and start
	pointer = 0;
	
	// Do some useless shit again
	while (1) {
		try {;//wcout << "scopes.size() = " << scopes.size() << endl;
		;//wcout << "pointer = " << pointer << endl;
			exec_bytecode();
			
			// Reached bytecode end
			break;
			
		} catch(const ck_exceptions::ck_message& msg) { //;wcout << "HANDLE_EXCEPTION_EXECUTER" << endl;
			// Aft
			;//wcout << "HANDLED_EXCEPTION_EXECUTER" << endl;
			if (msg.get_type() == ck_message_type::CK_OBJECT)
				follow_exception(msg);
			else
				follow_exception(new Error(msg));
		} catch (const std::exception& ex) {
			follow_exception(new Error(ck_message(ex)));
		} catch (...) {
			follow_exception(new Error(ck_message(ck_exceptions::ck_message_type::NATIVE_EXCEPTION)));
		} 
	}
	
	// Try to restore scope
	restore_frame(window_stack, window_stack_id, window_id);
	
	return;
};

ck_vobject::vobject* ck_executer::call_object(ck_vobject::vobject* obj, ck_vobject::vobject* ref, const std::vector<ck_vobject::vobject*>& args, const std::wstring& name) { 

	if (obj == nullptr)
		throw ck_message(wstring(L"undefined call to ") + name, ck_message_type::CK_TYPE_ERROR);
	
	// Construct scope
	vscope* scope = nullptr;
	
	if (obj->is_typeof<BytecodeFunction>()) {
		// Apply new scope
		scope = ((BytecodeFunction*) obj)->apply(args);
	} else
		scope = new vscope(scopes.size() == 0 ? nullptr : scopes.back());
	
	if (ref != nullptr)
		scope->put(L"__self", ref);
	;//wcout << "call scopes.size() 1 = " << scopes.size() << endl;
	// Push scope
	scope->root();
	scopes.push_back(scope);
	;//wcout << "call scopes.size() 2 = " << scopes.size() << endl;
	
	// Push call frame and mark own scope
	store_frame(call_stack, call_stack_id, name, 1);
	
	// Apply script
	if (obj->is_typeof<BytecodeFunction>())
		scripts.push_back(((BytecodeFunction*) obj)->get_script());
	
	// Save expected call id
	int call_id = call_stack.size() - 1;
	
	// Do some useless shit
	
	if (obj->is_typeof<BytecodeFunction>()) {
		// Reset pointer to 0 and start
		pointer = 0;
		
		while (1) {
			try {
				exec_bytecode();
				
				// Correct function finish
				obj = Undefined::instance();
				
				break;
				
			} catch(const ck_exceptions::ck_message& msg) { 
				if (msg.get_type() == ck_message_type::CK_RETURN) { ;//wcout << "HANDLE_RETURN_FUNCTION" << endl;
					obj = msg.get_object();//wcout << obj->string_value() << endl;
					break;
				} else { //;wcout << "HANDLE_EXCEPTION_FUNCTION" << endl;
					if (msg.get_type() == ck_message_type::CK_OBJECT)
						follow_exception(msg);
					else
						follow_exception(new Error(msg));
					//;wcout << "HANDLED_EXCEPTION_FUNCTION" << endl;
				}
			} catch (const std::exception& ex) {
				follow_exception(new Error(ck_message(ex)));
			} catch (...) {
				follow_exception(new Error(ck_message(ck_exceptions::ck_message_type::NATIVE_EXCEPTION)));
			} 
		}
	} else {
		try {
			obj = obj->call(scope, args);
		} catch(const ck_exceptions::ck_message& msg) { 
			if (msg.get_type() == ck_message_type::CK_RETURN)
				obj = msg.get_object();
			else if (msg.get_type() == ck_message_type::CK_OBJECT)
				follow_exception(msg);
			else
				follow_exception(new Error(msg));
		} catch (const std::exception& ex) {
			follow_exception(new Error(ck_message(ex)));
		} catch (...) {
			follow_exception(new Error(ck_message(ck_exceptions::ck_message_type::NATIVE_EXCEPTION)));
		} 
	}
	;//wcout << "BEFORE CALL POP" << endl;
	// Try to restore scope
	restore_frame(call_stack, call_stack_id, call_id);
	;//wcout << "AFTER CALL POP" << endl;
	;//wcout << "call scopes.size() 3 = " << scopes.size() << endl;
	scopes.pop_back();
	
	if (obj)
		;//wcout << "RETURNED: " << obj->string_value() << endl;
	else
		;//wcout << "RETURNED: " << "NULL" << endl;
	return obj;
};

void ck_executer::goto_address(int bytecode_address) {
	if (bytecode_address < 0 || bytecode_address > scripts.back()->bytecode.bytemap.size())
		throw ck_message(L"goto out of range [" + to_wstring(bytecode_address) + L"] for range [0, " + to_wstring(scripts.back()->bytecode.bytemap.size()) + L"]" , ck_message_type::CK_INVALID_STATE);
	
	pointer = bytecode_address;
};

void ck_executer::clear() {
	
	// Delete evereseng
	
	for (int i = window_stack.size() - 1; i >= 0; --i) {
		if (window_stack.back().scope_id >= 0 && window_stack.back().scope_id < scopes.size()) 
			if (scopes[window_stack.back().scope_id]) {
				if (window_stack.back().own_scope)
						scopes[window_stack.back().scope_id]->unroot();
				
				scopes[window_stack.back().scope_id] = nullptr;
			}
		
		window_stack.pop_back();
	}	
	
	for (int i = call_stack.size() - 1; i >= 0; --i) {
		if (call_stack.back().scope_id >= 0 && call_stack.back().scope_id < scopes.size()) 
			if (scopes[call_stack.back().scope_id]) {
				if (call_stack.back().own_scope)
						scopes[call_stack.back().scope_id]->unroot();
				
				scopes[call_stack.back().scope_id] = nullptr;
			}
		
		call_stack.pop_back();
	}
	
	try_stack.clear();
	
	for (int i = scopes.size() - 1; i >= 0; --i) {
		if (scopes[i])
			scopes[i]->unroot();
		
		scopes.pop_back();
	}
	
	for (int i = objects.size() - 1; i >= 0; --i) 
		objects.pop_back();
};

// TO BE CONTINUED...