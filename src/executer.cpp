#include "executer.h"

#include <vector>
#include <map>

#include "GIL2.h"
#include "vobject.h"
#include "script.h"
#include "translator.h"
#include "stack_utils.h"

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
#include "objects/NativeFunction.h"
#include "objects/Cake.h"


// #define DEBUG_OUTPUT


using namespace std;
using namespace ck_core;
using namespace ck_vobject;
using namespace ck_objects;
using namespace ck_exceptions;


// D E B U G _ F U N C T I O N S

#define PRINT_OBJECTS print_objects(this->objects)
static void print_objects(vector<vobject*>& objects) {	
	for (int i = 0; i < objects.size(); ++i) {
		wcout << '[' << i << ']' << ' ';
		if (objects[i])
			wcout << objects[i]->string_value() << endl;
		else
			wcout << "nullptr" << endl;
	}
};


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
		
	vector<late_call_instance>& late_call = exec_instance->late_call;
		
	for (int i = 0; i < late_call.size(); ++i) {
		if (late_call[i].obj && !late_call[i].obj->gc_reachable)
			late_call[i].obj->gc_mark();
		if (late_call[i].ref && !late_call[i].ref->gc_reachable)
			late_call[i].ref->gc_mark();
		if (late_call[i].scope && !late_call[i].scope->gc_reachable)
			late_call[i].scope->gc_mark();
		
		for (int j = 0; j < late_call[i].args.size(); ++j)
			if (late_call[i].args[j] && !late_call[i].args[j]->gc_reachable)
				late_call[i].args[j]->gc_mark();
	}
};

void ck_executer_gc_object::gc_finalize() {};


// C K _ E X E C U T E R

ck_executer::ck_executer() {
	// Will be disposed by GC.
	gc_marker = new ck_executer_gc_object(this);
		
	// Limit size of total stack usage when calculating
	//  call, try and windows stacks sizes in constructor.
	// ck_constants::ck_executer::def_system_stack_offset
	
	// Assuming that exec_bytecode, call_object and executer summary takes no more than 1024 bytes.
	// ck_constants::ck_executer::def_stack_frame_size;
	
	int system_stack_size = ck_util::get_system_stack_size();
	// Limiting stack by 2MB for user calls
	int bounded_stack_size = system_stack_size - ck_constants::ck_executer::def_system_stack_offset;
	// Resulting allowed size of stack
	int result_stack_size = bounded_stack_size / ck_constants::ck_executer::def_stack_frame_size;
	// Limit stack sizes
	try_stack_limit = result_stack_size;
	execution_stack_limit = result_stack_size;
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
	return !scripts.back() || pointer >= scripts.back()->bytecode.bytemap.size() || scripts.back()->bytecode.bytemap[pointer] == ck_bytecodes::BCEND || pointer == -1;
};

ck_vobject::vobject* ck_executer::vpop() {
	if (objects.size() == 0)
		throw StackCorruption(L"objects stack corrupted");
	
	vobject* t = objects.back();
	objects.pop_back();
	
	return t;
};

ck_vobject::vobject* ck_executer::vpeek() {
	if (objects.size() == 0)
		throw StackCorruption(L"objects stack corrupted");
	
	return objects.back();
};

void ck_executer::vpush(ck_vobject::vobject* o) {
	// We have to handle out of memory exceptions because stack can not grow to the infinity.
	try {
		objects.push_back(o);
	} catch(std::bad_alloc) {
		throw OutOfMemory(L"allocation out of memory");
	}
};

void ck_executer::vswap() {
	if (objects.size() < 2)
		throw StackCorruption(L"objects stack corrupted");
	
	std::iter_swap(objects.end() - 1, objects.end() - 2);
};

void ck_executer::vswap1() {
	if (objects.size() < 3)
		throw StackCorruption(L"objects stack corrupted");
	
	std::iter_swap(objects.end() - 2, objects.end() - 3);
};

void ck_executer::vswap2() {
	if (objects.size() < 4)
		throw StackCorruption(L"objects stack corrupted");
	
	std::iter_swap(objects.end() - 3, objects.end() - 4);
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
	
	if ((stack_id == call_stack_id || stack_id == window_stack_id) && call_stack.size() + window_stack.size() == execution_stack_limit)
		throw StackOverflow(L"stack overflow");
	else if (stack_id == try_stack_id && stack.size() == try_stack_limit)
		throw StackOverflow(L"try stack overflow");
		
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
	
	if (restored_frame_id > (int) stack.size() - 1)
		return;

#ifdef DEBUG_OUTPUT
	wcout << endl;
	wcout << "-------------> call   stack id current  " << (int)call_stack.size()-1 << endl;
	wcout << "-------------> try    stack id current  " << (int)try_stack.size() -1<< endl;
	wcout << "-------------> window stack id current  " << (int)window_stack.size()-1 << endl;
	wcout << "-------------> scripts      id current  " << (int)scripts.size()-1 << endl;
	wcout << "-------------> scopes       id current  " << (int)scopes.size()-1 << endl;
#endif
	
	if (stack_id == call_stack_id && stack.size() == 0)
		throw StackCorruption(L"call stack corrupted");
	else if (stack_id == try_stack_id && stack.size() == 0)
		throw StackCorruption(L"try stack corrupted");
	else if (stack_id == window_stack_id && stack.size() == 0)
		throw StackCorruption(L"window stack corrupted");
	
	int window_id = stack.back().window_id;
	int try_id    = stack.back().try_id;
	int call_id   = stack.back().call_id;
	int script_id = stack.back().script_id;
	int scope_id  = stack.back().scope_id;
	int object_id = stack.back().object_id;
	int pointer_v = stack.back().pointer;
	
#ifdef DEBUG_OUTPUT
	wcout << "-------------> call   stack id expected " << call_id   << endl;
	wcout << "-------------> try    stack id expected " << try_id    << endl;
	wcout << "-------------> window stack id expected " << window_id << endl;
	wcout << "-------------> scripts      id expected " << script_id << endl;
	wcout << "-------------> scopes       id expected " << scope_id << endl;
#endif
	
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
	
#ifdef DEBUG_OUTPUT
	wcout << "-------------> call   stack id result   " << (int)call_stack.size()-1 << endl;
	wcout << "-------------> try    stack id result   " << (int)try_stack.size() -1<< endl;
	wcout << "-------------> window stack id result   " << (int)window_stack.size()-1 << endl;
	wcout << "-------------> scripts      id result   " << (int)scripts.size()-1 << endl;
	wcout << "-------------> scopes       id result   " << (int)scopes.size()-1 << endl;
#endif
};

