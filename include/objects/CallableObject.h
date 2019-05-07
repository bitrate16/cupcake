#pragma once

#include <functional>

#include "Object.h"

// Container for prototypes that allow overloading call with lambda-expression.
namespace ck_objects {	

	class CallableObject : public ck_objects::Object {
		
	protected:
		
		// CallableObject::call override
		ck_vobject::vobject* (*call_handler) (ck_vobject::vscope*, const std::vector<ck_vobject::vobject*>&);
		
	public:
		
		CallableObject(ck_vobject::vobject* (*handler) (ck_vobject::vscope*, const std::vector<ck_vobject::vobject*>&));
		
		vobject* get         (ck_vobject::vscope*, const std::wstring&);
		void     put         (ck_vobject::vscope*, const std::wstring&, vobject*);
		bool     contains    (ck_vobject::vscope*, const std::wstring&);
		bool     remove      (ck_vobject::vscope*, const std::wstring&);
		virtual vobject* call(ck_vobject::vscope*, const std::vector<vobject*>&);
	};
};