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