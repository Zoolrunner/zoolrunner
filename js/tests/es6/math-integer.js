/* Portable ES2015 integer Math operations and binary64 rounding boundaries. */
var checks = 0;
function check(ok, label) {
    ++checks;
    if (!ok) throw Error('FAIL Math: ' + label);
}
function same(actual, expected, label) {
    check(actual === expected ? actual !== 0 || 1 / actual === 1 / expected
                             : actual !== actual && expected !== expected, label);
}
var names = ['sign', 'trunc', 'clz32', 'imul'];
for (var i = 0; i < names.length; ++i) {
    var name = names[i], fn = Math[name];
    check(typeof fn === 'function', name + ' exists');
    check(fn.length === (name === 'imul' ? 2 : 1), name + ' arity');
    var desc = Object.getOwnPropertyDescriptor(Math, name);
    check(desc.writable && desc.configurable && !desc.enumerable, name + ' property');
    check(!fn.hasOwnProperty('prototype'), name + ' no prototype');
    var threw = false;
    try { new fn(); } catch (e) { threw = e instanceof TypeError; }
    check(threw, name + ' not constructible');
    var calls = 0, sentinel = {};
    var coercible = {valueOf: function () { ++calls; gc(); return 3.75; }};
    same(fn.call(null, coercible, 2), fn(3.75, 2), name + ' coercion');
    check(calls === 1, name + ' once');
    threw = false;
    try { fn({valueOf: function () { throw sentinel; }}, 1); }
    catch (e) { threw = e === sentinel; }
    check(threw, name + ' preserves exception');
}
var values = [undefined, NaN, 0, -0, Infinity, -Infinity, 0.25, -0.25,
              1.75, -1.75, Number.MIN_VALUE, -Number.MIN_VALUE,
              Number.MAX_VALUE, -Number.MAX_VALUE, null, '3.75'];
var signs = [NaN, NaN, 0, -0, 1, -1, 1, -1, 1, -1, 1, -1, 1, -1, 0, 1];
var truncs = [NaN, NaN, 0, -0, Infinity, -Infinity, 0, -0, 1, -1, 0, -0,
              Number.MAX_VALUE, -Number.MAX_VALUE, 0, 3];
for (i = 0; i < values.length; ++i) {
    same(Math.sign(values[i]), signs[i], 'sign ' + i);
    same(Math.trunc(values[i]), truncs[i], 'trunc ' + i);
}
same(Math.sign(), NaN, 'missing sign');
same(Math.trunc(), NaN, 'missing trunc');
same(Math.clz32(), 32, 'missing clz32');
same(Math.imul(), 0, 'missing imul');
same(Math.imul(3), 0, 'missing imul operand');
for (i = 0; i < 32; ++i) {
    same(Math.clz32(Math.pow(2, i)), 31 - i, 'clz32 bit ' + i);
    same(Math.imul(Math.pow(2, i), 2), i === 31 ? 0 : i === 30 ? -2147483648 : Math.pow(2, i + 1), 'imul bit ' + i);
}
var clz = [[-1, 0], [0, 32], [4294967296, 32], [4294967297, 31],
           [Infinity, 32], [NaN, 32], [-0, 32], [1.99, 31], ['16', 27]];
for (i = 0; i < clz.length; ++i) same(Math.clz32(clz[i][0]), clz[i][1], 'clz conversion ' + i);
var products = [[4294967295, 4294967295, 1], [4294967295, 5, -5],
                [2147483648, -1, -2147483648], [-3.9, 4.9, -12],
                [4294967297, 7, 7], [NaN, 3, 0], [Infinity, -1, 0], [-0, 2, 0]];
for (i = 0; i < products.length; ++i) same(Math.imul(products[i][0], products[i][1]), products[i][2], 'product ' + i);
var order = '';
same(Math.imul({valueOf:function(){order+='a';gc();return 0;}},
               {valueOf:function(){order+='b';gc();return 9;}}), 0, 'zero still coerces both');
check(order === 'ab', 'left to right conversion');
order = ''; var caught = false;
try { Math.imul({valueOf:function(){order+='a';throw sentinel;}},
                {valueOf:function(){order+='b';return 1;}}); }
catch (e) { caught = e === sentinel; }
check(caught && order === 'a', 'first conversion stops on exception');
var rounds = [[0.49999999999999994, 0], [0.5, 1], [-0.5, -0],
              [-0.5000000000000001, -1], [-0.49999999999999994, -0],
              [4503599627370497, 4503599627370497],
              [-4503599627370497, -4503599627370497],
              [4503599627370495.5, 4503599627370496],
              [-4503599627370495.5, -4503599627370495],
              [Number.MIN_VALUE, 0], [-Number.MIN_VALUE, -0],
              [Infinity, Infinity], [-Infinity, -Infinity], [NaN, NaN], [0, 0], [-0, -0]];
for (i = 0; i < rounds.length; ++i) same(Math.round(rounds[i][0]), rounds[i][1], 'round ' + i);
print('ES6-MATH-INTEGER checks=' + checks + ' failures=0');
