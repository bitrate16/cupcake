#pragma once

#include <vector>
#include <string>

// Simple file class.
// Allows representing input path as a tree of file paths
// Structure of path might look like this:
// <path> ::= <path>/<key>
// <key>  ::= <any key but '/' or '\'>

namespace ck_sfile {
	 
	std::wstring get_current_working_dir();
	
	class sfile {
		std::vector<std::wstring> path;
		
		// Set to 1 if input path starts with drive letter or '/' on linux.
		bool absolute_path = 0;
		
	public:
	
		// Empty path
		sfile() {};
	
		// Construct object from string splitting it by '/' and '\'
		sfile(const std::wstring& str) {
			// Parse input string to path vector
			int last_cursor = 0;
			for (int i = 0; i < str.size(); ++i) {
				// Iter on '/' or '\' and skip first '/' on linux
				if (str[i] == '\\' || str[i] == '/') {
					if (i == 0)
						absolute_path = 1;
					else {
						std::wstring temp;
						for (int k = last_cursor; k < i; ++k)
							temp += str[k];
						++i;
						path.push_back(temp);
					}
				}
			}
		};
		
		// Create new path by appending subpath to the passed ref.
		sfile(const sfile& ref, const std::wstring& subpath) {
			*this = ref;
			*this += subpath;
		};
		
		
		// Construct object from string splitting it by '/' and '\'
		sfile(const std::string& str) : sfile(std::wstring(str.begin(), str.end())) {};
		
		// Create new path by appending subpath to the passed ref.
		sfile(const sfile& ref, const std::string& subpath) : sfile(ref, std::wstring(subpath.begin(), subpath.end())) {};
		
		// Returns amount of entries in path
		inline int size() const {
			return path.size();
		};
		
		// Returns 1 if current path recognized as absolute path
		inline bool is_absolute() const {
			return absolute_path;
		};
		
		// Returns absolute path by appending result of get_current_working_dir() at the beginning.
		inline sfile get_absolute() const {
			if (absolute_path)
				return *this;
			
			sfile f(get_current_working_dir());
			f += *this;
			f.absolute_path = 1;
			return f;
		};
		
		
		// Returns path entry reference
		// GETTER
		sfile operator[](int index) const {
			return path[index];
		};
		
		sfile& operator=(const sfile& f) {
			path.clear();
			
			for (int i = 0; i < f.size(); ++i)
				path.push_back(f.path[i]);
			
			absolute_path = f.absolute_path;
			
			return *this;
		};
		
		sfile& operator+=(const sfile& sub) {
			
			// Can not append absolute path
			if (sub.absolute_path)
				return *this;
			
			for (int i = 0; i < sub.size(); ++i)
				path.push_back(sub.path[i]);
			
			return *this;
		};
		
		sfile& operator+(const sfile& sub) {
			sfile f = *this;
			
			// Can not append absolute path
			if (sub.absolute_path)
				return f;
			
			f += sub;
			return f;
		};
		
		bool operator==(const sfile& o) {
			if (is_absolute() != o.is_absolute())
				return 0;
			
			if (size() != o.size())
				return 0;
			
			for (int i = 0; i < size(); ++i)
				if (path[i] != o.path[i])
					return 0;
			
			return 1;
		};
		
		// Allow creating paths at runtime with <path> / <subpath>
		sfile operator/(const sfile& s) {
			sfile f = *this;
			f += s;
			return f;
		};
		
		// Converts given object to string representing file path
		std::wstring to_string() const {
			std::wstring string_path;
			
			if (absolute_path) {
#ifdef _WIN32
				string_path += get_current_working_dir()[0]; // <-- Gets the current drive letter
				string_path += U':';
#else
				string_path += U'/';
#endif
			}

			for (int i = 0; i < path.size(); ++i) {
				string_path += path[i]; // <-- Gets the current drive letter
#ifdef _WIN32
				string_path += U'\\';
#else
				string_path += U'/';
#endif		
			}
			
			return string_path;
		};
	
		
		static sfile current_directory() {
			return get_current_working_dir();
		};
	};
};