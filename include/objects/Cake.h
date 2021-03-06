#pragma once

#include <vector>
#include <cwchar>

#include "../exceptions.h"

#include "Object.h"
#include "CallableObject.h"

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
		
		
		inline void set_type(const std::wstring& t) {
			type = t;
		};
		
		inline void set_message(const std::wstring& m) {
			message = m;
		};
		
		inline const std::wstring& get_type() {
			return type;
		};
		
		inline const std::wstring& get_message() {
			return message;
		};
		
		inline std::vector<ck_exceptions::BacktraceFrame>& get_backtrace() {
			return backtrace;
		};
		
		// XXX: allow specify output stream
		// Print backtrace to stderr
		void print_backtrace();
		
		virtual int64_t int_value();
		
		// Returns string
		virtual std::wstring string_value();
		
		// Called on interpreter start to initialize prototype
		static vobject* create_proto();
	};
	
	// Defined on interpreter start.
	static CallableObject* CakeProto = nullptr;
};