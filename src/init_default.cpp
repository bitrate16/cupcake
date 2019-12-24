#include "init_default.h"

#include <cstdlib>
#include <iostream>

#include "GIL2.h"
#include "executer.h"
#include "exceptions.h"
#include "parser.h"
#include "translator.h"
#include "script.h"
#include "ast.h"

#include "vscope.h"
#include "objects/Object.h"
#include "objects/Array.h"
#include "objects/String.h"
#include "objects/Int.h"
#include "objects/Double.h"
#include "objects/Bool.h"
#include "objects/Null.h"
#include "objects/Undefined.h"
#include "objects/BytecodeFunction.h"
#include "objects/Cake.h"
#include "objects/NativeFunction.h"
#include "objects/Thread.h"
#include "objects/Native.h"
#include "objects/File.h"

using namespace std;
using namespace ck_core;
using namespace ck_objects;
using namespace ck_vobject;


// F U N C T I O N S

// Print without newline
static vobject* f_print(vscope* scope, const vector<vobject*>& args) {
	for (int i = 0; i < args.size(); ++i)
		if (args[i] != nullptr)
			if (args[i]->as_type<Cake>())
				((Cake*) args[i])->print_backtrace();
			else {
				if (i == args.size() - 1)
					wcout << args[i]->string_value();
				else
					wcout << args[i]->string_value() << ' ';
			}
		else
			if (i == args.size() - 1)
				wcout << "null";
			else
				wcout << "null ";
		
	wcout.flush();
		
	return Undefined::instance();
};

// Print with newline
static vobject* f_println(vscope* scope, const vector<vobject*>& args) {
	for (int i = 0; i < args.size(); ++i)
		if (args[i] != nullptr)
			if (args[i]->as_type<Cake>())
				((Cake*) args[i])->print_backtrace();
			else {
				if (i == args.size() - 1)
					wcout << args[i]->string_value();
				else
					wcout << args[i]->string_value() << ' ';
			}
		else
			if (i == args.size() - 1)
				wcout << "null";
			else
				wcout << "null ";
	
	wcout << endl;
		
	wcout.flush();
		
	return Undefined::instance();
};

// Read single line
static vobject* f_readln(vscope* scope, const vector<vobject*>& args) {
	// Read input string
	std::wstring input;
	if (std::getline(std::wcin, input))
		return new String(input);
	return Undefined::instance();
};

// read single character
static vobject* f_read(vscope* scope, const vector<vobject*>& args) {
	// Read single character
	std::wstring s = L" ";
	wcin >> s[0];
	
	return new String(s);
};

// Do c++ system(cmd);
static vobject* f_system(vscope* scope, const vector<vobject*>& args) {
	if (args.size() == 0)
		return Undefined::instance();
	
	std::wstring arg = args[0]->string_value();
	std::string  exe(arg.begin(), arg.end());
	
	int status = system(exe.c_str());
		
	return new Int(status);
};

// GIL::stop()
static vobject* f_exit(vscope* scope, const vector<vobject*>& args) {
	GIL::instance()->stop();
	return Undefined::instance();
};

// Perform parsing of argument as a code string.
// args[0] - source string for the code.
// args[1] - variable names used in the new code function.
// Example:
//  parse('return a + b;', 'a', 'b');
// Will return Function(a, b).
// Returns Function on success, Array with messages on failture.
static vobject* f_parse(vscope* scope, const vector<vobject*>& args) {
	if (args.size() == 0)
		return Undefined::instance();
	
	if (args[0] == nullptr)
		return Undefined::instance();
	
	std::wstring input_string = args[0]->string_value();
	
	// Convert input file to AST
	ck_parser::stream_wrapper sw(input_string);
	ck_parser::parser_massages pm(GIL::executer_instance()->get_script()->filename);
	ck_parser::parser* p = new ck_parser::parser(pm, sw);
	ck_ast::ASTNode* n = p->parse();
	delete p;
	
	if (pm.errors()) {
		Array* errors = new Array();
		
		for (int i = 0; i < pm.get_messages().size(); ++i) {
			std::wstring message;
			
			if (pm.get_messages()[i].type == ck_parser::parser_message::MSG_ERROR)
				message = L"error: " + pm.get_messages()[i].message + L", at <" + pm.get_filename() + L">:[" + std::to_wstring(pm.get_messages()[i].lineno) + L":" + std::to_wstring(pm.get_messages()[i].charno) + L"]";
			
			if (pm.get_messages()[i].type == ck_parser::parser_message::MSG_WARNING)
				message = L"warning: " + pm.get_messages()[i].message + L", at <" + pm.get_filename() + L">:[" + std::to_wstring(pm.get_messages()[i].lineno) + L":" + std::to_wstring(pm.get_messages()[i].charno) + L"]";
			
			errors->items().push_back(new String(message));
		}
		
		delete n;
		return errors;
	}
	
	// Convert AST to bytecodes & initialize script instance
	ck_core::ck_script* main_script = new ck_script();
	main_script->directory = GIL::executer_instance()->get_script()->directory;
	main_script->filename  = GIL::executer_instance()->get_script()->filename;
	ck_translator::translate_function(main_script->bytecode.bytemap, main_script->bytecode.lineno_table, n);
	
	// Free up memory
	delete n;
	
	// Collect arguments
	std::vector<std::wstring> argn;
	for (int i = 1; i < args.size(); ++i)
		if (args[i])
			argn.push_back(args[i]->string_value());
	
	// Return result
	return new BytecodeFunction(scope, main_script, argn);
};