void ck_executer::follow_exception(const cake& msg) {
	
	if (try_stack.size() == 0) {
		if (msg.has_backtrace())
			throw msg;
		
		// Collect backtrace for this cake (and do not collect for object)
		cake copy(msg);
		if (!copy.has_backtrace() && copy.get_type_id() != cake_type::CK_OBJECT) copy.collect_backtrace();
		
		throw copy;	
	}
	
	int type          = try_stack.back().try_type;
	int catch_address = try_stack.back().catch_node;
	int window_id     = try_stack.back().window_id;
	int call_id       = try_stack.back().call_id;
	int try_id        = try_stack.back().try_id;
	wstring handler   = try_stack.back().name;
	
#ifdef DEBUG_OUTPUT
	wcout << "---CURRENT---> call   stack id current  " << (int)call_stack.size()-1 << endl;
	wcout << "---CURRENT---> try    stack id current  " << (int)try_stack.size() -1 << endl;
	wcout << "---CURRENT---> win    stack id current  " << (int)window_stack.size() -1 << endl;
	wcout << "=============> call   stack id expected " << call_id << endl;
	wcout << "=============> try    stack id expected " << try_id << endl;
	wcout << "=============> win    stack id expected " << window_id << endl;
	wcout << "~~~~~~~~~~~~~> call_stack_try    id     " << call_stack.back().try_id << endl;
	wcout << "~~~~~~~~~~~~~> window_stack_try  id     " << window_stack.back().try_id << endl;
	wcout << "~~~~~~~~~~~~~> call_stack_call   id     " << call_stack.back().call_id << endl;
	wcout << "~~~~~~~~~~~~~> window_stack_call id     " << window_stack.back().call_id << endl << endl;
#endif

	// Check for valid call
	if (call_stack.size() != 0 && call_stack.back().try_id == try_stack.size()-1) { 
		if (msg.has_backtrace()) {
			restore_frame(call_stack, call_stack_id, call_id - 1);
			throw msg;
		}
		
		// Collect backtrace for this cake (and do not collect for object)
		cake copy(msg);
		if (!copy.has_backtrace() && copy.get_type_id() != cake_type::CK_OBJECT) copy.collect_backtrace();
		
		restore_frame(call_stack, call_stack_id, call_id - 1);
		throw copy;
	}
	
	// Check for valid script
	if (window_stack.size() != 0 && window_stack.back().try_id == try_stack.size()-1) {
		if (msg.has_backtrace()) {
			restore_frame(window_stack, window_stack_id, window_id - 1);
			throw msg;
		}
		
		// Collect backtrace for this cake (and do not collect for object)
		cake copy(msg);
		if (!copy.has_backtrace() && copy.get_type_id() != cake_type::CK_OBJECT) copy.collect_backtrace();
		
		restore_frame(window_stack, window_stack_id, window_id - 1);
		throw copy;
	}
	
	if (msg.has_backtrace()) {
		switch(type) {
			case ck_bytecodes::TRY_NO_ARG:
			case ck_bytecodes::TRY_NO_CATCH:
				goto_address(catch_address); 
				break;
				
			case ck_bytecodes::TRY_WITH_ARG: {
				vscope* scope = new iscope(scopes.size() == 0 ? nullptr : scopes.back());
				scope->root();
				scope->put(handler, msg.get_type_id() == cake_type::CK_OBJECT ? msg.get_object() : new Cake(msg), 0, 1);
				scopes.push_back(scope);
				goto_address(catch_address); 
				break;
			}
		}
	} else {
	
		// Collect backtrace for this cake
		cake copy(msg);
		if (!copy.has_backtrace() && copy.get_type_id() != cake_type::CK_OBJECT) copy.collect_backtrace();
		
		
		// The default behaviour is to pop try_frame out when handling exception.
		// Normally try_trame should be popped by POP_TRY when try block finishes work without error.
		restore_frame(try_stack, try_stack_id, try_stack.size() - 1);
		
		switch(type) {
			case ck_bytecodes::TRY_NO_ARG:
			case ck_bytecodes::TRY_NO_CATCH:
				goto_address(catch_address); 
				break;
				
			case ck_bytecodes::TRY_WITH_ARG: {
				vscope* scope = new iscope(scopes.size() == 0 ? nullptr : scopes.back());
				scope->root();
				scope->put(handler, copy.get_type_id() == cake_type::CK_OBJECT ? copy.get_object() : new Cake(copy), 0, 1);
				scopes.push_back(scope);
				goto_address(catch_address); 
				break;
			}
		}
	}
};


