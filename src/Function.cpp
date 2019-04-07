#include "objects/Function.h"

#include <string>

#include "GIL2.h"

using namespace std;
using namespace ck_vobject;
using namespace ck_objects;
using namespace ck_core;

void Function::gc_mark() { gc_reach(); if (this_bind && !this_bind->gc_reachable) this_bind->gc_mark(); };

std::wstring Function::string_value() { return std::wstring(L"[Function ") + std::to_wstring((int) (intptr_t) this) + std::wstring(L"]"); };