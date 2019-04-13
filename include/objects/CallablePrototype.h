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
		virtual ~CallablePrototype();
		
		virtual vobject* get     (ck_vobject::vscope*, const std::wstring&);
		virtual void     put     (ck_vobject::vscope*, const std::wstring&, vobject*);
		virtual bool     contains(ck_vobject::vscope*, const std::wstring&);
		virtual bool     remove  (ck_vobject::vscope*, const std::wstring&);
		virtual vobject* call    (ck_vobject::vscope*, const std::vector<vobject*>&);
		
		virtual void gc_mark();
		virtual void gc_finalize();
		
		// Must return integer representation of an object
		virtual long long int_value();
		
		// Must return string representation of an object
		virtual std::wstring string_value();
	};
};