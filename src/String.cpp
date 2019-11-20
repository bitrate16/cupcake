#include "objects/String.h"

#include <string>
#include <sstream>
#include <algorithm>

#include "exceptions.h"
#include "GIL2.h"

#include "objects/Double.h"
#include "objects/NativeFunction.h"
#include "objects/Undefined.h"
#include "objects/Bool.h"
#include "objects/Int.h"
#include "objects/Array.h"

using namespace std;
using namespace ck_exceptions;
using namespace ck_vobject;
using namespace ck_objects;
using namespace ck_core;


static vobject* call_handler(vscope* scope, const vector<vobject*>& args) {
	if (args.size() == 0)
		return new String();
	
	if (args[0]->as_type<String>())
		return new String(((String*) args[0])->value());
		
	return new String(args[0]->string_value());
};

vobject* String::create_proto() {
	if (StringProto != nullptr)
		return StringProto;
	
	StringProto = new CallableObject(call_handler);
	GIL::gc_instance()->attach_root(StringProto);
	
	StringProto->Object::put(L"__typename", new String(L"String"));	
	StringProto->Object::put(L"isCharacter", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			// Validate __this
			if (!scope) return Undefined::instance();
			vobject* __this = scope->get(L"__this", 1);
			if (!__this || !__this->as_type<String>())
				return Undefined::instance();
			
			String* s = static_cast<String*>(__this);
			
			return Bool::instance(s->value().size() == 1);
		}));
	StringProto->Object::put(L"charAt", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			// Validate __this
			if (!scope) return Undefined::instance();
			vobject* __this = scope->get(L"__this", 1);
			if (!__this || !__this->as_type<String>())
				return Undefined::instance();
			
			String* s = static_cast<String*>(__this);
			
			std::wstringstream charat;
			for (int i = 0; i < args.size(); ++i) 
				if (!args[i])
					return Undefined::instance();
				else
					charat << s->charAt(args[i]->int_value());
			
			return new String(charat.str());
		}));
	StringProto->Object::put(L"cancatenate", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {			
			std::wstringstream cat;
			for (int i = 0; i < args.size(); ++i) 
				if (!args[i])
					return Undefined::instance();
				else
					cat << args[i]->string_value();
			
			return new String(cat.str());
		}));
	StringProto->Object::put(L"stripLeading", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			// Validate __this
			if (!scope) return Undefined::instance();
			vobject* __this = scope->get(L"__this", 1);
			if (!__this || !__this->as_type<String>())
				return Undefined::instance();
			
			String* s = static_cast<String*>(__this);
			
			return new String(s->stripLeading());
		}));
	StringProto->Object::put(L"stripTrailing", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			// Validate __this
			if (!scope) return Undefined::instance();
			vobject* __this = scope->get(L"__this", 1);
			if (!__this || !__this->as_type<String>())
				return Undefined::instance();
			
			String* s = static_cast<String*>(__this);
			
			return new String(s->stripTrailing());
		}));
	StringProto->Object::put(L"strip", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			// Validate __this
			if (!scope) return Undefined::instance();
			vobject* __this = scope->get(L"__this", 1);
			if (!__this || !__this->as_type<String>())
				return Undefined::instance();
			
			String* s = static_cast<String*>(__this);
			
			return new String(s->strip());
		}));
	StringProto->Object::put(L"isEmpty", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			// Validate __this
			if (!scope) return Undefined::instance();
			vobject* __this = scope->get(L"__this", 1);
			if (!__this || !__this->as_type<String>())
				return Undefined::instance();
			
			String* s = static_cast<String*>(__this);
			
			return Bool::instance(s->isEmpty());
		}));
	StringProto->Object::put(L"isBlank", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			// Validate __this
			if (!scope) return Undefined::instance();
			vobject* __this = scope->get(L"__this", 1);
			if (!__this || !__this->as_type<String>())
				return Undefined::instance();
			
			String* s = static_cast<String*>(__this);
			
			return Bool::instance(s->isBlank());
		}));
	StringProto->Object::put(L"indexOf", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			// Validate __this
			if (!scope) return Undefined::instance();
			vobject* __this = scope->get(L"__this", 1);
			if (!__this || !__this->as_type<String>())
				return Undefined::instance();
			
			String* s = static_cast<String*>(__this);
			
			if (args.size() == 0 || !args[0])
				return Undefined::instance();
			
			return new Int(s->indexOf(args[0]->string_value()));
		}));
	StringProto->Object::put(L"lastIndexOf", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			// Validate __this
			if (!scope) return Undefined::instance();
			vobject* __this = scope->get(L"__this", 1);
			if (!__this || !__this->as_type<String>())
				return Undefined::instance();
			
			String* s = static_cast<String*>(__this);
			
			if (args.size() == 0 || !args[0])
				return Undefined::instance();
			
			return new Int(s->lastIndexOf(args[0]->string_value()));
		}));
	StringProto->Object::put(L"replace", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			// Validate __this
			if (!scope) return Undefined::instance();
			vobject* __this = scope->get(L"__this", 1);
			if (!__this || !__this->as_type<String>())
				return Undefined::instance();
			
			String* s = static_cast<String*>(__this);
			
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			return new String(s->replace(args[0]->string_value(), args[1]->string_value()));
		}));
	StringProto->Object::put(L"replaceAll", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			// Validate __this
			if (!scope) return Undefined::instance();
			vobject* __this = scope->get(L"__this", 1);
			if (!__this || !__this->as_type<String>())
				return Undefined::instance();
			
			String* s = static_cast<String*>(__this);
			
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			return new String(s->replaceAll(args[0]->string_value(), args[1]->string_value()));
		}));
	StringProto->Object::put(L"containsString", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			// Validate __this
			if (!scope) return Undefined::instance();
			vobject* __this = scope->get(L"__this", 1);
			if (!__this || !__this->as_type<String>())
				return Undefined::instance();
			
			String* s = static_cast<String*>(__this);
			
			if (args.size() < 1 || !args[0])
				return Undefined::instance();
			
			return Bool::instance(s->contains(args[0]->string_value()));
		}));
	StringProto->Object::put(L"containsString", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			// Validate __this
			if (!scope) return Undefined::instance();
			vobject* __this = scope->get(L"__this", 1);
			if (!__this || !__this->as_type<String>())
				return Undefined::instance();
			
			String* s = static_cast<String*>(__this);
			
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			return new String(s->substring(args[0]->int_value(), args[1]->int_value()));
		}));
	StringProto->Object::put(L"split", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			// Validate __this
			if (!scope) return Undefined::instance();
			vobject* __this = scope->get(L"__this", 1);
			if (!__this || !__this->as_type<String>())
				return Undefined::instance();
			
			String* s = static_cast<String*>(__this);
			
			if (args.size() == 1 && args[0]) {
				auto splt = s->split(args[0]->string_value());
				std::vector<ck_vobject::vobject*> arr(splt.size());
				
				for (int i = 0; i < splt.size(); ++i)
					arr[i] = new String(splt[i]);
				
				return new Array(arr);
			} else if (args.size() > 1 && args[0] && args[1]) {
				auto splt = s->split(args[0]->string_value(), args[1]->int_value());
				std::vector<ck_vobject::vobject*> arr(splt.size());
				
				for (int i = 0; i < splt.size(); ++i)
					arr[i] = new String(splt[i]);
				
				return new Array(arr);
			} else
				return Undefined::instance();
		}));
	
	// Operators
	StringProto->Object::put(L"__operator==", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			return Bool::instance(args[0]->string_value() == args[1]->string_value());
		}));
	StringProto->Object::put(L"__operator!=", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			return Bool::instance(args[0]->string_value() != args[1]->string_value());
		}));
	
	StringProto->Object::put(L"__operator!x", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			// Substract substring
			if (String* i = dynamic_cast<String*>(args[0]); i) 
				return Bool::instance(i->value().size() != 0);
			
			return Undefined::instance();
		}));
		
	StringProto->Object::put(L"__operator+", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			if (String* i = dynamic_cast<String*>(args[0]); i) {
				if (String* s = dynamic_cast<String*>(args[1]); s) 
					return new String(i->value() + s->value());
				return new String(i->value() + args[1]->string_value());
			}
			return Undefined::instance();
		}));
	// Erase substring if exists
	StringProto->Object::put(L"__operator-", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			// Substract substring
			if (String* i = dynamic_cast<String*>(args[0]); i) {
				std::wstring sub = args[1]->string_value();
				size_t found = i->value().find(sub); 
				if (found != string::npos) {
					std::wstring cpy = i->value();
					cpy.erase(found, sub.size());
					return new String(cpy);
				}
				return i;
			}
			return Undefined::instance();
		}));
	// Duplicate string N times
	StringProto->Object::put(L"__operator*", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			// Duplicate string N times
			if (String* i = dynamic_cast<String*>(args[0]); i) {
				std::wostringstream wos;
				int N = args[1]->int_value();
				if (N < 0) {
					std::wstring rev = i->value();
					std::reverse(rev.begin(), rev.end());
					N = -N;
					for (int i = 0; i < N; ++i)
						wos << rev;
				} else if (N > 0)
					for (int j = 0; j < N; ++j)
						wos << i->value();
				return new String(wos.str());
			}
			return Undefined::instance();
		}));
	// Shift string left
	StringProto->Object::put(L"__operator<<", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			// Duplicate string N times
			if (String* i = dynamic_cast<String*>(args[0]); i) {
				if (i->value().size() == 0)
					return i;
				
				int shift = args[1]->int_value() % i->value().size();
				if (shift == 0)
					return i;
				
				std::wstring sh = i->value();
				std::rotate(sh.begin(), sh.begin() + shift, sh.end());
				return new String(sh);
			}
			return Undefined::instance();
		}));
	// Shift string right
	StringProto->Object::put(L"__operator>>", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			// Duplicate string N times
			if (String* i = dynamic_cast<String*>(args[0]); i) {
				if (i->value().size() == 0)
					return i;
				
				int shift = (i->value().size() - args[1]->int_value() % i->value().size()) % i->value().size();
				if (shift == 0)
					return i;
				
				std::wstring sh = i->value();
				std::rotate(sh.begin(), sh.begin() + shift, sh.end());
				return new String(sh);
			}
			return Undefined::instance();
		}));
	// Check for substring containment
	StringProto->Object::put(L"__operator#", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			// Duplicate string N times
			if (String* i = dynamic_cast<String*>(args[0]); i) {
				std::wstring sub = args[1]->string_value();
				size_t found = i->value().find(sub); 
				return Bool::instance(found != string::npos);
			}
			return Undefined::instance();
		}));
	
	
	return StringProto;
};