void ck_executer::validate_scope() {
	if (scopes.size() == 0 || scopes.back() == nullptr)
		throw StackCorruption(L"scopes stack corrupted");		
};

vobject* ck_executer::exec_bytecode() { 

	while (!is_eof()) {
		
		// Check if thread is dead (suspended or anything else)
		if (!GIL::current_thread()->is_alive())
			return nullptr;
		
		// Check for pending late calls
		while (late_call.size()) {
			
			late_call_instance instance = late_call.back();
			
			// Remove call from the list
			late_call.pop_back();
			
			// Exceptions automatically rethrown up
			call_object(instance.obj, instance.ref, instance.args, instance.name, instance.scope);
		}
		
#ifdef DEBUG_OUTPUT
		wcout << "[" << pointer << "] ";
#endif

		switch(scripts.back()->bytecode.bytemap[pointer++]) {
			case ck_bytecodes::LINENO: {
				int lineno; 
				read(sizeof(int), &lineno);
#ifdef DEBUG_OUTPUT
				wcout << "LINENO: " << lineno << endl;
#endif
				break;
			}
			
			case ck_bytecodes::NOP: {
#ifdef DEBUG_OUTPUT
				wcout << "> NOP" << endl;
#endif
				break;
			}
			
			case ck_bytecodes::PUSH_CONST_INT: {
				long long i; 
				read(sizeof(long long), &i);
#ifdef DEBUG_OUTPUT
				wcout << "> PUSH_CONST[int]: " << i << endl;
#endif
				vpush(new Int(i));
				break;
			}
			
			case ck_bytecodes::PUSH_CONST_DOUBLE: {
				double i; 
				read(sizeof(double), &i);
#ifdef DEBUG_OUTPUT
				wcout << "> PUSH_CONST[double]: " << i << endl;
#endif
				vpush(new Double(i));
				break;
			}
			
			case ck_bytecodes::PUSH_CONST_BOOLEAN: {
				bool i; 
				read(sizeof(bool), &i);
#ifdef DEBUG_OUTPUT
				wcout << "> PUSH_CONST[boolean]: " << i << endl;
#endif
				vpush(new Bool(i));
				break;
			}
			
			case ck_bytecodes::PUSH_CONST_NULL: {
#ifdef DEBUG_OUTPUT
				wcout << "> PUSH_CONST: null" << endl;
#endif
				vpush(Null::instance());
				break;
			}
			
			case ck_bytecodes::PUSH_CONST_UNDEFINED: {
#ifdef DEBUG_OUTPUT
				wcout << "> PUSH_CONST: undefined" << endl;
#endif
				vpush(Undefined::instance());
				break;
			}
			
			case ck_bytecodes::PUSH_CONST_STRING: {
				int size;
				read(sizeof(int), &size);
				wchar_t cstr[size+1];
				read(sizeof(wchar_t) * size, cstr);
				cstr[size] = 0;
#ifdef DEBUG_OUTPUT
				wcout << "> PUSH_CONST[string]: \"" << cstr << '"' << endl;
#endif
				
				vpush(new String(cstr));
				break;
			}
			
			case ck_bytecodes::LOAD_VAR: {
				int size;
				read(sizeof(int), &size);
				wchar_t cstr[size+1];
				read(sizeof(wchar_t) * size, cstr);
				cstr[size] = 0;
#ifdef DEBUG_OUTPUT
				wcout << "> LOAD_VAR: " << cstr << endl;
#endif
				
				// Check for valid scope
				validate_scope();
				
				// Scope should return nullptr if value does not exist.
				vobject* o = scopes.back()->get(cstr, 1);
				if (o == nullptr)
					throw TypeError(wstring(L"undefined reference to ") + cstr);
				
				vpush(o);
				break;
			}
			
			case ck_bytecodes::VSTACK_POP: {
#ifdef DEBUG_OUTPUT
				wcout << "> VSTACK_POP" << endl;
#endif
				vpop();
				break;
			}
			
			case ck_bytecodes::PUSH_CONST_ARRAY: {
				int size; 
				read(sizeof(int), &size);
				
#ifdef DEBUG_OUTPUT
				wcout << "> PUSH_CONST[array]: [" << size << ']' << endl;
#endif
				
				vector<vobject*> array;
				for (int i = 0; i < size; ++i)
					array.push_back(vpop());
				vpush(new Array(array));
				break;
			}
			
			case ck_bytecodes::PUSH_CONST_OBJECT: {
				int size; 
				read(sizeof(int), &size);
#ifdef DEBUG_OUTPUT
				wcout << "> PUSH_CONST[object]: {";
#endif
				
				map<wstring, vobject*> objects;
				
				for (int i = 0; i < size; ++i) {
					int ssize;
					read(sizeof(int), &ssize);
					wchar_t cstr[ssize+1];
					read(sizeof(wchar_t) * ssize, cstr);
					cstr[ssize] = 0;
					
					objects[cstr] = vpop();
					
#ifdef DEBUG_OUTPUT
					wcout << cstr;
					if (i != size-1)
						wcout << ", ";
#endif
				}
				
#ifdef DEBUG_OUTPUT
				wcout << '}' << endl;
#endif
				
				vpush(new Object(objects));
				break;
			}
			
			case ck_bytecodes::DEFINE_VAR: {
				int amount; 
				read(sizeof(int), &amount);
#ifdef DEBUG_OUTPUT
				wcout << "> DEFINE_VAR: ";
#endif
				
				for (int i = 0; i < amount; ++i) {
					int ssize = 0;
					read(sizeof(int), &ssize);
					
					wchar_t cstr[ssize+1];
					read(sizeof(wchar_t) * ssize, cstr);
					cstr[ssize] = 0;
#ifdef DEBUG_OUTPUT
					wcout << cstr;
#endif
					
					unsigned char ops = 0;
					read(sizeof(unsigned char), &ops);
					
					// Check for valid scope
					validate_scope();			
					
					if ((ops & 0b1000) == 0) {
						scopes.back()->put(cstr, Undefined::instance(), 0, 1);
#ifdef DEBUG_OUTPUT
						wcout << " = [undefined]";
#endif
					} else
						scopes.back()->put(cstr, vpop(), 0, 1);
					
#ifdef DEBUG_OUTPUT
					if (i != amount-1)
						wcout << ", ";
#endif
				}
				
#ifdef DEBUG_OUTPUT
				wcout << endl;
#endif
				break;
			}
		
			case ck_bytecodes::VSTACK_DUP: {
#ifdef DEBUG_OUTPUT
				wcout << "> VSTACK_DUP" << endl;
#endif
				vpush(vpeek());
				break;
			}
			
			case ck_bytecodes::CALL: {
				// bytecode: CALL [argc]
				// stack: argN..arg0 fun
				
				int argc; 
				read(sizeof(int), &argc);
				
#ifdef DEBUG_OUTPUT
				wcout << "> CALL [" << argc << ']' << endl;
#endif
				
				if (objects.size() < argc + 1)
					throw StackCorruption(L"objects stack corrupted"); 
				
				// Copy args
				vector<vobject*> args;
				for (int k = 0; k < argc; ++k)
					args.push_back(objects.rbegin()[argc-k-1 + 1]);
				
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
				
#ifdef DEBUG_OUTPUT
				wcout << "> CALL_FIELD [" << argc << "] [" << cstr << ']' << endl;
#endif
				if (objects.size() < argc + 1)
					throw StackCorruption(L"objects stack corrupted"); 
				
				if (objects.back() == nullptr)
					throw TypeError(wstring(L"undefined reference to ") + cstr);
					
				validate_scope();
					
				vpush(objects.back()->get(scopes.back(), cstr));
				// stack: argN..arg0 ref fun
				
				// Copy args
				vector<vobject*> args;
				for (int k = 0; k < argc; ++k)
					args.push_back(objects.rbegin()[argc-k-1 + 2]);
				
				vobject* obj = call_object(objects.back(), objects.rbegin()[1], args, cstr);
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
				
#ifdef DEBUG_OUTPUT
				wcout << "> CALL_NAME [" << argc << "] [" << cstr << ']' << endl;
#endif
				
				if (objects.size() < argc)
					throw StackCorruption(L"objects stack corrupted"); 
					
				validate_scope();
					
				vpush(scopes.back()->get(scopes.back(), cstr));
				// stack: argN..arg0 fun
				
				// Copy args
				vector<vobject*> args;
				for (int k = 0; k < argc; ++k)
					args.push_back(objects.rbegin()[argc-k-1 + 1]);
				
				vobject* obj = call_object(objects.rbegin()[0], nullptr, args, cstr);
				for (int k = 0; k < argc + 1; ++k)
					objects.pop_back();
				vpush(obj);
#ifdef DEBUG_OUTPUT
				wcout << "scipts_size = " << scripts.size() << endl;
#endif
				break;
			}
			
			case ck_bytecodes::CALL_MEMBER: {
				// bytecode: CALL [argc] [name]
				// stack: argN..arg0 ref key
				
				int argc; 
				read(sizeof(int), &argc);
				
				if (objects.size() < argc + 2)
					throw StackCorruption(L"objects stack corrupted"); 
				
				if (objects.rbegin()[0] == nullptr)
					throw TypeError(L"undefined reference to member");
				
				wstring key = objects.rbegin()[0]->string_value();
				
#ifdef DEBUG_OUTPUT
				wcout << "> CALL_MEMBER [" << argc << "] [" << key << ']' << endl;
#endif
				
				if (objects.rbegin()[1] == nullptr)
					throw TypeError(L"undefined reference to " + key);
					
				validate_scope();
					
				vpush(objects.rbegin()[1]->get(scopes.back(), key));
				// stack: argN..arg0 ref key fun
				
				// Copy args
				vector<vobject*> args;
				for (int k = 0; k < argc; ++k)
					args.push_back(objects.rbegin()[argc-k-1 + 3]);
				
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
					throw StackCorruption(L"objects stack corrupted");
				
				vobject *ref = objects.rbegin()[1];
				vobject *fun = nullptr;
				
				if (ref == nullptr)
					throw TypeError(L"undefined reference to operator");
				
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
				
#ifdef DEBUG_OUTPUT
				wcout << "> OPERATOR [" << fun_name << ']' << endl;
#endif
				
				fun = ref->get(scopes.back(), L"__operator" + fun_name);
				
				// rvalue operator
				if (fun == nullptr || fun->is_typeof<Undefined>() || fun->is_typeof<Null>()) 
					fun = ref->get(scopes.back(), L"__roperator" + fun_name);
				else
					fun_name = L"__operator" + fun_name;
				
				fun = ref->get(scopes.back(), L"__operator" + fun_name);				
					
				// no operator
				if (fun == nullptr || fun->is_typeof<Undefined>() || fun->is_typeof<Null>()) 
					throw TypeError(L"undefined reference to operator " + fun_name);
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
				
				scopes.back()->put(cstr, vpop(), 1, 1);
				
#ifdef DEBUG_OUTPUT
				wcout << "> STORE_VAR: " << cstr << endl;
#endif
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
					throw TypeError(L"undefined reference to " + wstring(cstr));
				
				// Check for scope
				validate_scope();		
				
				ref->put(scopes.back(), cstr, val);
				
#ifdef DEBUG_OUTPUT
				wcout << "> STORE_FIELD: " << cstr << endl;
#endif
				break;
			}
			
			case ck_bytecodes::STORE_MEMBER: {
				vobject* val = vpop();
				vobject* key = vpop();
				vobject* ref = vpop();
				
				if (ref == nullptr)
					throw TypeError(L"undefined reference");
				
				if (key == nullptr)
					throw TypeError(L"undefined reference to member");
				
				// Check for scope
				validate_scope();		
				
				ref->put(scopes.back(), key->string_value(), val);
				
#ifdef DEBUG_OUTPUT
				wcout << "> STORE_MEMBER " << endl;
#endif
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
					throw TypeError(L"undefined reference to " + wstring(cstr));
				
				// Check for scope
				validate_scope();		
				
				vpush(ref->get(scopes.back(), cstr));
				
#ifdef DEBUG_OUTPUT
				wcout << "> LOAD_FIELD: " << cstr << endl;
#endif
				break;
			}
			
			case ck_bytecodes::LOAD_MEMBER: {
				vobject* key = vpop();
				vobject* ref = vpop();
				
				if (ref == nullptr)
					throw TypeError(L"undefined reference");
				
				if (key == nullptr)
					throw TypeError(L"undefined reference to member");
				
				// Check for scope
				validate_scope();		
				
				vpush(ref->get(scopes.back(), key->string_value()));
				
#ifdef DEBUG_OUTPUT
				wcout << "> LOAD_MEMBER " << endl;
#endif
				break;
			}
		
			case ck_bytecodes::UNARY_OPERATOR: {				
				unsigned char i; 
				read(sizeof(unsigned char), &i);
				
				validate_scope();
				
				if (objects.size() == 0)
					throw StackCorruption(L"objects stack corrupted");
				
				vobject *ref = objects.rbegin()[0];
				vobject *fun = nullptr;
				
				if (ref == nullptr)
					throw TypeError(L"undefined reference to operator");
				
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
				
#ifdef DEBUG_OUTPUT
				wcout << "> OPERATOR [" << fun_name << ']' << endl;
#endif
				
				fun = ref->get(scopes.back(), L"__operator" + fun_name);
				
				// rvalue operator
				if (fun == nullptr || fun->is_typeof<Undefined>() || fun->is_typeof<Null>()) 
					fun = ref->get(scopes.back(), L"__roperator" + fun_name);
				else
					fun_name = L"__operator" + fun_name;
				
				fun = ref->get(scopes.back(), L"__operator" + fun_name);				
					
				// no operator
				if (fun == nullptr || fun->is_typeof<Undefined>() || fun->is_typeof<Null>()) 
					throw TypeError(L"undefined reference to operator " + fun_name);
				else
					fun_name = L"__roperator" + fun_name;
				
				vobject* res = call_object(fun, ref, { ref }, fun_name);
				
				vpop(); 
				vpush(res);
				
				break;
			}
			
			case ck_bytecodes::VSTACK_SWAP: {
#ifdef DEBUG_OUTPUT
				wcout << "> VSTACK_SWAP" << endl;
#endif
				vswap();
				break;
			}
			
			case ck_bytecodes::VSTACK_SWAP1: {
#ifdef DEBUG_OUTPUT
				wcout << "> VSTACK_SWAP1" << endl;
#endif
				vswap1();
				break;
			}
			
			case ck_bytecodes::VSTACK_SWAP2: {
#ifdef DEBUG_OUTPUT
				wcout << "> VSTACK_SWAP2" << endl;
#endif
				vswap2();
				break;
			}
			
			case ck_bytecodes::VSTATE_PUSH_SCOPE: {
#ifdef DEBUG_OUTPUT
				wcout << "> VSTATE_PUSH_SCOPE" << endl;
#endif
				vscope* s = new iscope(scopes.size() == 0 ? nullptr : scopes.back());
				GIL::gc_instance()->attach_root(s);
				scopes.push_back(s);
				break;
			}
			
			case ck_bytecodes::VSTATE_POP_SCOPE: {
#ifdef DEBUG_OUTPUT
				wcout << "> VSTATE_POP_SCOPE" << endl;
#endif
				
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
				
#ifdef DEBUG_OUTPUT
				wcout << "> JMP_IF_ZERO [" << i << ']' << endl;
#endif
				
				vobject* o = vpop();
				if (o == nullptr || o->int_value() == 0)
					goto_address(i);
				break;
			}
			
			case ck_bytecodes::JMP: {
				int i; 
				read(sizeof(int), &i);
				
#ifdef DEBUG_OUTPUT
				wcout << "> JMP [" << i << ']' << endl;
#endif
				
				goto_address(i);
				break;
			}
			
			case ck_bytecodes::BCEND: {
#ifdef DEBUG_OUTPUT
				wcout << "> BCEND" << endl;
#endif
				
				// Make pointer point at BCEND
				--pointer;
				return nullptr;
				
				break;
			}
			
			case ck_bytecodes::THROW_NOARG: {
#ifdef DEBUG_OUTPUT
				wcout << "> THROW_NOARG" << endl;
#endif
				
				throw ObjectCake(nullptr);
				
				break;
			}
			
			case ck_bytecodes::THROW: {		
#ifdef DEBUG_OUTPUT		
				wcout << "> THROW" << endl;
#endif
				
				throw ObjectCake(vpop());
				
				break;
			}
			
			case ck_bytecodes::THROW_STRING: {
				int size;
				read(sizeof(int), &size);
				wchar_t cstr[size+1];
				read(sizeof(wchar_t) * size, cstr);
				cstr[size] = 0;
				
#ifdef DEBUG_OUTPUT
				wcout << "> THROW_STRING: \"" << cstr << '"' << endl;
#endif
				
				throw ObjectCake(new String(cstr));
				
				break;
			}
			
			case ck_bytecodes::VSTATE_POP_SCOPES: {
				int i; 
				read(sizeof(int), &i);
#ifdef DEBUG_OUTPUT
				wcout << "> VSTATE_POP_SCOPES [" << i << ']' << endl;
#endif
				
				if (scopes.size() < i)
					throw StackCorruption(L"scopes stack corrupted");
				
				for (int k = 0; k < i; ++k)
					scopes.pop_back();
				
				break;
			}
			
			case ck_bytecodes::RETURN_VALUE: {
#ifdef DEBUG_OUTPUT
				wcout << "> RETURN_VALUE" << endl;
#endif
				
				// Returning as a normal result from a function.
				// Deal with throwing cakes.
				return vpop();
				
				break;
			}
			
			case ck_bytecodes::PUSH_CONST_FUNCTION: {
				
				// Check for scope
				validate_scope();		
				
				int argc; 
				read(sizeof(int), &argc);
#ifdef DEBUG_OUTPUT
				wcout << "> PUSH_CONST_FUNCTION (" << argc << ") (";
#endif
				
				vector<wstring> argn;
				
				for (int i = 0; i < argc; ++i) {
					int ssize = 0;
					read(sizeof(int), &ssize);
					
					wchar_t cstr[ssize+1];
					read(sizeof(wchar_t) * ssize, cstr);
					cstr[ssize] = 0;
					
					argn.push_back(cstr);
					
#ifdef DEBUG_OUTPUT
					wcout << cstr;
					
					if (i != argc-1)
						wcout << ", ";
#endif
				}
				
				int sizeof_block; 
				read(sizeof(int), &sizeof_block);
				
#ifdef DEBUG_OUTPUT
				wcout << ") [" << sizeof_block << "]" << endl;
#endif
				
				ck_script* script = new ck_script();
				script->directory = scripts.back()->directory;
				script->filename  = scripts.back()->filename;
				
				// Copy range of bytecodes
				
				script->bytecode.bytemap = vector<unsigned char>(scripts.back()->bytecode.bytemap.begin() + pointer, scripts.back()->bytecode.bytemap.begin() + pointer + sizeof_block);
				
				// Copy range of lineno table
				
				int i_a = -1;
				int i_b = -1;
				for (int i = 0; i < scripts.back()->bytecode.lineno_table.size() - 2; i += 2) {
					int byteof = scripts.back()->bytecode.lineno_table[i+1];
					int byteofd = scripts.back()->bytecode.lineno_table[i+3];
					
					if (pointer >= byteof && pointer < byteofd) 
						i_a      = i;
					if (pointer + sizeof_block >= byteof && pointer + sizeof_block < byteofd)
						i_b      = i;
					
					if (i_a != -1 && i_b != -1)
						break;
				}
				
				for (int i = i_a; i <= i_b;) {
					int lineno = scripts.back()->bytecode.lineno_table[i++];
					int byteof = scripts.back()->bytecode.lineno_table[i++];
					
					script->bytecode.lineno_table.push_back(lineno);
					script->bytecode.lineno_table.push_back((byteof - pointer) < 0 ? 0 : byteof - pointer);
				}
					
				
				// Append last marker
				script->bytecode.lineno_table.push_back(-1);
				script->bytecode.lineno_table.push_back(sizeof_block);
				
#ifdef DEBUG_OUTPUT
				wcout << "Function Bytecode: " << endl;
				ck_translator::print(script->bytecode.bytemap);
				wcout << endl;
				
				wcout << "Function Lineno Table: " << endl;
				ck_translator::print_lineno_table(script->bytecode.lineno_table);
				wcout << endl;		
#endif
				
				pointer += sizeof_block;
				
				vpush(new BytecodeFunction(scopes.back(), script, argn));
				
				break;
			}
		
			case ck_bytecodes::VSTATE_POP_TRY: {
				if (try_stack.size() == 0)
					throw StackCorruption(L"try stack corrupted");
				
				// No explicit restoration. 
				// Everything expected to be fine.
				// Simply pop the frame
				int pointer_tmp = pointer;
				restore_frame(try_stack, try_stack_id, try_stack.size() - 1);
				pointer = pointer_tmp;
				
#ifdef DEBUG_OUTPUT
				wcout << "> VSTATE_POP_TRY" << endl;
#endif
				break;
			}

			case ck_bytecodes::VSTATE_PUSH_TRY: {	
		
				if (try_stack.size() == try_stack_limit)
					throw StackOverflow(L"try stack overflow");
				
				int try_node = 0;
				int catch_node = 0;
				wstring handler_name;
				unsigned char type;
				read(sizeof(unsigned char), &type);
				
				if (type == ck_bytecodes::TRY_NO_CATCH) {					
					read(sizeof(int), &try_node);
					read(sizeof(int), &catch_node);
					
#ifdef DEBUG_OUTPUT
					wcout << "> VSTATE_PUSH_TRY [TRY_NO_CATCH] [" << try_node << "] [" << catch_node << ']' << endl;
#endif
				} else if (type == ck_bytecodes::TRY_NO_ARG) {			
					read(sizeof(int), &try_node);
					read(sizeof(int), &catch_node);
					
#ifdef DEBUG_OUTPUT
					wcout << "> VSTATE_PUSH_TRY [TRY_NO_ARG] [" << try_node << "] [" << catch_node << ']' << endl;
#endif
				} else {
					int name_size;
					
					read(sizeof(int), &try_node);
					read(sizeof(int), &catch_node);
					read(sizeof(int), &name_size);
										
					wchar_t cstr[name_size+1];
					read(sizeof(wchar_t) * name_size, cstr);
					cstr[name_size] = 0;
					
					handler_name = cstr;
					
#ifdef DEBUG_OUTPUT
					wcout << "> VSTATE_PUSH_TRY [TRY_WITH_ARG] (" << cstr << ") [" << try_node << "] [" << catch_node << ']' << endl;
#endif
				}
				
				store_frame(try_stack, try_stack_id, handler_name, 0);
				try_stack.back().try_type = type;
				try_stack.back().catch_node = catch_node;
				
				break;
			}
			
			case ck_bytecodes::PUSH_THIS: {
#ifdef DEBUG_OUTPUT
				wcout << "> PUSH_THIS" << endl;
#endif
				
				// Check for valid scope
				validate_scope();
				
				// Get __this value from enclosing scope.
				vpush(scopes.back()->get(L"__this"));
				
				break;
			}
			
			default: throw InvalidState(L"invalid bytecode [" + to_wstring(scripts.back()->bytecode.bytemap[pointer-1]) + L"]");
		}
		
		// Respond to GIL requests
		GIL::instance()->accept_lock();
		
		// Perform GC collection ?
		GIL::gc_instance()->collect();
	}
	
	return nullptr;
};


