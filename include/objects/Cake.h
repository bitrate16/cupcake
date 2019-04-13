#pragma once

#include <vector>
#include <cwchar>

#include "../exceptions.h"

#include "Object.h"
#include "CallablePrototype.h"

namespace ck_objects {

	class Cake : public ck_objects::Object {
		
	protected:
	
		std::vector<ck_exceptions::BacktraceFrame> backtrace;
		std::wstring type;
		std::wstring message;
		
		void collect_backtrace();
		
	public:
		
		Cake(const std::wstring& type = L"", const std::wstring& message = L"");
		Cake(const ck_exceptions::cake& c);
		
		virtual vobject* get     (ck_vobject::vscope*, const std::wstring&);
		virtual void     put     (ck_vobject::vscope*, const std::wstring&, vobject*);
		virtual bool     contains(ck_vobject::vscope*, const std::wstring&);
		virtual bool     remove  (ck_vobject::vscope*, const std::wstring&);
		virtual vobject* call    (ck_vobject::vscope*, const std::vector<vobject*>&);
		
		virtual void gc_mark();
		virtual void gc_finalize();
		
		
		inline const std::wstring& get_type() {
			return type;
		};
		
		inline const std::wstring& get_message() {
			return message;
		};
		
		inline std::vector<ck_exceptions::BacktraceFrame>& get_backtrace() {
			return backtrace;
		};
		
		void print_backtrace();
		
		virtual long long int_value();
		
		// Returns string
		virtual std::wstring string_value();
		
		// Called on interpreter start to initialize prototype
		static vobject* create_proto();
	};
	
	// Defined on interpreter start.
	static CallablePrototype* CakeProto = nullptr;
};