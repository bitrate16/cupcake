#include "objects/BytecodeFunction.h"

#include <string>

#include "exceptions.h"
#include "GIL2.h"
#include "executer.h"
#include "exceptions.h"
#include "parser.h"
#include "translator.h"
#include "script.h"
#include "ast.h"

#include "objects/Array.h"
#include "objects/String.h"
#include "objects/Undefined.h"

using namespace std;
using namespace ck_exceptions;
using namespace ck_vobject;
using namespace ck_objects;
using namespace ck_core;


static vobject* call_handler(vscope* scope, const vector<vobject*>& args) {
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

vobject* BytecodeFunction::create_proto() {
	if (BytecodeFunctionProto != nullptr)
		return BytecodeFunctionProto;
	
	BytecodeFunctionProto = new CallableObject(call_handler);
	GIL::gc_instance()->attach_root(BytecodeFunctionProto);
	
	BytecodeFunctionProto->Object::put(L"__typename", new String(L"Function"));	
	
	return BytecodeFunctionProto;
};


BytecodeFunction::BytecodeFunction(ck_vobject::vscope* definition_scope, ck_script* function_script, const std::vector<std::wstring>& argnames) : scope(definition_scope), script(function_script), argn(argnames) {};

BytecodeFunction::~BytecodeFunction() {
	delete script;
};

// Delegate to prototype
vobject* BytecodeFunction::get(ck_vobject::vscope* scope, const std::wstring& name) {
	if (name == L"__proto")
		return BytecodeFunctionProto;
	
	return BytecodeFunctionProto ? BytecodeFunctionProto->Object::get(scope, name) : nullptr;
};

void BytecodeFunction::put(ck_vobject::vscope* scope, const std::wstring& name, vobject* object) {
	throw UnsupportedOperation(L"BytecodeFunction is not container");
};

// Delegate to prototype
bool BytecodeFunction::contains(ck_vobject::vscope* scope, const std::wstring& name) {	
	if (name == L"__proto")
		return 1;
	
	return BytecodeFunctionProto && BytecodeFunctionProto->Object::contains(scope, name);
};

bool BytecodeFunction::remove(ck_vobject::vscope* scope, const std::wstring& name) {
	throw UnsupportedOperation(L"BytecodeFunction is not container");
};

vobject* BytecodeFunction::call(ck_vobject::vscope* scope, const std::vector<vobject*>& args) {
	throw UnsupportedOperation(L"BytecodeFunction is not directly callable");
};

vscope* BytecodeFunction::apply(ck_vobject::vobject* this_bind, const std::vector<ck_vobject::vobject*>& args, ck_vobject::vscope* caller_scope) {
	vscope* nscope = new iscope(scope);
	
	// Apply args
	Array* arguments = new Array(args);
	nscope->put(L"__args", arguments);
	
	int min = argn.size();
	min = min < args.size() ? min : args.size();
	
	for (int i = 0; i < min; ++i)
		nscope->put(argn[i], args[i], 0, 1);
	
	// Bind __this
	nscope->put(L"__this", this_bind);
	
	return nscope;
};

void BytecodeFunction::gc_mark() {
	if (gc_reachable)
		return;
	
	Function::gc_mark();
	
	if (scope && !scope->gc_reachable)
		scope->gc_mark();
};


wstring BytecodeFunction::string_value() {
	return std::wstring(L"[Function ") + std::to_wstring((intptr_t) this) + std::wstring(L"]"); 
};

