#include "objects/File.h"

#include <string>
#include <sstream>
#include <algorithm>

#include "exceptions.h"

#include "objects/Bool.h"
#include "objects/String.h"
#include "objects/NativeFunction.h"
#include "objects/Undefined.h"
#include "objects/Array.h"
#include "objects/Int.h"

using namespace std;
using namespace ck_exceptions;
using namespace ck_vobject;
using namespace ck_objects;
using namespace ck_core;

// File() - empry
// File(File / String) - copy
// File(File / Strng, File / String) - concatenate
static vobject* call_handler(vscope* scope, const vector<vobject*>& args) {
	if (args.size() == 0)
		return new File();
	
	ck_sfile::sfile file;
	
	for (int i = 0; i < args.size(); ++i) {
		if (args[i])
			if (args[i]->as_type<File>())
				file = ck_sfile::sfile(file, static_cast<File*>(args[i])->value());
			else
				file = ck_sfile::sfile(file, args[i]->string_value());
		else
			file = ck_sfile::sfile(file, L"null");
	}
	
	return new File(file);
};

vobject* File::create_proto() {
	if (FileProto != nullptr)
		return FileProto;
	
	FileProto = new CallableObject(call_handler);
	GIL::gc_instance()->attach_root(FileProto);
	
	FileProto->Object::put(L"__typename", new String(L"File"));	
	FileProto->Object::put(L"getPath", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			// Validate __this
			if (!scope) return Undefined::instance();
			vobject* __this = scope->get(L"__this", 1);
			if (!__this || !__this->as_type<File>())
				return Undefined::instance();
			
			File* f = static_cast<File*>(__this);
			
			return new String(f->getPath());
		}));
	FileProto->Object::put(L"getAbsolutePath", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			// Validate __this
			if (!scope) return Undefined::instance();
			vobject* __this = scope->get(L"__this", 1);
			if (!__this || !__this->as_type<File>())
				return Undefined::instance();
			
			File* f = static_cast<File*>(__this);
			
			return new String(f->getAbsolutePath());
		}));
	FileProto->Object::put(L"isAbsolute", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			// Validate __this
			if (!scope) return Undefined::instance();
			vobject* __this = scope->get(L"__this", 1);
			if (!__this || !__this->as_type<File>())
				return Undefined::instance();
			
			File* f = static_cast<File*>(__this);
			
			return Bool::instance(f->isAbsolute());
		}));
	FileProto->Object::put(L"getParent", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			// Validate __this
			if (!scope) return Undefined::instance();
			vobject* __this = scope->get(L"__this", 1);
			if (!__this || !__this->as_type<File>())
				return Undefined::instance();
			
			File* f = static_cast<File*>(__this);
			
			return f->getParent();
		}));
	FileProto->Object::put(L"getParentPath", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			// Validate __this
			if (!scope) return Undefined::instance();
			vobject* __this = scope->get(L"__this", 1);
			if (!__this || !__this->as_type<File>())
				return Undefined::instance();
			
			File* f = static_cast<File*>(__this);
			
			return new String(f->getParentPath());
		}));
	FileProto->Object::put(L"exists", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			// Validate __this
			if (!scope) return Undefined::instance();
			vobject* __this = scope->get(L"__this", 1);
			if (!__this || !__this->as_type<File>())
				return Undefined::instance();
			
			File* f = static_cast<File*>(__this);
			
			return Bool::instance(f->exists());
		}));
	FileProto->Object::put(L"mkdir", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			// Validate __this
			if (!scope) return Undefined::instance();
			vobject* __this = scope->get(L"__this", 1);
			if (!__this || !__this->as_type<File>())
				return Undefined::instance();
			
			File* f = static_cast<File*>(__this);
			
			return Bool::instance(f->mkdir());
		}));
	FileProto->Object::put(L"createFile", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			// Validate __this
			if (!scope) return Undefined::instance();
			vobject* __this = scope->get(L"__this", 1);
			if (!__this || !__this->as_type<File>())
				return Undefined::instance();
			
			File* f = static_cast<File*>(__this);
			
			return Bool::instance(f->createFile());
		}));
	FileProto->Object::put(L"isDirectory", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			// Validate __this
			if (!scope) return Undefined::instance();
			vobject* __this = scope->get(L"__this", 1);
			if (!__this || !__this->as_type<File>())
				return Undefined::instance();
			
			File* f = static_cast<File*>(__this);
			
			if (!f->exists())
				throw ck_exceptions::IOError(L"File " + f->value().to_string() + L" does not exist");
			
			return Bool::instance(f->isDirectory());
		}));
	FileProto->Object::put(L"deleteFile", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			// Validate __this
			if (!scope) return Undefined::instance();
			vobject* __this = scope->get(L"__this", 1);
			if (!__this || !__this->as_type<File>())
				return Undefined::instance();
			
			File* f = static_cast<File*>(__this);
			
			if (!f->exists())
				return Bool::False();
			
			return Bool::instance(f->deleteFile());
		}));
	FileProto->Object::put(L"currentDirectory", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			return File::currentDirectory();
		}));
	FileProto->Object::put(L"listFiles", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			// Validate __this
			if (!scope) return Undefined::instance();
			vobject* __this = scope->get(L"__this", 1);
			if (!__this || !__this->as_type<File>())
				return Undefined::instance();
			
			File* f = static_cast<File*>(__this);
			
			if (!f->exists())
				throw ck_exceptions::IOError(L"File " + f->value().to_string() + L" does not exist");
			
			Array* array = new Array();
			std::vector<File*> files = f->listFiles();
			
			for (int i = 0; i < files.size(); ++i)
				array->items().push_back(files[i]);
			
			return array;
		}));
	FileProto->Object::put(L"listFilesPath", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			// Validate __this
			if (!scope) return Undefined::instance();
			vobject* __this = scope->get(L"__this", 1);
			if (!__this || !__this->as_type<File>())
				return Undefined::instance();
			
			File* f = static_cast<File*>(__this);
			
			if (!f->exists())
				throw ck_exceptions::IOError(L"File " + f->value().to_string() + L" does not exist");
			
			Array* array = new Array();
			std::vector<std::wstring> files = f->listFilesPath();
			
			for (int i = 0; i < files.size(); ++i)
				array->items().push_back(new String(files[i]));
			
			return array;
		}));
	
	
	// Operators
	FileProto->Object::put(L"__operator==", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			if (!args[0]->as_type<File>() || !args[1]->as_type<File>())
				return Bool::False();
			
			return Bool::instance(static_cast<File*>(args[0])->value() == static_cast<File*>(args[1])->value());
		}));
	FileProto->Object::put(L"__operator!=", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			if (!args[0]->as_type<File>() || !args[1]->as_type<File>())
				return Bool::True();
			
			return Bool::instance(static_cast<File*>(args[0])->value() != static_cast<File*>(args[1])->value());
		}));
	FileProto->Object::put(L"__operator/", new NativeFunction(
		[](vscope* scope, const vector<vobject*>& args) -> vobject* {
			if (args.size() < 2 || !args[0] || !args[1])
				return Undefined::instance();
			
			std::wstring patha = args[0]->string_value();
			std::wstring pathb = args[1]->string_value();
			
			return new File(ck_sfile::sfile(patha, pathb));
		}));
	
	return FileProto;
};


// Map any sign index to [0, size]
vobject* File::get(ck_vobject::vscope* scope, const std::wstring& name) {
	if (name == L"__proto")
		return FileProto;
	
	return FileProto ? FileProto->get(scope, name) : nullptr;
};

// Map any sign index to [0, size]
// Disallow assignment of characters.
void File::put(ck_vobject::vscope* scope, const std::wstring& name, vobject* object) {
	throw ck_exceptions::UnsupportedOperation(L"File is not container");
};

bool File::contains(ck_vobject::vscope* scope, const std::wstring& name) {
	if (name == L"__proto")
		return 1;
	
	return FileProto && FileProto->Object::contains(name);
};

bool File::remove(ck_vobject::vscope* scope, const std::wstring& name) {
	throw ck_exceptions::UnsupportedOperation(L"File is not container");
};

vobject* File::call(ck_vobject::vscope* scope, const std::vector<vobject*>& args) {
	throw ck_exceptions::UnsupportedOperation(L"File is not callable");
};
