#include "primary_init.h"

#include "vscope.h"
#include "objects/Object.h"
#include "objects/Array.h"
#include "objects/String.h"
#include "objects/Int.h"
#include "objects/Double.h"
#include "objects/Bool.h"
#include "objects/Null.h"
#include "objects/Undefined.h"

using namespace ck_objects;
using namespace ck_vobject;

void ck_objects::primary_init(vscope* scope) {
	
	// Define root prototypes
	scope->put(L"vscope",    vscope   ::create_proto());
	scope->put(L"String",    String   ::create_proto());
	scope->put(L"Array",     Array    ::create_proto());
	scope->put(L"Object",    Object   ::create_proto());
	scope->put(L"Int",       Int      ::create_proto());
	scope->put(L"Double",    Double   ::create_proto());
	scope->put(L"Bool",      Bool     ::create_proto());
	scope->put(L"Null",      Null     ::create_proto());
	scope->put(L"Undefined", Undefined::create_proto());
	
	// Define other objects and fields
};