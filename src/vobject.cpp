#include "vobject.h"

#include "GC.h"

using namespace ck_core;
using namespace ck_vobject;
	

// V O B J E C T
	
vobject::vobject() {};
vobject::~vobject() {};

vobject* vobject::get     (vscope* s, const std::wstring n) {};
vobject* vobject::put     (vscope* s, const std::wstring n, vobject* o) {};
bool     vobject::contains(vscope* s, const std::wstring n) {};
bool     vobject::remove  (vscope* s, const std::wstring n) {};
vobject* vobject::call    (vscope* s, std::vector<vobject*> args) {};

void vobject::gc_mark() { ck_core::gc_object::gc_reachable = 1; };
void vobject::gc_finalize() {};


// S C O P E

vscope* parent;

vscope::vscope() : vobject::vobject() {};
vscope::~vscope() {};

vobject* vscope::get     (vscope*s, const std::wstring n) {};
vobject* vscope::put     (vscope*s, const std::wstring n, vobject* o) {};
bool     vscope::contains(vscope*s, const std::wstring n) {};
bool     vscope::remove  (vscope*s, const std::wstring n) {};
vobject* vscope::call    (vscope*s, std::vector<vobject*> args) {};

// Force declare variable in this scope.
void vscope::declare(const std::wstring, vobject* obj) {};

// Makes scope being gc_root
void vscope::root() {};
// Unmakes scope being gc_root
void vscope::unroot() {};

void vscope::gc_mark() { ck_core::gc_object::gc_reachable = 1; };
void vscope::gc_finalize() {};
