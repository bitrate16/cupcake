#pragma once

#include <exception>
#include <iostream>
#include <string>
#include <cwchar>


// Exception type used for sending messages, interrupts and 
// local information over the cupcake VM.
// Allows twrowing from any place of script context and
// will be safety processed by cupcake try/catch statements.
namespace ck_exceptions {		
	enum ck_message_type {
		CK_WMESSAGE,
		CK_MESSAGE,
		CK_STRING,
		// Failed new()
		BAD_ALLOC,
		// Failed new[]()
		BAD_ALLOC2,
		// catch(...) rethrow
		UNDEFINED_BEHAVIOUR,
		// catch(exception) rethrow
		NATIVE_EXCEPTION
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
		
	public:		
		
		ck_message(const wchar_t* message) throw() : native_wmessage(message), message_type(CK_WMESSAGE) {};
		ck_message(const char* message) throw() : native_message(message), message_type(CK_MESSAGE) {};
		ck_message(const std::wstring& message) throw() : native_string(message), message_type(CK_STRING) {};
		ck_message(const std::exception& ex) throw() : native_exception(ex), message_type(NATIVE_EXCEPTION) {};
		
		ck_message(ck_message_type type) : message_type(type) {};

		
		virtual ~ck_message() throw();
		
		friend std::wostream& operator<<(std::wostream& os, const ck_message& m);
	};
	
	std::wostream& operator<<(std::wostream& os, const ck_message& m);
};