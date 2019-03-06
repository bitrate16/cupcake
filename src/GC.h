#pragma once

#include "GIL2.h"
#include "vobject.h"

namespace ck_core {		
	/*
	 * Garbage collector object, tracked at creation.
	 */
	class GC;
	class gc_list;
	class gc_object {
		public:
		bool gc_reachable = 0;
		
		gc_object();
		virtual ~gc_object();
		
		// Called when GC indexes all reachable objects
		virtual void gc_mark();
		
		// Called when GC destroyes current object
		virtual void gc_finalize();
		
		private:
		// Allow access only from GC class.
		friend class GC;
		
		bool gc_record;
		bool   gc_root;
		bool   gc_lock;
		
		gc_list *gc_chain;
		gc_list *gc_lock_chain;
		gc_list *gc_root_chain;
		
		void operator delete  (void* ptr);
		void operator delete[](void* ptr);
	};
	
	/*
	 * Garbage collector chain.
	 */
	class gc_list {
		public:
		gc_list  *next;
		gc_object *obj;
		// For stupid users, that decide to GC by themself
		bool deleted_ptr;
		
		gc_list();
	};

	/*
	 * Garbage collector. Collects your shit.
	 */
	class GC {
	
	private:
		// Protects object from multiple threads access.
		std::mutex protect_lock;
		int collecting;
		int size;
		int roots_size;
		int locks_size;
		gc_list *objects;
		gc_list *roots;
		gc_list *locks;
		
		// Number of objects created since last gc_collect pass
		std::atomic<int> created_interval;
		// Number of minimum objects to be created before next GC
		const int MIN_CREATED_INTERVAL;
		
	public:
		
		GC();
		~GC();
		
		public:
		// Called on object creation.
		void attach(ck_vobject::vobject *o);
		
		// Called to make given object root object
		void attach_root(ck_vobject::vobject *o);
		void deattach_root(ck_vobject::vobject *o);
		
		// Called to lock given object from deletion.
		void lock(ck_vobject::vobject *o);
		void unlock(ck_vobject::vobject *o);
		
		// Amount of objects registered by GC.
		int count();
		
		// Amount of roots
		int roots_count();
		
		// Amount of locked obejcts
		int locks_count();
		
		void collect();
		void dispose();
	};
};