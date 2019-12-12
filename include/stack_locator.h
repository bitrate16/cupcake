#pragma once

#include <cstdint>
#include <cstdlib>

// Utility used to move execution stack.
// Allocates requered space in heap, stores ESP of current 
//  stack & does migrating of stack.
namespace ck_core {
	namespace stack_locator {	
		// Define base types for ESP / RSP
		
#if defined(__x86_64__)
		typedef type_int uint64_t;
#endif
#if defined(__i386__)
		typedef type_int uint32_t;
#endif

		// Value of old ESP pointer
		thread_local type_int old_esp = 0;
		// Value of new stack allocation block unit
		thread_local type_int new_stack = 0;
		// Value of new stack pointer
		thread_local type_int new_esp = 0;
		// Size of newly allocated stack
		thread_local type_int new_stack_size = 0;
		
		// Indicates if stack ever been moved
		thread_local bool stack_moved = 0;
		
		// Value of argc for new call wrap
		thread_local int argc = 0;
		// Value of argv for new call wrap
		thread_local void* argv = nullptr;
		// Pointer to the function that should be executed 
		//  on new stack with passed args
		thread_local void* wrap_function(int, int, void*) = nullptr;
	};
};

// Macro for performing stack replacement
// _argc - amount of the arguments for _wrap_function
// _argv - argument array pointer for _wrap_function
// _wrap_function - function to be called on new stack
// _error_flag - flag indicating if there is an error during operation
#if defined(__x86_64__)
	#define CALL_REPLACE_STACK(_argc, _argv, _wrap_function, _stack_size, _error_flag) {                                \
		if (!(_wrap_function)) {                                                                                        \
			*(_error_flag) = 1;                                                                                         \
			goto end_macro;                                                                                             \
		}                                                                                                               \
																														\
		if (ck_core::stack_locator::stack_moved) {                                                                      \
			*(_error_flag) = 1;                                                                                         \
			goto end_macro;                                                                                             \
		}                                                                                                               \
																														\
		ck_core::stack_locator::new_stack_size = (8 * 1024 * 1024 < (_stack_size) ? : (_stack_size) : 8 * 1024 * 1024); \
																														\
		ck_core::stack_locator::new_stack = (type_int) (uintptr_t) malloc(ck_core::stack_locator::new_stack_size);      \
		if (!ck_core::stack_locator::new_stack) {                                                                       \
			*(_error_flag) = 1;                                                                                         \
			goto end_macro;                                                                                             \
		}                                                                                                               \
																														\
		ck_core::stack_locator::stack_moved = 1;                                                                        \
																														\
		ck_core::stack_locator::new_esp = ck_core::stack_locator::new_stack + ck_core::stack_locator::new_stack_size;   \
		ck_core::stack_locator::new_stack_size -= ck_core::stack_locator::new_esp & 0xf;                                \
		ck_core::stack_locator::new_esp &= 0xf;                                                                         \
																														\
		ck_core::stack_locator::argc = (_argc);                                                                         \
		ck_core::stack_locator::argv = (_argv);                                                                         \
		ck_core::stack_locator::wrap_function = (_wrap_function);                                                       \
																														\
		__asm__ (                                                                                                       \
			  "mov %%rsp, %%rax\n"                                                                                      \
			  "mov %%rbx, %%rsp\n"                                                                                      \
			: "=a"(ck_core::stack_locator::old_esp)                                                                     \
			: "b"(ck_core::stack_locator::new_esp)                                                                      \
			);                                                                                                          \
																														\
		try {                                                                                                           \
			(_wrap_function)((_argc), (_argv));                                                                         \
		} catch(...) {                                                                                                  \
			*(_error_flag) = 1;                                                                                         \
		}                                                                                                               \
																														\
		__asm__(                                                                                                        \
			  "mov %%rax, %%rsp"                                                                                        \
			: "=a"(ck_core::stack_locator::old_esp)                                                                     \
			: "a"(ck_core::stack_locator::old_esp)                                                                      \
			);                                                                                                          \
																														\
		free(ck_core::stack_locator::new_stack);                                                                        \
																														\
		ck_core::stack_locator::stack_moved = 0;                                                                        \
																														\
		end_macro:                                                                                                      \
	}
