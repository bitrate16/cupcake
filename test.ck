

t = {}
t.f = function() { print('object: '); print(this.f); print(', [__this]: '); print(__this); };
t.f();