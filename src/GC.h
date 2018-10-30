#pragma once

#include "GIL"

namespace ck_core {	
	/*
	 * Garbage collector object, tracked at creation.
	 */
	class gc_object {
		public:
		bool gc_record;
		bool gc_root;
		bool gc_reachable;
		bool gc_lock;
		
		gc_list *gc_chain;
		gc_list *gc_lock_chain;
		gc_list *gc_root_chain;
		
		gc_object();
		virtual ~gc_object();
		
		// Called when GC indexes all reachable objects
		virtual void gc_mark();
		
		// Called when GC destroyes current object
		virtual void gc_finalize();
	};
	
	/*
	 * Garbage collector chain.
	 */
	class gc_list {
		public:
		gc_list *next;
		gc_object *obj;
		// For stupid users, that decide to GC by themself & stack
		bool deleted_ptr;
		
		gc_chain();
	};

	/*
	 * Garbage collector. Collects your shit.
	 */
	class GC {
		public:
		int collecting;
		int size;
		int roots_size;
		gc_chain *roots;
		gc_chain *locks;
		gc_chain *objects;
		
		GC();
		~GC();
		
		void attach();
		void lock();
		void unlock();
		void attach_root();
		void deattach_root();
		int  count();
		void collect();
		void dispose();
	};
};