#endif
#if defined(__i386__)
	#define CALL_REPLACE_STACK(_argc, _argv, _wrap_function, _stack_size, _error_flag) {                                \
		if (!(_wrap_function)) {                                                                                        \
			*(_error_flag) = 1;                                                                                         \
			goto end_macro;                                                                                             \
		}                                                                                                               \
																														\
		if (ck_core::stack_locator::stack_moved) {                                                                      \
			*(_error_flag) = 1;                                                                                         \
			goto end_macro;                                                                                             \
		}                                                                                                               \
																														\
		ck_core::stack_locator::new_stack_size = (8 * 1024 * 1024 < (_stack_size) ? : (_stack_size) : 8 * 1024 * 1024); \
																														\
		ck_core::stack_locator::new_stack = (type_int) (uintptr_t) malloc(ck_core::stack_locator::new_stack_size);      \
		if (!ck_core::stack_locator::new_stack) {                                                                       \
			*(_error_flag) = 1;                                                                                         \
			goto end_macro;                                                                                             \
		}                                                                                                               \
																														\
		ck_core::stack_locator::stack_moved = 1;                                                                        \
																														\
		ck_core::stack_locator::new_esp = ck_core::stack_locator::new_stack + ck_core::stack_locator::new_stack_size;   \
		ck_core::stack_locator::new_stack_size -= ck_core::stack_locator::new_esp & 0xf;                                \
		ck_core::stack_locator::new_esp &= 0xf;                                                                         \
																														\
		ck_core::stack_locator::argc = (_argc);                                                                         \
		ck_core::stack_locator::argv = (_argv);                                                                         \
		ck_core::stack_locator::wrap_function = (_wrap_function);                                                       \
																														\
		__asm__ (                                                                                                       \
			  "mov %%esp, %%eax\n"                                                                                      \
			  "mov %%ebx, %%esp\n"                                                                                      \
			: "=a"(ck_core::stack_locator::old_esp)                                                                     \
			: "b"(ck_core::stack_locator::new_esp)                                                                      \
			);                                                                                                          \
																														\
		try {                                                                                                           \
			(_wrap_function)((_argc), (_argv));                                                                         \
		} catch(...) {                                                                                                  \
			*(_error_flag) = 1;                                                                                         \
		}                                                                                                               \
																														\
		__asm__(                                                                                                        \
			  "mov %%eax, %%esp"                                                                                        \
			: "=a"(ck_core::stack_locator::old_esp)                                                                     \
			: "a"(ck_core::stack_locator::old_esp)                                                                      \
			);                                                                                                          \
																														\
		free(ck_core::stack_locator::new_stack);                                                                        \
																														\
		ck_core::stack_locator::stack_moved = 0;                                                                        \
																														\
		end_macro:                                                                                                      \
	}
