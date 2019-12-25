#include "executer.h"

#include <vector>
#include <map>

#include "GIL2.h"
#include "vobject.h"
#include "script.h"
#include "translator.h"
#include "stack_locator.h"

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
// #define DEBUG_CALL


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
	GIL::gc_instance()->attach_root(gc_marker);
};

ck_executer::~ck_executer() {
	GIL::gc_instance()->deattach_root(gc_marker);
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

bool ck_executer::read(int size, std::wstring& str) {
	if (!scripts.back())
		return 0;
	
	if (pointer + size * sizeof(wchar_t) > scripts.back()->bytecode.bytemap.size())
		return 0;
	
	str.assign(reinterpret_cast<wstring::const_pointer>(&scripts.back()->bytecode.bytemap[pointer]), size);
	pointer += size * sizeof(wchar_t);
	
	return 1;
};

bool ck_executer::read(std::wstring& str) {
	if (!scripts.back())
		return 0;
	
	int size;
	if (!read(sizeof(int), &size))
		return 0;
	
	if (pointer + size * sizeof(wchar_t) > scripts.back()->bytecode.bytemap.size())
		return 0;
	
	str.assign(reinterpret_cast<wstring::const_pointer>(&scripts.back()->bytecode.bytemap[pointer]), size);
	pointer += size * sizeof(wchar_t);
	
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
		if (pointer >= scripts.back()->bytecode.lineno_table[i + 1] && pointer < scripts.back()->bytecode.lineno_table[i + 3])
			return scripts.back()->bytecode.lineno_table[i];
	}
	
	return -1;
};


void ck_executer::restore_try_frame(int restored_frame_id) {	
	if (try_stack.size() == 0)
		throw ck_exceptions::StackCorruption(L"try stack corrupted");
	
	if (restored_frame_id > try_stack.size() - 1)
		return;
	
	if (restored_frame_id < 0)
		throw ck_exceptions::StackCorruption(L"try stack id out of range");

	int window_id = try_stack[restored_frame_id].window_id;
	int try_id    = try_stack[restored_frame_id].try_id;
	int call_id   = try_stack[restored_frame_id].call_id;
	int script_id = try_stack[restored_frame_id].script_id;
	int scope_id  = try_stack[restored_frame_id].scope_id;
	int object_id = try_stack[restored_frame_id].object_id;
	int pointer_v = try_stack[restored_frame_id].pointer;

	// Restore window scopes
	// id of scope attached to this window is window_stack[i].scope_id + 1
	if (window_stack.size())
		for (int i = window_stack.size() - 1; i > window_id; --i)
			if (window_stack[i].scope_id + 1 > scope_id 
			&& window_stack[i].scope_id + 1 < scopes.size()
			&& scopes[window_stack[i].scope_id + 1] != nullptr) {
				if (window_stack[i].own_scope)
					scopes[window_stack[i].scope_id + 1]->unroot();
				
				scopes[window_stack[i].scope_id + 1] = nullptr;
			}
	
	// Restore call stack
	// id of scope attached to this call is call_stack[i].scope_id + 1
	if (call_stack.size())
		for (int i = call_stack.size() - 1; i > call_id; --i) {
			if (call_stack[i].scope_id + 1 > scope_id 
			&& call_stack[i].scope_id + 1 < scopes.size()
			&& scopes[call_stack[i].scope_id + 1] != nullptr) {
				if (call_stack[i].own_scope)
					scopes[call_stack[i].scope_id + 1]->unroot();
				
				scopes[call_stack[i].scope_id + 1] = nullptr;
			}
		}
			
	// Clear all scopes untill scope_id value and .unroot() them.
	// All resting scopes (except scope_id+1 is created by VSTATE_PUSH_SCOPE
	//  and marked as root by default).
	for (int i = scopes.size() - 1; i > scope_id + 1; --i)
		if (scopes[i])
			scopes[i]->unroot();
	
	// Restore stacks position
	window_stack.resize(window_id + 1);
	call_stack.resize(call_id + 1);
	try_stack.resize(try_id + 1);
	
	scopes.resize(scope_id + 1);
	scripts.resize(script_id + 1);
	objects.resize(object_id + 1);
	
	// Restore address pointer
	pointer = pointer_v;
};

