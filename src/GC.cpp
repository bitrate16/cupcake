#include "GC.h"

#include <exception>
#include <new>

#include "GIL2.h"
#include "exceptions.h"

using namespace ck_core;
using namespace ck_exceptions;


// gc_object		
gc_object::gc_object() : 
			gc_reachable(0),
			gc_record(0),
			gc_root(0),
			gc_lock(0),
			gc_chain(nullptr),
			gc_lock_chain(nullptr),
			gc_root_chain(nullptr) {};
			
gc_object::~gc_object() {};
		
void gc_object::gc_mark() {
	gc_root = 1;
};
		
void gc_object::gc_finalize() {};

void* gc_object::operator new(std::size_t count) {
	try {
		return ::operator new(count);
	} catch (std::bad_alloc) {
		throw ck_message(ck_message_type::BAD_ALLOC);
	}
};

void* gc_object::operator new[](std::size_t count) {
	try {
		return ::operator new[](count);
	} catch (std::bad_alloc) {
		throw ck_message(ck_message_type::BAD_ALLOC2);
	}
};

void gc_object::operator delete  (void* ptr) {
	// Assuming that operator delete is not accessible
	::delete(ptr);
};

void gc_object::operator delete[](void* ptr) {
	// Assuming that operator delete is not accessible
	::delete[](ptr);
};

// gc_list
gc_list::gc_list() : 
				next(nullptr), 
				obj(nullptr), 
				deleted_ptr(0) {};
				
// GC		
GC::GC() :
		size(0),
		roots_size(0),
		locks_size(0),
		roots(nullptr),
		locks(nullptr),
		objects(nullptr), 
		created_interval(0) {};

GC::~GC() {
	dispose();
	
	while (objects) {
		gc_list *c = objects;
		objects = objects->next;
		delete c;
	}
	
	while (roots) {
		gc_list *c = roots;
		roots = roots->next;
		delete c;
	}
	
	while (locks) {
		gc_list *c = locks;
		locks = locks->next;
		delete c;
	}
};

// Called on object creation.
void GC::attach(gc_object *o) {
	if (o == nullptr)
		return;
	
	std::unique_lock<std::mutex> guard(protect_lock);
	if (o->gc_record)
		return;
	
	gc_list *c = new gc_list;
	if (!c)
		throw ck_message("GC error");
	
	++created_interval;
	o->gc_record    = 1;
	o->gc_reachable = 0;
	o->gc_chain     = c;
	
	c->next = objects;
	c->obj  = o;
	objects = c;
	++size;
};

// Called to make given object root object
void GC::attach_root(gc_object *o) {
	if (o == nullptr)
		return;
	
	std::unique_lock<std::mutex> guard(protect_lock);
	if (o->gc_lock)
		return;
	
	gc_list *c = new gc_list;
	if (!c)
		throw ck_message("GC error");
	
	o->gc_root       = 1;
	o->gc_reachable  = 0;
	o->gc_root_chain = c;
	
	c->next = roots;
	c->obj  = o;
	
	roots = c;
	++roots_size;
};

void GC::deattach_root(gc_object *o) {
	if (o == nullptr)
		return;
	
	std::unique_lock<std::mutex> guard(protect_lock);
	if (!o->gc_root)
		return;
	
	o->gc_root = 0;
	--roots_size;
};

// Called to lock given object from deletion.
void GC::lock(gc_object *o) {
	if (o == nullptr)
		return;
	
	std::unique_lock<std::mutex> guard(protect_lock);
	if (o->gc_lock)
		return;
	
	gc_list *c = new gc_list;
	if (!c)
		throw ck_message("GC error");
	
	o->gc_lock       = 1;
	o->gc_reachable  = 0;
	o->gc_lock_chain = c;
	
	c->next = locks;
	c->obj  = o;
	
	locks = c;
	++locks_size;
};

void GC::unlock(gc_object *o) {
	if (o == nullptr)
		return;
	
	std::unique_lock<std::mutex> guard(protect_lock);
	if (!o->gc_lock)
		return;
	
	o->gc_lock = 0;
	--locks_size;
};

// Amount of objects registered by GC.
int GC::count() { return size; };

// Amount of roots
int GC::roots_count() { return roots_size; };

// Amount of locked obejcts
int GC::locks_count() { return locks_size; };

void GC::collect() {
	std::unique_lock<std::mutex> guard(protect_lock);
	if (collecting)
		return;
	
	if (created_interval <= GC::MIN_CREATED_INTERVAL)
		return;
	
	// Call lock on GIL to prevent interruption
	if (!GIL::instance()->try_request_lock())
		return;
	
	created_interval = 0;
	collecting = 1;
	
	if (objects == nullptr) {
		collecting = 0;
		return;
	}
	
	// Mark all roots
	gc_list *chain = roots;
	gc_list *list  = nullptr;
	while (chain) {
		if (chain->deleted_ptr) {
			gc_list *tmp = chain;
			chain         = chain->next;
			delete tmp;
		} else if (!chain->obj->gc_root) {
			attach(chain->obj);
			chain->obj->gc_root_chain = nullptr;
			
			gc_list *tmp = chain;
			chain         = chain->next;
			delete tmp;
		} else {
			chain->obj->gc_mark();
			gc_list *tmp = chain;
			chain         = chain->next;
			
			tmp->next = list;
			list      = tmp;
		}
	}
	
	roots = list;
	
	// Mark all locked objects
	chain = locks;
	list  = nullptr;
	while (chain) {
		if (chain->deleted_ptr) {
			gc_list *tmp = chain;
			chain         = chain->next;
			delete tmp;
		} else if (!chain->obj->gc_lock) {
			// <> already recorded
			// attach(chain->obj);
			chain->obj->gc_lock_chain = nullptr;
			
			gc_list *tmp = chain;
			chain         = chain->next;
			delete tmp;
		} else {
			chain->obj->gc_mark();
			gc_list *tmp = chain;
			chain        = chain->next;
			
			tmp->next = list;
			list      = tmp;
		}
	}
	
	locks = list;
	
	// Mark all objects
	chain = objects;
	list  = nullptr;
	while (chain) {	
		if (chain->deleted_ptr) {
			gc_list *tmp = chain;
			chain = chain->next;
			--size;			
			delete tmp;
		} else if (!chain->obj->gc_reachable && !chain->obj->gc_root && !chain->obj->gc_lock) {			
			gc_list *tmp = chain;
			chain = chain->next;
			--size;
			tmp->obj->gc_finalize();

			delete tmp->obj;
			delete tmp;			
		} else {
			// Reset
			chain->obj->gc_reachable = 0;
			gc_list *tmp = chain;
			chain         = chain->next;
			
			tmp->next = list;
			list      = tmp;
		}
	};
	
	objects = list;

	collecting = 0;	
	
	GIL::instance()->free_lock();
};

void GC::dispose() {	
	// Called on GIL dispose, so no GIL.lock needed.

	std::unique_lock<std::mutex> guard(protect_lock);
	if (collecting)
		return;
	
	collecting = 1;
	
	if (objects == nullptr) {
		collecting = 0;
		return;
	}
	
	while (roots) {
		gc_list *tmp = roots->next;
		if (!roots->deleted_ptr)
			roots->obj->gc_root_chain = nullptr;
		delete roots;
		roots = tmp;
	}
	
	
	// Delete all unused objects
	while (objects) {
		gc_list *tmp = objects->next;
		if (objects->deleted_ptr) {
			delete objects;
		} else {
			--size;
			objects->obj->gc_finalize();
			
			delete objects->obj;
			delete objects;
		} 
		objects = tmp;
	}
	
	collecting = 0;
};

