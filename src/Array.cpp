#include "objects/Array.h"

#include <string>
#include <sstream>

#include "exceptions.h"
#include "GIL2.h"

#include "objects/Object.h"
#include "objects/Array.h"
#include "objects/Bool.h"
#include "objects/NativeFunction.h"
#include "objects/Undefined.h"
#include "objects/String.h"

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
	
	ArrayProto = new CallableObject(call_handler);
	GIL::gc_instance()->attach_root(ArrayProto);
	
	ArrayProto->Object::put(L"__typename", new String(L"Array"));
	ArrayProto->Object::put(L"size", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			// Validate __this
			if (!scope) return Undefined::instance();
			vobject* __this = scope->get(L"__this", 1);
			if (!__this || !__this->is_typeof<Object>())
				return Undefined::instance();
			
			// Validate args
			bool con = 1;
			for (auto &i : args)
				if (!i || !static_cast<Object*>(__this)->contains(i->string_value())) {
					con = 0;
					break;
				}
			
			if (con)
				return Bool::True();
			else
				return Bool::False();
		}));
	ArrayProto->Object::put(L"clear", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			// Validate __this
			if (!scope) return Undefined::instance();
			vobject* __this = scope->get(L"__this", 1);
			if (!__this || !__this->is_typeof<Object>())
				return Undefined::instance();
			
			if (static_cast<Array*>(__this)->elements.size()) {
				static_cast<Array*>(__this)->elements.clear();
				return Bool::True();
			}
			return Bool::False();
		}));
	ArrayProto->Object::put(L"isEmpty", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			// Validate __this
			if (!scope) return Undefined::instance();
			vobject* __this = scope->get(L"__this", 1);
			if (!__this || !__this->is_typeof<Object>())
				return Undefined::instance();
			
			return Bool::instance(static_cast<Array*>(__this)->elements.size());
		}));
	ArrayProto->Object::put(L"push", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			// Validate __this
			if (!scope) return Undefined::instance();
			vobject* __this = scope->get(L"__this", 1);
			if (!__this || !__this->is_typeof<Object>())
				return Undefined::instance();
			
			for (int i = 0; i < args.size(); ++i)
				static_cast<Array*>(__this)->elements.push_back(args[i]);
			
			return Undefined::instance();
		}));
	ArrayProto->Object::put(L"push_front", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			// Validate __this
			if (!scope) return Undefined::instance();
			vobject* __this = scope->get(L"__this", 1);
			if (!__this || !__this->is_typeof<Object>())
				return Undefined::instance();
			
			Array* a = static_cast<Array*>(__this);
			
			for (int i = 0; i < args.size(); ++i)
				a->elements.insert(a->elements.begin(), args[i]);
			
			return Undefined::instance();
		}));
	ArrayProto->Object::put(L"pop", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			// Validate __this
			if (!scope) return Undefined::instance();
			vobject* __this = scope->get(L"__this", 1);
			if (!__this || !__this->is_typeof<Object>())
				return Undefined::instance();
			
			Array *a = static_cast<Array*>(__this);
			int n_pop = 0;
			
			// Pop N args
			if (args.size() && args[0])
				if ((n_pop = args[0]->int_value()) < 0)
					return Undefined::instance();
				else if (a->elements.size() < n_pop) {
					for (int i = 0; i < n_pop - 1; ++i)
						a->elements.pop_back();
					
					vobject* o = a->elements.back();
					a->elements.pop_back();
					return o;
				} else
					return Undefined::instance();
			
			vobject* o = a->elements.back();
			a->elements.pop_back();
			return o;
		}));
	ArrayProto->Object::put(L"pop_front", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			// Validate __this
			if (!scope) return Undefined::instance();
			vobject* __this = scope->get(L"__this", 1);
			if (!__this || !__this->is_typeof<Object>())
				return Undefined::instance();
			
			Array *a = static_cast<Array*>(__this);
			int n_pop = 0;
			
			// Pop N args
			if (args.size() && args[0])
				if ((n_pop = args[0]->int_value()) < 0)
					return Undefined::instance();
				else if (a->elements.size() < n_pop) {
					for (int i = 0; i < n_pop - 1; ++i)
						a->elements.erase(a->elements.begin());
					
					vobject* o = a->elements.front();
					a->elements.erase(a->elements.begin());
					return o;
				} else
					return Undefined::instance();
			
			vobject* o = a->elements.front();
			a->elements.erase(a->elements.begin());
			return o;
		}));
	
	
	return ArrayProto;
};


Array::Array() {};

Array::Array(const std::vector<ck_vobject::vobject*>& array) : Array() {
	elements = array;
};

		
Array::~Array() {
	
};
		
			
// index < 0 ~ return wrap to the [0, size]
// index > size ~ return nullptr
vobject* Array::get(vscope* scope, const wstring& name) {
	if (name == L"__proto")
		return ArrayProto;
	
	{
		#ifndef CK_SINGLETHREAD
			ck_pthread::mutex_lock lck(mutex());
		#endif
		
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
	if (name == L"__proto")
		return;
	
	{
		#ifndef CK_SINGLETHREAD
			ck_pthread::mutex_lock lck(mutex());
		#endif
		
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
	}
	
	// Else put variable by name
	Object::put(name, object);
};

// index < 0 ~ bound to [0, size] -> contains
// index > size ~ no containment
bool Array::contains(vscope* scope, const wstring& name) {
	if (name == L"__proto")
		return 1;
	
	{
		#ifndef CK_SINGLETHREAD
			ck_pthread::mutex_lock lck(mutex());
		#endif
		
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
	}
	
	return Object::contains(name) || (ArrayProto && ArrayProto->Object::contains(name));
};

// index < 0 ~ bound to [0, size] -> remove
// index > size ~ do nothing
bool Array::remove(vscope* scope, const wstring& name) {
	if (name == L"__proto")
		return 0;
	
	{
		#ifndef CK_SINGLETHREAD
			ck_pthread::mutex_lock lck(mutex());
		#endif
		
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
	}
	
	if (Object::remove(name))
		return 1;
	if (ArrayProto && ArrayProto->Object::remove(name))
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

// Returns size of an array as integer value
long long Array::int_value() { 
	return elements.size(); 
};

// Returns string representation of array elements.
// Unsafe due printing string values of elements.
//  To avoid recursion loop, prints arrays as [...]
std::wstring Array::string_value() { 
	std::wstringstream wss;
	
	wss << '[';
	for (int i = 0; i < elements.size(); ++i) {
		if (elements[i] == nullptr)
			wss << "null";
		else if (dynamic_cast<Array*>(elements[i]))
			wss << "[...]";
		else
			wss << elements[i]->string_value();
		
		if (i != elements.size() - 1)
			wss << ", ";
	}
	wss << ']';
	
	return wss.str();
};

