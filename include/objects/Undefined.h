#pragma once

#include <vector>
#include <cwchar>

#include "Object.h"

namespace ck_objects {	

	class Undefined : public ck_vobject::vobject {
		
	public:
		
		Undefined();
		virtual ~Undefined();
		
		virtual vobject* get     (ck_vobject::vscope*, const std::wstring&);
		virtual void     put     (ck_vobject::vscope*, const std::wstring&, vobject*);
		virtual bool     contains(ck_vobject::vscope*, const std::wstring&);
		virtual bool     remove  (ck_vobject::vscope*, const std::wstring&);
		virtual vobject* call    (ck_vobject::vscope*, std::vector<vobject*>);
		
		virtual void gc_mark();
		virtual void gc_finalize();
		
		
		// Returns value
		virtual long long int_value();
		
		// Returns int to string
		virtual std::wstring string_value();
		
		// Called on interpreter start to initialize prototype
		static vobject* create_proto();
		
		// Returns pointer to existing instance of Undefined
		static Undefined* instance();
	};
	
	// Defined on any call to Undefined::instance().
	static Undefined* UndefinedInstance = nullptr;
	
	// Defined on interpreter start.
	static Object* UndefinedProto = nullptr;
};