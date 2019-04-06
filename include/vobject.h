#pragma once

#include <string>
#include <vector>

#include "GC.h"


namespace ck_vobject {	

	class vscope;
	class vobject : public ck_core::gc_object {
		
	public:
		
		vobject();
		virtual ~vobject();
		
		virtual vobject* get     (vscope*, const std::wstring&);
		virtual void     put     (vscope*, const std::wstring&, vobject*);
		virtual bool     contains(vscope*, const std::wstring&);
		virtual bool     remove  (vscope*, const std::wstring&);
		virtual vobject* call    (vscope*, const std::vector<vobject*>);
		
		virtual void gc_mark();
		virtual void gc_finalize();
		
		// Utility to convert between types and determine storage type.
		// Returns pointer to desired type on success, nullptr else.
		template<typename T, typename std::enable_if<std::is_base_of<vobject, T>::value>::type* = nullptr> T* is_typeof() {
			return dynamic_cast<T*>(this);
		};
		
		// Must return integer representation of an object
		virtual long long int_value();
		
		// Must return string representation of an object
		virtual std::wstring string_value();
	};
	
	// vobject that supports synchronization via lock() and unlock().
	//  vsobject.
	// Must be used in multithreaded mode to avoid priority race breaks.
	class vsobject : public vobject {
		// Synchronization lock protectors
		// ???: Thinking about using recursive_mutex
		std::mutex lock_mutex;
		
	protected: // absolutely 200 bytes shit
		
		// Synchronization methods.
		//  must be called on any access to object's fields inside 
		//  of get/put/call/contains/remode, e.t.c.
		// PROF: better to use unique_lock
		inline void lock() {
			lock_mutex.lock();
		};
		
		inline void unlock() {
			lock_mutex.unlock();
		};
		
		inline std::mutex& mutex() {
			return lock_mutex;
		};
	};
	
		// XXX: check for working
		// Each object must implement type ierarchy to provide
		// correct functionality and type recognition.
		// static const vtype type(nullptr, VOBJECT_TYPE);
	
	// Following constant represents none type.
	// extern const vtype none(nullptr, NONE_TYPE);
	
	// Virtual object representation. Core object type in ck interpreter.
	// Basically not thread safe. Allows any thread call 
	// get/put/remove/contains in any moment without blocking.
	
	// To synchrinize input/output, use vsyncobject type.
	// vsyncobject:
	// sync() -> Any thread that tries to call unsafe operation
	//           will be added to queue by calling lock()
	// unsync() -> Disable locking
	// lock() -> Fdds this thread in lock queue of this object. 
	//           Allows operating with values in synchronized context.
	// unlock() -> Unlocks, logically.
	// get/put/remove/contains are overriden by sync_<name>.
	//                         basically they lock() if object is sync()
	
	// Virtual synchronized object.
	// Wrapper for vobject but allows unique access to the fields.
	// First, object must be synchronized with vsobject::sync().
	// Then caller invokes sync_<func> to access object fields in singlethreaded mode.
	//
	// Locking on an object is GIL-safe operation, so it doesn't need to lock
	// GIL::sync_lock or notify GIL::sync_sondition.
	//
	// Operation of an object lock is dome in the middle section:
	// "A thread GIL-free window between it started working with object
	//  and checked GIL::lock_requested to block itself"
	// That means that this operation by definition MUST unblock before 
	// thread responds to the GIL::lock_requested.
	/* class vsobject : vobject {
		// XXX: check for working too
		// Each object must implement type ierarchy to provide
		// correct functionality and type recognition.
		static const vtype type(&ck_vobject::vobject::type, VCOBJECT_TYPE);
		
		// Locks access to the object while doing something with it's values.
		std::mutex sync_lock;
		// Flas that determines whenever object is being protected access.
		std::atomic<bool> sync = 0;
		
		vsobject();
		
		bool is_sync();
		void sync();
		void unsync();
		
		// Returns 1 if object can be locked (i.e. sync is 1)
		bool lock();
		bool unlock();
		
		// Overloaded by developer same as for vobject.
		vobject *get(vscope*, std::wstring);
		vobject *put(vscope*, std::wstring, vobject*);
		bool contains(vscope*, std::wstring);
		bool remove(vscope*, std::wstring);
		
		// May be called by user side:
		// myObj.sync_get(nullptr, "myVar");
		vobject *sync_get(vscope*, std::wstring);
		vobject *sync_put(vscope*, std::wstring, vobject*);
		bool sync_contains(vscope*, std::wstring);
		bool sync_remove(vscope*, std::wstring);
		
		vobject *call(vscope*, std::vector<vobject*> args);
	}; */
};