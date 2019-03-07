#pragma once

#include "translator.h"
#include "vobject.h"
#include "script.h"


namespace ck_core {
	
	class executer {
	
		std::vector<ck_core::script> scripts;
		std::vector<ck_vobject::vscope*> scopes;
		
	public:
		
		executer();
		~executer();
		
		// Executes passed bytecode.
		// new script is being constructed from old script by taking it's filename and directory path
		void execute(ck_translator::ck_bytecode* bytecode);
		
		// Executes passed bytecode.
		// new script is being constructed from given directory path and filename
		void execute(ck_translator::ck_bytecode* bytecode, std::wstring directory, std::wstring filename);
		
		// Allows executing object as a function.
		// Expected only two branches: 
		// 1. obj is typeof native_function and supports direct call
		// 2. obj is any other type and does not support direct call. Then obj.::vobject::call() is called.
		void call_object(ck_vobject::vobject* obj);
		
		// Jumps on the address of bytecode map
		void goto_address(int bytecode_address);
	};
};