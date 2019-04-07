#pragma once

#include <string>
#include <vector>
#include <cwchar>

#include "vobject.h"
#include "vscope.h"

namespace ck_objects {	

	class Function : public ck_vobject::vobject {
		
	protected:
	
		// Used to determine this reference during the function call.
		ck_vobject::vobject* this_bind = nullptr;
		
	public:
		
		Function() {};
		virtual ~Function() {};
		
		virtual vobject* call(ck_vobject::vscope*, const std::vector<vobject*>) = 0;
		
		// Fix this reference for a current instance of function.
		// Normally does not return modified function, just binds a reference.
		Function* bind(ck_vobject::vobject* this_bind) { this->this_bind = this_bind; return this; };
		
		// Returns a scope with applied arguments list and __this value
		virtual ck_vobject::vscope* apply(ck_vobject::vobject* this_bind, const std::vector<ck_vobject::vobject*>& argv, ck_vobject::vscope* caller_scope = nullptr) = 0;
		
		// Returns binded reference of .this value
		inline ck_vobject::vobject* get_bind() { return this_bind; };
		
		virtual void gc_mark();
		
		// Converts to string
		virtual std::wstring string_value();
	};
};