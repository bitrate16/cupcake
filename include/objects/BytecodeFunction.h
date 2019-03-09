#pragma once

#include <vector>
#include <cwchar>

#include "vobject.h"
#include "vscope.h"
#include "script.h"

namespace ck_objects {	

	class BytecodeFunction : public ck_vobject::vobject {
		
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
		virtual vobject* call    (ck_vobject::vscope*, const std::vector<vobject*>);
		
		virtual void gc_mark();
		virtual void gc_finalize();
		
		
		// Apply arguments tu the function and return scope
		ck_vobject::vscope* apply(const std::vector<ck_vobject::vobject*>&);
		
		// Returns value
		virtual long long int_value();
		
		// Returns int to string
		virtual std::wstring string_value();
		
		// Called on interpreter start to initialize prototype
		static ck_vobject::vobject* create_proto();
	};
	
	// Defined on any call to BytecodeFunction::instance().
	static BytecodeFunction* BytecodeFunctionInstance = nullptr;
	
	// Defined on interpreter start.
	static Object* BytecodeFunctionProto = nullptr;
};