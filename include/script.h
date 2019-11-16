#pragma once

#include <string>

#include "sfile.h"
#include "translator.h"


namespace ck_core {

	// Script is attached to a single file
	struct ck_script {
		
		ck_script() {};
		
		// XXX: Use single durectory path for file and all functions that was created inside it.
		//       For example, can use clever pointer for reference counting & creating only one 
		//        copy of path between multiple functions from one file.
		// Store information about execution directory (absolute path)
		//  and a file name that this script belongs to.
		// Will be used for loading subscripts and browsing local to script directory. 
		//  Better to use sfile.
		// Each function and each file both store directory path.
		//  So, trying to change sontext script of a function will change path only for function.
		ck_sfile::sfile directory;
		
		// Used for debug printing, better to use string
		std::wstring filename;
		
		// bytecode tha will be executed
		ck_translator::ck_bytecode bytecode;
	};
};