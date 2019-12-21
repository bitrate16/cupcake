#pragma once

#include <string>
#include <vector>
#include <thread>

#include "GC.h"


namespace ck_vobject {	

	class vscope;
	
	// Represents single virtual object in an interpreter.
	// Used in local OOP prototype system.
	class vobject : public ck_core::gc_object {
		
	public:
		
		vobject();
		virtual ~vobject();
		
		virtual vobject* get     (vscope*, const std::wstring&);
		virtual void     put     (vscope*, const std::wstring&, vobject*);
		virtual bool     contains(vscope*, const std::wstring&);
		virtual bool     remove  (vscope*, const std::wstring&);
		virtual vobject* call    (vscope*, const std::vector<vobject*>&);
		
		virtual void gc_mark();
		virtual void gc_finalize();
		
		// Utility to convert between types and determine storage type.
		// Returns pointer to desired type on success, nullptr else.
		template<typename T, typename std::enable_if<std::is_base_of<vobject, T>::value>::type* = nullptr> inline T* as_type() {
			return dynamic_cast<T*>(this);
		};
		
		// Must return integer representation of an object
		virtual int64_t int_value();
		
		// Must return string representation of an object
		virtual std::wstring string_value();
	};
	
	// vobject that supports synchronization via lock() and unlock().
	//  vsobject.
	// Must be used in multithreaded mode to avoid priority race breaks.
	class vsobject : public vobject {
		
	public:
		
		// Utility for acquiring life-time lock on object.
		// Locked on create, unlocked on destruct.
		// Example: vslock lk(this);
		class vslock {
		#ifndef CK_SINGLETHREAD
			vsobject* o;
			
		public:
		
			vslock(vsobject* o) {
				this->o = o;
				this->o->lock();
			};
			
			~vslock() {
				o->unlock();
			};
		#else
	
		public:
	
			vslock(vsobject* o) {};
		
			~vslock() {};
		#endif
		};
		
		#ifndef CK_SINGLETHREAD
			
			// Synchronization mutex.
			//  The default type of this mutex is recursive_mutex
			std::recursive_mutex lock_mutex;
			
		protected:
			
			// Synchronization methods.
			//  must be called on any access to object's fields inside 
			//  of get/put/call/contains/remode, e.t.c.
			inline void lock() {
				lock_mutex.lock();
			};
			
			inline void unlock() {
				lock_mutex.unlock();
			};
			
			inline std::recursive_mutex& mutex() {
				return lock_mutex;
			};
		
		#else
			
		protected:
			
			inline void lock() {};
			
			inline void unlock() {};
		#endif
	};
};