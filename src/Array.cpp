#include "objects/Array.h"

#include <string>

#include "exceptions.h"
#include "GIL2.h"

using namespace std;
using namespace ck_exceptions;
using namespace ck_vobject;
using namespace ck_objects;
using namespace ck_core;


static vobject* call_handler(vscope* scope, const vector<vobject*>& args) {
	return new Array(args);
};

vobject* Array::create_proto() {
	if (ArrayProto != nullptr)
		return ArrayProto;
	
	ArrayProto = new CallablePrototype(call_handler);
	GIL::gc_instance()->attach_root(ArrayProto);
	
	// ...
	
	return ArrayProto;
};


Array::Array() {
	if (ArrayProto == nullptr)
		Array::create_proto();
	
	Object::put(wstring(L"proto"), ArrayProto);
};

Array::Array(const std::vector<ck_vobject::vobject*>& array) : Array() {
	elements = array;
};

		
Array::~Array() {
	
};
		
			
// index < 0 ~ return wrap to the [0, size]
// index > size ~ return nullptr
vobject* Array::get(vscope* scope, const wstring& name) {
	// Check if string is valid number
	bool is_int = 1;
	int chk_ind = 0;
	if (name[chk_ind] == U'-' || name[chk_ind] == U'+')
		++chk_ind;
	for (; chk_ind < name.size(); ++chk_ind)
		if (U'0' <= name[chk_ind] && U'9' <= name[chk_ind]) {
			is_int = 0;
			break;
		}
		
	if (is_int) {
		int index = std::stoi(name);
		if (index < 0)
			index = ((index % elements.size()) + elements.size()) % elements.size();
		
		while (index >= elements.size())
			elements.push_back(nullptr);
		
		return elements[index];
	}
	
	// Else return variable by name
	vobject* ret = Object::get(name);
	if (!ret && ArrayProto)
		return ArrayProto->get(scope, name);
	return ret;
};

// index < 0 ~ bound to [0, size]
// index > size ~ allocate till [0, index]
void Array::put(vscope* scope, const wstring& name, vobject* object) {
	// Check if string is valid number
	bool is_int = 1;
	int chk_ind = 0;
	if (name[chk_ind] == U'-' || name[chk_ind] == U'+')
		++chk_ind;
	for (; chk_ind < name.size(); ++chk_ind)
		if (U'0' <= name[chk_ind] && U'9' <= name[chk_ind]) {
			is_int = 0;
			break;
		}
	
	// If valid integer
	if (is_int) {
		int index = std::stoi(name);
		if (index < 0)
			index = ((index % elements.size()) + elements.size()) % elements.size();
		
		while (index >= elements.size())
			elements.push_back(nullptr);
		
		elements[index] = object;
		return;
	}
	
	// Else put variable by name
	Object::put(name, object);
};

// index < 0 ~ bound to [0, size] -> contains
// index > size ~ no containment
bool Array::contains(vscope* scope, const wstring& name) {
	// Check if string is valid number
	bool is_int = 1;
	int chk_ind = 0;
	if (name[chk_ind] == U'-' || name[chk_ind] == U'+')
		++chk_ind;
	for (; chk_ind < name.size(); ++chk_ind)
		if (U'0' <= name[chk_ind] && U'9' <= name[chk_ind]) {
			is_int = 0;
			break;
		}
	
	// If valid integer
	if (is_int) {
		int index = std::stoi(name);
		if (index < 0)
			return 0;
		
		if (index < elements.size())
			return 1;
		
		return 0;
	}
	
	return Object::contains(name) || (ArrayProto && ArrayProto->contains(scope, name));
};

// index < 0 ~ bound to [0, size] -> remove
// index > size ~ do nothing
bool Array::remove(vscope* scope, const wstring& name) {
	// Check if string is valid number
	bool is_int = 1;
	int chk_ind = 0;
	if (name[chk_ind] == U'-' || name[chk_ind] == U'+')
		++chk_ind;
	for (; chk_ind < name.size(); ++chk_ind)
		if (U'0' <= name[chk_ind] && U'9' <= name[chk_ind]) {
			is_int = 0;
			break;
		}
	
	// If valid integer
	if (is_int) {
		int index = std::stoi(name);
		if (index < 0)
			index = ((index % elements.size()) + elements.size()) % elements.size();
		
		while (index >= elements.size())
			return 0;
		
		elements.erase(elements.begin() + index);
		return 1;
	}
	
	if (Object::remove(name))
		return 1;
	if (ArrayProto && ArrayProto->remove(scope, name))
		return 1;
	return 0;
};

vobject* Array::call(vscope* scope, const vector<vobject*>& args) {
	// XXX: Construct object from input
	throw UnsupportedOperation(L"Array is not callable");
};

void Array::gc_mark() {
	if (gc_reachable)
		return;
	
	gc_reach();
	
	for (const auto& any : objects) 
		if (any.second && !any.second->gc_reachable)
			any.second->gc_mark();
	
	for (int i = 0; i < elements.size(); ++i)
		if (elements[i] && !elements[i]->gc_reachable)
			elements[i]->gc_mark();
};

void Array::gc_finalize() {};
		
		
// Array functions only

void Array::append(Array* array) {
	if (!array)
		return;
	
	elements.insert(elements.end(), array->elements.begin(), array->elements.end());
};

int Array::size() { return elements.size(); };

vobject* Array::get_item(int index, bool range_wrap) {
	if (index < 0 && range_wrap)
		index = ((index % elements.size()) + elements.size()) % elements.size();
	else if (range_wrap)
		index = index % elements.size();
	
	if (0 > index || index >= elements.size())
		return nullptr;
	
	return elements[index];
};

bool Array::set_item(int index, vobject* object, bool range_wrap, bool range_check) {
	if (index < 0 && range_wrap)
		index = ((index % elements.size()) + elements.size()) % elements.size();
	else if (range_wrap)
		index = index % elements.size();
	
	if (0 > index)
		return 0;
	else while (index >= elements.size())
		elements.push_back(nullptr);
	
	elements[index] = object;
	return 1;
};

