#include "objects/String.h"

#include <string>

#include "exceptions.h"
#include "GIL2.h"

using namespace std;
using namespace ck_exceptions;
using namespace ck_vobject;
using namespace ck_objects;
using namespace ck_core;

vobject* String::create_proto() {
	StringProto = new Object();
	GIL::gc_instance()->attach_root(StringProto);
	
	// ...
	
	return StringProto;
};


String::String() {
	if (StringProto == nullptr)
		create_proto();
	
	Object::put(wstring(L"proto"), StringProto);
};

String::String(const std::wstring* s) {
	if (StringProto == nullptr)
		create_proto();
	
	str = *s;
	
	Object::put(wstring(L"proto"), StringProto);
};

String::String(const std::wstring& s) {
	if (StringProto == nullptr)
		create_proto();
	
	str = s;
	
	Object::put(wstring(L"proto"), StringProto);
};

String::~String() {
	
};

// Map any sign index to [0, size]
vobject* String::get(ck_vobject::vscope* scope, const std::wstring& name) {
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
		index = ((index % str.size()) + str.size()) % str.size();
		
		// Substring of size 1
		return new String(str.substr(index, 1));
	}
	
	// Else return variable by name
	vobject* ret = Object::get(name);
	if (!ret && StringProto)
		return StringProto->get(scope, name);
	return ret;
};

// Map any sign index to [0, size]
// Disallow assignment of characters.
void String::put(ck_vobject::vscope* scope, const std::wstring& name, vobject* object) {
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
	if (is_int) 
		throw ck_message(L"can not change const string", ck_message_type::CK_UNSUPPORTED_OPERATION);
	
	// Else put variable by name
	if (Object::contains(name))
		Object::put(name, object);
	else if(StringProto)
		StringProto->put(scope, name, object);
};

bool String::contains(ck_vobject::vscope* scope, const std::wstring& name) {
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
		
		if (index < str.size())
			return 1;
		
		return 0;
	}
	
	return Object::contains(name) || (StringProto && StringProto->contains(scope, name));
};

bool String::remove(ck_vobject::vscope* scope, const std::wstring& name) {
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
	if (is_int) 
		throw ck_message(L"can not change const string", ck_message_type::CK_UNSUPPORTED_OPERATION);
	
	if (Object::remove(name))
		return 1;
	if (StringProto && StringProto->remove(scope, name))
		return 1;
	return 0;
};

vobject* String::call(ck_vobject::vscope* scope, std::vector<vobject*> args) {
	throw ck_message(L"String is not callable", ck_message_type::CK_UNSUPPORTED_OPERATION);
};

void String::gc_mark() {
	Object::gc_mark();
};

void String::gc_finalize() {
	Object::gc_finalize();
};


// String functions only

// I. If string length == 1 -> returns charcode at [0].
// II. If string length == 0 -> returns 0.
// III. If string is format of int -> return parsed int.
// IIII. Else return length.
long long String::int_value() {
	if (str.size() == 1) 
		return str[0];
	if (str.size() == 0)
		return 0;
	
	bool is_int = 1;
	int chk_ind = 0;
	if (str[chk_ind] == U'-' || str[chk_ind] == U'+')
		++chk_ind;
	for (; chk_ind < str.size(); ++chk_ind)
		if (U'0' <= str[chk_ind] && U'9' <= str[chk_ind]) {
			is_int = 0;
			break;
		}
	
	if (is_int)
		return std::stoi(str);;
	
	return str.size();
};

// Returns string
wstring String::string_value() {
	return str;
};

// Returns length of the string
int String::length() {
	return str.size();
};
/*
// Returns reference to an index of string
wchar_t& operator[](int index);

// Returns value of string index
wchar_t& operator[](int index) const;

// Compare two strings
bool operator==(const String&);

// Uncompare two strings
bool operator!=(const String&);
*/

// String operations
wchar_t charAt(int index);
/* {
// XXX: Maybe throw range error?
if (index < 0 || index >= string.size())
return -1;

return string[index];
};*/

std::wstring concatenate(std::wstring&);

// Remove all whitespace characters in front of the string
std::wstring stringLeading();

// Remove all whitespace characters in back of the string
std::wstring stringTrailing();

// ~ str.stripLeading().stripTrailing()
std::wstring strip();

// Returns 1 if string is empty
bool isEmpty();

// Returns 1 if string consists of whitespace characters
bool isBlank();

// Returns index of character in string or -1
int indexOf(wchar_t);

// Returns index of character in string from back or -1
int lastIndexOf(wchar_t);

// Returns index of sobstring in string from or -1
int indexOf(std::wstring);

// Returns index of sobstring in string from back or -1
int lastIndexOf(std::wstring);

// Replace $1 with $2
std::wstring replace(wchar_t, wchar_t);

// Replace $1 with $2
std::wstring replace(std::wstring, std::wstring);

// Check for $1 is contained in string
std::wstring contains(wchar_t);

// Check for $1 is substring
std::wstring contains(std::wstring);

// Returns sobstring [0, $1]
std::wstring substring(int);

// Returns sobstring [$1, $2]
std::wstring substring(int, int);