void ck_executer::execute(ck_core::ck_script* scr, ck_vobject::vscope* scope, std::vector<std::wstring>* argn, std::vector<ck_vobject::vobject*>* argv) {
	
	if (call_stack.size() + window_stack.size() == execution_stack_limit)
		throw StackOverflow(L"stack overflow");
	
	bool own_scope = scope == nullptr;
	
	// Apply new scope
	if (scope == nullptr) {
		scope = new iscope();
		scope->root();
	}
	
	// Overwrite __this to avoid access to the super-parent __this value
	scope->put(L"__this", Undefined::instance());
		
	if (argn != nullptr && argv != nullptr) {
		int argc = argn->size() < argv->size() ? argn->size() : argv->size();
		for (int i = 0; i < argc; ++i)
			scope->put((*argn)[i], (*argv)[i]);
	}
	
	// Push scope
	scopes.push_back(scope);
	
	// Push call frame and mark own scope
	store_frame(window_stack, window_stack_id, L"", own_scope);
	
	// Save expected call id
	int window_id = window_stack.size() - 1;
	
	// Push script instance to the bottom
	scripts.push_back(scr);
	
	// Reset pointer to 0 and start
	pointer = 0;
	
	// Do some useless shit again
	while (1) {
		try {
			exec_bytecode();
			GIL::current_thread()->clear_blocks();
			
			// Reached bytecode end
			break;
			
		} catch(const ck_exceptions::cake& msg) { 
			GIL::current_thread()->clear_blocks();
			
			follow_exception(msg);
		} catch (const std::exception& ex) {
			GIL::current_thread()->clear_blocks();
			
			follow_exception(NativeException(ex));
		} catch (...) {
			GIL::current_thread()->clear_blocks();
			
			follow_exception(UnknownException());
		} 
	}
	
	// Try to restore scope
	restore_frame(window_stack, window_stack_id, window_id);
	
	return;
};

