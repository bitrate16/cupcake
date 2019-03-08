#pragma once

#include <vector>

#include "Object.h"

namespace ck_objects {	

	class Array : public ck_objects::Object {
		
	protected:
		
		// Array elements. Can be accessed via array[index:int]
		std::vector<ck_vobject::vobject*> elements;
		
	public:
		
		Array();
		virtual ~Array();
		
		virtual vobject* get     (ck_vobject::vscope*, const std::wstring&);
		virtual void     put     (ck_vobject::vscope*, const std::wstring&, vobject*);
		virtual bool     contains(ck_vobject::vscope*, const std::wstring&);
		virtual bool     remove  (ck_vobject::vscope*, const std::wstring&);
		virtual vobject* call    (ck_vobject::vscope*, std::vector<vobject*>);
		
		virtual void gc_mark();
		virtual void gc_finalize();
		
		// Array functions only
		
		void append(Array*);
		int size();
		
		// Scope-independent getter-setter
		vobject* get_item(int, bool range_wrap = 0);
		
		// If range_check is 1 then (index-size) null object will be appended to the array.
		// Else on out of range returns 0 and does nothing.
		bool     set_item(int, vobject*, bool range_wrap = 0, bool range_check = 0);
	};
	
	// Defined on interpreter start.
	static Object* ArrayProto = nullptr;
};