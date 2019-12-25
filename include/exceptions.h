#pragma once

#include <exception>
#include <iostream>
#include <string>
#include <cwchar>
#include <codecvt>
#include <locale>

#include <vobject.h>


// Exception type used for sending messages, interrupts and 
// local information over the cupcake VM.
// Allows twrowing from any place of script context and
// will be safety processed by cupcake try/catch statements.
// yes, you can throw a cake.
namespace ck_exceptions {		
	// Defines type of cake than could be thrown.
	enum cake_type {
		// Just an empty message for temporary storing messages ¯\_(ツ)_/¯
		CK_EMPTY,
		
		// Any kind of thrown cakes.
		// It might be runtime error, invalid state, e.t.c.
		// Contains .what() for std::exception
		CK_CAKE, // + type + string
		
		// Thrown by sctipt raise statement or any king of rethrowing vobject*
		CK_OBJECT, // + vobject
		
		// catch(...)
		CK_UNKNOWN_EXCEPTION
	};
	
	// Represents instance of single backtrace witdow.
	struct BacktraceFrame {
		
		BacktraceFrame() {};
		
		int lineno;
		std::wstring filename;
		std::wstring function;
	};
	
	// Represents instance of cake in native than can be thrown.
	// Can wrap instance of ck_objects::Cake.
	// Can unsafe wrap instance of std::exception.
	// Can wrap string message and type for error.
	class cake {
		
		// Integer type of a message
		cake_type type_id;
		
		// String type of message telling about type of a cake
		std::wstring type;
		
		// String message telling something about the error
		std::wstring message;
		
		// Pointer to thrown object.
		// Must be rethrown only in save non-locked cuntext to avoid deletion by GC.
		ck_vobject::vobject* object;
		
		// Marks that instance of cake contains backtrace
		bool contains_backtrace = 0;
		
		// Indicates if message can be hander in __defcakehandler
		bool handleable = 1;
		
		std::vector<BacktraceFrame> backtrace;
		
	public:		
	
		// Empty initializer
		cake() : type_id(CK_EMPTY) {};
		
		// Rehandling std::exception (unsafe) or catch(...)
		cake(const std::exception& ex, bool handleable = 1) : 
										handleable(handleable),
										type(L"NativeException"), 
										type_id(CK_CAKE) {
			
			// Convert to wstring
			std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
			message = converter.from_bytes(ex.what());
			
			// collect_backtrace();
		};
		
		// Used by RuntimeCake(type, message, type_id)
		cake(const std::wstring& _type, const std::wstring& _message = L"", bool handleable = 1, cake_type _type_id = CK_CAKE) : 
																										handleable(handleable), 
																										type(_type), 
																										message(_message), 
																										type_id(_type_id) {
			// collect_backtrace();
		};
		
		// Typed message with object
		cake(ck_vobject::vobject* object, bool handleable = 1) : 
										handleable(handleable), 
										object(object), 
										type_id(CK_OBJECT) {
			// collect_backtrace();
		};
												
		cake(const cake& c) {
			type_id    = c.type_id;
			type       = c.type;
			message    = c.message;
			object     = c.object;
			handleable = c.handleable;
			
			contains_backtrace = c.contains_backtrace;
			backtrace          = c.backtrace;
		};
		
		// Perform collection of backtrace for current executer instance.
		// May be useful for debugging and used in executer exceptions rethrowing.
		// Collect backtrace executed only if has_backtrace is 0.
		void collect_backtrace();
		
		// Expects collect_backtrace() to be called first.
		inline const std::vector<BacktraceFrame>& get_backtrace() const {
			return backtrace;
		};
		
		inline bool has_backtrace() const {
			return contains_backtrace;
		};
		
		// Performs printing out backtrace in an STDERR stream.
		void print_backtrace();

		inline cake_type get_type_id() const {
			return type_id;
		};

		inline const std::wstring& get_type() const {
			return type;
		};

		inline const std::wstring& get_message() const {
			return message;
		};
		
		inline ck_vobject::vobject* get_object() const {
			return object;
		};
		
		inline bool is_handleable() {
			return handleable;
		}
		
		inline void set_handleable(bool h) {
			handleable = h;
		}
		
		friend std::wostream& operator<<(std::wostream& os, const cake& m);
	};
	
	std::wostream& operator<<(std::wostream& os, const cake& m);
	
	
	// Decorators for different cake type_id's
	
	static inline cake ObjectCake(ck_vobject::vobject* o, bool handleable = 1) {
		return cake(o, handleable);
	};
	
	static inline cake UnknownException(bool handleable = 1) {
		return cake(L"", L"", handleable, CK_UNKNOWN_EXCEPTION);
	};
	
	// Decorators for different exception types
	
	static inline cake RangeError(const std::wstring& message = L"", bool handleable = 1) {
		return cake(L"RangeError", message, handleable);
	};
	
	static inline cake Error(const std::wstring& message = L"", bool handleable = 1) {
		return cake(L"Error", message, handleable);
	};
	
	static inline cake IllegalStateError(const std::wstring& message = L"", bool handleable = 1) {
		return cake(L"StateError", message, handleable);
	};
	
	static inline cake IllegalArgumentError(const std::wstring& message = L"", bool handleable = 1) {
		return cake(L"IllegalArgumentError", message, handleable);
	};
	
	static inline cake NumberFormatError(const std::wstring& message = L"", bool handleable = 1) {
		return cake(L"NumberFormatError", message, handleable);
	};
	
	static inline cake UndefinedError(const std::wstring& message = L"", bool handleable = 1) {
		return cake(L"UndefinedError", message, handleable);
	};
	
	static inline cake NullPointerError(const std::wstring& message = L"", bool handleable = 1) {
		return cake(L"NullPointerError", message, handleable);
	};
	
	static inline cake IOError(const std::wstring& message = L"", bool handleable = 1) {
		return cake(L"IOError", message, handleable);
	};
	
	static inline cake RuntimeError(const std::wstring& message = L"", bool handleable = 1) {
		return cake(L"RuntimeError", message, handleable);
	};
	
	static inline cake TypeError(const std::wstring& message = L"", bool handleable = 1) {
		return cake(L"TypeError", message, handleable);
	};
	
	static inline cake StackOverflow(const std::wstring& message = L"", bool handleable = 1) {
		return cake(L"StackOverflow", message, handleable);
	};
	
	static inline cake StackCorruption(const std::wstring& message = L"", bool handleable = 1) {
		return cake(L"StackCorruption", message, handleable);
	};
	
	static inline cake UnsupportedOperation(const std::wstring& message = L"", bool handleable = 1) {
		return cake(L"UnsupportedOperation", message, handleable);
	};
	
	// Rethrow native exception
	static inline cake NativeException(const std::exception& rethrow_exception, bool handleable = 1) {
		std::string message_string(rethrow_exception.what());
		std::wstring message_wstring(message_string.begin(), message_string.end());
		return cake(L"NativeException", message_wstring, handleable);
	};
	
	// i.e. Bad_Alloc
	static inline cake OutOfMemory(const std::wstring& message = L"", bool handleable = 1) {
		return cake(L"OutOfMemory", message, handleable);
	};
};

// but this cake is lie
