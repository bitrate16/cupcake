#include "exceptions.h"

#include <exception>
#include <iostream>
#include <string>
#include <cwchar>

using namespace std;
using namespace ck_exceptions;


wostream& ck_exceptions::operator<<(wostream& os, const ck_message& m) {
	switch(m.message_type) {
		case ck_message_type::CK_MESSAGE: {
			os << "ck_message: " << m.native_message << endl;
			break;
		}
		
		case ck_message_type::CK_WMESSAGE: {
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
	}
	
	return os;
};


ck_message::~ck_message() throw() {
	switch(message_type) {
		
	};
};