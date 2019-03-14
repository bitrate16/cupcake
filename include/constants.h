#pragma once

/*
 * Namespace contains constants for different classes in cupcake executer to easilly tweak 
 *  interpreter during tests.
 */
namespace ck_constants {
	namespace ck_executer {
		// Constants to limit size of execution stacks.
		const int def_execution_stack_limit = 4096;
		const int def_try_stack_limit       = 4096;
	};
};