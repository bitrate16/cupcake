#pragma once

#include <functional>

#include "Object.h"

// Container for prototypes that allow overloading call with lambda-expression.
namespace ck_objects {	

	class CallablePrototype : public ck_objects::Object {
		
	protected:
		
		// CallablePrototype::call override
		ck_vobject::vobject* (*call_handler) (ck_vobject::vscope*, const std::vector<ck_vobject::vobject*>&);
		
	public:
		
		CallablePrototype(ck_vobject::vobject* (*handler) (ck_vobject::vscope*, const std::vector<ck_vobject::vobject*>&));
		
		virtual vobject* call    (ck_vobject::vscope*, const std::vector<vobject*>&);
	};
};