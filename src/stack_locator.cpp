#include "stack_locator.h"

#include "ck_platform.h"

thread_local std::vector<ck_core::stack_locator::stack_descriptor> ck_core::stack_locator::descriptors;