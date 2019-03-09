#include "executer.h"

#include <vector>
#include <map>

#include "GIL2.h"
#include "vobject.h"
#include "script.h"
#include "translator.h"

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


using namespace std;
using namespace ck_core;
using namespace ck_vobject;
using namespace ck_objects;
using namespace ck_exceptions;


// C K _ E X E C U T E R _ G C _ O B J E C T

ck_executer_gc_object::ck_executer_gc_object(ck_executer* exec_instance) {
	this->exec_instance = exec_instance;
	GIL::gc_instance()->attach_root(this);
};

ck_executer_gc_object::~ck_executer_gc_object() {};

void ck_executer_gc_object::gc_mark() {
	for (int i = 0; i < exec_instance->scopes.size(); ++i)
		if (exec_instance->scopes[i])
			exec_instance->scopes[i]->gc_mark();
	for (int i = 0; i < exec_instance->objects.size(); ++i)
		if (exec_instance->objects[i])
			exec_instance->objects[i]->gc_mark();
};

void ck_executer_gc_object::gc_finalize() {};


// C K _ E X E C U T E R

ck_executer::ck_executer() {
	// Will be disposed by GC.
	gc_marker = new ck_executer_gc_object(this);
};

ck_executer::~ck_executer() {
	
};


bool ck_executer::read(int size, void* buf) {
	if (!scripts.back())
		return 0;
	
	if (pointer + size > scripts.back()->bytecode.bytemap.size())
		return 0;
	
	for (int i = 0; i < size; ++i)
		((unsigned char*) buf)[i] = scripts.back()->bytecode.bytemap[pointer++];
	
	return 1;
};

inline bool ck_executer::is_eof() {
	return !scripts.back() || pointer >= scripts.back()->bytecode.bytemap.size() || scripts.back()->bytecode.bytemap[pointer] == ck_bytecodes::HALT || pointer == -1;
};

ck_vobject::vobject* ck_executer::vpop() {
	if (objects.size() == 0)
		throw ck_message(L"objects stack corrupted", ck_message_type::CK_STACK_CORRUPTED);
	
	vobject* t = objects.back();
	objects.pop_back();
	
	return t;
};

ck_vobject::vobject* ck_executer::vpeek() {
	if (objects.size() == 0)
		throw ck_message(L"objects stack corrupted", ck_message_type::CK_STACK_CORRUPTED);
	
	return objects.back();
};

void ck_executer::vpush(ck_vobject::vobject* o) {	
	objects.push_back(o);
};

void ck_executer::vswap() {
	if (objects.size() < 2)
		throw ck_message(L"objects stack corrupted", ck_message_type::CK_STACK_CORRUPTED);
	
	std::iter_swap(objects.end(), objects.end() - 1);
};

void ck_executer::vswap1() {
	if (objects.size() < 3)
		throw ck_message(L"objects stack corrupted", ck_message_type::CK_STACK_CORRUPTED);
	
	std::iter_swap(objects.end() - 1, objects.end() - 2);
};

void ck_executer::vswap2() {
	if (objects.size() < 4)
		throw ck_message(L"objects stack corrupted", ck_message_type::CK_STACK_CORRUPTED);
	
	std::iter_swap(objects.end() - 2, objects.end() - 3);
};

int ck_executer::lineno() {
	if (scripts.back()->bytecode.lineno_table.size() == 0)
		return 0;
	
	if (pointer >= scripts.back()->bytecode.bytemap.size()) 
		return scripts.back()->bytecode.lineno_table.rbegin()[1];
	
	for (int i = 0; i < scripts.back()->bytecode.lineno_table.size() - 2; i += 2) {
		if (i >= scripts.back()->bytecode.lineno_table[i + 1] && i < scripts.back()->bytecode.lineno_table[i + 3])
			return scripts.back()->bytecode.lineno_table[i];
	}
	
	return -1;
};

void ck_executer::validate_scope() {
	if (scopes.size() == 0 || scopes.back() == nullptr)
		throw ck_message(L"scopes stack corrupted", ck_message_type::CK_STACK_CORRUPTED);		
};

