var a = function() { b() }
var b = function() { c() }
var c = function() { d() }
var d = function() { e() }
var e = function() { f() }
var f = function() { g() }
var g = function() { h() }
var h = function() { i() }
var i = function() { throw Error() }

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

__defexceptionhandler = function(k) {  print(__defexceptionhandler) __defexceptionhandler = null print(k) }

//throw 'kek'

__defsignalhandler = function(s) { print('signum = ', s, '\n') }

//while (1);

print('a', 'b', 'c')

var o = {}
o.f = print;

print(o)
print(o.f)
o.f('a', 'b', 'c')