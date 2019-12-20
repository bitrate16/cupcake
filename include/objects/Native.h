#pragma once

#include <vector>
#include <string>

#include "Object.h"

namespace ck_objects {	
	
	// Prototype for load function
	// load(scope, args);
	typedef void (*_native_load_func) (ck_vobject::vscope*, const std::vector<ck_vobject::vobject*>&);

	// Prototype for unload function
	// unload();
	typedef void (*_native_unload_func) ();

	struct Module {
		// Path to module file
		std::wstring file;
		// Instance of loaded native
		void* module;
	};

	class Native : public ck_objects::Object {
	
		// Static container.
		// Modules are inserted by calling load().
		// Modules are disposed on exit_listener strike.
		static std::vector<Module> modules;
	
	public:
		
		Native();
		virtual ~Native();
		
		virtual vobject* call(ck_vobject::vscope*, const std::vector<vobject*>&);
		
		// Loads single module from file.
		// Returns 1 on success.
		static bool load(const std::wstring& filename, ck_vobject::vscope*, const std::vector<ck_vobject::vobject*>&);
		
		// Must return integer representation of an object
		virtual int64_t int_value();
		
		// Must return string representation of an object
		virtual std::wstring string_value();
		
		// Called on interpreter start to initialize prototype
		static vobject* create_proto();
	};
	
	// Defined on interpreter start.
	static Native* NativeProto = nullptr;
};