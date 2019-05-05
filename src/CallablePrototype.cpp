#include "objects/CallablePrototype.h"

using namespace std;
using namespace ck_vobject;
using namespace ck_objects;
using namespace ck_core;


CallablePrototype::CallablePrototype(ck_vobject::vobject* (*handler) (ck_vobject::vscope*, const std::vector<ck_vobject::vobject*>&)) {
	call_handler = handler;
};

vobject* CallablePrototype::call(ck_vobject::vscope* scope, const std::vector<vobject*>& args) {
	return call_handler(scope, args);
};

