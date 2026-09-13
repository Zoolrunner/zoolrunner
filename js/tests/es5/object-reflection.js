/* Run with xpcshell -f object-reflection.js. No descriptor-writing helpers. */
var checks = 0;
function same(a, b, label) {
    ++checks;
    if (a !== b) throw new Error(label + ': expected ' + b + ', got ' + a);
}
function typeError(action, label) {
    ++checks;
    try { action(); } catch (e) {
        if (e instanceof TypeError) return;
        throw e;
    }
    throw new Error(label + ': expected TypeError');
}
var primitives = [undefined, null, true, 3, 'abc'];
for (var i = 0; i < primitives.length; ++i) {
    typeError(function () { Object.keys(primitives[i]); }, 'keys primitive');
    typeError(function () { Object.getPrototypeOf(primitives[i]); }, 'prototype primitive');
    typeError(function () { Object.getOwnPropertyDescriptor(primitives[i], 'x'); }, 'descriptor primitive');
}
var proto = {inherited: 1};
var obj = {__proto__: proto, own: 2};
same(Object.getPrototypeOf(obj), proto, 'actual prototype');
same(Object.getPrototypeOf(Object.prototype), null, 'null prototype');
same(Object.keys(obj).join(','), 'own', 'only own enumerable keys');
same(Object.keys([,7]).join(','), '1', 'array holes and length omitted');
var sparse = [,,undefined,,];
same(sparse.length, 4, 'trailing elision contributes to length');
same(sparse.hasOwnProperty('0'), false, 'leading elision absent');
same(sparse.hasOwnProperty('2'), true, 'explicit undefined present');
same(sparse.hasOwnProperty('3'), false, 'trailing elision absent');
var sparseCopy = eval(sparse.toSource());
same(sparseCopy.length, 4, 'sparse toSource length');
same(sparseCopy.hasOwnProperty('0'), false, 'sparse toSource hole');
function literal() { return [,,undefined,,]; }
sparseCopy = eval('(' + literal.toString() + ')')();
same(sparseCopy.length, 4, 'bytecode decompilation length');
same(sparseCopy.hasOwnProperty('0'), false, 'bytecode decompilation hole');
same(sparseCopy.hasOwnProperty('2'), true, 'bytecode decompilation value');
var literals = ['[]','[,]','[,,]','[1,]','[1,,]','[,1,]'];
for (i = 0; i < literals.length; ++i) {
    var expected = eval(literals[i]);
    var fromSource = eval(expected.toSource());
    same(fromSource.length, expected.length, 'array source length ' + literals[i]);
    same(Object.keys(fromSource).join(','), Object.keys(expected).join(','),
         'array source holes ' + literals[i]);
    var factory = eval('(function () { return ' + literals[i] + '; })');
    var fromFunction = eval('(' + factory.toString() + ')')();
    same(fromFunction.length, expected.length, 'function source length ' + literals[i]);
    same(Object.keys(fromFunction).join(','), Object.keys(expected).join(','),
         'function source holes ' + literals[i]);
}
Array.prototype[0] = 42;
try {
    var inheritedHole = [,];
    same(inheritedHole[0], 42, 'elision reads inherited element');
    same(inheritedHole.hasOwnProperty('0'), false, 'inherited element remains absent');
} finally { delete Array.prototype[0]; }
same(Object.keys(new String('ab')).join(','), '0,1', 'boxed string indices');
function closure() { return function (a,b) {}; }
var f = closure();
f.own = 1;
same(Object.getPrototypeOf(f), Function.prototype, 'closure implementation prototype hidden');
same(Object.keys(f).join(','), 'own', 'function prototype not enumerable');
same(f.propertyIsEnumerable('prototype'), false, 'prototype enumeration');
(function (a,b) {
    same(Object.keys(arguments).join(','), '0,1,2', 'mapped and extra arguments enumerable');
    a = 9;
    same(Object.getOwnPropertyDescriptor(arguments, '0').value, 9, 'mapped argument descriptor');
})(1,2,3);
var read = 0;
function getter() { ++read; return 99; }
function setter(v) { read += v; }
obj.__defineGetter__('accessor', getter);
obj.__defineSetter__('accessor', setter);
var desc = Object.getOwnPropertyDescriptor(obj, 'accessor');
same(read, 0, 'getter not invoked');
same(desc.get, getter, 'getter identity');
same(desc.set, setter, 'setter identity');
same(desc.enumerable, true, 'accessor enumeration');
same(desc.configurable, true, 'accessor configurable');
same('value' in desc, false, 'no accessor value');
same('writable' in desc, false, 'no accessor writable');
desc = Object.getOwnPropertyDescriptor(obj, 'own');
same(desc.value, 2, 'data value');
same(desc.writable, true, 'data writable');
same(desc.enumerable, true, 'data enumerable');
same(desc.configurable, true, 'data configurable');
same(Object.getPrototypeOf(desc), Object.prototype, 'descriptor ordinary object');
desc.value = 8;
same(obj.own, 2, 'descriptor is a snapshot');
same(Object.getOwnPropertyDescriptor(obj, 'inherited'), undefined, 'inherited descriptor absent');
same(Object.getOwnPropertyDescriptor(obj, 'missing'), undefined, 'missing descriptor absent');
same(Object.getOwnPropertyDescriptor(f, 'length').value, 2, 'virtual function length');
same(Object.getOwnPropertyDescriptor(new String('abc'), 'length').value, 3, 'virtual string length');
same(Object.getOwnPropertyDescriptor(/a/g, 'global').value, true, 'virtual RegExp field');
var key = {toString: function () { return 'own'; }};
same(Object.getOwnPropertyDescriptor(obj, key).value, 2, 'property key ToString');
obj['-0'] = 3; obj['0'] = 4;
same(Object.getOwnPropertyDescriptor(obj, '-0').value, 3, 'negative zero string key distinct');
same(Object.getOwnPropertyDescriptor(obj, -0).value, 4, 'negative zero numeric key');
var global = this;
var constants = ['undefined','NaN','Infinity'];
for (i = 0; i < constants.length; ++i) {
    desc = Object.getOwnPropertyDescriptor(global, constants[i]);
    same(desc.writable, false, 'global read-only ' + constants[i]);
    same(desc.enumerable, false, 'global non-enumerable ' + constants[i]);
    same(desc.configurable, false, 'global permanent ' + constants[i]);
}
var undefined = 7; var Infinity = 9; var NaN = 11;
same(global.undefined, void 0, 'direct var preserves undefined');
same(global.Infinity, 1/0, 'direct var preserves Infinity');
same(global.NaN !== global.NaN, true, 'direct var preserves NaN');
eval('var undefined = 7; var Infinity = 9; var NaN = 11;');
same(global.undefined, void 0, 'var cannot overwrite global undefined');
same(global.Infinity, 1/0, 'var cannot overwrite global Infinity');
same(global.NaN !== global.NaN, true, 'var cannot overwrite global NaN');
eval('const legacyConstantForES5Probe = 12;');
var legacyRedeclared = false;
try { eval('var legacyConstantForES5Probe;'); }
catch (e) { legacyRedeclared = e instanceof TypeError; }
same(legacyRedeclared, true, 'legacy const redeclaration remains an error');
var intercepted = 0;
Array.prototype.__defineSetter__('0', function () { ++intercepted; });
try {
    var literalArray = [7];
    same(literalArray[0], 7, 'literal defines own element');
    same(intercepted, 0, 'literal bypasses inherited setter');
    var keys = Object.keys({x: 1});
    same(keys[0], 'x', 'returned array defines own indices');
    same(intercepted, 0, 'inherited array setter not called');
} finally { delete Array.prototype[0]; }
print('ES5-OBJECT-REFLECTION checks=' + checks + ' failures=0');
