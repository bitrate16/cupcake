#include "exceptions.h"

#include <exception>
#include <iostream>
#include <string>
#include <cwchar>

#include "GIL2.h"
#include "executer.h"
#include "script.h"

using namespace std;
using namespace ck_exceptions;
using namespace ck_core;


void cake::collect_backtrace() {
	if (contains_backtrace)
		return;
	
	contains_backtrace = 1;
	
	if (GIL::executer_instance() == nullptr)
		return;
	
	for (int i = 0; i < GIL::executer_instance()->call_stack.size(); ++i) {
		backtrace.push_back(BacktraceFrame());
		
		int pointer = GIL::executer_instance()->call_stack[i].pointer;
		int script_id = GIL::executer_instance()->call_stack[i].script_id;
		int lineno = 0;
		if (script_id >= GIL::executer_instance()->scripts.size())
			continue;
		
		ck_script* script = GIL::executer_instance()->scripts[script_id];
		
		if (script->bytecode.lineno_table.size() == 0)
			lineno = -1;
		
		if (pointer >= script->bytecode.bytemap.size()) 
			lineno = script->bytecode.lineno_table.rbegin()[1];
		
		for (int i = 0; i < script->bytecode.lineno_table.size() - 2; i += 2) {
			if (pointer >= script->bytecode.lineno_table[i + 1] && pointer < script->bytecode.lineno_table[i + 3]) {
				lineno = script->bytecode.lineno_table[i];
				break;
			}
		}
		
		backtrace.back().lineno = lineno;
		backtrace.back().filename = script->filename;
		backtrace.back().function = GIL::executer_instance()->call_stack[i].name;
	}
	
	// If no calls still, extract information from closest script
	if (GIL::executer_instance()->call_stack.size() == 0)
		if (GIL::executer_instance()->window_stack.size() != 0) {
			int script_id = GIL::executer_instance()->window_stack.back().script_id + 1;
			
			ck_script* script = GIL::executer_instance()->scripts[script_id];
		
			int lineno = GIL::executer_instance()->lineno();
			
			backtrace.push_back(BacktraceFrame());
			backtrace.back().lineno = lineno;
			backtrace.back().filename = script->filename;
		}
};

void cake::print_backtrace() {
	if (type == L"" && message != L"")
		wcout << "cake: " << message << endl;
	else if (type != L"" && message == L"")
		wcout << type << endl;
	else if (type != L"" && message != L"")
		wcout << type << ": " << message << endl;
	else
		wcout << "cake thrown" << endl;
	
	if (has_backtrace())
		for (int i = 0; i < backtrace.size(); ++i) {
			wcout << " at File <" << backtrace[i].filename << "> line " << backtrace[i].lineno;
			
			if (backtrace[i].function.size() != 0)
				wcout << " " << backtrace[i].function << "()";
			
			int amount = 0;
			while (i + amount + 1 < backtrace.size()) {
				if (backtrace[i].function == backtrace[i + amount + 1].function
					&&
					backtrace[i].lineno   == backtrace[i + amount + 1].lineno
					&&
					backtrace[i].filename == backtrace[i + amount + 1].filename)
					++amount;
				else
					break;
			}
			i += amount;
			
			if (amount) 
				wcout << " + " << amount << " more" << endl;
			else
				wcout << endl;
		}
	else
		wcout << "<no backtrace>" << endl;
};

wostream& ck_exceptions::operator<<(wostream& os, const cake& m) {
	switch(m.type_id) {
		case cake_type::CK_EMPTY: {
			os << "empty cake" << endl;
			break;
		}
		
		case cake_type::CK_CAKE: {
			if (m.type == L"" && m.message == L"")
				os << "cake thrown" << endl;
			else if (m.type != L"" && m.message == L"")
				os << m.type << endl;
			else if (m.type == L"" && m.message != L"")
				os << "cake: " << m.message << endl;
			else
				os << m.type << L": " << m.message << endl;
			break;
		}
		
		case cake_type::CK_OBJECT: {
			os << (m.object ? L"nullptr" : m.object->string_value()) << endl;
			break;
		}
		
		case cake_type::CK_NATIVE_EXCEPTION: {
			os << "std::exception: " << m.exception.what() << endl;
			break;
		}
		
		case cake_type::CK_UNKNOWN_EXCEPTION: {
			os << "unknown exception" << endl;
			break;
		}
	}
	
	// Then print backtrace
	if (m.has_backtrace())
		for (int i = 0; i < m.backtrace.size(); ++i) {
			os << " at File <" << m.backtrace[i].filename << "> line " << m.backtrace[i].lineno;
			
			if (m.backtrace[i].function.size() != 0)
				os << " " << m.backtrace[i].function << "()";
			
			int amount = 0;
			while (i + amount + 1 < m.backtrace.size()) {
				if (m.backtrace[i].function == m.backtrace[i + amount + 1].function
					&&
					m.backtrace[i].lineno   == m.backtrace[i + amount + 1].lineno
					&&
					m.backtrace[i].filename == m.backtrace[i + amount + 1].filename)
					++amount;
				else
					break;
			}
			i += amount;
			
			if (amount) 
				os << " + " << amount << " more" << endl;
			else
				os << endl;
		}
	else
		os << "<no backtrace>" << endl;
	
	return os;
};



