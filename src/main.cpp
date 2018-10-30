
#include <iostream>
#include <cstdio>
#include <cstdlib>

int main(int argc, char **argv) {
	// 1. Initialize GIL:
	// --- Set up thread manager
	// --- Set up global context
	// --- Set up Garbage Collector
	// 2. Current thread assigns as first thread
	// 3. Create Thread Context
	// --- Create Root Scope
	// ---- Put default objects
	// --- Assign Root file path
	// --- Assign Root environment path
	// 4. Parse input source code into Representable Structure (RS)
	// --- Handle all error messages & output them as fault
	// 5. Execute (RS) in Executer instance via exec()
	
	// Executer recursive stach handling:
	// ------------------------------- <-- exec()
	// |                             |
	// |  BLOCK  OF CURRENT PROGRAM  |
	// |                             |
	// ------------------------------- <-- exec()
	// |       STACK DELIMITER       |
	// -------------------------------
	// |                             |
	// |    BLOCK OF NEXT PROGRAM    |
	// |                             |
	// - - - - - - - - - - - - - - - -
	// |                             |
	
	// Multithread pause:
	// 1. lock GIL.lock for access.
	// 2. mark all threads to pause
	// 3. each thread checks for pause request on any iteration
	// 4. locker thread waits for every thread to pause/lock
	// 5. operation in single thread mode
	// 6. send each thread unpause signal
	
	// Garbage collection:
	// 1. Lock Garbage Collector
	// 2. Execute task in multithreaded lock
	// 3. Collect garbage
	// 4. Exit multithreaded lock
};