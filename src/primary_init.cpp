#include "primary_init.h"

#include "objects/Object.h"
#include "objects/Array.h"
#include "objects/String.h"
#include "objects/Int.h"
#include "objects/Double.h"

using namespace ck_objects;
using namespace ck_vobject;

void ck_objects::primary_init(vscope* scope) {
	
	// Define root prototypes
	scope->declare(L"String", String::create_proto());
	scope->declare(L"Array",  Array ::create_proto());
	scope->declare(L"Object", Object::create_proto());
	scope->declare(L"Int",    Object::create_proto());
	scope->declare(L"Double", Object::create_proto());
	
	// Define other objects and fields
};