#include "vobject.h"

#include "GC.h"

using namespace ck_core;
using namespace ck_vobject;
	

// V O B J E C T
	
vobject::vobject() {};
vobject::~vobject() {};

vobject* vobject::get     (vscope* scope, const std::wstring& name) { return nullptr; };
vobject* vobject::put     (vscope* scope, const std::wstring& name, vobject* obj) { return nullptr; };
bool     vobject::contains(vscope* scope, const std::wstring& name) {return 0; };
bool     vobject::remove  (vscope* scope, const std::wstring& name) { return 0; };
vobject* vobject::call    (vscope* scope, std::vector<vobject*> args) { return nullptr; };

void vobject::gc_mark() { ck_core::gc_object::gc_reachable = 1; };
void vobject::gc_finalize() {};


// S C O P E

vscope* parent;

vscope::vscope() : vobject::vobject() {};
vscope::~vscope() {};

vobject* vscope::get     (vscope* scope, const std::wstring& name) { return nullptr; };
vobject* vscope::put     (vscope* scope, const std::wstring& name, vobject* obj) { return nullptr; };
bool     vscope::contains(vscope* scope, const std::wstring& name) {};
bool     vscope::remove  (vscope* scope, const std::wstring& name) {};
vobject* vscope::call    (vscope* scope, std::vector<vobject*> args) { return nullptr; };

// Force declare variable in this scope.
void vscope::declare(const std::wstring& scope, vobject* obj) {};

// Makes scope being gc_root
void vscope::root() {};
// Unmakes scope being gc_root
void vscope::unroot() {};

void vscope::gc_mark() { ck_core::gc_object::gc_reachable = 1; };
void vscope::gc_finalize() {};