#endif
	
	
// Macro for performing stack replacement without function call
// _error_flag - flag indicating if there is an error during operation
#if defined(__x86_64__)
	#define REPLACE_STACK(_stack_size, _error_flag) {																	\								
		if (ck_core::stack_locator::stack_moved) {                                                                      \
			*(_error_flag) = 1;                                                                                         \
			goto end_macro;                                                                                             \
		}                                                                                                               \
																														\
		ck_core::stack_locator::new_stack_size = (8 * 1024 * 1024 < (_stack_size) ? : (_stack_size) : 8 * 1024 * 1024); \
																														\
		ck_core::stack_locator::new_stack = (type_int) (uintptr_t) malloc(ck_core::stack_locator::new_stack_size);      \
		if (!ck_core::stack_locator::new_stack) {                                                                       \
			*(_error_flag) = 1;                                                                                         \
			goto end_macro;                                                                                             \
		}                                                                                                               \
																														\
		ck_core::stack_locator::stack_moved = 1;                                                                        \
																														\
		ck_core::stack_locator::new_esp = ck_core::stack_locator::new_stack + ck_core::stack_locator::new_stack_size;   \
		ck_core::stack_locator::new_stack_size -= ck_core::stack_locator::new_esp & 0xf;                                \
		ck_core::stack_locator::new_esp &= 0xf;                                                                         \
																														\
		__asm__ (                                                                                                       \
			  "mov %%rsp, %%rax\n"                                                                                      \
			  "mov %%rbx, %%rsp\n"                                                                                      \
			: "=a"(ck_core::stack_locator::old_esp)                                                                     \
			: "b"(ck_core::stack_locator::new_esp)                                                                      \
			);                                                                                                          \
																														\
		end_macro:                                                                                                      \
	}
	
	#define RESTORE_STACK(_error_flag) {            \
		if (!ck_core::stack_locator::stack_moved) { \                                                                     
			*(_error_flag) = 1;                     \                                                                    
			goto end_macro;                         \                                                                    
		}                                           \
													\						
		__asm__(                                    \                                                                    
			  "mov %%rax, %%rsp"                    \                                                                    
			: "=a"(ck_core::stack_locator::old_esp) \                                                                    
			: "a"(ck_core::stack_locator::old_esp)  \                                                                    
			);                                      \                                                                    
													\																	
		free(ck_core::stack_locator::new_stack);    \                                                                    
													\																	
		ck_core::stack_locator::stack_moved = 0;    \
													\
		end_macro:                                  \                                                                    
	}
#endif
#if defined(__i386__)
	#define REPLACE_STACK(_stack_size, _error_flag) {																	\								
		if (ck_core::stack_locator::stack_moved) {                                                                      \
			*(_error_flag) = 1;                                                                                         \
			goto end_macro;                                                                                             \
		}                                                                                                               \
																														\
		ck_core::stack_locator::new_stack_size = (8 * 1024 * 1024 < (_stack_size) ? : (_stack_size) : 8 * 1024 * 1024); \
																														\
		ck_core::stack_locator::new_stack = (type_int) (uintptr_t) malloc(ck_core::stack_locator::new_stack_size);      \
		if (!ck_core::stack_locator::new_stack) {                                                                       \
			*(_error_flag) = 1;                                                                                         \
			goto end_macro;                                                                                             \
		}                                                                                                               \
																														\
		ck_core::stack_locator::stack_moved = 1;                                                                        \
																														\
		ck_core::stack_locator::new_esp = ck_core::stack_locator::new_stack + ck_core::stack_locator::new_stack_size;   \
		ck_core::stack_locator::new_stack_size -= ck_core::stack_locator::new_esp & 0xf;                                \
		ck_core::stack_locator::new_esp &= 0xf;                                                                         \
																														\
		__asm__ (                                                                                                       \
			  "mov %%esp, %%eax\n"                                                                                      \
			  "mov %%ebx, %%esp\n"                                                                                      \
			: "=a"(ck_core::stack_locator::old_esp)                                                                     \
			: "b"(ck_core::stack_locator::new_esp)                                                                      \
			);                                                                                                          \
																														\
		end_macro:                                                                                                      \
	}
	
	#define RESTORE_STACK(_error_flag) {            \
		if (!ck_core::stack_locator::stack_moved) { \                                                                     
			*(_error_flag) = 1;                     \                                                                    
			goto end_macro;                         \                                                                    
		}                                           \
													\						
		__asm__(                                    \                                                                    
			  "mov %%eax, %%esp"                    \                                                                    
			: "=a"(ck_core::stack_locator::old_esp) \                                                                    
			: "a"(ck_core::stack_locator::old_esp)  \                                                                    
			);                                      \                                                                    
													\																	
		free(ck_core::stack_locator::new_stack);    \                                                                    
													\																	
		ck_core::stack_locator::stack_moved = 0;    \
													\
		end_macro:                                  \                                                                    
	}
#endif
	