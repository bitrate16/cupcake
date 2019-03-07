#pragma once

#include <string>

#include "translator.h"


namespace ck_core {
	
	// Script is attached to a single file
	struct script {
		// Directory specifications
		std::wstring directory;
		std::wstring filename;
		
		// bytecode tha will be executed
		ck_translator::ck_bytecode bytecode;
		
		// Other information needed to restore previous contexts
		// ...
	};
};