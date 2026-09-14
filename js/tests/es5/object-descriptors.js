/* Native ES5 descriptor and integrity regressions. Run with xpcshell -f. */
var checks = 0;
function same(actual, expected, label) {
    ++checks;
    if (actual !== expected) throw Error(label + ': expected ' + expected + ', got ' + actual);
}
function throwsType(fn, kind, label) {
    ++checks;
    try { fn(); } catch (e) {
        if (e instanceof kind) return;
        throw Error(label + ': wrong exception ' + e);
    }
    throw Error(label + ': expected ' + kind.name);
}
var obj = {}, getterCalls = 0, setterCalls = 0;
Object.defineProperty(obj, 'hidden', {value: 1});
var d = Object.getOwnPropertyDescriptor(obj, 'hidden');
same(d.value, 1, 'default value');
same(d.writable, false, 'default writable');
same(d.enumerable, false, 'default enumerable');
same(d.configurable, false, 'default configurable');
same(Object.defineProperty(obj, 'hidden', {value: 1}), obj, 'identical definition');
throwsType(function () { Object.defineProperty(obj, 'hidden', {value: 2}); }, TypeError, 'permanent value');
Object.defineProperty(obj, 'nan', {value: NaN});
Object.defineProperty(obj, 'nan', {value: NaN});
Object.defineProperty(obj, 'zero', {value: 0});
throwsType(function () { Object.defineProperty(obj, 'zero', {value: -0}); }, TypeError, 'signed zero');
function getter() { ++getterCalls; return 42; }
function setter(v) { ++setterCalls; }
Object.defineProperty(obj, 'access', {get: getter, set: setter, configurable: true});
d = Object.getOwnPropertyDescriptor(obj, 'access');
same(getterCalls, 0, 'descriptor does not invoke getter');
same(d.get, getter, 'getter identity');
same(d.set, setter, 'setter identity');
Object.defineProperty(obj, 'access', {get: undefined});
same(obj.access, undefined, 'cleared getter');
obj.access = 7;
same(setterCalls, 1, 'retained setter');
Object.defineProperty(obj, 'access', {value: 12, writable: true});
same(obj.access, 12, 'accessor to data');
obj.access = 13;
same(obj.access, 13, 'writable converted property');
Object.defineProperty(obj, 'access', {get: getter});
same(obj.access, 42, 'data to accessor');
Object.defineProperty(obj, 'empty', {get: undefined, set: undefined});
same(obj.empty, undefined, 'empty accessor');
obj.empty = 9;
same(obj.empty, undefined, 'empty accessor write ignored');
throwsType(function () { Object.defineProperty({}, 'x', {get: {}, value: 2}); }, TypeError, 'invalid accessor');
var proto = {};
Object.defineProperty(proto, 'inherited', {get: getter, enumerable: true});
var child = Object.create(proto);
same(child.hasOwnProperty('inherited'), false, 'accessor is inherited');
same(Object.getOwnPropertyDescriptor(child, 'inherited'), undefined, 'no own descriptor');
child.inherited = 3;
same(child.inherited, 42, 'inherited getter without setter');
var dict = Object.create(null);
same(Object.getPrototypeOf(dict), null, 'null prototype');
Object.defineProperty(dict, '__proto__', {value: 8});
same(dict.__proto__, 8, 'ordinary __proto__ property');
var order = [], desc = {};
['enumerable', 'configurable', 'value', 'writable', 'get', 'set'].forEach(function (name) {
    Object.defineProperty(desc, name, {get: function () {
        order.push(name);
        if (typeof gc === 'function') gc();
        return name === 'value' ? {rooted: true} : undefined;
    }});
});
throwsType(function () { Object.defineProperty({}, 'x', desc); }, TypeError, 'mixed descriptor');
same(order.join(','), 'enumerable,configurable,value,writable,get,set', 'descriptor conversion order');
var target = {}, props = {first: {value: 1}, last: {get: 3}};
throwsType(function () { Object.defineProperties(target, props); }, TypeError, 'conversion before writes');
same(Object.keys(target).length, 0, 'no partial conversion writes');
props = {};
Object.defineProperty(props, 'first', {enumerable: true, get: function () {
    delete props.last;
    return {value: 1, enumerable: true};
}});
props.last = {value: 2};
Object.defineProperties(target, props);
same(target.first, 1, 'first descriptor applied');
same(target.hasOwnProperty('last'), false, 'deleted descriptor skipped');
var closed = {x: 1};
same(Object.preventExtensions(closed), closed, 'preventExtensions result');
same(Object.isExtensible(closed), false, 'closed object');
closed.x = 2;
closed.y = 3;
same(closed.x, 2, 'existing property remains writable');
same(closed.hasOwnProperty('y'), false, 'assignment cannot extend');
throwsType(function () { Object.defineProperty(closed, 'z', {value: 4}); }, TypeError, 'definition cannot extend');
Object.defineProperty(closed, 'x', {value: 5});
same(closed.x, 5, 'existing definition allowed');
same(Object.isExtensible(Object.create(closed)), true, 'extensibility not inherited');
var sealed = Object.seal({x: 1});
same(Object.isSealed(sealed), true, 'sealed');
same(Object.isFrozen(sealed), false, 'sealed still writable');
sealed.x = 2;
same(sealed.x, 2, 'sealed write');
same(delete sealed.x, false, 'sealed delete');
Object.freeze(sealed);
same(Object.isFrozen(sealed), true, 'frozen');
sealed.x = 3;
same(sealed.x, 2, 'frozen write ignored');
var sink = 0, frozen = {};
Object.defineProperty(frozen, 'x', {set: function (v) { sink = v; }});
Object.freeze(frozen);
frozen.x = 11;
same(sink, 11, 'frozen accessor setter remains callable');
var arr = [0, 1, 2, 3, 4];
Object.defineProperty(arr, '2', {configurable: false});
throwsType(function () { Object.defineProperty(arr, 'length', {value: 1, writable: false}); }, TypeError, 'blocked shrink');
same(arr.length, 3, 'blocked shrink stops at permanent index');
same(arr.hasOwnProperty('4'), false, 'higher index deleted');
same(Object.getOwnPropertyDescriptor(arr, 'length').writable, false, 'blocked shrink still makes length readonly');
throwsType(function () { Object.defineProperty(arr, '3', {value: 3}); }, TypeError, 'readonly length forbids growth');
arr[3] = 3;
same(arr.hasOwnProperty('3'), false, 'assignment respects readonly length');
var shrinking = [0, 1, 2, 3];
Object.defineProperty(shrinking, '1', {configurable: false});
shrinking.length = 0;
same(shrinking.length, 2, 'assignment shrink stops at permanent index');
same(shrinking.hasOwnProperty('3'), false, 'assignment shrink deletes higher indices');
var sparse = [];
Object.defineProperty(sparse, '4294967294', {value: 7, configurable: true});
same(sparse.length, 4294967295, 'large index length');
Object.defineProperty(sparse, 'length', {value: 0});
same(sparse.hasOwnProperty('4294967294'), false, 'large nonenumerable index deleted');
(function (arg) {
    Object.defineProperty(arguments, '0', {value: 17});
    same(arg, 17, 'descriptor updates mapped argument');
    Object.defineProperty(arguments, '0', {value: 18, writable: false});
    same(arg, 18, 'readonly descriptor updates parameter first');
    arg = 19;
    same(arguments[0], 18, 'readonly descriptor disconnects parameter');
}(1));
(function (arg) {
    Object.defineProperty(arguments, '0', {get: function () { return 20; }});
    arg = 21;
    same(arguments[0], 20, 'accessor disconnects parameter');
    same(Object.prototype.toString.call(arguments), '[object Arguments]', 'arguments class tag');
}(1));
(function () {
    var sink = 0;
    Object.defineProperty(arguments, 'setterOnly', {set: function (v) { sink = v; }});
    same(arguments.setterOnly, undefined, 'arguments setter-only property has no native getter');
    arguments.setterOnly = 23;
    same(sink, 23, 'arguments setter-only setter');
}(1, 2, 3));
var re = /x/g, converted = 0;
var indexValue = {valueOf: function () { ++converted; return 1.5; }};
re.lastIndex = indexValue;
same(converted, 0, 'lastIndex assignment does not convert');
same(re.lastIndex, indexValue, 'lastIndex retains object identity');
same(re.exec('ax')[0], 'x', 'exec converts and truncates lastIndex');
same(converted, 1, 'exec converts once');
Object.defineProperty(re, 'lastIndex', {value: indexValue});
same(re.lastIndex, indexValue, 'descriptor updates native lastIndex');
re.lastIndex = {
    valueOf: function () { re.lastIndex = null; if (typeof gc === 'function') gc(); return {}; },
    toString: function () { if (typeof gc === 'function') gc(); return '0'; }
};
same(re.exec('x')[0], 'x', 'lastIndex survives reentrant conversion and GC');
Object.defineProperty(re, 'lastIndex', {value: 0, writable: false});
throwsType(function () { re.exec('x'); }, TypeError, 'exec cannot update readonly lastIndex');
var names = Object.getOwnPropertyNames(obj).sort().join(',');
same(names, 'access,empty,hidden,nan,zero', 'all own names, no inherited names');
print('ES5-OBJECT-DESCRIPTORS checks=' + checks + ' failures=0');
