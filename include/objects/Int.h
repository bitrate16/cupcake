#pragma once

#include <vector>
#include <cwchar>

#include "Object.h"
#include "CallableObject.h"

namespace ck_objects {	

	class Int : public ck_vobject::vobject {
		
	protected:
		
		// Оёо Б3ърпф
		int64_t val;
		
	public:
		
		Int(int64_t value = 0);
		virtual ~Int();
		
		virtual vobject* get     (ck_vobject::vscope*, const std::wstring&);
		virtual void     put     (ck_vobject::vscope*, const std::wstring&, vobject*);
		virtual bool     contains(ck_vobject::vscope*, const std::wstring&);
		virtual bool     remove  (ck_vobject::vscope*, const std::wstring&);
		virtual vobject* call    (ck_vobject::vscope*, const std::vector<vobject*>&);
		
		
		// Returns value
		virtual int64_t int_value();
		
		// Returns int to string
		virtual std::wstring string_value();
		
		virtual int64_t value() {
			return val;
		};
		
		// Called on interpreter start to initialize prototype
		static vobject* create_proto();
	};
	
	// Defined on interpreter start.
	static CallableObject* IntProto = nullptr;
};