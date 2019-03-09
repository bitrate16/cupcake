#pragma once

#include <exception>
#include <iostream>
#include <string>
#include <cwchar>

#include <vobject.h>


// Exception type used for sending messages, interrupts and 
// local information over the cupcake VM.
// Allows twrowing from any place of script context and
// will be safety processed by cupcake try/catch statements.
namespace ck_exceptions {		
	enum ck_message_type {
		CK_WCMESSAGE, // + wchar_t*
		CK_CMESSAGE, // + char*
		CK_STRING, // + string
		// Failed new()
		BAD_ALLOC,
		// Failed new[]()
		BAD_ALLOC2,
		
		// User-friendly:
		
		// catch(...) rethrow
		UNDEFINED_BEHAVIOUR,
		// catch(exception) rethrow
		NATIVE_EXCEPTION,  // + exception
		// For example when trying to assign index of string
		// Message expected in wstring
		CK_UNSUPPORTED_OPERATION, // + string
		// Any king of ck runtime errors
		// Message expected in wstring
		CK_RUNTIME_ERROR, // + string
		// Thrown on executer stack corruption
		CK_STACK_CORRUPTED, // + string
		// Thrown on invalid operaions on types
		CK_TYPE_ERROR, // + string
		// Thrown on invalid state
		CK_INVALID_STATE, // + string,
		// Thrown by sctipt raise statement or any king of rethrowing vobject*
		CK_OBJECT, // + vobject
		// Returning value from script function
		CK_RETURN // + vobject
	};
	
	class ck_message {
		ck_message_type message_type;
		
		// CK_WMESSAGE: Onlys tring message that is thrown to up
		const wchar_t* native_wmessage = nullptr;
		
		// CK_MESSAGE: Onlys tring message that is thrown to up
		const char* native_message = nullptr;
		
		// CK_STRING: Onlys tring message that is thrown to up
		std::wstring native_string = nullptr;
		
		// Copy of axception
		std::exception native_exception;
		
		ck_vobject::vobject* script_object;
		
	public:		
		
		ck_message(const wchar_t* message) throw() : native_wmessage(message), message_type(CK_WCMESSAGE) {};
		ck_message(const char* message) throw() : native_message(message), message_type(CK_CMESSAGE) {};
		ck_message(const std::wstring& message) throw() : native_string(message), message_type(CK_STRING) {};
		ck_message(const std::exception& ex) throw() : native_exception(ex), message_type(NATIVE_EXCEPTION) {};
		
		ck_message(const std::wstring& message, ck_message_type type) throw() : native_string(message), message_type(type) {};
		ck_message(ck_message_type type) : message_type(type) {};
		ck_message(ck_vobject::vobject* object, ck_message_type type = CK_OBJECT) : script_object(object), message_type(type) {};

		
		virtual ~ck_message() throw();
		
		friend std::wostream& operator<<(std::wostream& os, const ck_message& m);
	};
	
	std::wostream& operator<<(std::wostream& os, const ck_message& m);
};