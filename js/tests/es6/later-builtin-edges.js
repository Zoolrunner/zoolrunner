/* Original ES2015 built-in edge cases found by later Test262 coverage. */
var checks = 0;
function check(value, label) { ++checks; if (!value) throw Error(label); }
function throwsType(fn, label) {
    var threw = false;
    try { fn(); } catch (e) { threw = e instanceof TypeError; }
    check(threw, label);
}
function length(fn, value) {
    var d = Object.getOwnPropertyDescriptor(fn, 'length');
    check(d.value === value && !d.writable && !d.enumerable && d.configurable,
          'method length descriptor');
}
length(Number.prototype.toString, 1);
length(RegExp.prototype.compile, 2);
check((31).toString(16) === '1f', 'radix argument');
check((31).toString() === '31', 'omitted radix');
var re = /a/;
re.compile('b', 'i');
check(re.test('B'), 'compile arguments');
check('abc'.substr(0, undefined) === 'abc', 'undefined length');
check('abc'.substr(-2, undefined) === 'bc', 'negative start');
check('abc'.substr(20, undefined) === '', 'past end');
check('abc'.substr(1, null) === '', 'null length');
check('abc'.substr(1, 1) === 'b', 'numeric length');
var order = [];
check(String.prototype.substr.call({toString:function(){order.push('this');return 'abc';}},
    {valueOf:function(){order.push('start');return 1;}},
    {valueOf:function(){order.push('length');return 2;}}) === 'bc', 'generic substr');
check(order.join(',') === 'this,start,length', 'conversion order');
class DerivedSymbol extends Symbol {}
check(Object.getPrototypeOf(DerivedSymbol) === Symbol, 'Symbol superclass');
check(Object.getPrototypeOf(DerivedSymbol.prototype) === Symbol.prototype, 'Symbol prototype');
throwsType(function(){new DerivedSymbol();}, 'default super throws');
throwsType(function(){new Symbol();}, 'direct construction throws');
var calls = 0, description = {toString:function(){++calls;return 'x';}};
throwsType(function(){new Symbol(description);}, 'throw before conversion');
check(calls === 0, 'construction does not convert description');
check(String(Symbol(description)) === 'Symbol(x)' && calls === 1, 'ordinary call converts');
class ReturnsObject extends Symbol { constructor() { return {value: 7}; } }
check(new ReturnsObject().value === 7, 'derived constructor returning object');
throwsType(function(){Reflect.construct(Symbol, []);}, 'Reflect construction throws');
print('LATER-BUILTIN-EDGES checks=' + checks + ' failures=0');
