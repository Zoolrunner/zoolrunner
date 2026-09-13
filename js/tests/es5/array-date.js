/* Focused native ES5 regressions. Run with xpcshell -f array-date.js. */
var checks = 0;
function same(actual, expected, message) {
    ++checks;
    if (actual !== expected)
        throw new Error(message + ': expected ' + expected + ', got ' + actual);
}
function raises(type, action, message) {
    ++checks;
    try { action(); } catch (e) {
        if (e instanceof type) return;
        throw new Error(message + ': unexpected ' + e);
    }
    throw new Error(message + ': did not throw');
}
same(Array.isArray([]), true, 'array');
same(Array.isArray(Array.prototype), true, 'array prototype');
same(Array.isArray(), false, 'missing argument');
same(Array.isArray(null), false, 'null');
same(Array.isArray({length: 0}), false, 'array-like');
same(Array.isArray({__proto__: Array.prototype}), false, 'inheriting from array');
same(Array.isArray(new String('abc')), false, 'boxed string');
same(Array.isArray.length, 1, 'isArray arity');
raises(TypeError, function () { new Array.isArray([]); }, 'not a constructor');
function sum(a, b) { return a + b; }
var sparse = new Array(5);
sparse[2] = 7;
sparse[4] = 11;
same(sparse.reduce(sum), 18, 'reduce skips leading holes');
same(sparse.reduceRight(sum), 18, 'reduceRight skips holes');
same(sparse.reduce(sum, 3), 21, 'explicit initial value');
same(new Array(3).reduce(sum, undefined), undefined, 'explicit undefined initial');
raises(TypeError, function () { new Array(3).reduce(sum); }, 'all holes');
raises(TypeError, function () { new Array(3).reduceRight(sum); }, 'all holes right');
raises(TypeError, function () { [].reduce(sum); }, 'empty');
raises(TypeError, function () { [].reduce(null, 1); }, 'empty still validates callback');
var proto = {1: 4, 3: 8};
var like = {__proto__: proto, length: 5};
same(Array.prototype.reduce.call(like, sum), 12, 'inherited accumulator and elements');
same(Array.prototype.reduceRight.call(like, sum), 12, 'inherited elements right');
var one = new Array(8);
one[4] = 15;
same(one.reduce(function () { throw new Error('unexpected callback'); }), 15, 'only one present');
same(one.reduceRight(function () { throw new Error('unexpected callback'); }), 15, 'only one right');
var order = [];
sparse.reduceRight(function (a, b, i, receiver) {
    same(receiver, sparse, 'original receiver');
    order.push(i);
    return a - b;
}, 20);
same(order.join(','), '4,2', 'right callback indices');
var changing = [1,2,3];
same(changing.reduce(function (a, b, i) {
    if (i === 0) { delete changing[1]; changing.push(9); }
    return a + b;
}, 0), 4, 'length snapshot and deleted elements');
var high = {length: 4294967295};
high[4294967294] = 17;
var stopped = false;
try {
    Array.prototype.reduceRight.call(high, function (a, b, i) {
        same(i, 4294967294, 'full-width callback index');
        same(b, 17, 'full-width element access');
        stopped = true;
        throw 'stop before scanning billions of holes';
    }, 0);
} catch (e) {
    if (e !== 'stop before scanning billions of holes') throw e;
}
same(stopped, true, 'large array-like callback invoked');
var cases = [
    [0, '1970-01-01T00:00:00.000Z'],
    [-1, '1969-12-31T23:59:59.999Z'],
    [951782400123, '2000-02-29T00:00:00.123Z'],
    [-62167219200000, '0000-01-01T00:00:00.000Z'],
    [-62198755200000, '-000001-01-01T00:00:00.000Z'],
    [253402300800000, '+010000-01-01T00:00:00.000Z'],
    [8640000000000000, '+275760-09-13T00:00:00.000Z'],
    [-8640000000000000, '-271821-04-20T00:00:00.000Z']
];
for (var i = 0; i < cases.length; ++i)
    same(new Date(cases[i][0]).toISOString(), cases[i][1], 'ISO date ' + i);
same(Date.prototype.toISOString.length, 0, 'ISO arity');
raises(RangeError, function () { new Date(NaN).toISOString(); }, 'invalid date');
raises(RangeError, function () { new Date(Infinity).toISOString(); }, 'infinite date');
raises(TypeError, function () { Date.prototype.toISOString.call({}); }, 'non-date receiver');
raises(TypeError, function () { new Date.prototype.toISOString(); }, 'ISO not a constructor');
same(Array.isArray.hasOwnProperty('prototype'), false, 'isArray has no prototype');
same(Date.prototype.toISOString.hasOwnProperty('prototype'), false, 'ISO has no prototype');
same(Function.prototype.hasOwnProperty('prototype'), false, 'Function.prototype has no prototype');
same(Array.isArray.prototype, undefined, 'no inherited implicit prototype');
same(Function.prototype(), undefined, 'Function.prototype remains callable');
raises(TypeError, function () { new Function.prototype(); }, 'Function.prototype is not a constructor');
raises(TypeError, function () { return {} instanceof Array.isArray; }, 'instanceof requires a prototype');
var prototypeRead = false;
Array.isArray.__defineGetter__('prototype', function () { prototypeRead = true; return {}; });
try {
    raises(TypeError, function () { new Array.isArray(); }, 'reject before prototype lookup');
    same(prototypeRead, false, 'non-constructor does not read prototype getter');
} finally {
    delete Array.isArray.prototype;
}
print('ES5-ARRAY-DATE checks=' + checks + ' failures=0');
