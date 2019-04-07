#pragma once

#include <functional>

#include "Function.h"
#include "Object.h"

// Container for prototypes that allow overloading call with lambda-expression.
namespace ck_objects {	

	class NativeFunction : public ck_objects::Function {
		
	protected:
		
		// NativeFunction::call override -> vobject* func(vscope*, vector<vobject*> args)
		ck_vobject::vobject* (*call_wrapper) (ck_vobject::vscope*, const std::vector<ck_vobject::vobject*>&);
		
	public:
		
		NativeFunction(ck_vobject::vobject* (*wrapper) (ck_vobject::vscope*, const std::vector<ck_vobject::vobject*>&));
		virtual ~NativeFunction();
		
		virtual vobject* get     (ck_vobject::vscope*, const std::wstring&);
		virtual void     put     (ck_vobject::vscope*, const std::wstring&, vobject*);
		virtual bool     contains(ck_vobject::vscope*, const std::wstring&);
		virtual bool     remove  (ck_vobject::vscope*, const std::wstring&);
		virtual vobject* call    (ck_vobject::vscope*, const std::vector<vobject*>);
		
		virtual void gc_mark();
		
		// Must return string representation of an object
		virtual std::wstring string_value();
		
		// Called on interpreter start to initialize prototype
		static vobject* create_proto();
		
		// Does absolutely nothing
		virtual ck_vobject::vscope* apply(ck_vobject::vobject* this_bind, const std::vector<ck_vobject::vobject*>& argv, ck_vobject::vscope* caller_scope) { return nullptr; };
		
		// Returns pointer to call_wrapper
		inline ck_vobject::vobject* (*get_call_wrapper()) (ck_vobject::vscope*, const std::vector<ck_vobject::vobject*>&) { return call_wrapper; };
	};
	
	// Defined on interpreter start.
	static Object* NativeFunctionProto = nullptr;
};