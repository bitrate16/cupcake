#pragma once

#include <vector>
#include <string>

#include "GC.h"
#include "GIL2.h"
#include "exceptions.h"
#include "stack_locator.h"

namespace ck_vobject {
	class vobject;
	class vscope;
};

// Allow both classes collect backtrace of this executer.
namespace ck_objects {
	class Cake;
};

namespace ck_exceptions {
	class cake;
};

namespace ck_core {
	
	// Single execution stack frame.
	struct stack_frame {		
		// Set to 1 if call owns this scope.
		bool own_scope = -1;
		
		// Index of last scope in the executer scopes array.
		int scope_id = -1;
		
		// Index of script that this frame refers to.
		int script_id = -1;
		
		// Index oflast vobject on object stack
		int object_id = -1;
		
		// Index of last stack_try frame.
		int try_id = -1;
		
		// Index of last stack_frame frame.
		int call_id = -1;
		
		// Index of enclosing window
		int window_id = -1;
		
		// Address of next command
		int pointer = -1;
		
		// Type of storen try/catch
		char try_type = -1;
		
		// Address of catch node in try/catch
		int catch_node = -1;
		
		// Name of the function
		std::wstring name;
	};
	
	// Item representing information about late_call_object
	struct late_call_instance {
		// Object to be called
		ck_vobject::vobject* obj;
		
		// Referencing object
		ck_vobject::vobject* ref;
		
		// Calling scope
		ck_vobject::vscope* scope;
		
		// Funciton name
		std::wstring name;
		
		// Arguments list
		std::vector<ck_vobject::vobject*> args;
		
		// Boolean flag for using scope
		bool use_scope_without_wrap;
		
		// Boolean flags recording information about ownership for passed objects.
		// Used to mark all of them as root to prevent deletion in unsafe area of execution.
		bool own_obj;
		bool own_ref;
		std::vector<bool> own_args;
	};
	
	// Performs marking all ck_executer objects in current thread on each GC step.
	// Used to prevent objects deletion.
	class ck_executer;
	class ck_executer_gc_object : public gc_object {
		ck_executer* exec_instance;
		
	public:
		
		ck_executer_gc_object(ck_executer* exec_instance);
		~ck_executer_gc_object();
		
		void gc_mark();
		void gc_finalize();
	};
	
	class ck_script;
	class ck_executer {
		
		// Pre-defined constants
		
		// Allow exceptions collect backtrace
		friend class ck_objects::Cake;
		friend class ck_exceptions::cake;
		
		// Allow GC object mark objects on stack
		friend class ck_executer_gc_object;
		
		ck_executer_gc_object* gc_marker;
		
		// List of late call function instances.
		// Each time the execute_bytecode() starts new loop step, 
		//  late_call list being checked for the next instance to execute.
		// After execution of the instance, it will be popped out of the list.
		// During GC cycle values in this list is being marked by gc_marker.
		std::vector<late_call_instance> late_call;
	
		std::vector<ck_core::ck_script*>  scripts;
		std::vector<ck_vobject::vscope*>  scopes;
		std::vector<ck_vobject::vobject*> objects;
		
		// Id's of stacks		
		const int call_stack_id    = 13;
		const int try_stack_id     = 19;
		const int window_stack_id  = 29;
		
		// New frame being inserted on object_call()
		std::vector<stack_frame> call_stack;
		// New frame being inserted on try{}catch(){}
		std::vector<stack_frame> try_stack;
		// New frame being inserted on execute()
		std::vector<stack_frame> window_stack;
		
		// Appended backtrace, used on threads hierarchy
		std::vector<ck_exceptions::BacktraceFrame> appended_backtrace;
		
		// Points to the current command address in bytemap.
		int pointer = 0;
		
		// Reads byte block from bytemap.
		bool read(int size, void* ptr);
		
		// Reads string of given size from bytecode.
		bool read(int size, std::wstring& str);
		
		// Reads string from bytecode. 
		// Must presenve size ang string contents.
		// [size : int]
		// [string : bytearray]
		bool read(std::wstring& str);
		
		// Performs bytecode execution in a loop.
		// Returns value of RETURN bytecode
		//  or nullptr if nothing returned or nothing should be returned.
		ck_vobject::vobject* exec_bytecode();
		
		// Checks if execution reached end or BCEND.
		bool is_eof();
		
		// Objects stack manipulation
		
		// Pop stack [top] or throw stack_corruption
		ck_vobject::vobject* vpop();
		
		// Returns [top] witout pop
		ck_vobject::vobject* vpeek();
		
		// Push stack [top]
		void vpush(ck_vobject::vobject*);
		
		// Swap [top] and [top-1] or throw stack_corruption
		void vswap();
		
		// Swap [top-1] and [top-2] or throw stack_corruption
		void vswap1();
		
		// Swap [top-2] and [top-3] or throw stack_corruption
		void vswap2();
		
		// if (scopes.size() == 0 || scopes.back() == nullptr)
		//	throw StackCorrupted();		
		inline void validate_scope() {
			if (scopes.size() == 0 || scopes.back() == nullptr)
				throw ck_exceptions::StackCorruption(L"scopes stack corrupted");	
		};
		
		// peek closest try_frame and follow it's catch block by jumping on it.
		// If no frames in current script left, rethrow message up.
		void follow_exception(const ck_exceptions::cake& msg);
		
