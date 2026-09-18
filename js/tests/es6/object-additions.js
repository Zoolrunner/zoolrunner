/* New Object methods must preserve descriptor, callback and GC semantics. */
var checks = 0;
function check(ok, label) { ++checks; if (!ok) throw Error('FAIL Object additions: ' + label); }
function throwsType(fn, label) { var caught = false; try { fn(); } catch(e) { caught = e instanceof TypeError; } check(caught, label); }
for (var i = 0; i < 2; ++i) {
    (function(name) {
        var method = Object[name], desc = Object.getOwnPropertyDescriptor(Object, name);
        check(typeof method === 'function' && method.length === 2, name + ' arity');
        check(desc.writable && desc.configurable && !desc.enumerable, name + ' descriptor');
        check(!method.hasOwnProperty('prototype'), name + ' no prototype');
        throwsType(function() { new method(); }, name + ' not constructor');
    })(i ? 'assign' : 'is');
}
var staticNames = ['getOwnPropertyNames', 'seal', 'freeze', 'isSealed', 'isFrozen',
    'defineProperty', 'defineProperties', 'create', 'isExtensible',
    'preventExtensions', 'getPrototypeOf', 'keys', 'getOwnPropertyDescriptor'];
for (i = 0; i < staticNames.length; ++i)
    check(!Object[staticNames[i]].hasOwnProperty('prototype'), staticNames[i] + ' no constructor prototype');
