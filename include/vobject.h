#pragma once

#include <string>
#include <vector>

#include "ck_pthread.h"
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
		virtual long long int_value();
		
		// Must return string representation of an object
		virtual std::wstring string_value();
	};
	
	// vobject that supports synchronization via lock() and unlock().
	//  vsobject.
	// Must be used in multithreaded mode to avoid priority race breaks.
	class vsobject : public vobject {
		
		#ifndef CK_SINGLETHREAD
			
			// Synchronization mutex.
			//  The default type of this mutex is recursive_mutex
			ck_pthread::recursive_mutex lock_mutex;
			
		protected:
			
			// Synchronization methods.
			//  must be called on any access to object's fields inside 
			//  of get/put/call/contains/remode, e.t.c.
			inline bool lock() {
				return !lock_mutex.lock();
			};
			
			inline bool unlock() {
				return !lock_mutex.unlock();
			};
			
			inline ck_pthread::recursive_mutex& mutex() {
				return lock_mutex;
			};
		
		#endif
	};
};