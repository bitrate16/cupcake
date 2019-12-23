var forKeys = function(object, functor) {
	println(keys())
	var keys = object.keys();
	for (var i = 0; i < keys.size; ++i)
		functor(keys[i], '=', object[keys[i]])
}

/*var a = function(arg) { return arg; };
println(a(1))

var func = parse('return a + b', 'a', 'b');
 
println('getUsedMemory  =', GC.getUsedMemory());
println('getObjectCount =', GC.getObjectCount());

/*Thread(function() {
	var arr = [];
	var i = 0;
	while (i < 10000)
		arr.push(i++);

	println('getUsedMemory  =', GC.getUsedMemory());
	println('getObjectCount =', GC.getObjectCount());
})*/

var forKeys = function(object, functor) {
	println(this.keys()) // this is required because {} pushes new scope
	var keys = object.keys();
	for (var i = 0; i < keys.size(); ++i)
		functor(keys[i], '=', object[keys[i]])
	
	return 'okie'
}

var stream = function(array, functor) {
	for (var i = 0; i < array.size(); ++i)
		functor(array[i])
}

var streamCall = function(array, callname, argument) {
	for (var i = 0; i < array.size(); ++i)
		array[i][callname](argument)
}

var forKeysStream = function(object, printer) {
	stream(object.keys(), function(key) {
		printer(key, '=', object[key])
	})
}

//println(a(1))
forKeys(Platform, println)
forKeysStream(Platform, println)
forKeysStream(Int, println)
forKeysStream(File, println)

println('File.currentDirectory():', File.currentDirectory())
println('File.currentDirectory().exists():', File.currentDirectory().exists())
//println('File("blah").mkdir():', File("blah").mkdir())
println('File("blah/mah").createFile():', File("blah/mah").createFile())
println('File.currentDirectory().listFiles():', File.currentDirectory().listFiles())
println('File("delete") / "sas":', File("delete") / "sas")
println('File("delete").createFile():', File("delete").createFile())
println('File("delete").deleteFile():', File("delete").deleteFile())

// Pseudo-repl
var context = {};
while (1) {
	try {
		print('> ');
		var line = readln();
		var result = eval(line, context);
		println(result);
	} catch (e) {
		print(e)
	}
	// eval(readln());
}

/*
println('keys()     = ', keys())
println('println    = ', println)
println('a          = ', a)
println('a          = ', a(1))
println('a()        = ', a())
println('func       = ', func)
println('func(3, 5) = ', func(3, 5))

println('StackSize     = ', Thread.getStackSize());
println('UsedStackSize = ', Thread.getUsedStackSize());

Thread(function() {
	println('ThreadStackSize     = ', Thread.StackSize);
	println('Hello from thread' + Thread.currentThread().getId())
	try {
		null()
	} catch (e) {
		println("Thread 1:", e)
		throw e
	}
})

var counter = 0;
var r = function() {
	//if (counter > 20000)
	//	println(counter);
	++counter;
	r() 
}
var g = function() {
	r()
	try {
		try {
			r() 
		} catch (e) {
			e.printBacktrace()
		}
	} catch (e) {
		e.printBacktrace()
	}
}
//g()

try {
	try {
		null()
	} catch (e) {
		println(e)
	}
} catch (e) {
	println('e = ', e)
}

null()

println('after')

/*

// println('SetStackSize  = ', Thread.setStackSize(4000));
// println('StackSize     = ', Thread.getStackSize());
// println('UsedStackSize = ', Thread.getUsedStackSize());

println('__args = ' + __args)
println('__args[0] ', __args[0])

println('__env = ' + __env)

var __env_split = [];
for (var i = 0; i < __env.size(); ++i)
	__env_split.append(__env[i].split('='))
println('__env_split = ' + __env_split)


var obj = { foo: 'bar' }

println('foo in obj =', foo in obj, obj.keys())

print(
`
This
Is
Multiline
String

`
);

var a = 1;

println(a.__proto.keys())
println(typeof a)
println( a istypeof "Int")
println( a as "Int")

a.__proto.__operator_as = function() {
	print("hello", '\n')
	return "Hue hehe"
};

println( a as "Int")

var recursion = function() { recursion(); };

println(keys, keys())

println(Array.keys())

// Segmentation fault (fixed) -> var arr = ['sas', 'huyas], 12,1 21,2,1,2,4,2,4,2,4,3,4,65,7,5,3,23,234,54,423,532,5432,45234,3254]
var arr = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20] // <-- nulls at 9, 19?
// var arr = ['sas', 'huyas', 12,1 21,2,1,2,4,2,4,2,4,3,4,65,7,5,3,23,234,54,423,532,5432,45234,3254]
for (var i = 0; i < arr.size(); ++i)
	println(i, arr[i], arr.size())

print(Native.load('module.so'))

// println(readln())
// println(read())

println(String.keys())
println(Array.keys())

/*
var i = 0
while(1) {
	++i
	arr.push(i)
	println(arr)
}

// recursion();

/*var lock = {}
lock.cnt = 0;

var f = function() while (1) print('ThreadId = ' + Thread.currentThread().getId(), '\n');

Thread(f)
Thread(f)
Thread(f)
//Thread(f)
f()
// f();

/*var t = Thread(function(x) {
	while (1)
		print(gc_count(), '\n')
}, 'foo')
*/

