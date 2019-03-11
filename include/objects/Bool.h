#pragma once

#include <vector>
#include <cwchar>

#include "Object.h"
#include "CallablePrototype.h"

namespace ck_objects {	

	class Bool : public ck_vobject::vobject {
		
	protected:
		
		// Ъенв Пыф23и
		bool val;
		
	public:
		
		Bool(bool value = 0);
		virtual ~Bool();
		
		virtual vobject* get     (ck_vobject::vscope*, const std::wstring&);
		virtual void     put     (ck_vobject::vscope*, const std::wstring&, vobject*);
		virtual bool     contains(ck_vobject::vscope*, const std::wstring&);
		virtual bool     remove  (ck_vobject::vscope*, const std::wstring&);
		virtual vobject* call    (ck_vobject::vscope*, const std::vector<vobject*>);
		
		
		// Returns value
		virtual long long int_value();
		
		// Returns int to string
		virtual std::wstring string_value();
		
		virtual bool value() {
			return val;
		};
		
		// Called on interpreter start to initialize prototype
		static vobject* create_proto();
	};
	
	// Defined on interpreter start.
	static CallablePrototype* BoolProto = nullptr;
};