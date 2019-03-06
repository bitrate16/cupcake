#include <stdexcept>

#include "GC.h"
#include "GIL2.h"

using namespace ck_core;
using namespace ck_vobject;

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

void gc_object::operator delete  (void* ptr) {
	::delete(ptr);
};

void gc_object::operator delete[](void* ptr) {
	::delete[](ptr);
};

// gc_chain
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
		gc_chain *c = objects;
		objects = objects->next;
		delete c;
	}
	
	while (roots) {
		gc_chain *c = roots;
		roots = roots->next;
		delete c;
	}
	
	while (locks) {
		gc_chain *c = locks;
		locks = locks->next;
		delete c;
	}
};

int GC::MIN_CREATED_INTERVAL = 16;

// Called on object creation.
void GC::attach(vobject *o) {
	if (o == nullptr)
		return;
	
	std::unique_lock<std::mutex> guard(protect_lock);
	if (o->gc_record)
		return;
	
	gc_chain *c = new gc_chain;
	if (!c)
		throw std::runtime_exception("GC error");
	
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
void GC::attach_root(vobject *o) {
	if (o == nullptr)
		return;
	
	std::unique_lock<std::mutex> guard(protect_lock);
	if (o->gc_lock)
		return;
	
	gc_chain *c = new gc_chain;
	if (!c)
		throw std::runtime_exception("GC error");
	
	o->gc_root       = 1;
	o->gc_reachable  = 0;
	o->gc_root_chain = c;
	
	c->next = roots;
	c->obj  = o;
	
	roots = c;
	++roots_size;
};

void GC::deattach_root(vobject *o) {
	if (o == nullptr)
		return;
	
	std::unique_lock<std::mutex> guard(protect_lock);
	if (!o->gc_root)
		return;
	
	o->gc_root = 0;
	--roots_size;
};

// Called to lock given object from deletion.
void GC::lock(vobject *o) {
	if (o == nullptr)
		return;
	
	std::unique_lock<std::mutex> guard(protect_lock);
	if (o->gc_lock)
		return;
	
	gc_chain *c = new gc_chain;
	if (!c)
		throw std::runtime_exception("GC error");
	
	o->gc_lock       = 1;
	o->gc_reachable  = 0;
	o->gc_lock_chain = c;
	
	c->next = locks;
	c->obj  = o;
	
	locks = c;
	++locks_size;
};

void GC::unlock(vobject *o) {
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
	
	if (created_interval <= GC.MIN_CREATED_INTERVAL)
		return;
	
	// Call lock on GIL to prevent interruption
	if (!gil->try_lock_threads())
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
			gc_attach(chain->obj);
			chain->obj->gc_lock_chain = nullptr;
			
			gc_list *tmp = chain;
			chain         = chain->next;
			delete tmp;
		} else {
			chain->obj->mark();
			gc_list *tmp = chain;
			chain         = chain->next;
			
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
			--gc_size;			
			delete tmp;
		} else if (!chain->obj->gc_reachable && !chain->obj->gc_root && !chain->obj->gc_lock) {			
			gc_list *tmp = chain;
			chain = chain->next;
			--gc_size;
			tmp->obj->gc_finalize();

			delete tmp->object;
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
	
	gil->unlock_threads();
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
			roots->object->root_chain = nullptr;
		delete roots;
		roots = tmp;
	}
	
	
	// Delete all unused objects
	while (objects) {
		gc_list *tmp = objects->next;
		if (objects->deleted_ptr) {
			delete objects;
		} else {
			--gc_size;
			objects->obj->gc_finalize();
			
			delete objects->obj;
			delete objects;
		} 
		objects = tmp;
	}
	
	collecting = 0;
};

