Operators reference
===================

Supported list of operators
---------------------------

* unary
  * <div>+x, -x, ++x, --x, x++, x-- (__operator--, __operator++), !x, ~x, @x</div>
* binary
  * <div>+, -, *, /, \, \\, >, <, >=, <=, ==, !=, ->, =>, #, %, &, &&, |, ||, ^</div>
* terniary
  * <div>?:</div>
* others
  * <div>ref[key], ref.name, ref({args})</div>
  
Overloading technics
--------------------

To overload an operator on object, define value for __operator<operator token>.<br>
Example usage of overload feature:
```
var obj = {};
obj['__operator*'] = function(self, other) { ... };
```

Overloaded call signature
-------------------------

The first argument passed to an operator function is instance of owning object.
The second argument is an instance of second operator argument object.

For example call signature for
```
a + b
```
should be
```
a['__operator+'](a, b)
```
  
Right-side expressions
---------------------

while evaluating expression
```
a + b
```
a may not have __operator+ defined. So __roperator+ would be called from b
```
__roperator+(b, a)
```
while normal __operator+ is called like
```
__operator+(a, b)
```

Usage of this in operator overload
----------------------------------

__this value is being defined during the call, so this will correctly refer to instance of operated object.