		// Store state of different frame types
		// Try/catch does not push the scope
		inline void store_try_frame(const std::wstring& handler_name) {
			stack_frame frame;
			frame.window_id = window_stack.size() - 1;
			frame.try_id    = try_stack.size()    - 1;
			frame.call_id   = call_stack.size()   - 1;
			frame.script_id = scripts.size()      - 1;
			frame.scope_id  = scopes.size()       - 1;
			frame.object_id = objects.size()      - 1;
			frame.own_scope = 0;
			frame.name      = handler_name;
			frame.pointer   = pointer;
			
			try_stack.push_back(frame);
		};
		
		inline void store_call_frame(const std::wstring& name, bool own_scope) {
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
			
			call_stack.push_back(frame);
		};
		
		inline void store_window_frame(const std::wstring& name, bool own_scope) {
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
			
			window_stack.push_back(frame);
		};
		
		// Restore state of different frame types
		// During execution, multiple scopes can be created on stack.
		// Scopes stack will have the following layout:
		//
		// |               |
		// |    scopeN     |        
		// |      ..       |    <-- Current call
		// |    scope1     |         
		// ----------------|                             
		// |      ..       |  
		// |               |    <-- Previous calls
		// |               |  
		// During call (store_stack) scope1 is the scope that is pushed by the call.
		// scope2-scopeN is scopes pushed by the VSTATE_PUSH_SCOPE.
		//  They all are marked as root and must be disposed by .unroot() call.
		void restore_try_frame(int restored_frame_id);
		
		void restore_call_frame(int restored_frame_id);
		
		void restore_window_frame(int restored_frame_id);
		
	public:
		
		// Restore to empty state.
		// Reatore all try_frame, call_frame, window_frame, deattach all scopes.
		void restore_all();
		
		ck_executer();
		
		~ck_executer();
		
		// Returns line number of pointer to command
		int lineno();
		
		// Returns current script
		inline ck_core::ck_script* get_script() {
			if (scripts.size())
				return scripts.back();
			return nullptr;
		};
		
		// Returns current scope
		inline ck_vobject::vscope* get_scope() {
			if (scopes.size())
				return scopes.back();
			return nullptr;
		};
		
		// Returns amount of pending late_call functions
		inline int late_call_size() { 
			return late_call.size(); 
		};
		
		// Executes passed script by allocating new stack frame.
		void execute(ck_core::ck_script* scr);
		
		// Executes passed script by allocating new stack frame.
		// If argv is non empty, it will be appended to the scope.
		// If scope is not null, it will be used as the main scope
		void execute(ck_core::ck_script* scr, ck_vobject::vscope* scope = nullptr, std::vector<std::wstring>* argn = nullptr, std::vector<ck_vobject::vobject*>* argv = nullptr);
		
		// Allows executing bytecode as a function.
		// Expected only two branches: 
		// 1. obj is typeof native_function and supports direct call
		// 2. obj is any other type and does not support direct call. Then obj.::vobject::call() is called.
		// Ref will be assigned to scope::self
		// Name is used in traceback (empty for none)
		// use_scope_without_wrap. If set to 1, passed scope will be used as execution scope without proxy, wrapping and applying function arguments.
		//  If 0, passed scope is used as regular.
		//  Scope has to be non-null, or scope will be calculated as regular for call.
		// return_non_null - If 1, if result is null, returns Undefined.
		//  If 0, returns null.
		ck_vobject::vobject* call_bytecode(ck_core::ck_script* scr, ck_vobject::vobject* ref, const std::vector<ck_vobject::vobject*>&, const std::wstring& name, ck_vobject::vscope* scope = nullptr, bool use_scope_without_wrap = 0, bool return_non_null = 1);
		
		// Allows executing object as a function.
		// Expected only two branches: 
		// 1. obj is typeof native_function and supports direct call
		// 2. obj is any other type and does not support direct call. Then obj.::vobject::call() is called.
		// Ref will be assigned to scope::self
		// Name is used in traceback (empty for none)
		// use_scope_without_wrap. If set to 1, passed scope will be used as execution scope without proxy, wrapping and applying function arguments.
		//  If 0, passed scope is used as regular.
		//  Scope has to be non-null, or scope will be calculated as regular for call.
		// return_non_null - If 1, if result is null, returns Undefined.
		//  If 0, returns null.
		ck_vobject::vobject* call_object(ck_vobject::vobject* obj, ck_vobject::vobject* ref, const std::vector<ck_vobject::vobject*>&, const std::wstring& name, ck_vobject::vscope* scope = nullptr, bool use_scope_without_wrap = 0, bool return_non_null = 1);
		
		// Performing late object call by execution passed function on the next executer step.
		// By default, all objects passed to this function will be marked as root objects to prevent their colleciton on GC.
		void late_call_object(ck_vobject::vobject* obj, ck_vobject::vobject* ref, const std::vector<ck_vobject::vobject*>& args, const std::wstring& name, ck_vobject::vscope* scope = nullptr, bool use_scope_without_wrap = 0);
		
		// Jumps on the address of bytecode map
		void goto_address(int bytecode_address);
		
		// Collectes existing backtrace
		std::vector<ck_exceptions::BacktraceFrame> collect_backtrace();
		
		inline void append_backtrace(const std::vector<ck_exceptions::BacktraceFrame>& bt) {
			appended_backtrace = bt;
		};
		
		inline const std::vector<ck_exceptions::BacktraceFrame>& get_appended_backtrace() {
			return appended_backtrace;
		};
		
		// Dangerous. Erase all data, stacks and pointers absolutely deallocating all infomation.
		//  Use only when processing high-level elevated exceptions.
		void clear();
	};
};