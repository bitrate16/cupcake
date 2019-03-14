#pragma once

// For intptr_t
#if defined(__BORLANDC__)
    typedef unsigned char uint8_t;
    typedef __int64 int64_t;
    typedef unsigned long uintptr_t;
#elif defined(_MSC_VER)
    typedef unsigned char uint8_t;
    typedef __int64 int64_t;
#else
    #include <stdint.h>
#endif

namespace ck_util {
	
	// Gets current system stack size available.
	int64_t get_system_stack_size();
	
	// Attempts to set new stack size.
	int set_system_stack_size(int64_t size);
	
	// Attempts to meximize stack size.
	int maximize_stack_size();
};