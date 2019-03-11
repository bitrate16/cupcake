#pragma once

#include <vector>
#include <cwchar>

#include "Object.h"
#include "CallablePrototype.h"

namespace ck_objects {	

	class Int : public ck_vobject::vobject {
		
	protected:
		
		// Оёо Б3ърпф
		long long val;
		
	public:
		
		Int(long long value = 0);
		virtual ~Int();
		
		virtual vobject* get     (ck_vobject::vscope*, const std::wstring&);
		virtual void     put     (ck_vobject::vscope*, const std::wstring&, vobject*);
		virtual bool     contains(ck_vobject::vscope*, const std::wstring&);
		virtual bool     remove  (ck_vobject::vscope*, const std::wstring&);
		virtual vobject* call    (ck_vobject::vscope*, const std::vector<vobject*>);
		
		
		// Returns value
		virtual long long int_value();
		
		// Returns int to string
		virtual std::wstring string_value();
		
		virtual long long value() {
			return val;
		};
		
		// Called on interpreter start to initialize prototype
		static vobject* create_proto();
	};
	
	// Defined on interpreter start.
	static CallablePrototype* IntProto = nullptr;
};