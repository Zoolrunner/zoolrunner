/* Custom instanceof hooks, ordinary semantics and legacy dispatch. */
var checks = 0;
function check(ok, label) { ++checks; if (!ok) throw Error('FAIL hasInstance: ' + label); }
function throwsType(fn, label) {
    var caught = false;
    try { fn(); } catch (e) { caught = e instanceof TypeError; }
    check(caught, label);
}
var hook = Function.prototype[Symbol.hasInstance];
var descriptor = Object.getOwnPropertyDescriptor(Function.prototype, Symbol.hasInstance);
check(typeof hook === 'function' && hook.name === '[Symbol.hasInstance]' && hook.length === 1,
      'builtin metadata');
check(!descriptor.writable && !descriptor.enumerable && !descriptor.configurable,
      'builtin descriptor');
check(!hook.hasOwnProperty('prototype'), 'builtin is not a constructor');
throwsType(function() { new hook(); }, 'construction rejected');
function F() {}
var instance = new F();
check(hook.call(F, instance) && instance instanceof F, 'ordinary instance');
check(!hook.call(F, {}) && !hook.call(F, F.prototype), 'ordinary negative');
var values = [undefined, null, false, 0, '', Symbol('value'), {}];
for (var i = 0; i < values.length; ++i)
    check(hook.call(values[i], instance) === false, 'non-callable receiver ' + i);
F.prototype = 3;
for (i = 0; i < values.length - 1; ++i)
    check(hook.call(F, values[i]) === false, 'primitive before prototype ' + i);
throwsType(function() { hook.call(F, {}); }, 'invalid ordinary prototype');
var target = {}, calls = 0, marker = {};
target[Symbol.hasInstance] = function(value) {
    ++calls;
    check(this === target && value === marker, 'custom receiver and argument');
    gc();
    return {valueOf: function() { throw Error('must not convert hook result'); }};
};
check(marker instanceof target && calls === 1, 'custom result truth');
target[Symbol.hasInstance] = function() { return 0; };
check(!(marker instanceof target), 'custom false');
target[Symbol.hasInstance] = 1;
throwsType(function() { marker instanceof target; }, 'noncallable hook');
target[Symbol.hasInstance] = null;
throwsType(function() { marker instanceof target; }, 'null hook with noncallable rhs');
target[Symbol.hasInstance] = undefined;
throwsType(function() { marker instanceof target; }, 'undefined hook with noncallable rhs');
Object.defineProperty(target, Symbol.hasInstance, {configurable:true, get:function() { gc(); throw marker; }});
var caught;
try { marker instanceof target; } catch (e) { caught = e; }
check(caught === marker, 'getter exception');
var prototype = {};
prototype[Symbol.hasInstance] = function(value) { return this.tag === value; };
target = Object.create(prototype); target.tag = 27;
check(27 instanceof target, 'inherited custom hook');
function G() {}
Object.defineProperty(G, Symbol.hasInstance, {configurable:true, value:function(value) {
    gc(); return this === G && value === marker;
}});
var bound = G.bind(null);
check(marker instanceof bound && hook.call(bound, marker), 'bound target custom hook');
check(!(0 instanceof bound), 'bound target negative');
Object.defineProperty(G, Symbol.hasInstance, {value:null});
check(new G() instanceof G && new G() instanceof bound, 'null hook ordinary fallback');
Object.defineProperty(G, Symbol.hasInstance, {value:function() { return false; }});
for (i = 0; i < 2; ++i) {
    version(i ? 170 : 0);
    var legacy = evaluate('(function(value, ctor){return value instanceof ctor;})', 'legacy-instanceof');
    version(2015);
    check(legacy(new G(), G), 'legacy class hook ' + i);
}
check(!(new G() instanceof G), 'modern custom hook after legacy call');
print('ES6-HAS-INSTANCE checks=' + checks + ' failures=0');
