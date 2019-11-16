#pragma once

#include <vector>
#include <string>

#include "GC.h"
#include "exceptions.h"

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
		
		stack_frame() {};
		
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
		ck_vobject::vobject* obj;
		ck_vobject::vobject* ref;
		ck_vobject::vscope* scope;
		std::wstring name;
		std::vector<ck_vobject::vobject*> args;
	};
	
	// Performs marking all ck_executer objects in current thread onn each GC step.
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
		// Allow gc object mark objects on stack
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
		
		// Limit size of summary execution staks:
		//  sizeof(call_stack) + sizeof(window_stack) < execution_stack_limit
		int execution_stack_limit = 0; // XXX: Calculate dynamically, depending on stack size
		// int try_stack_limit       = 0; // unused due no limits on exceptions
		
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
		
		// Points to the current command address.
		int pointer = 0;
		
		// Reads object from bytemap.
		bool read(int size, void* ptr);
		
		// Performs bytecode execution in a loop.
		// Returns value of RETURN bytecode
		//  or nullptr if nothing returned or nothing should be returned.
		inline ck_vobject::vobject* exec_bytecode();
		
		// Checks if execution reached end or BCEND.
		bool is_eof();
		
		// Objects stack manipulation
		
		// Pop stack [top] or throw stack_corruption
		inline ck_vobject::vobject* vpop();
		
		// Returns [top] witout pop
		inline ck_vobject::vobject* vpeek();
		
		// Push stack [top]
		inline void vpush(ck_vobject::vobject*);
		
		// Swap [top] and [top-1] or throw stack_corruption
		inline void vswap();
		
		// Swap [top-1] and [top-2] or throw stack_corruption
		inline void vswap1();
		
		// Swap [top-2] and [top-3] or throw stack_corruption
		inline void vswap2();
		
		// if (scopes.size() == 0 || scopes.back() == nullptr)
		//	throw StackCorrupted();		
		inline void validate_scope();
		
		// peek closest try_frame and follow it's catch block by jumping on it.
		// If no frames in current script left, rethrow message up.
		inline void follow_exception(const ck_exceptions::cake& msg);
		
		// Store stack frame
		void store_frame(std::vector<stack_frame>& stack, int stack_id, const std::wstring& name, bool own_scope);
		
		// Restore stack frame.
		//  Restored_frame_id used to prevent frame stack corruption 
		//   by multiple calls to restore same frame.
		void restore_frame(std::vector<stack_frame>& stack, int stack_id, int restored_frame_id);
		
	public:
		
		ck_executer();
		~ck_executer();
		
		// Expected max stack size used for single recursion call
		static const int def_stack_frame_size = 4096; // In real it is about 2048 bytes, but assuming that additional recursive functions require more than 2048 bytes
		
		// Forcibly recalculate stack limits.
		// Can be used by external libraries to force this executer to 
		//  recalculate thread stack limits after using set_stack_size.
		void calculate_stack_limits();
		
		// Returns limit for current stack size
		inline int get_stack_size_limit() { return execution_stack_limit; };
		
		// Returns currently used stack space
		inline int get_stack_usage() { return call_stack.size() + window_stack.size(); };
		
		// Returns line number of pointer to command
		int lineno();
		
		// Returns current script
		inline ck_core::ck_script* get_script() {
			return scripts.back();
		};
		
		// Returns current scope
		inline ck_vobject::vscope* get_scope() {
			return scopes.back();
		};
		
		// Returns amount of pending late_call functions
		inline int late_call_size() { return late_call.size(); };
		
		// Executes passed script by allocating new stack frame.
		void execute(ck_core::ck_script* scr);
		
		// Executes passed script by allocating new stack frame.
		// If argv is non empty, it will be appended to the scope.
		// If scope is not null, it will be used as the main scope
		void execute(ck_core::ck_script* scr, ck_vobject::vscope* scope = nullptr, std::vector<std::wstring>* argn = nullptr, std::vector<ck_vobject::vobject*>* argv = nullptr);
		
		// Allows executing object as a function.
		// Expected only two branches: 
		// 1. obj is typeof native_function and supports direct call
		// 2. obj is any other type and does not support direct call. Then obj.::vobject::call() is called.
		// Ref will be assigned to scope::self
		// Name is used in traceback (empty for none)
		ck_vobject::vobject* call_object(ck_vobject::vobject* obj, ck_vobject::vobject* ref, const std::vector<ck_vobject::vobject*>&, const std::wstring& name, ck_vobject::vscope* scope = nullptr);
		
		// Performing late object call by execution passed function on the next executer step
		void late_call_object(ck_vobject::vobject* obj, ck_vobject::vobject* ref, const std::vector<ck_vobject::vobject*>&, const std::wstring& name, ck_vobject::vscope* scope = nullptr);
		
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