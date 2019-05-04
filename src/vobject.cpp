#include "vobject.h"

// Used for pointer casting
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
#include "vscope.h"

using namespace ck_core;
using namespace ck_vobject;
	

// V O B J E C T
	
vobject::vobject() {};
vobject::~vobject() {};

vobject* vobject::get     (vscope* scope, const std::wstring& name)               { return nullptr; };
void     vobject::put     (vscope* scope, const std::wstring& name, vobject* obj) {};
bool     vobject::contains(vscope* scope, const std::wstring& name)               { return 0; };
bool     vobject::remove  (vscope* scope, const std::wstring& name)               { return 0; };
vobject* vobject::call    (vscope* scope, const std::vector<vobject*>& args)      { return nullptr; };

void vobject::gc_mark()     { ck_core::gc_object::gc_reachable = 1; };
void vobject::gc_finalize() {};

// Must return integer representation of an object
long long vobject::int_value() { 
	return (int) (intptr_t) this; 
};

// Must return string representation of an object
std::wstring vobject::string_value() { 
	return std::wstring(L"[vobject ") + std::to_wstring((int) (intptr_t) this) + std::wstring(L"]"); 
};
