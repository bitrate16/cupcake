#include "exec_state.h"

volatile bool _global_int_state   = 0;
volatile bool _global_error_state = 0;
volatile bool _global_exec_state  = 1;
volatile bool _global_exit_code   = 0;

void _interpreter_exit(int exit_code) {
	_global_exec_state = 0;
	_global_exit_code  = exit_code;
};

void _interpreter_exit() {
	_global_exec_state = 0;
};

void _interpreter_error() {
	_global_exec_state  = 0;
	_global_error_state = 1;
};

void _interpreter_int() {
	_global_exec_state = 0;
	_global_int_state  = 1;
};