void ck_executer::exec_bytecode() {	
	while (!is_eof()) {
		
		switch(scripts.back()->bytecode.bytemap[pointer++]) {
			case ck_bytecodes::LINENO: {
				int lineno; 
				read(sizeof(int), &lineno);
				wcout << "LINENO: " << lineno << endl;
				break;
			}
			
			case ck_bytecodes::NOP: {
				wcout << "> NOP" << endl;
				break;
			}
			
			case ck_bytecodes::PUSH_CONST_INT: {
				long long i; 
				read(sizeof(long long), &i);
				vpush(new Int(i));
				wcout << "> PUSH_CONST[int]: " << i << endl;
				break;
			}
			
			case ck_bytecodes::PUSH_CONST_DOUBLE: {
				double i; 
				read(sizeof(double), &i);
				vpush(new Double(i));
				wcout << "> PUSH_CONST[double]: " << i << endl;
				break;
			}
			
			case ck_bytecodes::PUSH_CONST_BOOLEAN: {
				bool i; 
				read(sizeof(bool), &i);
				vpush(new Bool(i));
				wcout << "> PUSH_CONST[boolean]: " << i << endl;
				break;
			}
			
			case ck_bytecodes::PUSH_CONST_NULL: {
				vpush(Null::instance());
				wcout << "> PUSH_CONST: null" << endl;
				break;
			}
			
			case ck_bytecodes::PUSH_CONST_UNDEFINED: {
				vpush(Undefined::instance());
				wcout << "> PUSH_CONST: undefined" << endl;
				break;
			}
			
			case ck_bytecodes::PUSH_CONST_STRING: {
				int size;
				read(sizeof(int), &size);
				wchar_t cstr[size+1];
				read(sizeof(wchar_t) * size, cstr);
				cstr[size] = 0;
				
				vpush(new String(cstr));
				wcout << "> PUSH_CONST[string]: \"" << cstr << '"' << endl;
				break;
			}
			
			case ck_bytecodes::LOAD_VAR: {
				int size;
				read(sizeof(int), &size);
				wchar_t cstr[size+1];
				read(sizeof(wchar_t) * size, cstr);
				cstr[size] = 0;
				
				// Check for valid scope
				validate_scope();
				
				// Scope should return nullptr if value does not exist.
				vobject* o = scopes.back()->get(cstr);
				if (o == nullptr)
					throw ck_message(wstring(L"undefined reference to ") + cstr, ck_message_type::CK_TYPE_ERROR);
				
				vpush(o);
				wcout << "> LOAD_VAR: " << cstr << endl;
				break;
			}
			
			case ck_bytecodes::VSTACK_POP: {
				vpop();
				wcout << "> VSTACK_POP" << endl;
				break;
			}
			
			case ck_bytecodes::PUSH_CONST_ARRAY: {
				int size; 
				read(sizeof(int), &size);
				
				vector<vobject*> array;
				for (int i = 0; i < size; ++i)
					array.push_back(vpop());
				vpush(new Array(array));
				
				wcout << "> PUSH_CONST[array]: [" << size << ']' << endl;
				break;
			}
			
			case ck_bytecodes::PUSH_CONST_OBJECT: {
				int size; 
				read(sizeof(int), &size);
				wcout << "> PUSH_CONST[object]: {";
				
				map<wstring, vobject*> objects;
				
				for (int i = 0; i < size; ++i) {
					int ssize;
					read(sizeof(int), &ssize);
					wchar_t cstr[ssize+1];
					read(sizeof(wchar_t) * ssize, cstr);
					cstr[ssize] = 0;
					
					objects[cstr] = vpop();
					
					wcout << cstr;
					if (i != size-1)
						wcout << ", ";
				}
				
				vpush(new Object(objects));
				
				wcout << '}' << endl;
				break;
			}
			
			case ck_bytecodes::DEFINE_VAR: {
				int amount; 
				read(sizeof(int), &amount);
				wcout << "> DEFINE_VAR: ";
				
				for (int i = 0; i < amount; ++i) {
					int ssize = 0;
					read(sizeof(int), &ssize);
					
					wchar_t cstr[ssize+1];
					read(sizeof(wchar_t) * ssize, cstr);
					cstr[ssize] = 0;
					wcout << cstr;
					
					unsigned char ops = 0;
					read(sizeof(unsigned char), &ops);
					
					// Check for valid scope
					validate_scope();			
					
					if ((ops & 0b1000) == 0) {
						scopes.back()->put(cstr, Undefined::instance(), 0, 1);
						wcout << " = [undefined]";
					} else
						scopes.back()->put(cstr, vpop(), 0, 1);
					
					if (i != amount-1)
						wcout << ", ";
				}
				
				wcout << endl;
				break;
			}
		
			case ck_bytecodes::VSTACK_DUP: {
				vpush(vpeek());
				wcout << "> VSTACK_DUP" << endl;
				break;
			}
			
			case ck_bytecodes::CALL: {
				// bytecode: CALL [argc]
				// stack: argN..arg0 fun
				
				int argc; 
				read(sizeof(int), &argc);
				
				wcout << "> CALL [" << argc << ']' << endl;
				
				if (objects.size() < argc + 1)
					throw ck_message(L"objects stack corrupted", ck_message_type::CK_INVALID_STATE); 
				
				// Copy args
				vector<vobject*> args;
				for (int k = 0; k < argc; ++k)
					args.push_back(objects.rbegin()[k + 1]);
				
				vobject* obj = call_object(objects.rbegin()[0], nullptr, args, L"");
				for (int k = 0; k < argc + 1; ++k)
					objects.pop_back();
				vpush(obj);
				
				break;
			}
			
			case ck_bytecodes::CALL_FIELD: {
				// bytecode: CALL [argc] [name]
				// stack: argN..arg0 ref
				
				int argc; 
				read(sizeof(int), &argc);
				
				int size;
				read(sizeof(int), &size);
				wchar_t cstr[size+1];
				read(sizeof(wchar_t) * size, cstr);
				cstr[size] = 0;
				
				wcout << "> CALL_FIELD [" << argc << "] [" << cstr << ']' << endl;
				
				if (objects.size() < argc + 1)
					throw ck_message(L"objects stack corrupted", ck_message_type::CK_INVALID_STATE); 
				
				if (objects.rbegin()[0] == nullptr)
					throw ck_message(wstring(L"undefined reference to ") + cstr, ck_message_type::CK_TYPE_ERROR);
					
				validate_scope();
					
				vpush(objects.rbegin()[0]->get(scopes.back(), cstr));
				// stack: argN..arg0 ref fun
				
				// Copy args
				vector<vobject*> args;
				for (int k = 0; k < argc; ++k)
					args.push_back(objects.rbegin()[k + 2]);
				
				vobject* obj = call_object(objects.rbegin()[0], objects.rbegin()[1], args, cstr);
				for (int k = 0; k < argc + 2; ++k)
					objects.pop_back();
				vpush(obj);
				
				break;
			}
			
			case ck_bytecodes::CALL_NAME: {
				// bytecode: CALL [argc] [name]
				// stack: argN..arg0
				
				int argc; 
				read(sizeof(int), &argc);
				
				int size;
				read(sizeof(int), &size);
				wchar_t cstr[size+1];
				read(sizeof(wchar_t) * size, cstr);
				cstr[size] = 0;
				
				wcout << "> CALL_NAME [" << argc << "] [" << cstr << ']' << endl;
				
				if (objects.size() < argc)
					throw ck_message(L"objects stack corrupted", ck_message_type::CK_INVALID_STATE); 
					
				validate_scope();
					
				vpush(scopes.back()->get(scopes.back(), cstr));
				// stack: argN..arg0 fun
				
				// Copy args
				vector<vobject*> args;
				for (int k = 0; k < argc; ++k)
					args.push_back(objects.rbegin()[k]);
				
				vobject* obj = call_object(objects.rbegin()[0], nullptr, args, cstr);
				for (int k = 0; k < argc; ++k)
					objects.pop_back();
				vpush(obj);
				
				break;
			}
			
			case ck_bytecodes::CALL_MEMBER: {
				// bytecode: CALL [argc] [name]
				// stack: argN..arg0 ref key
				
				int argc; 
				read(sizeof(int), &argc);
				
				if (objects.size() < argc + 2)
					throw ck_message(L"objects stack corrupted", ck_message_type::CK_INVALID_STATE); 
				
				if (objects.rbegin()[0] == nullptr)
					throw ck_message(wstring(L"undefined reference to member"), ck_message_type::CK_TYPE_ERROR);
				
				wstring key = objects.rbegin()[0]->string_value();
				
				wcout << "> CALL_MEMBER [" << argc << "] [" << key << ']' << endl;
				
				if (objects.rbegin()[1] == nullptr)
					throw ck_message(wstring(L"undefined reference to ") + key, ck_message_type::CK_TYPE_ERROR);
					
				validate_scope();
					
				vpush(objects.rbegin()[1]->get(scopes.back(), key));
				// stack: argN..arg0 ref key fun
				
				// Copy args
				vector<vobject*> args;
				for (int k = 0; k < argc; ++k)
					args.push_back(objects.rbegin()[k + 3]);
				
				vobject* obj = call_object(objects.rbegin()[0], objects.rbegin()[2], args, L"[" + key + L"]");
				for (int k = 0; k < argc + 3; ++k)
					objects.pop_back();
				vpush(obj);
				
				break;
			}
			
			case ck_bytecodes::OPERATOR: {
				unsigned char i; 
				read(sizeof(unsigned char), &i);
				
				validate_scope();
				
				if (objects.size() < 2)
					throw ck_message(L"objects stack corrupted", ck_message_type::CK_STACK_CORRUPTED);
				
				vobject *ref = objects.rbegin()[1];
				vobject *fun = nullptr;
				
				if (ref == nullptr)
					throw ck_message(wstring(L"undefined reference to operator"), ck_message_type::CK_TYPE_ERROR);
				
				wstring fun_name;
				
				// lvalue operator
				switch (i) {
					case ck_bytecodes::OPT_ADD    : fun_name = (L"+"); break;
					case ck_bytecodes::OPT_SUB    : fun_name = (L"-"); break;
					case ck_bytecodes::OPT_MUL    : fun_name = (L"*"); break;
					case ck_bytecodes::OPT_DIV    : fun_name = (L"/"); break;
					case ck_bytecodes::OPT_BITRSH : fun_name = (L">>"); break;
					case ck_bytecodes::OPT_BITLSH : fun_name = (L"<<"); break;
					case ck_bytecodes::OPT_BITURSH: fun_name = (L">>>"); break;
					case ck_bytecodes::OPT_DIR    : fun_name = (L"\\"); break;
					case ck_bytecodes::OPT_PATH   : fun_name = (L"\\\\"); break;
					case ck_bytecodes::OPT_MOD    : fun_name = (L"%"); break;
					case ck_bytecodes::OPT_BITOR  : fun_name = (L"|"); break;
					case ck_bytecodes::OPT_BITAND : fun_name = (L"&"); break;
					case ck_bytecodes::OPT_HASH   : fun_name = (L"#"); break;
					case ck_bytecodes::OPT_EQ     : fun_name = (L"=="); break;
					case ck_bytecodes::OPT_NEQ    : fun_name = (L"!="); break;
					case ck_bytecodes::OPT_OR     : fun_name = (L"||"); break;
					case ck_bytecodes::OPT_AND    : fun_name = (L"&&"); break;
					case ck_bytecodes::OPT_GT     : fun_name = (L">"); break;
					case ck_bytecodes::OPT_GE     : fun_name = (L">="); break;
					case ck_bytecodes::OPT_LT     : fun_name = (L"<"); break;
					case ck_bytecodes::OPT_LE     : fun_name = (L"<="); break;
					case ck_bytecodes::OPT_PUSH   : fun_name = (L"=>"); break;
					case ck_bytecodes::OPT_BITXOR : fun_name = (L"^"); break;
				}
				
				wcout << "> OPERATOR [" << fun_name << ']' << endl;
				
				fun = ref->get(scopes.back(), L"__operator" + fun_name);
				
				// rvalue operator
				if (fun == nullptr || fun->is_typeof<Undefined>() || fun->is_typeof<Null>()) 
					fun = ref->get(scopes.back(), L"__roperator" + fun_name);
				else
					fun_name = L"__operator" + fun_name;
				
				fun = ref->get(scopes.back(), L"__operator" + fun_name);				
					
				// no operator
				if (fun == nullptr || fun->is_typeof<Undefined>() || fun->is_typeof<Null>()) 
					throw ck_message(wstring(L"undefined reference to operator ") + fun_name, ck_message_type::CK_TYPE_ERROR);
				else
					fun_name = L"__roperator" + fun_name;
				
				vobject* res = call_object(fun, ref, { ref, objects.rbegin()[0] }, fun_name);
				
				vpop(); 
				vpop();
				vpush(res);
				
				break;
			}
			
			case ck_bytecodes::STORE_VAR: {
				int size;
				read(sizeof(int), &size);
				wchar_t cstr[size+1];
				read(sizeof(wchar_t) * size, cstr);
				cstr[size] = 0;
				
				// Check for scope
				validate_scope();		
				
				scopes.back()->put(cstr, vpop(), 0, 1);
				
				wcout << "> STORE_VAR: " << cstr << endl;
				break;
			}
			
			case ck_bytecodes::STORE_FIELD: {
				int size;
				read(sizeof(int), &size);
				wchar_t cstr[size+1];
				read(sizeof(wchar_t) * size, cstr);
				cstr[size] = 0;
				
				vobject* val = vpop();
				vobject* ref = vpop();
				
				if (ref == nullptr)
					throw ck_message(wstring(L"undefined reference to ") + cstr, ck_message_type::CK_TYPE_ERROR);
				
				// Check for scope
				validate_scope();		
				
				ref->put(scopes.back(), cstr, val);
				
				wcout << "> STORE_FIELD: " << cstr << endl;
				break;
			}
			
			case ck_bytecodes::STORE_MEMBER: {
				vobject* val = vpop();
				vobject* key = vpop();
				vobject* ref = vpop();
				
				if (ref == nullptr)
					throw ck_message(wstring(L"undefined reference"), ck_message_type::CK_TYPE_ERROR);
				
				if (key == nullptr)
					throw ck_message(wstring(L"undefined reference to member"), ck_message_type::CK_TYPE_ERROR);
				
				// Check for scope
				validate_scope();		
				
				ref->put(scopes.back(), key->string_value(), val);
				
				wcout << "> STORE_MEMBER " << endl;
				break;
			}
			
			case ck_bytecodes::LOAD_FIELD: {
				int size;
				read(sizeof(int), &size);
				wchar_t cstr[size+1];
				read(sizeof(wchar_t) * size, cstr);
				cstr[size] = 0;
				
				vobject* ref = vpop();
				
				if (ref == nullptr)
					throw ck_message(wstring(L"undefined reference to ") + cstr, ck_message_type::CK_TYPE_ERROR);
				
				// Check for scope
				validate_scope();		
				
				vpush(ref->get(scopes.back(), cstr));
				
				wcout << "> LOAD_FIELD: " << cstr << endl;
				break;
			}
			
			case ck_bytecodes::LOAD_MEMBER: {
				vobject* key = vpop();
				vobject* ref = vpop();
				
				if (ref == nullptr)
					throw ck_message(wstring(L"undefined reference"), ck_message_type::CK_TYPE_ERROR);
				
				if (key == nullptr)
					throw ck_message(wstring(L"undefined reference to member"), ck_message_type::CK_TYPE_ERROR);
				
				// Check for scope
				validate_scope();		
				
				vpush(ref->get(scopes.back(), key->string_value()));
				
				wcout << "> LOAD_MEMBER " << endl;
				break;
			}
		
			case ck_bytecodes::UNARY_OPERATOR: {				
				unsigned char i; 
				read(sizeof(unsigned char), &i);
				
				validate_scope();
				
				if (objects.size() == 0)
					throw ck_message(L"objects stack corrupted", ck_message_type::CK_STACK_CORRUPTED);
				
				vobject *ref = objects.rbegin()[0];
				vobject *fun = nullptr;
				
				if (ref == nullptr)
					throw ck_message(wstring(L"undefined reference to operator"), ck_message_type::CK_TYPE_ERROR);
				
				wstring fun_name;
				
				// lvalue operator
				switch (i) {
					case ck_bytecodes::OPT_DOG   : fun_name = (L"@x"); break;
					case ck_bytecodes::OPT_NOT   : fun_name = (L"!x"); break;
					case ck_bytecodes::OPT_BITNOT: fun_name = (L"~x"); break;
					case ck_bytecodes::OPT_POS   : fun_name = (L"+x"); break;
					case ck_bytecodes::OPT_NEG   : fun_name = (L"-x"); break;
					case ck_bytecodes::OPT_INC   : fun_name = (L"++x"); break;
					case ck_bytecodes::OPT_DEC   : fun_name = (L"--x"); break;
				}
				
				wcout << "> OPERATOR [" << fun_name << ']' << endl;
				
				fun = ref->get(scopes.back(), L"__operator" + fun_name);
				
				// rvalue operator
				if (fun == nullptr || fun->is_typeof<Undefined>() || fun->is_typeof<Null>()) 
					fun = ref->get(scopes.back(), L"__roperator" + fun_name);
				else
					fun_name = L"__operator" + fun_name;
				
				fun = ref->get(scopes.back(), L"__operator" + fun_name);				
					
				// no operator
				if (fun == nullptr || fun->is_typeof<Undefined>() || fun->is_typeof<Null>()) 
					throw ck_message(wstring(L"undefined reference to operator ") + fun_name, ck_message_type::CK_TYPE_ERROR);
				else
					fun_name = L"__roperator" + fun_name;
				
				vobject* res = call_object(fun, ref, { ref }, fun_name);
				
				vpop(); 
				vpush(res);
				
				break;
			}
			
			case ck_bytecodes::VSTACK_SWAP: {
				vswap();
				
				wcout << "> VSTACK_SWAP" << endl;
				break;
			}
			
			case ck_bytecodes::VSTACK_SWAP1: {
				vswap1();
				
				wcout << "> VSTACK_SWAP1" << endl;
				break;
			}
			
			case ck_bytecodes::VSTACK_SWAP2: {
				vswap2();
				
				wcout << "> VSTACK_SWAP2" << endl;
				break;
			}
			
			case ck_bytecodes::VSTATE_PUSH_SCOPE: {
				vscope* s = new vscope(scopes.size() == 0 ? nullptr : scopes.back());
				GIL::gc_instance()->attach_root(s);
				scopes.push_back(s);
				
				wcout << "> VSTATE_PUSH_SCOPE" << endl;
				break;
			}
			
			case ck_bytecodes::VSTATE_POP_SCOPE: {
				// Check for scope
				validate_scope();	
				
				vscope* s = scopes.back();
				GIL::gc_instance()->deattach_root(s);
				scopes.pop_back();
				
				wcout << "> VSTATE_POP_SCOPE" << endl;
				break;
			}
			
			case ck_bytecodes::JMP_IF_ZERO: {
				int i; 
				read(sizeof(int), &i);
				
				vobject* o = vpop();
				if (o == nullptr || o->int_value() == 0)
					goto_address(i);
				
				wcout << "> JMP_IF_ZERO [" << i << ']' << endl;
				break;
			}
			
			case ck_bytecodes::JMP: {
				int i; 
				read(sizeof(int), &i);
				
				goto_address(i);
				
				wcout << "> JMP [" << i << ']' << endl;
				break;
			}
			
			case ck_bytecodes::HALT: {
				wcout << "> HALT" << endl;
				
				--pointer;
				return;
				
				break;
			}
			
			case ck_bytecodes::RAISE_NOARG: {
				wcout << "> RAISE_NOARG" << endl;
				
				throw ck_message((vobject*) nullptr);
				
				break;
			}
			
			case ck_bytecodes::RAISE: {				
				wcout << "> RAISE" << endl;
				
				throw ck_message(vpop());
				
				break;
			}
			
			case ck_bytecodes::RAISE_STRING: {
				int size;
				read(sizeof(int), &size);
				wchar_t cstr[size+1];
				read(sizeof(wchar_t) * size, cstr);
				cstr[size] = 0;
				
				wcout << "> RAISE_STRING: \"" << cstr << '"' << endl;
				
				throw ck_message(new String(cstr));
				
				break;
			}
			
			case ck_bytecodes::VSTATE_POP_SCOPES: {
				int i; 
				read(sizeof(int), &i);
				wcout << "> VSTATE_POP_SCOPES [" << i << ']' << endl;
				
				if (scopes.size() < i)
					throw ck_message(L"scopes stack corrupted", ck_message_type::CK_STACK_CORRUPTED);
				
				for (int k = 0; k < i; ++k)
					scopes.pop_back();
				
				break;
			}
			
			case ck_bytecodes::RETURN_VALUE: {
				wcout << "> RETURN_VALUE" << endl;
				
				throw ck_message(vpop(), ck_message_type::CK_RETURN);
				
				break;
			}
			
			case ck_bytecodes::PUSH_CONST_FUNCTION: {
				
				// Check for scope
				validate_scope();		
				
				int argc; 
				read(sizeof(int), &argc);
				wcout << "> PUSH_CONST_FUNCTION (" << argc << ") (";
				
				vector<wstring> argn;
				
				for (int i = 0; i < argc; ++i) {
					int ssize = 0;
					read(sizeof(int), &ssize);
					
					wchar_t cstr[ssize+1];
					read(sizeof(wchar_t) * ssize, cstr);
					cstr[ssize] = 0;
					wcout << cstr;
					
					argn.push_back(cstr);
					
					if (i != argc-1)
						wcout << ", ";
				}
				
				int sizeof_block; 
				read(sizeof(int), &sizeof_block);
				
				wcout << ") [" << sizeof_block << "]" << endl;
				
				ck_script* script = new ck_script();
				script->directory = scripts.back()->directory;
				script->filename  = scripts.back()->filename;
				
				// Copy range of bytecodes
				
				script->bytecode.bytemap = vector<unsigned char>(scripts.back()->bytecode.bytemap.begin() + pointer, scripts.back()->bytecode.bytemap.begin() + pointer + sizeof_block);
				
				// int lineno_a = lineno();
				// int lineno_b = lineno();
				
				// Copy range of lineno table
				
				for (int i = 0; i < scripts.back()->bytecode.lineno_table.size();) {
					int lineno = scripts.back()->bytecode.lineno_table[i++];
					int byteof = scripts.back()->bytecode.lineno_table[i++];
					
					if (pointer >= byteof || pointer + sizeof_block >= byteof) {
						script->bytecode.lineno_table.push_back(lineno);
						script->bytecode.lineno_table.push_back((byteof - pointer) < 0 ? 0 : byteof - pointer); // Offset roll back
					}
				}
				
				// Append last marker
				script->bytecode.lineno_table.push_back(-1);
				script->bytecode.lineno_table.push_back(sizeof_block);
				
				/*
				wcout << "Function Bytecode: " << endl;
				ck_translator::print(script->bytecode.bytemap);
				wcout << endl;
				
				wcout << "Function Lineno Table: " << endl;
				ck_translator::print_lineno_table(script->bytecode.lineno_table);
				wcout << endl;
				*/				
				
				pointer += sizeof_block;
				
				vpush(new BytecodeFunction(scopes.back(), script, argn));
				
				break;
			}
		
			case ck_bytecodes::VSTATE_POP_TRY: {
				if (try_stack.size() == 0)
					throw ck_message(L"try stack corrupted", ck_message_type::CK_STACK_CORRUPTED);
				
				// No explicit restoration. 
				// Everything expected to be fine.
				try_stack.pop_back();
				
				wcout << "> VSTATE_POP_TRY" << endl;
				break;
			}

			case ck_bytecodes::VSTATE_PUSH_TRY: {
				int try_node = 0;
				int catch_node = 0;
				wstring handler_name = 0;
				unsigned char type;
				read(sizeof(unsigned char), &type);
				
				if (type == ck_bytecodes::TRY_NO_CATCH) {
					try_node;
					int exit;
					
					read(sizeof(int), &try_node);
					read(sizeof(int), &exit);
					
					wcout << "> VSTATE_PUSH_TRY [TRY_NO_CATCH] [" << try_node << "] [" << exit << ']' << endl;
				} else if (type == ck_bytecodes::TRY_NO_ARG) {
					try_node;
					int catch_node;
					
					read(sizeof(int), &try_node);
					read(sizeof(int), &catch_node);
					
					wcout << "> VSTATE_PUSH_TRY [TRY_NO_ARG] [" << try_node << "] [" << catch_node << ']' << endl;
				} else {
					try_node;
					catch_node;
					int name_size;
					
					read(sizeof(int), &try_node);
					read(sizeof(int), &catch_node);
					read(sizeof(int), &name_size);
										
					wchar_t cstr[name_size+1];
					read(sizeof(wchar_t) * name_size, cstr);
					cstr[name_size] = 0;
					handler_name = cstr;
					
					wcout << "> VSTATE_PUSH_TRY [TRY_WITH_ARG] (" << cstr << ") [" << try_node << "] [" << catch_node << ']' << endl;
				}
				
				try_stack.push_back(stack_try());
				try_stack.back().scope_id = scopes.size() - 1;
				try_stack.back().script_id = scripts.size() - 1;
				try_stack.back().object_id = objects.size() - 1;
				try_stack.back().window_id = windows.size() - 1;
				try_stack.back().pointer = pointer;
				try_stack.back().try_node = try_node;
				try_stack.back().catch_node = catch_node;
				try_stack.back().type = type;
				try_stack.back().handler = handler_name;
				
				break;
			}
			
			default: throw ck_message(L"invalid bytecode [" + to_wstring(scripts.back()->bytecode.bytemap[pointer++]) + L"]", ck_message_type::CK_INVALID_STATE);
		}
		
		// Respond to GIL requests
		GIL::instance()->accept_lock();
		
		// Perform GC collection ?
	}
};

