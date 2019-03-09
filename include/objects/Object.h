#pragma once

#include <map>

#include "../vobject.h"

namespace ck_objects {	// WARNING: This and all standard objects uses prototype chain to fetch objects.

	class Object : public ck_vobject::vobject {
		
	protected:
		
		// Map with all objects stored by this class instance.
		std::map<std::wstring, ck_vobject::vobject*> objects;
		
	public:
		
		Object(std::map<std::wstring, ck_vobject::vobject*>&);
		Object();
		virtual ~Object();
		
		virtual vobject* get     (ck_vobject::vscope*, const std::wstring&);
		virtual void     put     (ck_vobject::vscope*, const std::wstring&, vobject*);
		virtual bool     contains(ck_vobject::vscope*, const std::wstring&);
		virtual bool     remove  (ck_vobject::vscope*, const std::wstring&);
		virtual vobject* call    (ck_vobject::vscope*, const std::vector<vobject*>);
		
		virtual void gc_mark();
		virtual void gc_finalize();
		
		// Object functions only
		
		void append(Object*);
		std::vector<std::wstring> keys();
		
		// Scope-independent getter-setter-checker.
		void     put     (const std::wstring&, vobject*);
		vobject* get     (const std::wstring&);
		bool     contains(const std::wstring&);
		bool     remove  (const std::wstring&);
		
		// Must return integer representation of an object
		virtual long long int_value();
		
		// Must return string representation of an object
		virtual std::wstring string_value();
		
		// Called on interpreter start to initialize prototype
		static vobject* create_proto();
	};
	
	// Defined on interpreter start.
	static Object* ObjectProto = nullptr;
};