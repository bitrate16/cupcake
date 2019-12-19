#pragma once

#include <string>
#include <map>
#include <vector>

// Utility used to parse input arguments.
// Accepts argc, argv as input and splits argv into two sets:
// 1. --CK::<argkey>, --CK::<argkey>=<argvalue>
// 2. Other arguments, that can not be matched as interpreter options.
namespace ck_core {
	
	namespace ck_args {
	
		// Map of <key, value> pairs for values with syntax 
		//  --CK::<argkey>, --CK::<argkey>=<argvalue>
		std::map<std::wstring, std::wstring> ck_options;
		// Vector of all other arguments, passed to the program.
		std::vector<std::wstring> ck_args;
		// Empty key, used for get_option(key)
		std::wstring empty_value;
		
		// Hold instance in global contest to make it auto-disposable.
		// Does not matter if it is visible, it has to be accessed 
		//  with ck_args class.
		// ck_args ck_args_instance;
		
		// Performs parsing of input arguments into two sets abowe.
		// Can be executed only once and executed from main() function.
		// After parsing, instance of ck_args is accessible with ck_args::instance();
		static void parse(int argc, const char** argv) {
			std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	
			for (int i = 0; i < argc; ++i)
				if (argv) {
					std::wstring argument = converter.from_bytes(argv[i]);
					
					if (argument.find(L"--CK::") == 0) {
						size_t value_start = argument.find(U'=');
						
						if (value_start == std::wstring::npos)
							ck_core::ck_args::ck_options[argument.substr(6)] = L"";
						else 
							ck_core::ck_args::ck_options[argument.substr(6, value_start - 6)] = argument.substr(value_start + 1);
					} else
						ck_core::ck_args::ck_args.push_back(argument);
				}
		};
		
		// Returns value mapped to given key.
		static inline const std::wstring& get_option(const std::wstring& key) {
			std::map<std::wstring, std::wstring>::iterator it = ck_core::ck_args::ck_options.find(key);
			
			if (it != ck_core::ck_args::ck_options.end()) 
				return it->second;
			
			return ck_core::ck_args::empty_value;
		};
		
		// Checks if passed key exists in the map
		static inline bool has_option(const std::wstring& key) {
			return ck_core::ck_args::ck_options.find(key) != ck_core::ck_args::ck_options.end();
		};
		
		static inline size_t options_size() {
			return ck_core::ck_args::ck_options.size();
		};
		
		static inline size_t args_size() {
			return ck_core::ck_args::ck_args.size();
		};
		
		// Returns reference to map of values
		static inline const std::map<std::wstring, std::wstring>& get_options() {
			return ck_core::ck_args::ck_options;
		}
		
		// Returns reference to vector of values
		static inline const std::vector<std::wstring>& get_args() {
			return ck_core::ck_args::ck_args;
		}
	};
}