#include "stack_utils.h"


#ifdef _WIN32
	#include <Windows.h>
#else
	#include <unistd.h>	
	#include <stdint.h>	
	#include <sys/resource.h>
#endif

#ifdef _WIN32
int64_t ck_util::get_system_stack_size() { return -1 };
#else
int64_t ck_util::get_system_stack_size() {
    struct rlimit rlim;
    getrlimit(RLIMIT_STACK, &rlim);
	return rlim.rlim_cur;
};
#endif

#ifdef _WIN32
int ck_util::set_system_stack_size(int64_t size) { return -1; };
#else
int ck_util::set_system_stack_size(int64_t size) {
	const rlim_t new_size = size;
	struct rlimit rl;
    int result;

    result = getrlimit(RLIMIT_STACK, &rl);
    if (result != 0)
		return result;
	
	rl.rlim_cur = new_size;

    result = setrlimit(RLIMIT_STACK, &rl);
	return result;
};
#endif
	
#ifdef _WIN32
int ck_util::maximize_stack_size() { return -1; };
#else
int ck_util::maximize_stack_size() {
	struct rlimit rl;
    int result;

    result = getrlimit(RLIMIT_STACK, &rl);
    if (result != 0)
		return result;
	
	rl.rlim_cur = rl.rlim_max;

    result = setrlimit(RLIMIT_STACK, &rl);
	return result;
};
#endif

