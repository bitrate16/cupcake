#pragma once

#include <string>

// Utility contains pre-defined macro definitions 
//  to determine execution platform.
// Includes definition for platform name, bit environment.
// Currently supported platform:
// WINDOWS
// LINUX
// ANDROID
// Currently supported environments:
// X32
// X64

// Define OS name
#if defined(__CYGWIN__) // && !defined(_WIN32) // Cygwin POSIX on Windows
	#define LINUX
	#define PLATFORM_NAME "linux"
#elif defined(_WIN32) // Windows 32 bit
	#define WINDOWS
	#define PLATFORM_NAME "windows"
#elif defined(_WIN64) // Windows 64 bit
	#define WINDOWS
	#define PLATFORM_NAME "windows"
#elif defined(__ANDROID__)
	#define LINUX
	#define POSIX
	// Sub-os type, android is linux.
	#define ANDROID
	#define PLATFORM_NAME "android"
#elif defined(__linux__)
	#define LINUX
	#define POSIX
	#define PLATFORM_NAME "linux"
#elif defined(__unix__) || !defined(__APPLE__) && defined(__MACH__)
	// Assume, using same building set for unix as for linux
	#define LINUX
	#define POSIX
	#define UNIX
	
	#include <sys/param.h>
	#if defined(BSD) // FreeBSD, NetBSD, OpenBSD, DragonFly BSD
		#define PLATFORM_NAME "bsd"
		// Sub-os
		#define BSD
	#else
		#define PLATFORM_NAME "unix"
	#endif
#elif defined(__hpux)
	#define PHUX
	#define PLATFORM_NAME "hp-ux"
#elif defined(_AIX)
	#define AIX
	#define PLATFORM_NAME "aix"
#elif defined(__APPLE__) && defiend(__MACH__)
	#define APPLE
	
	#include <TargetConditionals.h>
    #if TARGET_IPHONE_SIMULATOR == 1
		// Apple iOS
		#define IOS
        #define PLATFORM_NAME "ios" 
    #elif TARGET_OS_IPHONE == 1
		// Apple iOS
		#define IOS
        #define PLATFORM_NAME "ios" 
    #elif TARGET_OS_MAC == 1
		// Apple OSX
		#define OSX
		#define MAC
		#define POSIX
        #define PLATFORM_NAME "osx" 
    #endif
#elif defined(macintosh) || defined(Macintosh)
	#define APPLE
	#define OS9
	#define MAC
	#define PLATFORM_NAME "os9" 
#elif defined(__sun) && defiend(__SVR4)
	#define SOLARIS
	#define POSIX
	#define PLATFORM_NAME "solaris"
#elif defined(__SYMBIAN32__)
	// Yes, you can try to compile this shit on symbian
	#define SYMBIAN
	#define PLATFORM_NAME "symbian"
#else
	#pragma error "Could not determine platform"
#endif

// Define cpu architecture (32/64)
#if defined(__GNUC__) || defined(__clang__) || defined(__INTEL_COMPILER)
	#if defined(__x86_64__) || defined(__powerpc64__) || defined(__aarch64__) || defined(__amd64__) || defined(__ia64__) // gcc 64bit
		#define X64
		#define PLATFORM_ENV "x64"
	#else
		#define X32
		#define PLATFORM_ENV "x32"
	#endif
#elif defined(__MINGW32__) || defined(__MINGW64__) || defined(_MSC_VER)
	#if defined(_WIN32)
		#define X32
		#define PLATFORM_ENV "x32"
	#elif defined(_WIN64)
		#define X64
		#define PLATFORM_ENV "x64"
	#endif
#elif defined (__SYMBIAN32__)
	#define X32
	#define PLATFORM_ENV "x32"
#else
	#pragma error "Could not determine cpu architecture"
#endif

// Define cpu architecture (ARM/Intel/Amd)
#if defined(__arm__)
	#define ARM32
	#define PLATFORM_CPU "arm32"
#elif defined(__aarch64__)
	#define ARM64
	#define PLATFORM_CPU "arm64"
#elif defined(__i386__)
	#define X86
	#define PLATFORM_CPU "x86"
#elif defined(__ia64__)
	#define IA64
	#define PLATFORM_CPU "ia64"
#elif defined(__amd64__) || defined(__x86_64__)
	#if defined(_LP64)
		#define AMD64
		#define PLATFORM_CPU "amd64"
	#elif defined(_ILP32)
		#define AMD32
		#define PLATFORM_CPU "amd32"
	#endif
#endif

// Check definitions
#if !defined(PLATFORM_NAME) || !defined(PLATFORM_ENV)
	#pragma error "Could not determine platform"
#endif

// Print definitions
#define ENABLE_PRINT_PLATFORM

#if defined(ENABLE_PRINT_PLATFORM)
	#if defined(WINDOWS)
		#pragma message "Target platform: WINDOWS"
	#elif defined(LINUX)
		#pragma message "Target platform: LINUX"
	#elif defined(APPLE)
		#pragma message "Target platform: APPLE"
	#endif

	#if defined(X32)
		#pragma message "Target architecture: x32"
	#elif defined(X64)
		#pragma message "Target architecture: x64"
	#endif

	#if defined(ARM)
		#pragma message "Target architecture: ARM"
	#elif defined(ARM64)
		#pragma message "Target architecture: ARM64"
	#elif defined(X86)
		#pragma message "Target architecture: X86"
	#elif defined(IA64)
		#pragma message "Target architecture: IA64"
	#elif defined(AMD64)
		#pragma message "Target architecture: AMD64"
	#endif
#endif
	
// Newline for each of platforms
#if defined(WINDOWS)
	#define PLATFORM_NEWLINE "\r\n"
#elif defined(OS9)
	#define PLATFORM_NEWLINE "\r"
#else
	#define PLATFORM_NEWLINE "\n"
#endif

// Path separator for each of platforms
#if defined(WINDOWS)
	#define PLATFORM_PATH_SEPARATOR '\\'
#else
	#define PLATFORM_PATH_SEPARATOR '/'
#endif

// Concatenate L and string
#define PPCAT(A, B) A ## B

// Concatenate L and string
#define PPCAT_EXP(A, B) PPCAT(A, B)

namespace ck_platform {
	// Returns name of target platform
	inline static std::wstring get_name() {
		return std::wstring(PPCAT_EXP(L, PLATFORM_NAME));
	};
	
	// Returns architecture environment for target platform
	inline static std::wstring get_cpu() {
		return std::wstring(PPCAT_EXP(L, PLATFORM_ENV));
	};
	
	// Returns true, if target is 32 bit (idk why i need it)
	inline static bool is_x32() {
#if defined(X32)
		return 1;
#else
		return 0;
#endif
	};
	
	// Returns true, if target is 64 bit (idk why i need it)
	inline static bool is_x64() {
#if defined(X64)
		return 1;
#else
		return 0;
#endif
	};
	
	// Returns newline on this platform
	inline static const char* newline() {
		return PLATFORM_NEWLINE;
	};
	
	// Returns newline as std::wstring
	inline static std::wstring newline_string() {
		return std::wstring(PPCAT_EXP(L, PLATFORM_NEWLINE));
	};
	
	// Returns cpu name on this platform
	inline static const char* cpu_name() {
		return PLATFORM_CPU;
	};
	
	// Returns cpu name as std::wstring
	inline static std::wstring cpu_name_string() {
		return std::wstring(PPCAT_EXP(L, PLATFORM_CPU));
	};
	
	// Returns path separator on this platform
	inline static const char path_separator() {
		return PLATFORM_PATH_SEPARATOR;
	};
}