void ck_executer::restore_call_frame(int restored_frame_id) {
	if (call_stack.size() == 0)
		throw ck_exceptions::StackCorruption(L"call stack corrupted");
	
	if (restored_frame_id > call_stack.size() - 1)
		return;
	
	if (restored_frame_id < 0)
		throw ck_exceptions::StackCorruption(L"call stack id out of range");

	int window_id = call_stack[restored_frame_id].window_id;
	int try_id    = call_stack[restored_frame_id].try_id;
	int call_id   = call_stack[restored_frame_id].call_id;
	int script_id = call_stack[restored_frame_id].script_id;
	int scope_id  = call_stack[restored_frame_id].scope_id;
	int object_id = call_stack[restored_frame_id].object_id;
	int pointer_v = call_stack[restored_frame_id].pointer;

	// Restore window scopes
	// id of scope attached to this window is window_stack[i].scope_id + 1
	if (window_stack.size())
		for (int i = window_stack.size() - 1; i > window_id; --i)
			if (window_stack[i].scope_id + 1 > scope_id 
			&& window_stack[i].scope_id + 1 < scopes.size()
			&& scopes[window_stack[i].scope_id + 1] != nullptr) {
				if (window_stack[i].own_scope)
					scopes[window_stack[i].scope_id + 1]->unroot();
				
				scopes[window_stack[i].scope_id + 1] = nullptr;
			}
	
	// Restore call stack
	// id of scope attached to this call is call_stack[i].scope_id + 1
	if (call_stack.size())
		for (int i = call_stack.size() - 1; i > call_id; --i) {
			if (call_stack[i].scope_id + 1 > scope_id 
			&& call_stack[i].scope_id + 1 < scopes.size()
			&& scopes[call_stack[i].scope_id + 1] != nullptr) {
				if (call_stack[i].own_scope)
					scopes[call_stack[i].scope_id + 1]->unroot();
				
				scopes[call_stack[i].scope_id + 1] = nullptr;
			}
		}
			
	// Clear all scopes untill scope_id value and .unroot() them.
	// All resting scopes (except scope_id+1 is created by VSTATE_PUSH_SCOPE
	//  and marked as root by default).
	for (int i = scopes.size() - 1; i > scope_id + 1; --i)
		if (scopes[i])
			scopes[i]->unroot();
	
	// Restore stacks position
	window_stack.resize(window_id + 1);
	call_stack.resize(call_id + 1);
	try_stack.resize(try_id + 1);
	
	scopes.resize(scope_id + 1);
	scripts.resize(script_id + 1);
	objects.resize(object_id + 1);
	
	// Restore address pointer
	pointer = pointer_v;
};

void ck_executer::restore_window_frame(int restored_frame_id) {	
	if (window_stack.size() == 0)
		throw ck_exceptions::StackCorruption(L"window stack corrupted");
	
	if (restored_frame_id > window_stack.size() - 1)
		return;
	
	if (restored_frame_id < 0)
		throw ck_exceptions::StackCorruption(L"window stack id out of range");

	int window_id = window_stack[restored_frame_id].window_id;
	int try_id    = window_stack[restored_frame_id].try_id;
	int call_id   = window_stack[restored_frame_id].call_id;
	int script_id = window_stack[restored_frame_id].script_id;
	int scope_id  = window_stack[restored_frame_id].scope_id;
	int object_id = window_stack[restored_frame_id].object_id;
	int pointer_v = window_stack[restored_frame_id].pointer;

	// Restore window scopes
	// id of scope attached to this window is window_stack[i].scope_id + 1
	if (window_stack.size())
		for (int i = window_stack.size() - 1; i > window_id; --i)
			if (window_stack[i].scope_id + 1 > scope_id 
			&& window_stack[i].scope_id + 1 < scopes.size()
			&& scopes[window_stack[i].scope_id + 1] != nullptr) {
				if (window_stack[i].own_scope)
					scopes[window_stack[i].scope_id + 1]->unroot();
				
				scopes[window_stack[i].scope_id + 1] = nullptr;
			}
	
	// Restore call stack
	// id of scope attached to this call is call_stack[i].scope_id + 1
	if (call_stack.size())
		for (int i = call_stack.size() - 1; i > call_id; --i) {
			if (call_stack[i].scope_id + 1 > scope_id 
			&& call_stack[i].scope_id + 1 < scopes.size()
			&& scopes[call_stack[i].scope_id + 1] != nullptr) {
				if (call_stack[i].own_scope)
					scopes[call_stack[i].scope_id + 1]->unroot();
				
				scopes[call_stack[i].scope_id + 1] = nullptr;
			}
		}
			
	// Clear all scopes untill scope_id value and .unroot() them.
	// All resting scopes (except scope_id+1 is created by VSTATE_PUSH_SCOPE
	//  and marked as root by default).
	for (int i = scopes.size() - 1; i > scope_id + 1; --i)
		if (scopes[i])
			scopes[i]->unroot();
	
	// Restore stacks position
	window_stack.resize(window_id + 1);
	call_stack.resize(call_id + 1);
	try_stack.resize(try_id + 1);
	
	scopes.resize(scope_id + 1);
	scripts.resize(script_id + 1);
	objects.resize(object_id + 1);
	
	// Restore address pointer
	pointer = pointer_v;
};

