#include "objects/String.h"

#include <string>
#include <algorithm>

#include "exceptions.h"
#include "GIL2.h"

using namespace std;
using namespace ck_exceptions;
using namespace ck_vobject;
using namespace ck_objects;
using namespace ck_core;


static vobject* call_handler(vscope* scope, const vector<vobject*>& args) {
	if (args.size() == 0)
		return new String();
	
	if (args[0]->is_typeof<String>())
		return new String(((String*) args[0])->value());
		
	return new String(args[0]->string_value());
};

vobject* String::create_proto() {
	if (StringProto != nullptr)
		return StringProto;
	
	StringProto = new CallablePrototype(call_handler);
	GIL::gc_instance()->attach_root(StringProto);
	
	// ...
	
	return StringProto;
};


String::String() {
	if (StringProto == nullptr)
		String::create_proto();
	
	Object::put(wstring(L"proto"), StringProto);
};

String::String(const std::wstring* s) : String() {
	str = *s;
};

String::String(const std::wstring& s) : String() {
	str = s;
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
	Object::put(name, object);
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

vobject* String::call(ck_vobject::vscope* scope, const std::vector<vobject*> args) {
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

// Returns reference to an index of string
wchar_t String::operator[](int index) const {
	return str[index];
};

// Returns value of string index
wchar_t& String::operator[](int index) {
	return str[index];
};

// Compare two strings
bool String::operator==(const String& s) {
	return s.str == str;
};

// Uncompare two strings
bool String::operator!=(const String& s) {
	return s.str != str;
};


// String operations

// Returns char at position.
//  Returns -1 if out of bounds.
wchar_t String::charAt(int index) {
	// XXX: Maybe throw range error?
	if (index < 0 || index >= str.size())
		return -1;

	return str[index];
};

std::wstring String::concatenate(std::wstring& string) {
	return str + string;
};

// Remove all whitespace characters in front of the string
std::wstring String::stripLeading() {
	int i = 0;
	while (i < str.size())
		if (str[i] == ' ' || str[i] == '\t' || str[i] == '\n' || str[i] == '\r')
			++i;
		else
			break;
	
	if (i == str.size() - 1)
		return L"";
	
	return str.substr(i, str.size() - i);
};

// Remove all whitespace characters in back of the string
std::wstring String::stripTrailing() {
	int i = str.size() - 1;
	while (i >= 0)
		if (str[i] == ' ' || str[i] == '\t' || str[i] == '\n' || str[i] == '\r')
			--i;
		else
			break;
	
	if (i == 0)
		return L"";
	
	return str.substr(0, i);
};

// ~ str.stripLeading().stripTrailing()
std::wstring String::strip() {
	int a = 0;
	while (a < str.size())
		if (str[a] == ' ' || str[a] == '\t' || str[a] == '\n' || str[a] == '\r')
			++a;
		else
			break;
	
	if (a == str.size() - 1)
		return L"";
	
	int b = str.size() - 1;
	while (b >= 0)
		if (str[b] == ' ' || str[b] == '\t' || str[b] == '\n' || str[b] == '\r')
			--b;
		else
			break;
	
	if (b == 0)
		return L"";
	
	return str.substr(a, b - a);
	
};

// Returns 1 if string is empty
bool String::isEmpty() {
	return str.size() == 0;
};

// Returns 1 if string consists of whitespace characters
bool String::isBlank() {
	int a = 0;
	while (a < str.size())
		if (str[a] == ' ' || str[a] == '\t' || str[a] == '\n' || str[a] == '\r')
			++a;
		else
			return 0;
	return 1;
};

// Returns index of character in string or -1
int String::indexOf(wchar_t c) {
	int i = 0;
	while (i < str.size())
		if (str[i] == c)
			return i;
		else
			++i;
	return -1;
};

// Returns index of character in string from back or -1
int String::lastIndexOf(wchar_t c) {
	int i = str.size() - 1;
	while (i >= 0)
		if (str[i] == c)
			return i;
		else
			--i;
	return -1;
};

// Returns index of sobstring in string from or -1
int String::indexOf(const std::wstring& s) {
	int ind = str.find(s);
	if (ind == wstring::npos)
		return -1;
	return ind;
};

// Returns index of sobstring in string from back or -1
int String::lastIndexOf(const std::wstring& s) {
	int ind = str.rfind(s);
	if (ind == wstring::npos)
		return -1;
	return ind;
};

// Replace $1 with $2
std::wstring String::replace(wchar_t c1, wchar_t c2) {
	wstring tmp = str;
	std::replace(tmp.begin(), tmp.end(), c1, c2);
	return tmp;
};

// Replace $1 with $2
std::wstring String::replace(const std::wstring& substring, const std::wstring& replacement) {
	wstring tmp = str;
	size_t pos = tmp.find(substring);
 
	if (pos != std::wstring::npos) 
		tmp.replace(pos, substring.size(), replacement);
	
	return tmp;
};

// Replace all $1 with $2
std::wstring String::replaceAll(const std::wstring& substring, const std::wstring& replacement) {
	wstring tmp = str;
	size_t pos = tmp.find(substring);
 
	while (pos != std::wstring::npos) {
		tmp.replace(pos, substring.size(), replacement);
		pos = tmp.find(substring, pos + replacement.size());
	}
	
	return tmp;
};

// Check for $1 is contained in string
bool String::contains(wchar_t c) {
	return indexOf(c) != -1;
};

// Check for $1 is substring
bool String::contains(const std::wstring& substr) {
	return str.find(substr) != wstring::npos;
};

// Returns sobstring [0, $1]
std::wstring String::substring(int start) {
	if (start < 0 || start >= str.size())
		return L"";
	
	return str.substr(start, str.size() - start);
};

// Returns sobstring [$1, $2]
std::wstring String::substring(int start, int length) {
	if (start < 0 || length < 0 || start + length >= str.size() || length == 0)
		return L"";
	
	return str.substr(start, length);
};

std::vector<std::wstring> String::split(const std::wstring& split, int count) {
	wstring str = this->str;
	vector<wstring> result;
	
	if (count < -1)
		count = -1;
	
	if (count == 0 || count == 1) {
		result.push_back(str);
		return result;
	}
	
	while (str.size()) {
		if (count != -1 && result.size() == count - 1) {
			result.push_back(str);
			break;
		}
		
		int index = str.find(split);
		if (index != string::npos) {
			result.push_back(str.substr(0, index));
			str = str.substr(index + split.size());
			
			if (str.size() == 0) 
				result.push_back(str);
		} else {
			result.push_back(str);
			break;
		}
	}
	
	return result;
};