void ck_executer::execute(ck_core::ck_script* scr, ck_vobject::vscope* scope, std::vector<std::wstring>* argn, std::vector<ck_vobject::vobject*>* argv) {
	// Create stack_window and save executer state
	windows.push_back(stack_window());
	windows.back().scope_id  = scopes.size()     - 1;
	windows.back().script_id = scripts.size()    - 1;
	windows.back().call_id   = call_stack.size() - 1;
	windows.back().try_id    = try_stack.size()  - 1;
	windows.back().object_id = objects.size()    - 1;
	windows.back().window_id = windows.size()    - 1;
	windows.back().pointer   = pointer;
	
	// Push script instance to the bottom
	scripts.push_back(scr);
	int script_id = scripts.size();
	
	if (scope == nullptr) {
		scope = new vscope();
		scope->root();
	}
		
	if (argn != nullptr && argv != nullptr) {
		int argc = argn->size() < argv->size() ? argn->size() : argv->size();
		for (int i = 0; i < argc; ++i)
			scope->put((*argn)[i], (*argv)[i]);
	}
	
	scopes.push_back(scope);
	
	// Reset pointer to 0 and start
	pointer = 0;
	
	try {
		exec_bytecode();
	} catch(const ck_exceptions::ck_message& msg) { 
		throw msg;
	} catch (const std::exception& ex) {
		throw(ex);
	}  catch (...) {
		throw(ck_exceptions::ck_message_type::NATIVE_EXCEPTION);
	} 
	
	// Restore scripts
	for (int i = scripts.size() - 1; i > script_id; --i) 
		scripts.pop_back();
	
	// Restore all scopes
	for (int i = scopes.size() - 1; i > windows.back().scope_id; --i) {
		if (scopes[i] && (scope == nullptr || i != windows.back().scope_id))
			scopes[i]->unroot();
		
		scopes.pop_back();
	}
	
	// Restore all try frames
	for (int i = try_stack.size() - 1; i > windows.back().try_id; --i) 
		try_stack.pop_back();
	
	// Restore all functional frames
	for (int i = call_stack.size() - 1; i > windows.back().call_id; --i) 
		call_stack.pop_back();
	
	// Restore objects stack
	for (int i = objects.size() - 1; i > windows.back().object_id; --i) 
		objects.pop_back();
	
	pointer = windows.back().pointer;
	windows.pop_back();
	return;
};