void ck_executer::restore_all() {
	// Restore window scopes
	// id of scope attached to this window is window_stack[i].scope_id + 1
	if (window_stack.size())
		for (int i = window_stack.size() - 1; i > 0; --i)
			if (window_stack[i].scope_id + 1 < scopes.size() && scopes[window_stack[i].scope_id + 1] != nullptr) {
				if (window_stack[i].own_scope)
					scopes[window_stack[i].scope_id + 1]->unroot();
				
				scopes[window_stack[i].scope_id + 1] = nullptr;
			}
	
	// Restore call stack
	// id of scope attached to this call is call_stack[i].scope_id + 1
	if (call_stack.size())
		for (int i = call_stack.size() - 1; i > 0; --i) {
			if (call_stack[i].scope_id + 1 < scopes.size() && scopes[call_stack[i].scope_id + 1] != nullptr) {
				if (call_stack[i].own_scope)
					scopes[call_stack[i].scope_id + 1]->unroot();
				
				scopes[call_stack[i].scope_id + 1] = nullptr;
			}
		}
		
	// Erase all resting scope that are made by VSTATE_PUSH_SCOPE
	for (int i = 0; i < scopes.size(); ++i)
		if (scopes[i])
			scopes[i]->unroot();
	
	// Erase all
	window_stack.resize(0);
	call_stack.resize(0);
	try_stack.resize(0);
	
	scopes.resize(0);
	scripts.resize(0);
	objects.resize(0);
};

