#include "exceptions.h"

#include <exception>
#include <iostream>
#include <string>
#include <cwchar>

using namespace std;
using namespace ck_exceptions;


wostream& ck_exceptions::operator<<(wostream& os, const ck_message& m) {
	switch(m.message_type) {
		case ck_message_type::CK_CMESSAGE: {
			os << "ck_message: " << m.native_message << endl;
			break;
		}
		
		case ck_message_type::CK_WCMESSAGE: {
			os << "ck_message: " << m.native_wmessage << endl;
			break;
		}
		
		case ck_message_type::CK_STRING: {
			os << "ck_message: " << m.native_string << endl;
			break;
		}
		
		case ck_message_type::BAD_ALLOC: 
		case ck_message_type::BAD_ALLOC2: {
			os << "ck_message: BAD_ALLOC" << endl;
			break;
		}
		
		case ck_message_type::UNDEFINED_BEHAVIOUR: {
			os << "ck_message: UNDEFINED_BEHAVIOUR rethrow catch(...)" << endl;
			break;
		}
		
		case ck_message_type::NATIVE_EXCEPTION: {
			os << "ck_message: NATIVE_EXCEPTION: " << m.native_exception.what() << endl;
			break;
		}
		
		case ck_message_type::CK_UNSUPPORTED_OPERATION: {
			os << "ck_message: CK_UNSUPPORTED_OPERATION: " << m.native_string << endl;
			break;
		}
		
		case ck_message_type::CK_RUNTIME_ERROR: {
			os << "ck_message: RUNTIME_ERROR: " << m.native_string << endl;
			break;
		}
		
		case ck_message_type::CK_STACK_CORRUPTED: {
			os << "ck_message: CK_STACK_CORRUPTED: " << m.native_string << endl;
			break;
		}
		
		case ck_message_type::CK_TYPE_ERROR: {
			os << "ck_message: CK_TYPE_ERROR: " << m.native_string << endl;
			break;
		}
		
		case ck_message_type::CK_INVALID_STATE: {
			os << "ck_message: CK_INVALID_STATE: " << m.native_string << endl;
			break;
		}
		
		case ck_message_type::CK_OBJECT: {
			os << "ck_message: CK_OBJECT" << endl;
			break;
		}
		
		case ck_message_type::CK_STACK_OVERFLOW: {
			os << "ck_message: CK_STACK_OVERFLOW: " << m.native_string << endl;
			break;
		}
		
		case ck_message_type::CK_EMPTY: {
			os << "ck_message: CK_EMPTY: " << endl;
			break;
		}
	}
	
	return os;
};


ck_message::~ck_message() throw() {
	switch(message_type) {
		
	};
};