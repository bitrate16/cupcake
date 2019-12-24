// This example demonstrates REPL with eval() function.

// Print invitation line
println('Pseudo-REPL console in cupcake')

// Handle all system signals and print them out
__defsignalhandler = function(signum) {
	println('__defsignalhandler can not hangle signum now, goodbye');
	exit();
	
	if (signum == 3)
		println('Keyboard Interrupt');
	else
		println('Signal ', signum);
}

// Pseudo-repl evaluation context
var context = {};

// Infinite loop to perform repl execution
while 1 {
	// Evaluate in safe context to handle all exceptions
	try {
		// Print invitation line
		print('> ');
		
		// Read used input
		var line = readln();
		
		// Evaluate user input and record result value
		var result = eval(line, context);
		
		// Output thre result value
		println(result);
	} catch (e) {
		print(e);
	}
}