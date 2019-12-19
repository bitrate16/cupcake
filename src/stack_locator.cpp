#include "stack_locator.h"

thread_local std::vector<ck_core::stack_locator::stack_descriptor> ck_core::stack_locator::descriptors;
