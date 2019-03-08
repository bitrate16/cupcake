#pragma once

#include <string>

#include "sfile.h"
#include "translator.h"


namespace ck_core {

	// Script is attached to a single file
	struct ck_script {
		
		ck_script() {};
		
		// Store information about execution directory (absolute path)
		//  and a file name that this script belongs to.
		// Will be used for loading subscripts and browsing local to script directory. 
		//  Better to use sfile.
		ck_sfile::sfile directory;
		
		// Used for debug printing, better to use string
		std::wstring filename;
		
		// bytecode tha will be executed
		ck_translator::ck_bytecode bytecode;
	};
};