String::String() {};

String::String(const std::wstring* s) : String() {
	str = *s;
};

String::String(const std::wstring& s) : String() {
	str = s;
};

String::~String() {};

// Map any sign index to [0, size]
vobject* String::get(ck_vobject::vscope* scope, const std::wstring& name) {
	if (name == L"__proto")
		return StringProto;
	
	// Check if string is valid number
	int index = 0;
	std::wistringstream num(name);

	num >> index;

	if(!num.fail() && num.eof()) {
		int index = std::stoi(name);
		index = ((index % str.size()) + str.size()) % str.size();
		
		// Substring of size 1
		return new String(str.substr(index, 1));
	}
	
	// Else return variable by name
	return StringProto ? StringProto->get(scope, name) : nullptr;
};

// Map any sign index to [0, size]
// Disallow assignment of characters.
void String::put(ck_vobject::vscope* scope, const std::wstring& name, vobject* object) {
	if (name == L"__proto")
		return;
	
	// Check if string is valid number
	int index = 0;
	std::wistringstream num(name);

	num >> index;

	if(!num.fail() && num.eof()) 
		throw UnsupportedOperation(L"can not change const string");
};

bool String::contains(ck_vobject::vscope* scope, const std::wstring& name) {
	if (name == L"__proto")
		return 1;
	
	// Check if string is valid number			
	int index = 0;
	std::wistringstream num(name);

	num >> index;

	if(!num.fail() && num.eof()) {
		int index = std::stoi(name);
		if (index < 0)
			return 0;
		
		if (index < str.size())
			return 1;
		
		return 0;
	}
	
	return StringProto && StringProto->Object::contains(name);
};

bool String::remove(ck_vobject::vscope* scope, const std::wstring& name) {
	// Check if string is valid number
	int index = 0;
	std::wistringstream num(name);

	num >> index;

	if(!num.fail() && num.eof()) 
		throw UnsupportedOperation(L"can not change const string");
	
	return StringProto && StringProto->Object::remove(name);
};

vobject* String::call(ck_vobject::vscope* scope, const std::vector<vobject*>& args) {
	throw UnsupportedOperation(L"String is not callable");
};

void String::gc_mark() {
	gc_reachable = 1;
};

void String::gc_finalize() {};


// String functions only

// Returns length
long long String::int_value() {
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
	if (str.size() == 0)
		throw RangeError(L"String is empty");
	
	index = (index % str.size() + str.size()) % str.size();
	
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

