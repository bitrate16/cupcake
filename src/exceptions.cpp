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
	}
	
	return os;
};


ck_message::~ck_message() throw() {
	switch(message_type) {
		
	};
};