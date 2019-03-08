#pragma once

namespace ck_vtype { 


	// XXX: Use typeid
	
	
	
	// Constants to represent diferent types of objects during runtime.
	const int vtype_none = -1;
	
	// Pre-installed objects
	const int vtype_vint               = 1; // 13
	const int vtype_vdouble            = 2; // 13.13
	const int vtype_vbool              = 3; // true
	const int vtype_vstring            = 4; // "qwertyu"
	const int vtype_vbytecode_function = 5; // function(f) { ... }
	const int vtype_varray             = 6; // [foo, 'iwi', ';w;']
	const int vtype_vobject            = 7; // { key: 'value', 'pey': 'waue' }
	
	// Their prototypes (extended from an object and no integral type is required)
	const int vtype_vpint               = 127 + vtype_vint;
	const int vtype_vpdouble            = 127 + vtype_vdouble;
	const int vtype_vpbool              = 127 + vtype_vbool;
	const int vtype_vpstring            = 127 + vtype_vstring;
	const int vtype_vpbytecode_function = 127 + vtype_vbytecode_function;
	const int vtype_vparray             = 127 + vtype_varray;
	const int vtype_vpobject            = 127 + vtype_vobject;
	
	
	// Represents type tree of an object.
	// Using integer type id's to allow determine object's 
	//  type at runtime without attempting to cast.
	struct vtype {
		vtype* super = nullptr;
		int type_id  = -1;
		
	public:
		
		vtype(vtype* parent = nullptr, int obj_type_id = -1) : super(parent), type_id(obj_type_id) {};
		
		virtual bool is_typeof(vtype& other) {
			vtype* t = this;
			while (t) {
				if (t->type_id == other.type_id)
					return 1;
				t = t->super;
			}
			return 0;
		};
		
		virtual bool is_typeof(int type_id) {
			vtype* t = this;
			while (t) {
				if (t->type_id == type_id)
					return 1;
				t = t->super;
			}
			return 0;
		};
		
		virtual inline bool is_type(int type_id) {
			return this->type_id == type_id;
		};
		
		virtual inline int get_type() {
			return type_id;
		};
	};
};