void ck_executer::follow_exception(const cake& msg) { 
	
	if (try_stack.size() == 0) { // No try-catch, rethrow upwards
		if (msg.has_backtrace())
			throw msg;
		
		// Collect backtrace for this cake (and do not collect for object)
		cake copy(msg);
		if (!copy.has_backtrace()) copy.collect_backtrace();
		
		throw copy;	
	}
	
	int type          = try_stack.back().try_type;
	int catch_address = try_stack.back().catch_node;
	int window_id     = try_stack.back().window_id;
	int call_id       = try_stack.back().call_id;
	int try_id        = try_stack.back().try_id;
	wstring handler   = try_stack.back().name;
	
	// Go one try-catch back
	restore_try_frame(try_stack.size() - 1);
	
	// Append backtrace & follow exception execution in catch block
	if (msg.has_backtrace()) {
		switch(type) {
			case ck_bytecodes::TRY_NO_ARG:
			case ck_bytecodes::TRY_NO_CATCH:
				goto_address(catch_address); 
				break;
				
			case ck_bytecodes::TRY_WITH_ARG: {
				if (scopes.size() == 0)
					throw StackCorruption(L"scopes stack corrupted");
				
				vscope* scope = new iscope(scopes.back());
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
		if (!copy.has_backtrace() && copy.get_type_id() != cake_type::CK_OBJECT) 
			copy.collect_backtrace();
		
		switch(type) {
			case ck_bytecodes::TRY_NO_ARG:
			case ck_bytecodes::TRY_NO_CATCH:
				goto_address(catch_address); 
				break;
				
			case ck_bytecodes::TRY_WITH_ARG: {
				if (scopes.size() == 0)
					throw StackCorruption(L"scopes stack corrupted");
				
				vscope* scope = new iscope(scopes.back());
				scope->root();
				scope->put(handler, copy.get_type_id() == cake_type::CK_OBJECT ? copy.get_object() : new Cake(copy), 0, 1);
				scopes.push_back(scope);
				goto_address(catch_address); 
				break;
			}
		}
	}
};


vobject* ck_executer::exec_bytecode() { 

	while (!is_eof()) {
		
		// Check if thread is dead (suspended or anything else)
		if (!GIL::current_thread()->is_running())
			return nullptr;
		
		// Check for pending late calls
		while (late_call.size()) {
			
			late_call_instance instance = late_call.back();
			
			// Restore root ownership
			if (instance.own_obj)
				instance.obj->gc_make_unroot();
			
			if (instance.own_ref)
				instance.ref->gc_make_unroot();
			
			for (int i = 0; i < instance.args.size(); ++i)
				if (instance.own_args[i]) 
					instance.args[i]->gc_make_unroot();
			
			// Remove call from the list
			late_call.pop_back();
			
			// Exceptions automatically rethrown up
			call_object(instance.obj, instance.ref, instance.args, instance.name, instance.scope, instance.use_scope_without_wrap);
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
				int64_t i; 
				read(sizeof(int64_t), &i);
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
				vpush(i ? Bool::True() : Bool::False());
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
				std::wstring str;
				read(str);
				
#ifdef DEBUG_OUTPUT
				wcout << "> PUSH_CONST[string]: \"" << str << '"' << endl;
#endif
				
				vpush(new String(str));
				break;
			}
			
			case ck_bytecodes::LOAD_VAR: {
				std::wstring str;
				read(str);
				
#ifdef DEBUG_OUTPUT
				wcout << "> LOAD_VAR: " << str << endl;
#endif
				
				// Check for valid scope
				validate_scope();
				
				// Scope should return nullptr if value does not exist.
				vobject* o = scopes.back()->get(str, 1, 1);
				if (o == nullptr)
					throw TypeError(wstring(L"undefined reference to ") + str);
				
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
					std::wstring str;
					read(str);
					
					objects[str] = vpop();
					
#ifdef DEBUG_OUTPUT
					wcout << str;
					if (i != size-1)
						str << ", ";
#endif
				}
				
#ifdef DEBUG_OUTPUT
				str << '}' << endl;
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
					std::wstring str;
					read(str);
					
#ifdef DEBUG_OUTPUT
					wcout << str;
#endif
					
					unsigned char ops = 0;
					read(sizeof(unsigned char), &ops);
					
					// Check for valid scope
					validate_scope();			
					
					if ((ops & 0b1000) == 0) {
						scopes.back()->put(str, Undefined::instance(), 0, 1);
#ifdef DEBUG_OUTPUT
						wcout << " = [undefined]";
#endif
					} else
						scopes.back()->put(str, vpop(), 0, 1);
					
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
				
				std::wstring str;
				read(str);
				
				
#ifdef DEBUG_OUTPUT
				wcout << "> CALL_FIELD [" << argc << "] [" << str << ']' << endl;
#endif
				if (objects.size() < argc + 1)
					throw StackCorruption(L"objects stack corrupted"); 
				
				if (objects.back() == nullptr)
					throw TypeError(wstring(L"undefined reference to ") + str);
					
				validate_scope();
					
				vpush(objects.back()->get(scopes.back(), str));
				// stack: argN..arg0 ref fun
				
				// Copy args
				vector<vobject*> args;
				for (int k = 0; k < argc; ++k)
					args.push_back(objects.rbegin()[argc-k-1 + 2]);
				
				vobject* obj = call_object(objects.back(), objects.rbegin()[1], args, str);
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
				
				std::wstring str;
				read(str);
				
				
#ifdef DEBUG_OUTPUT
				wcout << "> CALL_NAME [" << argc << "] [" << str << ']' << endl;
#endif
				
				if (objects.size() < argc)
					throw StackCorruption(L"objects stack corrupted"); 
					
				validate_scope();
					
				vpush(scopes.back()->get(scopes.back(), str));
				// stack: argN..arg0 fun
				
				// Copy args
				vector<vobject*> args;
				for (int k = 0; k < argc; ++k)
					args.push_back(objects.rbegin()[argc-k-1 + 1]);
				
				vobject* obj = call_object(objects.rbegin()[0], scopes.back(), args, str);
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
				
				// ref <op> rref
				// ref.__opreator<op>
				// rref.__roperator<op>
				
				// reference
				vobject *ref = objects.rbegin()[1];
				// right reference
				vobject *rref = objects.rbegin()[0];
				vobject *fun = nullptr;
				
				wstring fun_name;
				
				// lvalue operator
				switch (i) {
					case ck_bytecodes::OPT_ADD      : fun_name = (L"+"); break;
					case ck_bytecodes::OPT_SUB      : fun_name = (L"-"); break;
					case ck_bytecodes::OPT_MUL      : fun_name = (L"*"); break;
					case ck_bytecodes::OPT_DIV      : fun_name = (L"/"); break;
					case ck_bytecodes::OPT_BITRSH   : fun_name = (L">>"); break;
					case ck_bytecodes::OPT_BITLSH   : fun_name = (L"<<"); break;
					case ck_bytecodes::OPT_BITURSH  : fun_name = (L">>>"); break;
					case ck_bytecodes::OPT_BITULSH  : fun_name = (L"<<<"); break;
					case ck_bytecodes::OPT_DIR      : fun_name = (L"\\\\"); break;
					case ck_bytecodes::OPT_PATH     : fun_name = (L"\\"); break;
					case ck_bytecodes::OPT_MOD      : fun_name = (L"%"); break;
					case ck_bytecodes::OPT_BITOR    : fun_name = (L"|"); break;
					case ck_bytecodes::OPT_BITAND   : fun_name = (L"&"); break;
					case ck_bytecodes::OPT_HASH     : fun_name = (L"#"); break;
					case ck_bytecodes::OPT_EQ       : fun_name = (L"=="); break;
					case ck_bytecodes::OPT_NEQ      : fun_name = (L"!="); break;
					case ck_bytecodes::OPT_LEQ      : fun_name = (L"==="); break;
					case ck_bytecodes::OPT_NLEQ     : fun_name = (L"!=="); break;
					case ck_bytecodes::OPT_OR       : fun_name = (L"||"); break;
					case ck_bytecodes::OPT_AND      : fun_name = (L"&&"); break;
					case ck_bytecodes::OPT_GT       : fun_name = (L">"); break;
					case ck_bytecodes::OPT_GE       : fun_name = (L">="); break;
					case ck_bytecodes::OPT_LT       : fun_name = (L"<"); break;
					case ck_bytecodes::OPT_LE       : fun_name = (L"<="); break;
					case ck_bytecodes::OPT_PUSH     : fun_name = (L"=>"); break;
					case ck_bytecodes::OPT_ARROW    : fun_name = (L"->"); break;
					case ck_bytecodes::OPT_BITXOR   : fun_name = (L"^"); break;
					case ck_bytecodes::OPT_ISTYPEOF : fun_name = (L"_istypeof"); break;
					case ck_bytecodes::OPT_AS       : fun_name = (L"_as"); break;
				}
				
				if (ref == nullptr)
					throw TypeError(L"undefined reference in operator " + fun_name);
				
				if (rref == nullptr)
					throw TypeError(L"undefined reference in operator rvalue " + fun_name);
				
#ifdef DEBUG_OUTPUT
				wcout << "> OPERATOR [" << fun_name << ']' << endl;
#endif
				
				// Get __operator<op> from left-side
				fun = ref->get(scopes.back(), L"__operator" + fun_name);
				
				// Check if it exists
				if (fun == nullptr || fun->as_type<Undefined>() || fun->as_type<Null>()) {
					// Try to call r-value operator
					fun = rref->get(scopes.back(), L"__roperator" + fun_name);
					
					if (fun == nullptr || fun->as_type<Undefined>() || fun->as_type<Null>()) 
						throw TypeError(L"undefined reference to operator " + fun_name);
					
					// Right-side operator has swapped arguments
					// aka: a + b -> __roperator+(b, a)
					vobject* res = call_object(fun, ref, { rref, ref }, fun_name); 
					
					vpop(); 
					vpop();
					vpush(res);
					
					break;
				} else {
					vobject* res = call_object(fun, ref, { ref, rref }, fun_name);
					
					vpop(); 
					vpop();
					vpush(res);
					
					break;
				}
			}
			
			case ck_bytecodes::STORE_VAR: {
				std::wstring str;
				read(str);
				
				// Check for scope
				validate_scope();		
				
				scopes.back()->put(str, vpop(), 1, 1);
				
#ifdef DEBUG_OUTPUT
				wcout << "> STORE_VAR: " << str << endl;
#endif
				break;
			}
			
			case ck_bytecodes::STORE_FIELD: {
				std::wstring str;
				read(str);
				
				vobject* val = vpop();
				vobject* ref = vpop();
				
				if (ref == nullptr)
					throw TypeError(L"undefined reference to " + wstring(str));
				
				// Check for scope
				validate_scope();		
				
				ref->put(scopes.back(), str, val);
				
#ifdef DEBUG_OUTPUT
				wcout << "> STORE_FIELD: " << str << endl;
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
				std::wstring str;
				read(str);
				
				vobject* ref = vpop();
				
				if (ref == nullptr)
					throw TypeError(L"undefined reference to " + wstring(str));
				
				// Check for scope
				validate_scope();		
				
				vpush(ref->get(scopes.back(), str));
				
#ifdef DEBUG_OUTPUT
				wcout << "> LOAD_FIELD: " << str << endl;
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
				
				wstring fun_name;
				
				// lvalue operator
				switch (i) {
					case ck_bytecodes::OPT_DOG   : fun_name = (L"@x");      break;
					case ck_bytecodes::OPT_NOT   : fun_name = (L"!x");      break;
					case ck_bytecodes::OPT_BITNOT: fun_name = (L"~x");      break;
					case ck_bytecodes::OPT_POS   : fun_name = (L"+x");      break;
					case ck_bytecodes::OPT_NEG   : fun_name = (L"-x");      break;
					case ck_bytecodes::OPT_INC   : fun_name = (L"++");      break;
					case ck_bytecodes::OPT_DEC   : fun_name = (L"--");      break;
					case ck_bytecodes::OPT_TYPEOF: fun_name = (L"_typeof"); break;
				}
				
				if (ref == nullptr)
					throw TypeError(L"undefined reference in operator " + fun_name);
				
#ifdef DEBUG_OUTPUT
				wcout << "> OPERATOR [" << fun_name << ']' << endl;
#endif
				
				fun = ref->get(scopes.back(), L"__operator" + fun_name);
				
				// no operator
				if (fun == nullptr || fun->as_type<Undefined>() || fun->as_type<Null>()) 
					throw TypeError(L"undefined reference to operator " + fun_name);
				
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
				if (scopes.size() == 0)
					throw StackCorruption(L"scopes stack corrupted");
				
				vscope* s = new iscope(scopes.back());
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
			
			case ck_bytecodes::JMP_IF_NOT_ZERO: {
				int i; 
				read(sizeof(int), &i);
				
#ifdef DEBUG_OUTPUT
				wcout << "> JMP_IF_NOT_ZERO [" << i << ']' << endl;
#endif
				
				vobject* o = vpop();
				if (o != nullptr && o->int_value() != 0)
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
				
				throw ObjectCake(Undefined::instance());
				
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
				std::wstring str;
				read(str);
				
#ifdef DEBUG_OUTPUT
				wcout << "> THROW_STRING: \"" << str << '"' << endl;
#endif
				
				throw ObjectCake(new String(str));
				
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
					std::wstring str;
					read(str);
					
					argn.push_back(str);
					
#ifdef DEBUG_OUTPUT
					wcout << str;
					
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
				restore_try_frame(try_stack.size() - 1);
				pointer = pointer_tmp;
				
#ifdef DEBUG_OUTPUT
				wcout << "> VSTATE_POP_TRY" << endl;
#endif
				
				// Return from containing try-catch
				return nullptr;
			}

			case ck_bytecodes::VSTATE_PUSH_TRY: {	
		
				// if (try_stack.size() == try_stack_limit)
				// 	throw StackOverflow(L"try stack overflow");
				
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
					
					read(handler_name);
					
#ifdef DEBUG_OUTPUT
					wcout << "> VSTATE_PUSH_TRY [TRY_WITH_ARG] (" << handler_name << ") [" << try_node << "] [" << catch_node << ']' << endl;
#endif
				}
				
				store_try_frame(handler_name);
				try_stack.back().try_type   = type;
				try_stack.back().catch_node = catch_node;
				
				// Limit rest of stack by 4 Mb
				if (ck_core::stack_locator::get_stack_remaining() < 4 * 1024 * 1024)
					throw StackOverflow(L"stack overflow");
				
				try {
					ck_vobject::vobject* result = exec_bytecode();
					
					GIL::current_thread()->clear_blocks();
					
					// Reached bytecode end
					if (result)
						return result;
					
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
				
				break;
			}
			
			case ck_bytecodes::PUSH_THIS: {
#ifdef DEBUG_OUTPUT
				wcout << "> PUSH_THIS" << endl;
#endif
				
				// Check for valid scope
				validate_scope();
				
				// Get __this value from enclosing scope.
				vpush(scopes.back()->get(L"__this", 1));
				
				break;
			}
			
			case ck_bytecodes::CONTAINS_KEY: {
				
				// .contains() must not lock the gil, or it will fall into object GC corruption.
				
				std::wstring str;
				read(str);
				
#ifdef DEBUG_OUTPUT
				wcout << "> CONTAINS_KEY [" << str << "]" << endl;
#endif			

				// Check for valid scope
				validate_scope();
				
				vobject* o = vpop(); // here
				
				if (!o) 
					vpush(Undefined::instance());
				else
					vpush(Bool::instance(o->contains(scopes.back(), str)));
				
				break;
			}
			
			default: throw IllegalStateError(L"invalid bytecode [" + to_wstring(scripts.back()->bytecode.bytemap[pointer-1]) + L"]");
		}
		
		// Respond to GIL requests

		GIL::instance()->accept_lock();

		// Perform GC collection ?
		GIL::gc_instance()->collect();
	}
	
	return nullptr;
};


void ck_executer::execute(ck_core::ck_script* scr, ck_vobject::vscope* scope, std::vector<std::wstring>* argn, std::vector<ck_vobject::vobject*>* argv) {
	
	// Limit rest of stack by 4 Mb
	if (ck_core::stack_locator::get_stack_remaining() < 4 * 1024 * 1024)
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
	
	// Push call frame and mark own scope
	store_window_frame(L"", own_scope);
	
	// Push scope
	scopes.push_back(scope);
	
	// Save expected call id
	int window_id = window_stack.size() - 1;
	
	// Push script instance to the bottom
	scripts.push_back(scr);
	
	// Reset pointer to 0 and start
	pointer = 0;
	
	// Do some useless shit again
	exec_bytecode();
	GIL::current_thread()->clear_blocks();
	
	// Try to restore scope
	restore_window_frame(window_id);
	
	return;
};

ck_vobject::vobject* ck_executer::call_bytecode(ck_core::ck_script* scr, ck_vobject::vobject* ref, const std::vector<ck_vobject::vobject*>& args, const std::wstring& name, vscope* scope, bool use_scope_without_wrap, bool return_non_null) { 

	// Limit rest of stack by 4 Mb
	if (ck_core::stack_locator::get_stack_remaining() < 4 * 1024 * 1024)
		throw StackOverflow(L"stack overflow");

	if (scr == nullptr)
		throw TypeError(L"undefined call to " + name);
	
	// Construct scope
	// vscope* scope = nullptr;
	bool own_scope = 0;
	bool scope_is_root = scope && scope->gc_is_root();
	
	// Pass given scope as proxy to avoid overwritting of __this value.
	if (!scope && !use_scope_without_wrap) {
		if (scope)
			scope = new xscope(scope);
		else if (scopes.size() == 0)
			throw StackCorruption(L"scopes stack corrupted");
		else
			scope = new iscope(scopes.back());
	}
	
	// Apply root
	if (!scope_is_root) {
		scope->root();
		own_scope = 1;
	}
	
	// Apply __this bind
	if (ref != nullptr)
		scope->put(L"__this", ref);

	// Push call frame and mark own scope
	store_call_frame(name, own_scope);
	
	// Push scope
	scopes.push_back(scope);
	
	// Apply script
	scripts.push_back(scr);
	
	// Save expected call id
	int call_id = call_stack.size() - 1;
	
	// Do some useless shit
	
	// Reset pointer to 0 and start
	goto_address(0);
	
	// This long loop without any protection of the objects is thread-safe because 
	//  other threads can not call GC
	ck_vobject::vobject* obj = exec_bytecode();
	
	GIL::current_thread()->clear_blocks();
	
	// Try to restore frame
	restore_call_frame(call_id);
	
	// scopes.pop_back();
	
#ifdef DEBUG_OUTPUT
	if (obj) 
		wcout << "RETURNED: " << obj->string_value() << endl;
	else
		wcout << "RETURNED: " << "NULL" << endl;
#endif

	if (!obj && return_non_null)
		return Undefined::instance();

	return obj;
};

ck_vobject::vobject* ck_executer::call_object(ck_vobject::vobject* obj, ck_vobject::vobject* ref, const std::vector<ck_vobject::vobject*>& args, const std::wstring& name, vscope* scope, bool use_scope_without_wrap, bool return_non_null) { 

	// Limit rest of stack by 4 Mb
	if (ck_core::stack_locator::get_stack_remaining() < 4 * 1024 * 1024)
		throw StackOverflow(L"stack overflow");

	if (obj == nullptr)
		throw TypeError(L"undefined call to " + name);
	
	// Construct scope
	// vscope* scope = nullptr;
	bool own_scope = 0;
	bool scope_is_root = scope && scope->gc_is_root();
	
	if (obj->as_type<BytecodeFunction>()) {
		// Apply new scope
		if (!scope || !use_scope_without_wrap) {
			BytecodeFunction* f = (BytecodeFunction*) obj;
			
			// Write binded __this reference if it doesnt exist
			ref = f->get_bind() ? f->get_bind() : ref;
			
			// Apply this & args on scope
			scope = f->apply(ref, args); // XXX: Remove apply and use something else.
		}
	} else {
		// Pass given scope as proxy to avoid overwritting of __this value.
		if (!scope && !use_scope_without_wrap) {
			if (scope)
				scope = new xscope(scope);
			else if (scopes.size() == 0)
				throw StackCorruption(L"scopes stack corrupted");
			else
				scope = new iscope(scopes.back());
		}
	}
	
	// Apply root
	if (!scope_is_root) {
		scope->root();
		own_scope = 1;
	}
	
	// Apply __this bind
	if (ref != nullptr)
		scope->put(L"__this", ref);

	// Push call frame and mark own scope
	store_call_frame(name, own_scope);
	
	// Push scope
	scopes.push_back(scope);
	
	// Apply script
	if (obj->as_type<BytecodeFunction>())
		scripts.push_back(((BytecodeFunction*) obj)->get_script());
	
	// Save expected call id
	int call_id = call_stack.size() - 1;
	
	// Do some useless shit
	
	if (obj->as_type<BytecodeFunction>()) {
		// Reset pointer to 0 and start
		goto_address(0);
		
		// This long loop without any protection of the objects is thread-safe because 
		//  other threads can not call GC
		obj = exec_bytecode();
		
		GIL::current_thread()->clear_blocks();
	} else if (obj->as_type<NativeFunction>()) {
		// Do some useless shit
		obj = ((NativeFunction*) obj)->get_call_wrapper()(scope, args);
		GIL::current_thread()->clear_blocks();
	} else {
		obj = obj->call(scope, args);
		GIL::current_thread()->clear_blocks();
	}
	
	// Try to restore frame
	restore_call_frame(call_id);
	
	// scopes.pop_back();
	
#ifdef DEBUG_OUTPUT
	if (obj) 
		wcout << "RETURNED: " << obj->string_value() << endl;
	else
		wcout << "RETURNED: " << "NULL" << endl;
#endif

	if (!obj && return_non_null)
		return Undefined::instance();

	return obj;
};

void ck_executer::late_call_object(ck_vobject::vobject* obj, ck_vobject::vobject* ref, const std::vector<ck_vobject::vobject*>& args, const std::wstring& name, vscope* exec_scope, bool use_scope_without_wrap) { 
	late_call_instance instance;
	instance.obj = obj;
	instance.ref = ref;
	instance.name = name;
	instance.args = args;
	instance.scope = exec_scope;
	instance.use_scope_without_wrap = use_scope_without_wrap;
	
	// Record root ownership
	if (obj && !obj->gc_is_root()) {
		obj->gc_make_root();
		instance.own_obj = 1;
	} else
		instance.own_obj = 0;
	
	if (ref && !ref->gc_is_root()) {
		ref->gc_make_root();
		instance.own_ref = 1;
	} else
		instance.own_ref = 0;
	
	instance.own_args.resize(args.size());
	for (int i = 0; i < args.size(); ++i) {	
		if (args[i] && !args[i]->gc_is_root()) {
			args[i]->gc_make_root();
			instance.own_args[i] = 1;
		} else
			instance.own_args[i] = 0;
	}
	
	late_call.push_back(instance);
};

void ck_executer::goto_address(int bytecode_address) {
	if (bytecode_address < 0 || bytecode_address > scripts.back()->bytecode.bytemap.size())
		throw IllegalStateError(L"goto out of range [" + to_wstring(bytecode_address) + L"] for range [0, " + to_wstring(scripts.back()->bytecode.bytemap.size()) + L"]");
	
	pointer = bytecode_address;
};

std::vector<ck_exceptions::BacktraceFrame> ck_executer::collect_backtrace() {
	std::vector<ck_exceptions::BacktraceFrame> backtrace;
	
	// Append appended frames
	backtrace.insert(backtrace.end(), appended_backtrace.begin(), appended_backtrace.end());
	
	// Other frames
	for (int i = 0; i < call_stack.size(); ++i) {		
		int pointer = call_stack[i].pointer;
		int script_id = call_stack[i].script_id;
		int lineno = 0;
		if (script_id >= scripts.size())
			continue;
		
		backtrace.push_back(ck_exceptions::BacktraceFrame());
		
		ck_script* script = scripts[script_id];
		
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
		if (i > 0)
			backtrace.back().function = call_stack[i - 1].name;		
	}
	
	// Last frame
	backtrace.push_back(ck_exceptions::BacktraceFrame());
	backtrace.back().lineno   = lineno();
	backtrace.back().filename = scripts.back()->filename;
	if (call_stack.size())
		backtrace.back().function = call_stack.back().name;
	
	return backtrace;
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