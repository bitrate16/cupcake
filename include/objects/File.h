#pragma once

#include <vector>
#include <string>
#include <cwchar>

#include "sfile.h"
#include "vobject.h"
#include "exceptions.h"

#include "Object.h"
#include "CallableObject.h"

namespace ck_objects {	

	class File : public ck_vobject::vobject {
		
	protected:
		
		// Contains instance of path, associated with this File.
		ck_sfile::sfile path;
		
	public:
		
		File() {};
		File(const std::wstring& path) { this->path = path; };
		File(const ck_sfile::sfile& path) { this->path = path; };
		
		virtual vobject* get     (ck_vobject::vscope*, const std::wstring&);
		virtual void     put     (ck_vobject::vscope*, const std::wstring&, vobject*);
		virtual bool     contains(ck_vobject::vscope*, const std::wstring&);
		virtual bool     remove  (ck_vobject::vscope*, const std::wstring&);
		virtual vobject* call    (ck_vobject::vscope*, const std::vector<vobject*>&);
		
		// File functions only
		
		// Returns path length
		virtual int64_t int_value() {
			return path.size();
		};
		
		// Returns path as string
		virtual std::wstring string_value() {
			return path.to_string();
		};
		
		// Returns associated path
		virtual ck_sfile::sfile& value() {
			return path;
		};
		
		// Returns reference to an index of path
		std::wstring& operator[](int index) {
			return path[index];
		};
		
		// Returns value of path index
		std::wstring operator[](int index) const {
			return path[index];
		};
		
		// Compare two files path as strings
		inline bool equals(File* other) {
			if (!other)
				return 0;
			
			if (path.size() != other->path.size())
				return 0;
			
			if (path.is_absolute() != other->path.is_absolute())
				return 0;
			
			for (int i = 0; i < path.size(); ++i)
				if (path[i] != other->path[i])
					return 0;
				
			return 1;
		};
		
		// returns 1 if path is absolute
		inline bool isAbsolute() {
			return path.is_absolute();
		};
		
		// Creates file with absolute path from existing file, 
		//  returns itself if already absolute
		inline File* getAbsolute() {
			if (path.is_absolute())
				return this;
			
			return new File(path.get_absolute());
		};
		
		// Returns path to the file
		inline std::wstring getPath() {
			return path.to_string();
		};
		
		// Returns absolute path to the file
		inline std::wstring getAbsolutePath() {
			if (path.is_absolute())
				return path.to_string();
			
			ck_sfile::sfile absolute = path.get_absolute();
			return absolute.to_string();
		};
		
		// Returns parent file path
		inline std::wstring getParentPath() {
			if (!path.has_parent())
				throw ck_exceptions::StateError(L"Could not get parent file");
			
			return path.get_parent().to_string();
		};
		
		// Returns parent file
		inline File* getParent() {
			if (!path.has_parent())
				throw ck_exceptions::StateError(L"Could not get parent file");
			
			return new File(path.get_parent());
		};
		
		// Returns 1, if file exists.
		inline bool exists() {
			return path.exists();
		};
		
		// Create single directory in path
		inline bool mkdir() {
			return path.mkdir();
		};
		
		inline bool createFile() {
			return path.create_file();
		};
		
		inline static File* currentDirectory() {
			return new File(ck_sfile::sfile::current_directory());
		};
		
		// Called on interpreter start to initialize prototype
		static vobject* create_proto();
	};
	
	// Defined on interpreter start.
	static CallableObject* FileProto = nullptr;
};