// Performs parsing of input script source and executing it as expression.
// args[0] - source string for the code.
// args[1] - scope to execute in
// Example:
//  eval('1 + 2') == 3
// Will return Function(a, b).
// Returns value on success, Array with messages on failture.
static vobject* f_eval(vscope* scope, const vector<vobject*>& args) {
	if (args.size() == 0)
		return Undefined::instance();
	
	if (args[0] == nullptr)
		return Undefined::instance();
	
	std::wstring input_string = args[0]->string_value();
	
	// Convert input file to AST
	ck_parser::stream_wrapper sw(input_string);
	ck_parser::parser_massages pm(GIL::executer_instance()->get_script()->filename);
	ck_parser::parser* p = new ck_parser::parser(pm, sw);
	ck_ast::ASTNode* n = p->parse();
	delete p;
	
	if (pm.errors()) {
		Array* errors = new Array();
		
		for (int i = 0; i < pm.get_messages().size(); ++i) {
			std::wstring message;
			
			if (pm.get_messages()[i].type == ck_parser::parser_message::MSG_ERROR)
				message = L"error: " + pm.get_messages()[i].message + L", at <" + pm.get_filename() + L">:[" + std::to_wstring(pm.get_messages()[i].lineno) + L":" + std::to_wstring(pm.get_messages()[i].charno) + L"]";
			
			if (pm.get_messages()[i].type == ck_parser::parser_message::MSG_WARNING)
				message = L"warning: " + pm.get_messages()[i].message + L", at <" + pm.get_filename() + L">:[" + std::to_wstring(pm.get_messages()[i].lineno) + L":" + std::to_wstring(pm.get_messages()[i].charno) + L"]";
			
			errors->items().push_back(new String(message));
		}
		
		delete n;
		return errors;
	}
	
	// Make execution scope for call
	ck_vobject::vscope* func_scope = scope;
	if (args.size() > 1 && args[1])
		if (args[1]->as_type<ck_vobject::vscope>())
			scope = static_cast<ck_vobject::vscope*>(args[1]);
		else
			scope = new ck_vobject::xscope(args[1], scope);
	
	// Record if passed scope was the root scope
	bool scope_was_root = func_scope->gc_is_root();
	if (!scope_was_root)
		func_scope->gc_make_root();
	
	// Iterate over all child nodes & execute each node as function, return last non-null value
	ck_ast::ASTNode* root_node = new ck_ast::ASTNode(0, ck_token::ASTROOT);
	
	// Handle return and return last non-null
	ck_vobject::vobject* ret_value = nullptr;
		
	// Iterate over all nodes of the root and put RETURN at the last expression.
	ck_ast::ASTNode* curr_node = n->left;
	while (curr_node) {
		// Eval current node
		root_node->left = curr_node;
		
		// Change expression type to return value
		if (curr_node && curr_node->type == ck_token::EXPRESSION)
			curr_node->type = ck_token::RETURN;
		
		// Convert AST to bytecodes & initialize script instance
		ck_core::ck_script* main_script = new ck_script();
		main_script->directory = GIL::executer_instance()->get_script()->directory;
		main_script->filename  = GIL::executer_instance()->get_script()->filename;
		ck_translator::translate_function(main_script->bytecode.bytemap, main_script->bytecode.lineno_table, root_node);
		
		// Handle local return
		ck_vobject::vobject* ret = nullptr;
		
		// Execute bytecode in safe context
		try {
			if (GIL::executer_instance())
				ret = GIL::executer_instance()->call_object(main_script, nullptr, {}, L"<eval>", scope, 1, 0);
			
			if (ret)
				ret_value = ret;

			delete main_script;
			
		} catch(const ck_exceptions::cake& msg) {
			// Dispose context
			if (!scope_was_root)
				func_scope->gc_make_unroot();
			root_node->left = nullptr;
			delete main_script;
			delete root_node;
			delete n;
				
			throw msg;
		} catch (const std::exception& ex) {
			// Dispose context
			if (!scope_was_root)
				func_scope->gc_make_unroot();
			root_node->left = nullptr;
			delete main_script;
			delete root_node;
			delete n;
				
			throw ck_exceptions::NativeException(ex);
		} catch (...) {
			// Dispose context
			if (!scope_was_root)
				func_scope->gc_make_unroot();
			root_node->left = nullptr;
			delete main_script;
			delete root_node;
			delete n;
				
			throw ck_exceptions::UnknownException();
		}
		
		curr_node = curr_node->next;
	}
	
	// Dispose context
	if (!scope_was_root)
		func_scope->gc_make_unroot();
	root_node->left = nullptr;
	delete root_node;
	delete n;
	
	if (!ret_value)
		ret_value = Undefined::instance();
	
	return ret_value;
};


