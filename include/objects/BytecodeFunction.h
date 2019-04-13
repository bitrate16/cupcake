#pragma once

#include <vector>
#include <cwchar>

#include "vobject.h"
#include "vscope.h"
#include "script.h"

#include "Function.h"
#include "CallablePrototype.h"

namespace ck_objects {	

	class BytecodeFunction : public ck_objects::Function {
		
	protected:
		
		// Creation scope
		ck_vobject::vscope* scope;
		ck_core::ck_script* script;
		std::vector<std::wstring> argn;
		
	public:
		
		BytecodeFunction(ck_vobject::vscope* definition_scope, ck_core::ck_script* script, const std::vector<std::wstring>& argn);
		virtual ~BytecodeFunction();
		
		virtual vobject* get     (ck_vobject::vscope*, const std::wstring&);
		virtual void     put     (ck_vobject::vscope*, const std::wstring&, vobject*);
		virtual bool     contains(ck_vobject::vscope*, const std::wstring&);
		virtual bool     remove  (ck_vobject::vscope*, const std::wstring&);
		virtual vobject* call    (ck_vobject::vscope*, const std::vector<vobject*>&);
		
		virtual void gc_mark();
		
		
		// Apply arguments tu the function and return scope
		virtual ck_vobject::vscope* apply(ck_vobject::vobject* this_bind, const std::vector<ck_vobject::vobject*>& argv, ck_vobject::vscope* caller_scope = nullptr);
		
		// Returns int to string
		virtual std::wstring string_value();
		
		inline ck_core::ck_script* get_script() { return script; };
		
		// Called on interpreter start to initialize prototype
		static ck_vobject::vobject* create_proto();
	};
	
	// Defined on interpreter start.
	static CallablePrototype* BytecodeFunctionProto = nullptr;
};