#include "sfile.h"

#ifdef _WIN32
	#include <direct.h>
	#define GetCurrentDir _getcwd
#else
	#include <unistd.h>
	#define GetCurrentDir getcwd
#endif


// Moved here, else linker producing errors
std::wstring ck_sfile::get_current_working_dir() {
	char buff[FILENAME_MAX];
	GetCurrentDir(buff, FILENAME_MAX);
	std::string str(buff);
	std::wstring current_working_dir(str.begin(), str.end());
	return current_working_dir;
};