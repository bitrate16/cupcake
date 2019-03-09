#pragma once

#include <vector>
#include <string>

#include "GC.h"

namespace ck_vobject {
	class vobject;
	class vscope;
};

namespace ck_core {
	
	// Single execution stack frame.
	struct stack_frame {
		
		stack_frame() {};
		
		// Index of last scope in the executer scopes array.
		int scope_id = -1;
		// Index of script that this frame refers to.
		int script_id = -1;
		// Index oflast vobject on object stack
		int object_id = -1;
		// Index of enclosing window
		int window_id = -1;
		// Address of next command
		int pointer = -1;
	};
	
	// Single execution stack frame.
	struct stack_try {
		
		stack_try() {};
		
		// Index of last scope in the executer scopes array.
		int scope_id = -1;
		// Index of script that this frame refers to.
		int script_id = -1;
		// Index oflast vobject on object stack
		int object_id = -1;
		// Index of enclosing window
		int window_id = -1;
		// Address of next command
		int pointer = -1;
		// Address of try_node
		int try_node = -1;
		// Address of catch node
		int catch_node = -1;
		// Type of try/catch statement.
		int type;
		// Handler variable name.
		std::wstring handler;
	};
	
	// Single execution script frame.
	struct stack_window {
		
		stack_window() {};
		
		// Index of last scope in the executer scopes array.
		int scope_id = -1;
		// Index of script that this frame refers to.
		int script_id = -1;
		// Index of last stack_try frame.
		int try_id = -1;
		// Index of last stack_frame frame.
		int call_id = -1;
		// Index oflast vobject on object stack
		int object_id = -1;
		// Index of enclosing window
		int window_id = -1;
		// Address of next command
		int pointer = -1;
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
		
		friend class ck_executer_gc_object;
		
		ck_executer_gc_object* gc_marker;
	
		std::vector<ck_core::ck_script*>  scripts;
		std::vector<ck_vobject::vscope*>  scopes;
		std::vector<ck_vobject::vobject*> objects;
		
		// Stack for each functional call (on call_object)
		std::vector<stack_frame> call_stack;
		// Stack for each try/catch
		std::vector<stack_try>    try_stack;
		// Stack for each script instance to split different instances (on execute)
		// ???: maybe merge with call_stack?
		std::vector<stack_window>   windows;
		
		// Points to the current command address.
		int pointer = 0;
		
		bool read(int size, void* ptr);
		void exec_bytecode();
		bool is_eof();
		
		// Objects stack manipulation
		
		// Pop stack [top] or throw stack_corruption
		inline ck_vobject::vobject* vpop();
		
		// Returns [top] witout pop
		inline ck_vobject::vobject* vpeek();
		
		// Push stack [top]
		// XXX: Overflow check
		inline void vpush(ck_vobject::vobject*);
		
		// Swap [top] and [top-1] or throw stack_corruption
		inline void vswap();
		
		// Swap [top-1] and [top-2] or throw stack_corruption
		inline void vswap1();
		
		// Swap [top-2] and [top-3] or throw stack_corruption
		inline void vswap2();
		
		// Returns line number of pointer to command
		int lineno();
		
		// if (scopes.size() == 0 || scopes.back() == nullptr)
		//	throw ck_message(ck_message_type::CK_STACK_CORRUPTED);		
		inline void validate_scope();
		
	public:
		
		ck_executer();
		~ck_executer();
		
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
		ck_vobject::vobject* call_object(ck_vobject::vobject* obj);
		
		// Jumps on the address of bytecode map
		void goto_address(int bytecode_address);
		
		// Dangerous. Erase all data, stacks and pointers absolutely deallocating all infomation.
		//  Use only when processing high-level elevated exceptions.
		void clear();
	};
};