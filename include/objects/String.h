#pragma once

#include <vector>
#include <cwchar>

#include "Object.h"

namespace ck_objects {	

	class String : public ck_objects::Object {
		
	protected:
		
		// Ъъееъёъъёёъбёъ
		std::wstring str;
		
	public:
		
		String();
		String(const std::wstring*);
		String(const std::wstring&);
		virtual ~String();
		
		virtual vobject* get     (ck_vobject::vscope*, const std::wstring&);
		virtual void     put     (ck_vobject::vscope*, const std::wstring&, vobject*);
		virtual bool     contains(ck_vobject::vscope*, const std::wstring&);
		virtual bool     remove  (ck_vobject::vscope*, const std::wstring&);
		virtual vobject* call    (ck_vobject::vscope*, std::vector<vobject*>);
		
		virtual void gc_mark();
		virtual void gc_finalize();
		
		
		// String functions only
		
		// I. If string length == 1 -> returns charcode at [0].
		// II. If string length == 0 -> returns 0.
		// III. If string is format of int -> return parsed int.
		// IIII. Else return length.
		virtual long long int_value();
		
		// Returns string
		virtual std::wstring string_value();
		
		// Returns length of the string
		int length();
		
		// Returns reference to an index of string
		wchar_t& operator[](int index);
		
		// Returns value of string index
		wchar_t operator[](int index) const;
		
		// Compare two strings
		bool operator==(const String&);
		
		// Uncompare two strings
		bool operator!=(const String&);
		
		
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
		std::wstring stripLeading();
		
		// Remove all whitespace characters in back of the string
		std::wstring stripTrailing();
		
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
		int indexOf(const std::wstring&);
		
		// Returns index of sobstring in string from back or -1
		int lastIndexOf(const std::wstring&);
		
		// Replace all $1 with $2
		std::wstring replace(wchar_t, wchar_t);
		
		// Replace $1 with $2
		std::wstring replace(const std::wstring&, const std::wstring&);
		
		// Replace all $1 with $2
		std::wstring replaceAll(const std::wstring&, const std::wstring&);
		
		// Check for $1 is contained in string
		bool contains(wchar_t);
		
		// Check for $1 is substring
		bool contains(const std::wstring&);
		
		// Returns sobstring [0, $1]
		std::wstring substring(int);
		
		// Returns sobstring [$1, $2]
		std::wstring substring(int, int);
		
		// Returns sobstring [$1, $2]
		std::vector<std::wstring> split(const std::wstring&, int count = -1);
		
		// Called on interpreter start to initialize prototype
		static vobject* create_proto();
	};
	
	// Defined on interpreter start.
	static Object* StringProto = nullptr;
};