ck_vobject::vobject* ck_executer::call_object(ck_vobject::vobject* obj, ck_vobject::vobject* ref, const std::vector<ck_vobject::vobject*>& args, const std::wstring& name) { 
	throw ck_message(L"Incomplete code, line: " + to_wstring(__LINE__), ck_message_type::CK_UNSUPPORTED_OPERATION); 
};

void ck_executer::goto_address(int bytecode_address) {
	if (bytecode_address < 0 || bytecode_address > scripts.back()->bytecode.bytemap.size())
		throw ck_message(L"goto out of range [" + to_wstring(bytecode_address) + L"] for range [0, " + to_wstring(scripts.back()->bytecode.bytemap.size()) + L"]" , ck_message_type::CK_INVALID_STATE);
	pointer = bytecode_address;
};

void ck_executer::clear() {
	// Delete evereseng
	
	for (int i = scopes.size() - 1; i >= 0; --i) {
		if (scopes[i])
			scopes[i]->unroot();
		
		scopes.pop_back();
	}
	
	for (int i = try_stack.size() - 1; i >= 0; --i) 
		try_stack.pop_back();
	
	for (int i = call_stack.size() - 1; i >= 0; --i) 
		call_stack.pop_back();
	
	for (int i = objects.size() - 1; i >= 0; --i) 
		objects.pop_back();
};

