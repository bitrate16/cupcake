#include "primary_init.h"

#include "objects/Object.h"
#include "objects/Array.h"
#include "objects/String.h"

using namespace ck_objects;
using namespace ck_vobject;

void ck_objects::primary_init(vscope* scope) {
	scope->declare(L"String", String::create_proto());
	scope->declare(L"Array",  Array ::create_proto());
	scope->declare(L"Object", Object::create_proto());
};