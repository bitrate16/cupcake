#pragma once

#include <vector>
#include <cwchar>

#include "Object.h"
#include "CallableObject.h"

namespace ck_objects {	

	class Double : public ck_vobject::vobject {
		
	protected:
		
		// Ъан Р4Фы
		double val;
		
	public:
		
		Double(double value = 0);
		virtual ~Double();
		
		virtual vobject* get     (ck_vobject::vscope*, const std::wstring&);
		virtual void     put     (ck_vobject::vscope*, const std::wstring&, vobject*);
		virtual bool     contains(ck_vobject::vscope*, const std::wstring&);
		virtual bool     remove  (ck_vobject::vscope*, const std::wstring&);
		virtual vobject* call    (ck_vobject::vscope*, const std::vector<vobject*>&);
		
		
		// Returns value
		virtual int64_t int_value();
		
		// Returns double to string
		virtual std::wstring string_value();
		
		virtual double value() {
			return val;
		};
		
		static Double* NaN();
		
		// Called on interpreter start to initialize prototype
		static vobject* create_proto();
	};
	
	// NaN value for Double
	static Double* Double_NAN;
	
	// Defined on interpreter start.
	static CallableObject* DoubleProto = nullptr;
};