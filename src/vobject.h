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
		static const vtype type;
		
		vobject();
		
		vobject *get(string);
		vobject *put(string, vobject*);
		bool contains(string);
		bool remove(string);
	};
};