check(Object.is(NaN, NaN), 'NaN identity');
check(Object.is(0 / 0, Math.sqrt(-1)), 'different NaN computations');
check(!Object.is(0, -0) && !Object.is(-0, 0), 'signed zero differs');
check(Object.is(-0, -0) && Object.is(0, 0), 'same signed zeros');
check(Object.is(Infinity, Infinity) && !Object.is(Infinity, -Infinity), 'infinities');
check(Object.is(1073741824, 1073741824) && !Object.is(1073741824, 1073741825), 'double-tagged integers');
check(Object.is(1, 1.0) && !Object.is(1, '1'), 'number equality without coercion');
check(Object.is('a' + 'b', 'xabz'.substring(1, 3)), 'string contents');
check(Object.is() && Object.is(undefined) && !Object.is(null, undefined), 'missing and null arguments');
check(Object.is(true, true) && !Object.is(true, 1) && !Object.is(false, 0), 'boolean types');
var object = {}, fn = function() {}, array = [];
check(Object.is(object, object) && !Object.is(object, {}), 'object identity');
check(Object.is(fn, fn) && !Object.is(fn, function() {}), 'function identity');
check(Object.is(array, array) && !Object.is(array, []), 'array identity');
object = {valueOf: function() { throw Error('must not coerce'); }, toString: function() { throw Error('must not coerce'); }};
check(Object.is(object, object) && !Object.is(object, 1), 'no coercion');
check(Object.is.call(null, 1, 1), 'is ignores receiver');
var target = {}, result = Object.assign(target, {a: 1}, {b: 2, a: 3});
check(result === target && target.a === 3 && target.b === 2, 'sources ordered and target returned');
check(Object.assign(target) === target && Object.assign(target, null, undefined) === target, 'no sources and null sources');
throwsType(function() { Object.assign(); }, 'missing target');
throwsType(function() { Object.assign(null, {}); }, 'null target');
throwsType(function() { Object.assign(undefined, {}); }, 'undefined target');
result = Object.assign(3, {x: 4}); check(typeof result === 'object' && result.valueOf() === 3 && result.x === 4, 'number target boxed');
result = Object.assign('ab', {x: 4}); check(result[0] === 'a' && result.x === 4, 'string target boxed');
result = Object.assign({}, 'abc', true, 8); check(result[0] === 'a' && result[2] === 'c' && Object.keys(result).length === 3, 'primitive sources');
var source = Object.create({inherited: 1}); source.own = 2;
Object.defineProperty(source, 'hidden', {value: 3}); result = Object.assign({}, source);
check(result.own === 2 && !result.hasOwnProperty('inherited') && !result.hasOwnProperty('hidden'), 'own enumerable only');
target = {valueOf: function() { throw Error('target must not convert'); }};
source = {a: 4}; Object.defineProperty(source, 'valueOf', {get: function() { throw Error('source must not convert'); }});
check(Object.assign(target, source) === target && target.a === 4, 'ToObject keeps object identity without conversion');
var order = '', seen = [];
source = {get a() { order += 'g'; gc(); return {x: 7}; }};
target = {set a(v) { order += 's'; gc(); check(this === target && v.x === 7, 'setter receiver and rooted value'); }};
check(Object.assign(target, source) === target && order === 'gs', 'get then set');
var locked = Object.freeze({x: 1});
Object.assign({set a(v) { locked.x = 2; check(locked.x === 1, 'sloppy setter remains sloppy'); }}, {a: 2});
source = {}; Object.defineProperty(source, 'a', {get: function() { return 9; }, enumerable: true});
result = Object.assign({}, source); var d = Object.getOwnPropertyDescriptor(result, 'a');
check(d.value === 9 && d.writable && d.configurable && d.enumerable && !('get' in d), 'copies value not accessor');
source = {get first() { delete source.second; source.third = 3; gc(); return 1; }, second: 2};
result = Object.assign({}, source); check(result.first === 1 && !result.hasOwnProperty('second') && !result.hasOwnProperty('third'), 'snapshot keys live deletion');
source = {get first() { Object.defineProperty(source, 'second', {enumerable: false}); Object.defineProperty(source, 'hidden', {enumerable: true}); return 1; }, second: 2};
Object.defineProperty(source, 'hidden', {value: 3, configurable: true});
result = Object.assign({}, source); check(result.first === 1 && !result.hasOwnProperty('second') && result.hidden === 3, 'live enumerability including initially hidden key');
source = {get first() { source.second = 8; return 1; }, second: 2};
check(Object.assign({}, source).second === 8, 'live values');
source = Object.create({second: 9}); Object.defineProperty(source, 'first', {get: function() { delete source.second; return 1; }, enumerable: true}); source.second = 2;
check(!Object.assign({}, source).hasOwnProperty('second'), 'deleted own property does not copy inherited replacement');
source = {}; var keys = ['z', '9007199254740991', '4294967295', '2', '0', '01', '-0', '9007199254740992', '1.0', 'a'];
for (i = 0; i < keys.length; ++i) (function(key) { Object.defineProperty(source, key, {get: function() { seen.push(key); gc(); return key; }, enumerable: true}); })(keys[i]);
Object.assign({}, source);
check(seen.join('|') === '0|2|4294967295|9007199254740991|z|01|-0|9007199254740992|1.0|a', 'ES2015 integer-index then creation order');
var sentinel = {}, caught = false;
source = {a: 1, get b() { throw sentinel; }, c: 3}; target = {};
try { Object.assign(target, source); } catch(e) { caught = e === sentinel; }
check(caught && target.a === 1 && !target.hasOwnProperty('b') && !target.hasOwnProperty('c'), 'getter exception retains preceding effects');
order = ''; caught = false; target = {set a(v) { throw sentinel; }};
try { Object.assign(target, {a: 1, get b() { order += 'b'; return 2; }}); } catch(e) { caught = e === sentinel; }
check(caught && order === '', 'setter exception stops later reads');
throwsType(function() { Object.assign(Object.freeze({a: 1}), {a: 2}); }, 'readonly target');
throwsType(function() { Object.assign(Object.preventExtensions({}), {a: 2}); }, 'nonextensible target');
throwsType(function() { Object.assign({get a() { return 1; }}, {a: 2}); }, 'getter-only target');
throwsType(function() { Object.assign('x', {0: 'y'}); }, 'immutable string target index');
var inherited = Object.defineProperty({}, 'a', {value: 1, writable: false});
throwsType(function() { Object.assign(Object.create(inherited), {a: 2}); }, 'inherited readonly target');
source = {get a() { check(Object.assign({}, {inner: 2}).inner === 2, 'reentrant assign'); gc(); return 5; }};
check(Object.assign({}, source).a === 5, 'outer reentrant assign');
source = {}; source['first'] = 1;
for (i = 0; i < 64; ++i) source['temporary-key-' + i] = i;
target = {set first(v) { for (var j = 0; j < 64; ++j) delete source['temporary-key-' + j]; gc(); gc(); }};
Object.assign(target, source); check(Object.keys(target).length === 1, 'snapshot names rooted after deletion and GC');
var newer = {z: 7}; source = {get a() { newer.z = 8; newer.extra = 9; return 1; }};
result = Object.assign({}, source, newer); check(result.z === 8 && result.extra === 9, 'each source snapshots when reached');
source = {}; Object.defineProperty(source, '__proto__', {value: inherited, enumerable: true}); target = {};
check(Object.assign(target, source) === target && Object.getPrototypeOf(target) === inherited && !target.hasOwnProperty('__proto__'), 'ordinary inherited setter dispatch');
check(!({}).hasOwnProperty('__proto__') && Object.prototype.hasOwnProperty('__proto__'), 'modern prototype ownership');
var savedEdition = version();
try {
    version(170);
    check(evaluate("({}).hasOwnProperty('__proto__')", 'legacy-proto-ownership'), 'legacy prototype ownership preserved');
} finally { version(savedEdition); }
var primitives = [undefined, null, true, false, 3, NaN, 'ab'];
for (i = 0; i < primitives.length; ++i) {
    var primitive = primitives[i];
    check(Object.isExtensible(primitive) === false, 'primitive not extensible ' + i);
    check(Object.isFrozen(primitive) && Object.isSealed(primitive), 'primitive integrity queries ' + i);
    check(Object.is(Object.freeze(primitive), primitive), 'freeze primitive identity ' + i);
    check(Object.is(Object.seal(primitive), primitive), 'seal primitive identity ' + i);
    check(Object.is(Object.preventExtensions(primitive), primitive), 'preventExtensions primitive identity ' + i);
}
check(Object.getPrototypeOf(3) === Number.prototype, 'number prototype');
check(Object.getPrototypeOf(true) === Boolean.prototype, 'boolean prototype');
check(Object.getPrototypeOf('ab') === String.prototype, 'string prototype');
check(Object.keys('ab').join() === '0,1', 'string enumerable keys');
check(Object.keys(3).length === 0 && Object.keys(false).length === 0, 'number and boolean keys');
check(Object.getOwnPropertyNames('ab').join() === '0,1,length', 'string own names');
check(Object.getOwnPropertyNames(3).length === 0, 'number own names');
d = Object.getOwnPropertyDescriptor('ab', '0');
check(d.value === 'a' && d.enumerable && !d.writable && !d.configurable, 'string index descriptor');
d = Object.getOwnPropertyDescriptor('ab', {toString: function() { gc(); return 'length'; }});
check(d.value === 2 && !d.enumerable && !d.writable && !d.configurable, 'boxed receiver rooted during key conversion');
check(Object.getOwnPropertyDescriptor(1, 'x') === undefined, 'number absent descriptor');
var reflection = ['getPrototypeOf', 'keys', 'getOwnPropertyNames', 'getOwnPropertyDescriptor'];
for (i = 0; i < reflection.length; ++i) (function(name) {
    throwsType(function() { Object[name](null); }, name + ' null still rejected');
    throwsType(function() { Object[name](undefined); }, name + ' undefined still rejected');
})(reflection[i]);
var reflectedOrder = {}; reflectedOrder.z = 1; reflectedOrder['4294967295'] = 2; reflectedOrder[2] = 3; reflectedOrder.a = 4;
check(Object.getOwnPropertyNames(reflectedOrder).join() === '2,4294967295,z,a', 'modern reflection integer-index order');
var oldMethods = reflection.concat(['freeze', 'seal', 'preventExtensions', 'isExtensible', 'isFrozen', 'isSealed']);
for (var editionIndex = 0; editionIndex < 2; ++editionIndex) {
    savedEdition = version();
    try {
        version(editionIndex ? 170 : 0);
        for (i = 0; i < oldMethods.length; ++i)
            check(evaluate("var oldRejected=false;try{Object." + oldMethods[i] + "(1);}catch(e){oldRejected=e instanceof TypeError;}oldRejected", 'old-object-argument'), 'old primitive rejection ' + oldMethods[i] + ' edition ' + editionIndex);
        check(evaluate("var oldOrder={z:1};oldOrder[2]=2;Object.getOwnPropertyNames(oldOrder).join()==='z,2'", 'old-key-order'), 'old own-name order ' + editionIndex);
    } finally { version(savedEdition); }
}
print('ES6-OBJECT-ADDITIONS checks=' + checks + ' failures=0');
