#pragma once

#include <vector>
#include <string>
#include <iostream>

// Simple file class.
// Allows representing input path as a tree of file paths
// Structure of path might look like this:
// <path> ::= <path>/<key>
// <key>  ::= <any key but '/' or '\'>

namespace ck_sfile {
	
	// XXX: Allow directory walking: ../, <path> + ../../......
	 
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
			std::wstring nstr = str + L"/";
			
			// Parse input string to path vector
			int last_cursor = 0;
			for (int i = 0; i < nstr.size(); ++i) { 
				// Iter on '/' or '\' and skip first '/' on linux
				if (nstr[i] == U'\\' || nstr[i] == U'/') {
#ifdef _WIN32
					// owo
#else               // > NOTWINDOWS
					if (i == 0) {
						absolute_path = 1; // linux root notation '/'
						last_cursor   = 1;
						continue;
					} else 			
#endif              // < NOTWINDOWS
					{
						if (i == last_cursor) {
							last_cursor = i;
							continue;
						}
						
						std::wstring temp;
						for (int k = last_cursor; k < i; ++k)
							temp += nstr[k];
						last_cursor = i + 1;
						
						path.push_back(temp);
						
#ifdef _WIN32           // > WINDOWS
						if (path.size() == 1 && nstr.back() == U':') // windows root notation <key>:
							absolute_path;
#endif                  // < WINDOWS

						continue;
					}
				}
			}
		};
		
		// Create new path by appending subpath to the passed ref.
		sfile(const sfile& ref, const sfile& subpath) {
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
		
		inline bool has_parent() const {
			return !absolute_path || path.size() != 0;
		};
		
		sfile get_parent(const sfile& parent_path) const {
			if (!has_parent())
				return sfile();
			
			sfile f = *this;
			if (path.size() == 0) 
				f = sfile(parent_path, f);
			
			if (f.path.size() != 0)
				f.path.pop_back();
			
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
	
		// Normalize current path using passed parent path.
		// Usually uses current_directory().
		// When amount of ../ as bigger than amount of path keys, 
		// path is concatenated with parent_path.
		void normalize(const sfile& parent_path) {
			// convert foo/bar/tar/sar/mar/kar/../../../par  -->  foo/bar/tar/par
			// convert ./foo --> /absolute/program/path/foo
			
			// Total weight of names
			int nweight = 0;
			
			for (int i = 0; i < path.size(); ++i) {
				if (path[i] == L"..") 
					--nweight;
				else
					++nweight;
				
				if (nweight < 0)
					break;
			}
			
			if (nweight < 0) {
				if (absolute_path)
					path.clear();
				else {
					sfile absolute(parent_path, *this);
					path = absolute.path;
					absolute_path = absolute.absolute_path;
					
					nweight = 0;
					for (int i = 0; i < path.size(); ++i) {
						if (path[i] == L"..") 
							--nweight;
						else
							++nweight;
						
						if (nweight < 0)
							break;
					}
						
					if (nweight < 0)
						path.clear();
					else {
						// Try to normalize absolute path assuming name weight > 0
						
						for (int i = 0; i < path.size() - 1; ++i) {
							if (path[i + 1] == L"..") {
								path.erase(path.begin() + i + 1);
								path.erase(path.begin() + i);
								i -= 2;
								continue;
							}
						}
					}
				}
			} else {
				// Try to normalize simple path assuming name weight > 0
				
				for (int i = 0; i < path.size() - 1; ++i) {
					if (path[i + 1] == L"..") {
						path.erase(path.begin() + i + 1);
						path.erase(path.begin() + i);
						i -= 2;
						continue;
					}
				}
			}
		};
		
		friend std::wostream& operator<<(std::wostream& os, const sfile& f) {
				if (f.absolute_path) {
#ifdef _WIN32
					
#else
					os << L"/ ";
#endif
				}
				
				for (int i = 0; i < f.path.size(); ++i) {			
#ifdef _WIN32
					if (i != path.size() - 1)
						os << f.path[i] << L" \\ ";
					else
						os << f.path[i];
#else
					if (i != f.path.size() - 1)
						os << f.path[i] << L" / ";
					else
						os << f.path[i];
#endif
				}
			
			return os;
		};
		
		
		static sfile current_directory() {
			return get_current_working_dir();
		};
	};
};