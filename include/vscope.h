#pragma once

#include <string>
#include <vector>

#include "objects/Object.h"


namespace ck_vobject {
	
	// Abstract scope class. Interface for the scope
	class vscope : public ck_vobject::vobject {
		
	public:
	
		vscope* parent;
		
		vscope(vscope* parent = nullptr) { this->parent = parent; };
		virtual ~vscope() {};
		
		virtual vobject* get     (vscope*, const std::wstring&) = 0;
		virtual void     put     (vscope*, const std::wstring&, vobject*) = 0;
		virtual bool     contains(vscope*, const std::wstring&) = 0;
		virtual bool     remove  (vscope*, const std::wstring&) = 0;
		virtual vobject* call    (vscope*, const std::vector<vobject*>) = 0;
		
		// Returns pointer to the root scope
		virtual vscope* get_root() = 0;
		
		virtual vscope* get_parent() = 0;
		
		// G C _ O P T I O N S 
		
		// Makes scope being gc_root
		virtual void root() = 0;
		// Unmakes scope being gc_root
		virtual void unroot() = 0;
		
		virtual void gc_mark() = 0;
		virtual void gc_finalize() = 0;
		
		// Must return integer representation of an object
		virtual long long int_value() = 0;
		
		// Must return string representation of an object
		virtual std::wstring string_value() = 0;
		
		// Scope functions
		// Check in this scope.
		//  If parent_get is set to 1 then values will be searched in the parent scope too.
		virtual vobject* get(const std::wstring& name, bool parent_get = 0) = 0;
		
		// Attempt to put in this scope.
		//  If parent_put is 1, then parent.put(name, object, 1, 0)
		//  Returns 0 if no value was inserted,
		//   If create_new is 1, then value will be created in this scope.
		virtual bool put(const std::wstring& name, vobject* object, bool parent_put = 0, bool create_new = 1) = 0;
		
		// If parent_search is 1, then search in prent too.
		// Returns 1 if value was found.
		virtual bool contains(const std::wstring& name, bool parent_search = 0) = 0;
		
		// If parent_remove is 1, then attempt to remove value in parent too.
		// Return 1 if value was removed.
		virtual bool remove(const std::wstring& name, bool parent_remove = 0) = 0;
	};
	
	
	class iscope : public vscope {
		
		std::map<std::wstring, ck_vobject::vobject*> objects;
		
	public:
		
		iscope(vscope* parent = nullptr);
		virtual ~iscope();
		
		vobject* get     (vscope*, const std::wstring&);
		void     put     (vscope*, const std::wstring&, vobject*);
		bool     contains(vscope*, const std::wstring&);
		bool     remove  (vscope*, const std::wstring&);
		vobject* call    (vscope*, const std::vector<vobject*>);
		
		// Returns pointer to the root scope
		vscope* get_root();
		
		vscope* get_parent();
		
		// G C _ O P T I O N S 
		
		// Makes scope being gc_root
		void root();
		// Unmakes scope being gc_root
		void unroot();
		
		virtual void gc_mark();
		virtual void gc_finalize();
		
		// Must return integer representation of an object
		virtual long long int_value();
		
		// Must return string representation of an object
		virtual std::wstring string_value();
		
		// Scope functions
		// Check in this scope.
		//  If parent_get is set to 1 then values will be searched in the parent scope too.
		vobject* get(const std::wstring& name, bool parent_get = 0);
		
		// Attempt to put in this scope.
		//  If parent_put is 1, then parent.put(name, object, 1, 0)
		//  Returns 0 if no value was inserted,
		//   If create_new is 1, then value will be created in this scope.
		bool put(const std::wstring& name, vobject* object, bool parent_put = 0, bool create_new = 1);
		
		// If parent_search is 1, then search in prent too.
		// Returns 1 if value was found.
		bool contains(const std::wstring& name, bool parent_search = 0);
		
		// If parent_remove is 1, then attempt to remove value in parent too.
		// Return 1 if value was removed.
		bool remove(const std::wstring& name, bool parent_remove = 0);
		
		// Called on interpreter start to initialize prototype
		static vobject* create_proto();
	};
	
	// Proxy scope type for redirecting all operations to the contained object instance.
	// Acts as a normal scope interface but allows handling object as a field.
	class xscope: public vscope {
		
		ck_vobject::vobject* proxy;
		
	public:
		
		xscope(ck_vobject::vobject* proxy = nullptr, vscope* parent = nullptr);
		~xscope();
		
		vobject* get     (vscope*, const std::wstring&);
		void     put     (vscope*, const std::wstring&, vobject*);
		bool     contains(vscope*, const std::wstring&);
		bool     remove  (vscope*, const std::wstring&);
		vobject* call    (vscope*, const std::vector<vobject*>);
		
		// Returns pointer to the root scope
		vscope* get_root();
		
		vscope* get_parent();
		
		// G C _ O P T I O N S 
		
		// Makes scope being gc_root
		void root();
		// Unmakes scope being gc_root
		void unroot();
		
		virtual void gc_mark();
		virtual void gc_finalize();
		
		// Must return integer representation of an object
		virtual long long int_value();
		
		// Must return string representation of an object
		virtual std::wstring string_value();
		
		// Scope functions
		// Check in this scope.
		//  If parent_get is set to 1 then values will be searched in the parent scope too.
		vobject* get(const std::wstring& name, bool parent_get = 0);
		
		// Attempt to put in this scope.
		//  If parent_put is 1, then parent.put(name, object, 1, 0)
		//  Returns 0 if no value was inserted,
		//   If create_new is 1, then value will be created in this scope.
		bool put(const std::wstring& name, vobject* object, bool parent_put = 0, bool create_new = 1);
		
		// If parent_search is 1, then search in prent too.
		// Returns 1 if value was found.
		bool contains(const std::wstring& name, bool parent_search = 0);
		
		// If parent_remove is 1, then attempt to remove value in parent too.
		// Return 1 if value was removed.
		bool remove(const std::wstring& name, bool parent_remove = 0);
		
		// Called on interpreter start to initialize prototype
		static vobject* create_proto();
	};
	
	
	// Defined on interpreter start.
	static ck_objects::Object* ScopeProto = nullptr;
	
	// Defined on interpreter start.
	static ck_objects::Object* ProxyScopeProto = nullptr;
};