/*
var e2 = function() { 

gc_count() }
var e1 = function() { e2() }
var f2 = function() { Thread(e1) }
var f1 = function() { f2() }

//f1()


var c = function() { f1() }
var b = function() { c() }
var a = function() { b() }

var f = function () {
	
	Thread(
	a)
	
	}

f()

/*var str = "1234567890"

__defsignalhandler = function() { str = ""; print('SIGNALED', '\n') }



while (!str) {
	str >>= 1
	print(str, '\n');
}

var arr = [];
print(Array.keys(), '\n')
print(Thread.keys(), '\n')
print([1, 2, 3] + [4, 'foo'], '\n')
system('pause')

/*var arr = ['foo', ['er', 12], 'bsds', 23]

ca = function() {
	print('CA ');
	return 1;
};
cb = function() {
	print('CB ');
	return 0;
};

print(ca() && ca() && ca() && cb())
print('\n')
print(cb() || cb() || cb() || ca())
print('\n')
print('args:', __args, '\n');

var i = 0
var b = false
print(b.__proto['__operator++'](b))
while (i < 1000) {
	if (i % 2)
		;print(i, '\n')
	b = ++b;
	print(b++)
	i += 1
}
/*
print ('"asdfdsa" == "1234" = ', 'asdfgfs' == '1231', '\n')
print ('"1234" == "1234" = ', '1234' == '1234', '\n')

print(arr)
print('Object keys: ', Object.keys(), '\n')
print('String keys: ', String.keys(), '\n')
print(13 + 10)

var a = function() { b() }
var b = function() { c() }
var c = function() { d() }
var d = function() { e() }
var e = function() { f() }
var f = function() { g() }
var g = function() { h() }
var h = function() { i() }
var i = function() { throw Cake() }

print(__typename);
t = {}print ('contsss ') ; print (t.contains('f'));
t.f = function() { print('object: '); print(this); print(', [__this]: '); print(__this); };
t.f();

try {
	a()
} catch (e) { print(e) }

var p = function() { try { return 'MNTR 7EU' } }

print(p())

var r = function() { 
throw 13
return 1 }

try{
r()
}catch(e) print(e)




//while (1)
	;//print('trye' + 'ses')

__defcakehandler = function(k) {  print(__defcakehandler) __defcakehandler = null print(k) }

//throw 'kek'

__defsignalhandler = function(s) { print('signum = ', s, '\n') throw 'throw from sighandler' }

//while (1);

print('a', 'b', 'c')

var o = {}
o.f = print;

print(o)
print(o.f)
o.f('a', 'b', 'c')


var recur = function() { recur() }

print ('asdfgыыh')

//recur()*/