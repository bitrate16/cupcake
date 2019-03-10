#pragma once

#include <vector>
#include <cwchar>

#include "../exceptions.h"

#include "Object.h"
#include "CallablePrototype.h"

namespace ck_objects {

	class Error : public ck_objects::Object {
		
		struct Frame {
			
			Frame() {};
			
			int lineno;
			std::wstring filename;
			std::wstring function;
		};
		
	protected:
	
		std::vector<Frame> backtrace;
		std::wstring message;
		
		void collect_backtrace();
		
	public:
		
		Error();
		Error(const std::wstring& str);
		Error(const ck_exceptions::ck_message& ex);
		virtual ~Error();
		
		virtual vobject* get     (ck_vobject::vscope*, const std::wstring&);
		virtual void     put     (ck_vobject::vscope*, const std::wstring&, vobject*);
		virtual bool     contains(ck_vobject::vscope*, const std::wstring&);
		virtual bool     remove  (ck_vobject::vscope*, const std::wstring&);
		virtual vobject* call    (ck_vobject::vscope*, const std::vector<vobject*>);
		
		virtual void gc_mark();
		virtual void gc_finalize();
		
		
		virtual long long int_value();
		
		// Returns string
		virtual std::wstring string_value();
		
		// Called on interpreter start to initialize prototype
		static vobject* create_proto();
	};
	
	// Defined on interpreter start.
	static CallablePrototype* ErrorProto = nullptr;
};