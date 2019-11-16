#pragma once

#include <map>

#include "Object.h"
#include "CallableObject.h"
#include "../GIL2.h"

namespace ck_objects {	

	class Thread : public ck_objects::Object {
		
	protected:
		
		// Id of binded thread
		uint64_t thread_id;
		
	public:
	
		// Bind thread id instance to this object, no check
		Thread(uint64_t tid);
		// Get current_thread from GIL, save id only
		Thread();
		virtual ~Thread();
		
		virtual vobject* get     (ck_vobject::vscope*, const std::wstring&);
		virtual void     put     (ck_vobject::vscope*, const std::wstring&, vobject*);
		virtual bool     contains(ck_vobject::vscope*, const std::wstring&);
		virtual bool     remove  (ck_vobject::vscope*, const std::wstring&);
		virtual vobject* call    (ck_vobject::vscope*, const std::vector<vobject*>&);
		
		virtual void gc_mark();
		virtual void gc_finalize();
		
		// Thread functions only
		
		inline void bind_id(uint64_t tid) { thread_id = tid; };
		inline uint64_t get_id() { return thread_id; };
		
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