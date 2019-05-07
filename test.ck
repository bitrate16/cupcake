var arr = ['foo', ['er', 12], 'bsds', 23]

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