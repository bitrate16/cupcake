#pragma once

#include <vector>
#include <string>

#include "translator.h"
#include "vobject.h"
#include "script.h"


namespace ck_core {
	
	// Single execution stack frame.
	struct stack_frame {
		// Index of last scope in the executer scopes array.
		int scope_id = -1;
		// Index of script that this frame refers to.
		int script_id = -1;
		// Address of next command
		int pointer = -1;
	};
	
	// Single execution stack frame.
	struct stack_try {
		// Index of last scope in the executer scopes array.
		int scope_id = -1;
		// Index of script that this frame refers to.
		int script_id = -1;
		// Address of next command
		int pointer = -1;
		// Type of try/catch statement.
		int type;
		// Handler variable name.
		std::wstring handler;
	};
	
	class ck_executer {
	
		std::vector<ck_core::ck_script*> scripts;
		std::vector<ck_vobject::vscope*> scopes;
		
		// Stack of all function calls including 
		std::vector<stack_frame> call_stack;
		std::vector<stack_try> try_stack;
		
		// Points to the current command address.
		int pointer = 0;
		
	public:
		
		ck_executer();
		~ck_executer();
		
		// Executes passed script by allocating new stack frame.
		void execute(ck_core::ck_script* scr);
		
		// Executes passed script by allocating new stack frame.
		// argn would be passed to new created scope with value of argv
		void execute(ck_core::ck_script* scr, std::wstring argn, ck_vobject::vobject* argv);
		
		// Allows executing object as a function.
		// Expected only two branches: 
		// 1. obj is typeof native_function and supports direct call
		// 2. obj is any other type and does not support direct call. Then obj.::vobject::call() is called.
		void call_object(ck_vobject::vobject* obj);
		
		// Jumps on the address of bytecode map
		void goto_address(int bytecode_address);
	};
};