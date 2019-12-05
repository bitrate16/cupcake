#include <vector>
#include <functional>

// Provides utility to handle after finish events.
// Used to execute some code after interpreter fully finishes it's work.
// Called after main() dispose the GIL, detaches all threads & goes to return code 0.
// Called after program catches:
//  SIGKILL
//  SIGSTOP
//  SIGTERM
//  SIGABRT
//  Or can not process the signal (has no __defsighandler defined).
// Not called on SEGFAULT.
namespace ck_core {
	namespace ck_exit_listener {		
		// Vector with all event listeners.
		static std::vector<std::function<void()>> listeners;
		
		// Subscribe to exit event.
		// Funciton pointer will be inserted into vector of listeners and on 
		//  main exit, it will be called in insertation precedence.
		inline void subscribe(std::function<void()> fun) {
			listeners.push_back(fun);
		};
		
		// Called from main to strike all events.
		// Deprecated from outer call, but there is no normal way to put it in 
		//  private scope without making a complex set of handler managers.
		// @Deprecated
		inline void strike() {
			for (int i = 0; i < listeners.size(); ++i) 
				listeners[i]();
		};
	};
};