#pragma once

#include <execinfo.h>

// Prints raw bactrace of stack calls.
static void print_stacktrace(int max_levels) {
	void *buffer[max_levels];
	int levels = backtrace(buffer, max_levels);
	
	// print to stdout (fd = 1), and remove this function from the trace
	backtrace_symbols_fd(buffer + 1, levels - 1, 1);
}