// G C

static vobject* f_gc_getUsedMemory(vscope* scope, const vector<vobject*>& args) {
	return new Int(GIL::gc_instance()->get_used_memory());
};

static vobject* f_gc_getObjectCount(vscope* scope, const vector<vobject*>& args) {
	return new Int(GIL::gc_instance()->count());
};

static vobject* c_gc() {
	Object* gc_object = new Object();
	gc_object->Object::put(L"getUsedMemory",  new NativeFunction(f_gc_getUsedMemory));
	gc_object->Object::put(L"getObjectCount", new NativeFunction(f_gc_getObjectCount));
	
	return gc_object;
};


// P L A T F O R M

static vobject* c_platform() {
	Object* gc_object = new Object();
	gc_object->Object::put(L"Name",    new String(ck_platform::get_name()));
	gc_object->Object::put(L"CpuEnv",  new String(ck_platform::get_cpu()));
	gc_object->Object::put(L"Newline", new String(ck_platform::newline_string()));
	gc_object->Object::put(L"Cpu",     new String(ck_platform::cpu_name_string()));
	
	return gc_object;
};

// Get amount of gc objects
static vobject* f_gc_objects(vscope* scope, const vector<vobject*>& args) {
	return new Int(GIL::gc_instance()->count());
};


// E N T R Y

// Initialize default types and functions in root scope
vscope* ck_objects::init_default() {
	
	vscope* scope = new iscope();
	
	// Define root prototypes in new root scope.
	//  By default these values are not destroyed till interpreter finishes it's work.
	
	scope->put(L"Object",           Object          ::create_proto());
	scope->put(L"NativeFunction",   NativeFunction  ::create_proto()); // XXX: Merge prototype with Function
	scope->put(L"Function",         BytecodeFunction::create_proto());
	scope->put(L"Scope",            iscope          ::create_proto()); // XXX: Merge prototype with Scope
	scope->put(L"XScope",           xscope          ::create_proto());
	scope->put(L"Undefined",        Undefined       ::create_proto());
	scope->put(L"Null",             Null            ::create_proto());
	scope->put(L"Int",              Int             ::create_proto());
	scope->put(L"Bool",             Bool            ::create_proto());
	scope->put(L"Double",           Double          ::create_proto());
	scope->put(L"String",           String          ::create_proto());
	scope->put(L"Array",            Array           ::create_proto());
	scope->put(L"Cake",             Cake            ::create_proto());
	scope->put(L"Thread",           Thread          ::create_proto());
	scope->put(L"Native",           Native          ::create_proto());
	scope->put(L"File",             File            ::create_proto());
	
	// O B J E C T S
	scope->put(L"GC", c_gc());
	// O B J E C T S
	scope->put(L"Platform", c_platform());
	
	// Define other objects and fields
	// I O
	scope->put(L"print",   new NativeFunction(f_print));
	scope->put(L"println", new NativeFunction(f_println));
	scope->put(L"readln",  new NativeFunction(f_readln));
	scope->put(L"read",    new NativeFunction(f_read));
	
	// P R O C E S S
	scope->put(L"exit",    new NativeFunction(f_exit));
	
	// P A R S E
	scope->put(L"parse",   new NativeFunction(f_parse));
	scope->put(L"eval",    new NativeFunction(f_eval));
	
	scope->put(L"system",  new NativeFunction(f_system));
	
	return scope;
};

