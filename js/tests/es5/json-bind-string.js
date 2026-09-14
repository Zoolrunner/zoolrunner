/* Native ES5 library regressions. Run with xpcshell -f. */
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
    throw Error(label + ': missing exception');
}
function collect() { if (typeof gc === 'function') gc(); }
var spaces = '\t\n\v\f\r \u00a0\u1680\u180e\u2000\u2001\u2002\u2003\u2004\u2005\u2006\u2007\u2008\u2009\u200a\u2028\u2029\u202f\u205f\u3000\ufeff';
same((spaces + 'x' + spaces).trim(), 'x', 'ES5 whitespace');
same(spaces.trim(), '', 'all whitespace');
same('\u0085x\u200b'.trim(), '\u0085x\u200b', 'non-whitespace retained');
same(String.prototype.trim.call(123), '123', 'primitive receiver');
same(String.prototype.trim.call({toString: function () { collect(); return ' x '; }}), 'x', 'receiver conversion and GC');
throwsType(function () { String.prototype.trim.call(null); }, TypeError, 'trim null');
throwsType(function () { String.prototype.trim.call(undefined); }, TypeError, 'trim undefined');
throwsType(function () { Array.prototype.push.call(null, 1); }, TypeError, 'array null');
throwsType(function () { new String.prototype.trim(); }, TypeError, 'trim not constructor');
var conversions = 0;
same((function (a, b) { return a + b; }).apply(null, {
    get length() { ++conversions; collect(); return 2; },
    get 0() { collect(); return 3; }, 1: 4
}), 7, 'apply arbitrary object');
same(conversions, 1, 'apply length read once');
throwsType(function () { Function.prototype.call.call({valueOf: function () { throw 1; }}); }, TypeError, 'call does not coerce callee');
same(JSON.parse('"\u2028\u2029"'), '\u2028\u2029', 'JSON line separators in strings');
same(1 / JSON.parse('-0'), -Infinity, 'JSON negative zero');
same(JSON.parse('1e400'), Infinity, 'JSON overflowing number');
var bad = ['01', '+1', '.1', '1.', '1e', '1e+', '[1,]', '{"a":1,}', '{a:1}', '"\t"', '"\\x41"', '\ufeff1', 'true false', '[undefined]'];
for (var i = 0; i < bad.length; i++) (function (text) {
    throwsType(function () { JSON.parse(text); }, SyntaxError, 'JSON grammar ' + text);
})(bad[i]);
var parsed = JSON.parse('{"__proto__":{"injected":true},"a":1,"a":2}');
same(Object.getPrototypeOf(parsed), Object.prototype, 'JSON prototype unaffected');
same(Object.prototype.hasOwnProperty.call(parsed, '__proto__'), true, 'JSON own __proto__ data');
same(parsed.injected, undefined, 'JSON does not mutate prototype');
same(parsed.a, 2, 'last duplicate member wins');
var order = [];
parsed = JSON.parse('{"a":[1,2]}', function (key, value) {
    collect();
    order.push(key);
    if (key === '0') return undefined;
    return value;
});
same(order.join(','), '0,1,a,', 'reviver bottom-up and root');
same(0 in parsed.a, false, 'reviver array deletion creates hole');
same(parsed.a.length, 2, 'reviver retains array length');
same(JSON.parse('1', function () { return undefined; }), undefined, 'reviver root deletion');
var obj = {a: 1, b: 2};
Object.defineProperty(obj, 'hidden', {value: 3});
same(JSON.stringify(obj), '{"a":1,"b":2}', 'own enumerable keys');
same(JSON.stringify([undefined, function () {}, NaN, Infinity, -0]), '[null,null,null,null,0]', 'array primitive substitutions');
same(JSON.stringify({x: undefined, f: function () {}}), '{}', 'object omissions');
same(JSON.stringify(undefined), undefined, 'top-level omission');
same(JSON.stringify({a: 1, b: 2}, ['b', 'b', new String('a')]), '{"b":2,"a":1}', 'replacer key ordering and deduplication');
same(JSON.stringify({a: 1}, null, 2), '{\n  "a": 1\n}', 'indentation');
same(JSON.stringify(new Boolean(false)), 'false', 'Boolean unboxing');
same(JSON.stringify(new Number(4)), '4', 'Number unboxing');
same(JSON.stringify(new String('x')), '"x"', 'String unboxing');
var shared = {x: 1};
same(JSON.stringify([shared, shared]), '[{"x":1},{"x":1}]', 'shared subobject allowed');
shared.self = shared;
throwsType(function () { JSON.stringify(shared); }, TypeError, 'cycle rejected');
var calls = [];
same(JSON.stringify({a: {toJSON: function (key) { collect(); calls.push('toJSON:' + key); return 2; }}}, function (key, value) {
    collect(); calls.push('replace:' + key); return value;
}), '{"a":2}', 'JSON callbacks with GC');
same(calls.join(','), 'replace:,toJSON:a,replace:a', 'JSON callback order');
same(JSON.stringify(new Date(0)), '"1970-01-01T00:00:00.000Z"', 'Date toJSON');
same(new Date(NaN).toJSON(), null, 'invalid Date toJSON');
same(Date.prototype.toJSON.call({valueOf: function () { return Infinity; }, get toISOString() { throw 1; }}), null, 'nonfinite skips ISO lookup');
same(Date.prototype.toJSON.call({valueOf: function () { return 0; }, toISOString: function () { collect(); return 'ok'; }}), 'ok', 'generic toJSON');
function Target(a, b) { this.total = a + b; return this; }
var receiver = {}, bound = Target.bind(receiver, 3);
same(bound.length, 1, 'bound length');
same(bound(4), receiver, 'bound receiver');
same(receiver.total, 7, 'bound arguments');
same(bound.hasOwnProperty('prototype'), false, 'bound has no implicit prototype');
Object.defineProperty(bound, 'prototype', {get: function () { throw Error('must not read bound prototype'); }});
var instance = new bound(5);
same(instance.total, 8, 'bound constructor arguments');
same(Object.getPrototypeOf(instance), Target.prototype, 'bound constructor target prototype');
same(instance instanceof bound, true, 'bound instanceof target');
same(receiver.total, 7, 'construction ignores bound receiver');
var twice = bound.bind({}, 6);
same(twice(), receiver, 'rebinding preserves first receiver');
same(receiver.total, 9, 'rebinding appends arguments');
collect();
same(new twice().total, 9, 'nested bound construction after GC');
var poison = Object.getOwnPropertyDescriptor(bound, 'caller');
same(poison.get, poison.set, 'shared poison getter and setter');
same(poison.get, Object.getOwnPropertyDescriptor(twice, 'arguments').get, 'shared realm poison function');
same(Object.isExtensible(poison.get), false, 'poison function nonextensible');
throwsType(function () { return bound.caller; }, TypeError, 'bound caller throws');
throwsType(function () { bound.arguments = 3; }, TypeError, 'bound arguments setter throws');
throwsType(function () { new (Math.max.bind(null))(); }, TypeError, 'bound native nonconstructor');
print('ES5-JSON-BIND-STRING checks=' + checks + ' failures=0');
