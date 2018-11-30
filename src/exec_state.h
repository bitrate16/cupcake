#pragma once

/*
 * Used for global instant application termination by a single flag.
 */

// set to 1 if program handles global kill
extern volatile bool _global_int_state;
// set to 1 if program catches unresolvable error
extern volatile bool _global_error_state;
// set to 0 when execution is stopped
extern volatile bool _global_exec_state;
// Exit code of the program
extern volatile bool _global_exit_code;

void _interpreter_exit(int exit_code);

void _interpreter_exit();

void _interpreter_error();

void _interpreter_int();