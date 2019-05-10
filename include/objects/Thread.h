#pragma once

#include <map>

#include "Object.h"
#include "CallableObject.h"
#include "../GIL2.h"

namespace ck_objects {	

	class Thread : public ck_objects::Object {
		
	protected:
		
		// Pointer to allocated thread/requested thread
		ck_core::ckthread* thread_ptr;
		
	public:
	
		// Bind instance to this object
		Thread(ck_core::ckthread* th, ck_vobject::vobject* runnable = nullptr);
		// Get current_thread from GIL
		Thread(ck_vobject::vobject* runnable = nullptr);
		virtual ~Thread();
		
		virtual vobject* get     (ck_vobject::vscope*, const std::wstring&);
		virtual void     put     (ck_vobject::vscope*, const std::wstring&, vobject*);
		virtual bool     contains(ck_vobject::vscope*, const std::wstring&);
		virtual bool     remove  (ck_vobject::vscope*, const std::wstring&);
		virtual vobject* call    (ck_vobject::vscope*, const std::vector<vobject*>&);
		
		virtual void gc_mark();
		virtual void gc_finalize();
		
		// Thread functions only
		
		inline void bind(ck_core::ckthread* th) { thread_ptr = th; };
		inline ck_core::ckthread* get() { return thread_ptr; };
		
		// Must return integer representation of an object
		virtual long long int_value();
		
		// Must return string representation of an object
		virtual std::wstring string_value();
		
		// Called on interpreter start to initialize prototype
		static vobject* create_proto();
	};
	
	// Defined on interpreter start.
	static CallableObject* ThreadProto = nullptr;
};