ck_vobject::vobject* ck_executer::call_object(ck_vobject::vobject* obj, ck_vobject::vobject* ref, const std::vector<ck_vobject::vobject*>& args, const std::wstring& name, vscope* scope) { 

	if (call_stack.size() + window_stack.size() == execution_stack_limit)
		throw StackOverflow(L"stack overflow");

	if (obj == nullptr)
		throw TypeError(L"undefined call to " + name);
	
	// Construct scope
	// vscope* scope = nullptr;
	bool own_scope = 0;
	
	if (obj->is_typeof<BytecodeFunction>()) {
		// Apply new scope
		Function* f = (Function*) obj;
		
		// Overwrite binded __this reference
		ref = f->get_bind() ? f->get_bind() : ref;
		
		// Apply this & args on scope
		scope = f->apply(ref, args);
		scope->root();
		own_scope = 1;
	} else {
		//if (scope != nullptr) {
		// XXX: Passing given scope with scope-wrapper.
		
		// Pass given scope as proxy to avoid overwritting of __this value.
		if (scope)
			scope = new xscope(scope);
		else
			scope = new iscope(scopes.size() == 0 ? nullptr : scopes.back());
		scope->root();
		own_scope = 1;
		
		if (ref != nullptr)
			scope->put(L"__this", ref);
		
			// own_scope = 0;
		/* } /* else {
			scope = new iscope(scopes.size() == 0 ? nullptr : scopes.back());
			scope->root();
			own_scope = 1;
			
			if (ref != nullptr)
				scope->put(L"__this", ref);
		} */
	}
	
	// Push scope
	scopes.push_back(scope);
	
	// Push call frame and mark own scope
	store_frame(call_stack, call_stack_id, name, own_scope);
	
	// Apply script
	if (obj->is_typeof<BytecodeFunction>())
		scripts.push_back(((BytecodeFunction*) obj)->get_script());
	
	// Save expected call id
	int call_id = call_stack.size() - 1;
	
	// Do some useless shit
	
	if (obj->is_typeof<BytecodeFunction>()) {
		// Reset pointer to 0 and start
		pointer = 0;
		
		// This long loop without any protection of the objects is thread-safe because 
		//  other threads can not call GC
		
		while (1) {
			try {
				obj = exec_bytecode();
				GIL::current_thread()->clear_blocks();
				
				break;
				
			} catch(const ck_exceptions::cake& msg) { 
				GIL::current_thread()->clear_blocks();
				
				// Process exception
				follow_exception(msg);
			} catch (const std::exception& ex) {
				GIL::current_thread()->clear_blocks();
				
				// Process exception
				follow_exception(NativeException(ex));
			} catch (...) {
				GIL::current_thread()->clear_blocks();
				
				// Process exception
				follow_exception(UnknownException());
			} 
		}
	} else if (obj->is_typeof<NativeFunction>()) {
		try {
			obj = ((NativeFunction*) obj)->get_call_wrapper()(scope, args);
			GIL::current_thread()->clear_blocks();
				
		} catch(const ck_exceptions::cake& msg) { 
			GIL::current_thread()->clear_blocks();
			
			// Process exception
			follow_exception(msg);
		} catch (const std::exception& ex) {
			GIL::current_thread()->clear_blocks();
			
			// Process exception
			follow_exception(NativeException(ex));
		} catch (...) {
			GIL::current_thread()->clear_blocks();
			
			// Process exception
			follow_exception(UnknownException());
		} 
	} else {
		try {
			obj = obj->call(scope, args);
			GIL::current_thread()->clear_blocks();
				
		} catch(const ck_exceptions::cake& msg) { 
			GIL::current_thread()->clear_blocks();
			
			// Process exception
			follow_exception(msg);
		} catch (const std::exception& ex) {
			GIL::current_thread()->clear_blocks();
			
			// Process exception
			follow_exception(NativeException(ex));
		} catch (...) {
			GIL::current_thread()->clear_blocks();
			
			// Process exception
			follow_exception(UnknownException());
		} 
	}
	
	// Try to restore frame
	restore_frame(call_stack, call_stack_id, call_id);
	
	scopes.pop_back();
	
#ifdef DEBUG_OUTPUT
	if (obj)
		wcout << "RETURNED: " << obj->string_value() << endl;
	else
		wcout << "RETURNED: " << "NULL" << endl;
#endif

	return obj;
};

void ck_executer::late_call_object(ck_vobject::vobject* obj, ck_vobject::vobject* ref, const std::vector<ck_vobject::vobject*>& args, const std::wstring& name, vscope* exec_scope) { 
	late_call_instance instance;
	instance.obj = obj;
	instance.ref = ref;
	instance.name = name;
	instance.args = args;
	instance.scope = exec_scope;
	
	late_call.push_back(instance);
};

void ck_executer::goto_address(int bytecode_address) {
	if (bytecode_address < 0 || bytecode_address > scripts.back()->bytecode.bytemap.size())
		throw InvalidState(L"goto out of range [" + to_wstring(bytecode_address) + L"] for range [0, " + to_wstring(scripts.back()->bytecode.bytemap.size()) + L"]");
	
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
	
	late_call.clear();
};

// TO BE CONTINUED...