#pragma once

#include "GIL"

namespace ck_vobject {
	// Describes type of an object
	class vtype {
		vtype *parent;
		int id;
		
		public:
		vtype();
		vtype(vtype *);
		
		bool is_typeof(int id);
		int get_id();
	};
	
	extern const vtype none;
	
	// Virtual object representation. Core object type in ck interpreter.
	// Basically not thread safe. Allows any thread call 
	// get/put/remove/contains in any moment without blocking.
	
	// To synchrinize input/output, use vsyncobject type.
	// vsyncobject:
	// sync() -> Any thread that tries to call unsafe operation
	//           will be added to queue by calling lock()
	// unsync() -> Disable locking
	// lock() -> Fdds this thread in lock queue of this object. 
	//           Allows operating with values in synchronized context.
	// unlock() -> Unlocks, logically.
	// get/put/remove/contains are overriden by sync_<name>.
	//                         basically they lock() if object is sync()
	class vobject : gc_object {
		// XXX: check for working
		static const vtype type = none;
		
		vobject();
		
		vobject *get(vscope*, string);
		vobject *put(vscope*, string, vobject*);
		bool contains(vscope*, string);
		bool remove(vscope*, string);
		vobject *call(vscope*, std::vector<vobject*> args);
	};
};