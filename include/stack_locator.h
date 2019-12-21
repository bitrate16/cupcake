#pragma once

#include <cstdint>
#include <cstdlib>
#include <vector>

// Utility used to move execution stack.
// Allocates requered space in heap, stores ESP of current
//  stack & does migrating of stack.
namespace ck_core {
	namespace stack_locator {
		
		// Define base types for ESP / RSP

#if defined(__x86_64__)
		typedef uint64_t type_int;
#endif
#if defined(__i386__)
		typedef uint32_t type_int;
#endif

		// Represents single stack descriptor entity.
		// Contains all required information to perform restoration
		//  and call of new wrapper function.
		struct stack_descriptor {

			// Stack parameters

			// Value of old ESP pointer
			type_int old_esp   = 0;
			// Value of new stack allocation block unit
			type_int new_stack = 0;
			// Value of new stack top pointer
			type_int new_esp   = 0;
			// Size of newly allocated stack
			type_int new_size  = 0;

			// Stack arguments

			// Number of arguments to pass to wrap function
			int argc = 0;
			// Number arguments pass to the wrap function
			void** argv = nullptr;
			// Wrap function
			void (*wrap_function) (int, void**) = nullptr;
			// indicates if exception was thrown during wrapper call
			bool exception_handled = 0;
		};

		// Vector of all descriptors
		thread_local extern std::vector<stack_descriptor> descriptors;

		// Macro for performing stack replacement
		// _argc - amount of the arguments for _wrap_function
		// _argv - argument array pointer for _wrap_function
		// _wrap_function - function to be called on new stack
		// Returns 1 on success, 0 on fail.
		static bool call_replace_stack(int argc, void** argv, void (*wrap_function) (int, void**), size_t stack_size) {
			if (!(wrap_function))
				return 0;
			
			// Create descriptor
			stack_descriptor descriptor;
			
			// Allocate required stack size and align by 0xF
			descriptor.new_size  = (8 * 1024 * 1024 < (stack_size) ? (stack_size) : 8 * 1024 * 1024);
			descriptor.new_stack = (type_int) (uintptr_t) malloc((size_t) descriptor.new_size);
			if (!descriptor.new_stack)
				return 0;
			
			// Align new stack by 0xF
			descriptor.new_esp   = descriptor.new_stack + descriptor.new_size;
			descriptor.new_size -= descriptor.new_esp & 0xf;
			descriptor.new_esp  &= ~0xf;

			// Save arguments & function to descriptor
			descriptor.argc = (argc);
			descriptor.argv = (argv);
			descriptor.wrap_function = (wrap_function);
			
			// Save descriptor
			descriptors.push_back(descriptor);
			
			// Swap ESP / RSP
#if defined(__x86_64__)
			__asm__ (
				  "mov %%rsp, %%rax\n"
				  "mov %%rbx, %%rsp\n"
				: "=a"(descriptors.back().old_esp)
				: "b"(descriptors.back().new_esp)
				);
#endif
#if defined(__i386__)
			__asm__ (
            	  "mov %%esp, %%eax\n"
            	  "mov %%ebx, %%esp\n"
            	: "=a"(descriptors.back().old_esp)
            	: "b"(descriptors.back().new_esp)
            	);
#endif
		
			// Call & record exception
			try {
				descriptors.back().wrap_function(descriptors.back().argc, descriptors.back().argv);
			} catch(...) {
				descriptors.back().exception_handled = 1;
			}
			
			// Restore ESP / RSP
#if defined(__x86_64__)
			__asm__(
				  "mov %%rax, %%rsp"
				: "=a"(descriptors.back().old_esp)
				: "a"(descriptors.back().old_esp)
				);
#endif
#if defined(__i386__)
			__asm__(
            	  "mov %%eax, %%esp"
            	: "=a"(descriptors.back().old_esp)
            	: "a"(descriptors.back().old_esp)
            	);
#endif
			
			// Dispose descriptor & attached memory
			descriptors.pop_back();
			
			free((void*) (uintptr_t) descriptor.new_stack);
			
			// Return exception state
			return !descriptor.exception_handled;
		};

		// Returns current position relative to the execution stack
		//  (top - current).
		// Returns 0 if stack was not replaced.
		inline type_int get_stack_position() {
			if (descriptors.size()) {
				char dummy = 0;
				
				return (type_int) (uintptr_t) (descriptors.back().new_esp - (type_int) (uintptr_t) &dummy);
			}

			return 0;
		};
		
		// Returns size of current stack in bytes.
		// Returns 0 if stack was not replaced.
		inline type_int get_stack_size() {
			if (descriptors.size()) 
				return descriptors.back().new_size;

			return 0;
		};
		
		// Returns remaining stack size for current descriptor.
		// Returns 0 if stack was not replaced.
		inline type_int get_stack_remaining() {
			if (descriptors.size()) {
				char dummy = 0;

				return descriptors.back().new_size - (type_int) (uintptr_t) (descriptors.back().new_esp - (type_int) (uintptr_t) &dummy);
			}

			return 0;
		};
	
		// Unsafe operation, used only in thread calls.
		// Erase all information about current stacks.
		inline void erase_all() {
			ck_core::stack_locator::descriptors.resize(0);
		};
	};
};

/*
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

*/