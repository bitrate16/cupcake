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