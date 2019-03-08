#include "vobject.h"

#if defined(__BORLANDC__)
    typedef unsigned char uint8_t;
    typedef __int64 int64_t;
    typedef unsigned long uintptr_t;
#elif defined(_MSC_VER)
    typedef unsigned char uint8_t;
    typedef __int64 int64_t;
#else
    #include <stdint.h>
#endif
#include <typeinfo>

#include "GC.h"

using namespace ck_core;
using namespace ck_vobject;
	

// V O B J E C T
	
vobject::vobject() {};
vobject::~vobject() {};

vobject* vobject::get     (vscope* scope, const std::wstring& name) { return nullptr; };
void     vobject::put     (vscope* scope, const std::wstring& name, vobject* obj) {};
bool     vobject::contains(vscope* scope, const std::wstring& name) {return 0; };
bool     vobject::remove  (vscope* scope, const std::wstring& name) { return 0; };
vobject* vobject::call    (vscope* scope, std::vector<vobject*> args) { return nullptr; };

void vobject::gc_mark() { ck_core::gc_object::gc_reachable = 1; };
void vobject::gc_finalize() {};

// Must return integer representation of an object
long long vobject::int_value() { 
	return (int) (intptr_t) this; 
};

// Must return string representation of an object
std::wstring vobject::string_value() { 
	return std::wstring(L"[vobject ") + std::to_wstring((int) (intptr_t) this) + std::wstring(L"]"); 
};
		

// S C O P E

vscope* parent;

vscope::vscope() : vobject::vobject() {};
vscope::~vscope() {};

vobject* vscope::get     (vscope* scope, const std::wstring& name) { return nullptr; };
void     vscope::put     (vscope* scope, const std::wstring& name, vobject* obj) {};
bool     vscope::contains(vscope* scope, const std::wstring& name) {};
bool     vscope::remove  (vscope* scope, const std::wstring& name) {};
vobject* vscope::call    (vscope* scope, std::vector<vobject*> args) { return nullptr; };

void vscope::declare(const std::wstring& scope, vobject* obj) {};

vscope* vscope::get_root() {
	vscope* t = this;
	while (t->parent)
		t = t->parent;
	return t;
};


void vscope::root() {};
void vscope::unroot() {};

void vscope::gc_mark() { ck_core::gc_object::gc_reachable = 1; };
void vscope::gc_finalize() {};
