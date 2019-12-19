#include "objects/Native.h"

#include <string>
#include <sstream>
#include <dlfcn.h>
#include <typeinfo>
#include <cstdlib>

#include "exceptions.h"
#include "GIL2.h"
#include "executer.h"
#include "script.h"

#include "objects/Object.h"
#include "objects/Native.h"
#include "objects/Bool.h"
#include "objects/Int.h"
#include "objects/NativeFunction.h"
#include "objects/Undefined.h"
#include "objects/String.h"

#include "exit_listener.h"

using namespace std;
using namespace ck_exceptions;
using namespace ck_vobject;
using namespace ck_objects;
using namespace ck_core;

std::vector<Module> Native::modules;

vobject* Native::create_proto() {
	if (NativeProto != nullptr)
		return NativeProto;
	
	NativeProto = new Native();
	GIL::gc_instance()->attach_root(NativeProto);
	
	NativeProto->Object::put(L"__typename", new String(L"Native"));
	
	// load(filename, { arguments })
	NativeProto->Object::put(L"load", new NativeFunction(
		[](vscope* scope, const std::vector<vobject*>& args) -> vobject* {
			if (args.size() == 0 && args[0])
				return Bool::False();
			
			std::vector<vobject*> args_crop(args.begin() + 1, args.end());
			
			return Bool::instance(Native::load(args[0]->string_value(), scope, args_crop));
		}));
	
	// Bing exit hook
	ck_core::ck_exit_listener::subscribe([](){
		for (int i = 0; i < Native::modules.size(); ++i) {
			void *module = Native::modules[i].module;
			void *sym    = dlsym(module, "unload");
			void *error  = dlerror();
			if (error) 
				dlclose(module);
			
			try {
				_native_unload_func onunload_f = (_native_unload_func) sym;
				
				if (!onunload_f) {
					dlclose(module);
					continue;
				}
				
				onunload_f();
			} catch(...) {}
			
			dlclose(module);
		}
	});
	
	return NativeProto;
};


Native::Native() {
	if (NativeProto != nullptr)
		throw ck_exceptions::StateError(L"Native instance duplicate");
};

Native::~Native() {};

vobject* Native::call(vscope* scope, const vector<vobject*>& args) {
	throw UnsupportedOperation(L"Native is not callable");
};

bool Native::load(const std::wstring& filename, ck_vobject::vscope* scope, const std::vector<ck_vobject::vobject*>& args) {
	NativeProto->lock();
	
	// Create only if have executer, script and current called path.
	ck_core::ck_executer* exec = GIL::executer_instance();
	if (!exec) {
		NativeProto->unlock();
		return 0;
	}
	
	ck_core::ck_script* scr = exec->get_script();
	if (!scr) {
		NativeProto->unlock();
		return 0;
	}
	
	ck_sfile::sfile object_path(scr->directory, filename);
	std::wstring abs_file = object_path.to_string();
	
	// Convert to std::string
	// std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> convert;
	std::string abs_file_c(abs_file.begin(), abs_file.end()); // = convert.to_bytes(abs_file);
	
	// Read module to buffer
	void *module = dlopen(abs_file_c.c_str(), RTLD_NOW | RTLD_GLOBAL);
	
	if (!module) {
		NativeProto->unlock();
		return 0;
	}
	
	void *sym = dlsym(module, "load");
	char *error = dlerror();
	if (error) {
		NativeProto->unlock();
		return 0;
	}
	
	try {
        _native_load_func onload_f = (_native_load_func) sym;
		
		if (!onload_f) {
			dlclose(module);
			NativeProto->unlock();
			return 0;
		}
		
		// Try to perform load
		onload_f(scope, args);
		
		Module nmodule;
		nmodule.file   = filename;
		nmodule.module = module;
		
		modules.push_back(nmodule);
    } catch(...) {
		dlclose(module);
		NativeProto->unlock();
        return 0;
    }
	
	NativeProto->unlock();
	return 1;
};
		
// Returns size of loaded modules + 1
long long Native::int_value() { 
	return modules.size() + 1;
};

// Returns string names of loaded module names
std::wstring Native::string_value() { 
	std::wstringstream wss;
	
	wss << '[';
	for (int i = 0; i < modules.size(); ++i) {
		wss << modules[i].file;
		
		if (i != modules.size() - 1)
			wss << ", ";
	}
	wss << ']';
	
	return wss.str();
};

