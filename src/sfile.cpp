#include "sfile.h"

#ifdef WINDOWS
	#include <direct.h>
	#define GetCurrentDir _getcwd
#else
	#include <unistd.h>
	#define GetCurrentDir getcwd
#endif


// Moved here, else linker producing errors
std::wstring ck_sfile::get_current_working_dir() {
	char buff[FILENAME_MAX];
	char* rv = GetCurrentDir(buff, FILENAME_MAX);
	
	// Could not get directory
	if (!rv)
		return L"";
	
	std::string str(buff);
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	return converter.from_bytes(str);
};