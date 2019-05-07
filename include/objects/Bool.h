#pragma once

#include <vector>
#include <cwchar>

#include "Object.h"
#include "CallableObject.h"

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
		virtual vobject* call    (ck_vobject::vscope*, const std::vector<vobject*>&);
		
		
		// Returns value
		virtual long long int_value();
		
		// Returns int to string
		virtual std::wstring string_value();
		
		virtual bool value() {
			return val;
		};
		
		// Returns esisting intance of true/false value for passed bool value.
		static inline Bool* instance(bool value) {
			return value ? Bool::True() : Bool::False();
		};
		
		static Bool* True();
		
		static Bool* False();
		
		// Called on interpreter start to initialize prototype
		static vobject* create_proto();
	};
	
	// Defined on any call to Bool::True().
	static Bool* TrueInstance = nullptr;
	
	// Defined on any call to Bool::False().
	static Bool* FalseInstance = nullptr;
	
	// Defined on interpreter start.
	static CallableObject* BoolProto = nullptr;
};