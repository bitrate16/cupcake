#pragma once

#include <thread>
#include <mutex>
#include <atomic>

namespace ck_core {		
	/*
	 * Garbage collector object, tracked at creation.
	 */
	class GC;
	class gc_list;
	class gc_object {
	
	public: // WARNING: put destructor and constructor in PUBLIC section
		
		// Override new, new[] to record size of dynamical objects.
		void* operator new(std::size_t count);
		// void* operator new[](std::size_t count);
		
		// Let's look like it is private
		//  Else that's not my problem if you delete it from somewhere.
		void operator delete(void* ptr);
		// void operator delete[](void* ptr);
		
		gc_object();
		
		virtual ~gc_object();
		
		// Called when GC indexes all reachable objects
		virtual void gc_mark();
		
		// Called when GC destroyes current object
		virtual void gc_finalize();
		
		// Mark current object as reachable
		inline void gc_reach() { gc_reachable = 1; };
		
		// Indicates if obejcts is reachable and can not be collected.
		bool gc_reachable = 0;
		
		// Returns size of current object requred on malloc.
		inline size_t get_size() { return self_size; };
		
	private:
	
		// Allow access only from GC class.
		friend class GC;
		
		// Set to 1 if object is being recorded
		bool gc_record;
		// Set to 1 if object is GC root
		bool   gc_root;
		// Set to 1 if object is locked (like root, but not root, okay?)
		bool   gc_lock;
		
		// Pointer to alignation in current GC chain
		gc_list *gc_chain;
		// Pointer to alignation to current GC lock
		gc_list *gc_lock_chain;
		// Pointer to alignation to current GC root
		gc_list *gc_root_chain;
		
		// Allow derived to know, how much it contains
		std::size_t self_size;
	};
	
	/*
	 * Garbage collector chain.
	 */
	class gc_list {
	
	public:
	
		gc_list  *next;
		gc_object *obj;
		
		// For stupid users, that decide to GC by themself even after delete is forbidden.
		bool deleted_ptr;
		
		gc_list();
	};

	/*
	 * Garbage collector. Collects your shit.
	 */
	class GC {
		
		friend class gc_object;
	
	private:
	
		// Protects object list from multiple threads access.
		std::recursive_mutex protect_lock;
		
		bool collecting;
		int32_t size;
		int32_t roots_size;
		int32_t locks_size;
		gc_list *objects;
		gc_list *roots;
		gc_list *locks;
		
		// Number of objects created since last gc_collect pass
		int32_t created_interval;
		
		// Current memory usage
		static std::atomic<int64_t> memory_usage;
		
        // Number of minimum objects to be created before next GC
        // Yes, i like number 64.
        // 64 is like 8 * 8 and 2 << (8 - 2).
        // Or it can be represented as sum of eight 1's.
        // 
        // Yes, i'm capitan and that's my ship.
        // 		                                               _  _
        //                                                    ' \/ '
        //    _  _                        <|
        //     \/              __'__     __'__      __'__
        //                    /    /    /    /     /    /
        //                   /\____\    \____\     \____\               _  _
        //                  / ___!___   ___!___    ___!___               \/
        //                // (      (  (      (   (      (
        //              / /   \______\  \______\   \______\
        //            /  /   ____!_____ ___!______ ____!_____
        //          /   /   /         //         //         /
        //       /    /   |         ||         ||         |
        //      /_____/     \         \\         \\         \
        //            \      \_________\\_________\\_________\
        //             \         |          |         |
        //              \________!__________!_________!________/
        //               \|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_|_/|
        //                \    _______________                /
        // ^^^%%%^%^^^%^%%^\_"/_)/_)_/_)__)/_)/)/)_)_"_'_"_//)/)/)/)%%%^^^%^^%%%%^
        // ^!!^^"!%%!^^^!^^^!!^^^%%%%%!!!!^^^%%^^^!!%%%%^^^!!!!!!%%%^^^^%^^%%%^^^!
        // ^!!!!^$$$!^!^!^!^$^!^$^!^  THIS IS THE BOTTOM  ^!!!$$$$^$^!$^!^!^!$^$$$
        // &&&&!^$^^^^$$$!!!!!!^$^!^   > YOU ARE HERE <   ^!&$$$$^&&&&&&^^^^^!!$$$
        // ^!!^^&^^^!^^^!!^^^%%%^^!&!!^^^%%^^^!!%%%%^^^!!!!!!%%%^^^^%^^%%%^^^!!&&^
        //
        // We've been travelling for too long over that sea.
        //
        //                      _==|            
        //            _==|   )__)  |
        //              )_)  )___) ))
        //             )___) )____))_)
        //        _    )____)_____))__)\
        //         \---__|____/|___|___-\\---
        // ^^^^^^^^^\   oo oo oo oo     /~~^^^^^^^
        //   ~^^^^ ~~~~^^~~~~^^~~^^~~~~~
        //     ~~^^      ~^^~     ~^~ ~^ ~^
        //          ~^~~        ~~~^^~
        // 
        // But now we have reached the target
        //
        //     ,-'"""`-,    
        //   ,' \ _|_ / `.  
        //  /`.,'\ | /`.,'\ 
        // (  /`. \|/ ,'\  )
        // |--|--;=@=:--|--|
        // (  \,' /|\ `./  )
        //  \,'`./ | \,'`./ 
        //   `. / """ \ ,'  
        //     '-._|_,-`    
        // 
        // Here it is:
        //
        //     /\/\/\/\/\/\/\/\
        //    / ╔══╗╔╗╔╗╔╗╔══╗ \
        //    \ ║╔╗║║║║║║║║╔╗║ /
        //    / ║║║║║║║║║║║║║║ \
        //    \ ║║║║║║║║║║║║║║ /
        //    / ║╚╝║║╚╝╚╝║║╚╝║ \
        //    \ ╚══╝╚═╝╚═╝╚══╝ /
        //     \/\/\/\/\/\/\/\/
        //
        //    ( ͡° ͜ʖ ͡°) what's this?
        // 
        // >> https://www.youtube.com/watch?v=osR1jctb47Y <<
        // 
		
	public:
		
		// Maximal allowed size of heap usage, 512Mb
		static int64_t MAX_HEAP_SIZE;
		// Minimal amount of objects to be created before GC collect, 64
		static int32_t MIN_GC_INTERVAL;
		
		GC();
		~GC();
		
		public:
		// Called on object creation to attach it to the current instance of GC.
		void attach(gc_object *o);
		
		// Called to make given object root object
		void attach_root(gc_object *o);
		void deattach_root(gc_object *o);
		
		// Called to lock given object from deletion.
		void lock(gc_object *o);
		void unlock(gc_object *o);
		
		// Amount of objects registered by GC.
		inline int32_t count() { return size; };

		// Amount of roots
		inline int32_t roots_count() { return roots_size; };

		// Amount of locked obejcts
		inline int32_t locks_count() { return locks_size; };
		
		inline int64_t get_used_memory() { return memory_usage; };
		
		// if forced_collect is 1, GC will ignore checking conditons for optimizing and perform collection.
		void collect(bool forced_collect = 0);
		void dispose();
	};
};