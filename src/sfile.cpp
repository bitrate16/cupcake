#include "sfile.h"

#ifdef WINDOWS
	#include <windows.h>
	#include <Fileapi.h>
#else
	#include <sys/stat.h>
	#include <unistd.h>
#endif


// Moved here, else linker producing errors
std::wstring ck_sfile::get_current_working_dir() {
	char buff[FILENAME_MAX];
	
#if defined(WINDOWS)
	char* rv = _getcwd(buff, FILENAME_MAX);
#else
	char* rv = getcwd(buff, FILENAME_MAX);
#endif
	
	// Could not get directory
	if (!rv)
		return L"";
	
	std::string str(buff);
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	return converter.from_bytes(str);
};

bool ck_sfile::sfile::mkdir() {
	std::string multibyte = std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>().to_bytes(to_string());
	
#if defined(WINDOWS)
	return !_mkdir(multibyte.c_str());
#else
	return ::mkdir(multibyte.c_str(), 0777) != -1;
#endif
	
	return 0;
};