Types reference
===============

Default types:
--------------
* Object
* Array
* String
* Int
* Bool
* Double
* Function
  * NativeFunction
  * BytecodeFunction

----

BytecodeFunction
----------------

| Value       | Description                                                                                                                            |
|-------------|----------------------------------------------------------------------------------------------------------------------------------------|
| proto       | Function : vsobject                                                                                                                    |
| __typename  | BytecodeFunction                                                                                                                       |
| Fields      | proto<br> __typename<br> contains(key)<br> remove(key)<br> bind(this_ref)<br> for bytecode see reference for dec object native lib:dec |
| Constructor | Function(souce string) or function() { ... }                                                                                           |
| Thread-safe | yes                                                                                                                                    |
| Description | BytecodeFunction represents a piece of source code of the script that can be executed.                                                 |

NativeFunction
--------------

| Value       | Description                                                                                                                           |
|-------------|---------------------------------------------------------------------------------------------------------------------------------------|
| proto       | Function : vsobject                                                                                                                   |
| __typename  | NativeFunction                                                                                                                        |
| Fields      | proto<br> __typename<br> contains(key)<br> remove(key)<br> __native_ptr<br> bind(this_ref)                                            |
| Constructor | none                                                                                                                                  |
| Thread-safe | yes                                                                                                                                   |
| Description | NativeFunction can be used to bind C++ native functions to a ck functions.<br> By the default they can not be create from the script. |

Bool
----

| Value       | Description                                                                                                                                         |
|-------------|-----------------------------------------------------------------------------------------------------------------------------------------------------|
| proto       | vobject                                                                                                                                             |
| __typename  | Bool                                                                                                                                                |
| Fields      | proto<br> __typename<br> contains(key)<br> remove(key)<br>  True - existing instance of True value<br> False - Existing instance of False value<br> |
| Constructor | Bool(string or value)                                                                                                                               |
| Thread-safe | ?                                                                                                                                                   |

Double
------

| Value       | Description                                                                                                                              |
|-------------|------------------------------------------------------------------------------------------------------------------------------------------|
| proto       | vobject                                                                                                                                  |
| __typename  | Double                                                                                                                                   |
| Fields      | proto<br> __typename<br> contains(key)<br> remove(key)<br> parseDouble(string)<br> MAX_DOUBLE<br> MIN_DOUBLE<br> NAN<br> toString(value) |
| Constructor | Double(value or string)                                                                                                                  |
| Thread-safe | ?                                                                                                                                        |

Int
---

| Value       | Description                                                                                                                   |
|-------------|-------------------------------------------------------------------------------------------------------------------------------|
| proto       | vobject                                                                                                                       |
| __typename  | Int                                                                                                                           |
| Fields      | proto<br> __typename<br> contains(key)<br> remove(key)<br> parseInt(string)<br> MAX_INT<br> MIN_INT<br> toString(value, base) |
| Constructor | Int(value or string)                                                                                                          |
| Thread-safe | ?                                                                                                                             |

Array
-----

| Value       | Description                                            |
|-------------|--------------------------------------------------------|
| proto       | Object                                                 |
| __typename  | Array                                                  |
| Fields      | proto<br> __typename<br> contains(key)<br> remove(key) |
| Constructor | Array(list of values or an array)                      |
| Thread-safe | yes                                                    |

Object
------

| Value       | Description                                            |
|-------------|--------------------------------------------------------|
| proto       | Object :: vsobject                                     |
| __typename  | Object                                                 |
| Fields      | proto<br> __typename<br> contains(key)<br> remove(key) |
| Constructor | Object(key-value pairs or other object)                |
| Thread-safe | yes                                                    |