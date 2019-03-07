#include "executer.h"

#include "GIL2.h"

using namespace std;
using namespace ck_core;
using namespace ck_vobject;


// C K _ E X E C U T E R _ G C _ O B J E C T

ck_executer_gc_object::ck_executer_gc_object(ck_executer* exec_instance) {
	this->exec_instance = exec_instance;
	GIL::gc_instance()->attach_root(this);
};

ck_executer_gc_object::~ck_executer_gc_object() {};

void ck_executer_gc_object::gc_mark() {
	for (int i = 0; i < exec_instance->scopes.size(); ++i)
		if (exec_instance->scopes[i])
			exec_instance->scopes[i]->gc_mark();
	for (int i = 0; i < exec_instance->objects.size(); ++i)
		if (exec_instance->objects[i])
			exec_instance->objects[i]->gc_mark();
};

void ck_executer_gc_object::gc_finalize() {};


// C K _ E X E C U T E R

ck_executer::ck_executer() {
	// Will be disposed by GC.
	gc_marker = new ck_executer_gc_object(this);
};

ck_executer::~ck_executer() {
	
};


void start_bytecode(ck_executer* self) { /* process bytecode */ };

void ck_executer::execute(ck_core::ck_script* scr) {
	// Create stack_window and save executer state
	windows.push_back(stack_window());
	windows.back().scope_id  = scopes.size();
	windows.back().script_id = scripts.size();
	windows.back().call_id   = call_stack.size();
	windows.back().try_id    = try_stack.size();
	windows.back().object_id = objects.size();
	windows.back().window_id = windows.size() - 1;
	windows.back().pointer   = pointer;
	
	// Push script instance to the bottom
	scripts.push_back(scr);
	int script_id = scripts.size();
	
	vscope* new_scope = new vscope();
	new_scope->root();
	scopes.push_back(new_scope);
	
	// Reset pointer to 0 and start
	pointer = 0;
	start_bytecode(this);
	
	// Restore previous state
	if (script_id != scripts.size()) {
		// Error happenned, stack shifted..
	}
	
	// Restore all scopes
	for (int i = scopes.size(); i > windows.back().scope_id; --i) {
		if (scopes[i])
			scopes[i]->unroot();
		
		scopes.pop_back();
	}
	
	// Restore all try frames
	for (int i = try_stack.size(); i > windows.back().try_id; --i) 
		try_stack.pop_back();
	
	// Restore all functional frames
	for (int i = call_stack.size(); i > windows.back().call_id; --i) 
		call_stack.pop_back();
	
	pointer = windows.back().pointer;
	windows.pop_back();
	return;
};

void ck_executer::execute(ck_core::ck_script* scr, std::wstring argn, ck_vobject::vobject* argv) {};

void ck_executer::call_object(ck_vobject::vobject* obj) {};

void ck_executer::goto_address(int bytecode_address) {};

void ck_executer::clear() {};

