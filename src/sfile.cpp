#include "sfile.h"

#ifdef WINDOWS
	#include <windows.h>
	#include <Fileapi.h>
	#include <dirent.h>
	#include <cstdio>
#else
	#include <sys/stat.h>
	#include <unistd.h>
	#include <dirent.h>
	#include <cstdio>
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

std::vector<ck_sfile::sfile> ck_sfile::sfile::list_files() {
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> convert;
	std::string multibyte = convert.to_bytes(to_string());
	std::vector<ck_sfile::sfile> files;
	
#if defined(WINDOWS)
	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir(multibyte.c_str())) != NULL) {
		while ((ent = readdir (dir)) != NULL)
			files.push_back(convert.from_bytes(ent->d_name));
		
		closedir(dir);
	}
#else
	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir(multibyte.c_str())) != NULL) {
		while ((ent = readdir (dir)) != NULL)
			files.push_back(convert.from_bytes(ent->d_name));
		
		closedir(dir);
	}
#endif
	
	return files;
};

bool ck_sfile::sfile::delete_file() {
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> convert;
	std::string multibyte = convert.to_bytes(to_string());
	
	return std::remove(multibyte.c_str()) == 0;
};

bool ck_sfile::sfile::is_directory() {
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> convert;
	std::string multibyte = convert.to_bytes(to_string());
	
#if defined(WINDOWS)
	DWORD ftyp = GetFileAttributesA(multibyte.c_str());
	if (ftyp == INVALID_FILE_ATTRIBUTES)
		return false;

	if (ftyp & FILE_ATTRIBUTE_DIRECTORY)
		return true;

	return false;
#else
	struct stat path_stat;
    stat(multibyte.c_str(), &path_stat);
    return !S_ISREG(path_stat.st_mode);
#endif
};

