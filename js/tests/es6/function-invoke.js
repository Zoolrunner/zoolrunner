/* Modern call/apply conversion order and argument-list bounds.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
(function () {
    var checks = 0;
    function check(value, label) { ++checks; if (!value) throw new Error(label); }
    function throws(kind, action) { var caught = false; try { action(); } catch(e) { caught = e instanceof kind; } check(caught, 'expected ' + kind.name); }
    function count() { return arguments.length; }
    check(count.apply(null, {length:-1}) === 0, 'negative ToLength');
    check(count.apply(null, {length:NaN}) === 0, 'NaN length');
    check(count.apply(null, {length:1.9,0:7}) === 1, 'fractional length');
    throws(RangeError, function () { count.apply(null, {length:4294967296}); });
    throws(RangeError, function () { count.apply(null, {length:Infinity}); });
    throws(TypeError, function () { count.apply(null, {length:Symbol()}); });
    throws(TypeError, function () { count.apply(null, 'ab'); });
    check(count.apply() === 0 && count.apply(null, null) === 0 && count.apply(null, void 0) === 0, 'empty lists');
    var calls = 0, invalid = {toString:function () { ++calls; throw 'conversion'; }};
    throws(TypeError, function () { Function.prototype.call.call(invalid); });
    throws(TypeError, function () { Function.prototype.apply.call(invalid); });
    throws(TypeError, function () { Function.prototype.apply.call(invalid, null, {get length() { ++calls; return 0; }}); });
    check(calls === 0, 'noncallable receiver conversion suppressed');
    function receiver() { 'use strict'; return this; }
    check(receiver.call(null) === null && receiver.apply(void 0) === void 0, 'raw null and undefined');
    check(receiver.call(3) === 3 && receiver.apply(4, []) === 4, 'raw number');
    var symbol = Symbol('receiver'); check(receiver.call(symbol) === symbol, 'raw symbol');
    var order = '', values = new Proxy({length:2,0:'a',1:'b'}, {
        get:function (target, name) { order += name + ';'; gc(); return target[name]; },
        has:function () { throw 'must not use HasProperty'; }
    });
    check(function (a,b) { return a+b; }.apply(null, values) === 'ab', 'proxy list');
    check(order === 'length;0;1;', 'get order');
    var list = {length:3};
    Object.defineProperty(list, '0', {get:function () { return {value:1}; }});
    Object.defineProperty(list, '1', {get:function () { gc(); return {value:2}; }});
    Object.defineProperty(list, '2', {get:function () { gc(); return {value:3}; }});
    check(function (a,b,c) { return a.value+b.value+c.value; }.apply(null,list) === 6, 'partial list rooting');
    var target = new Proxy(function () {}, {apply:function (fn, thisValue, args) { return thisValue === symbol && args.length === 1 && args[0] === 9; }});
    check(target.apply(symbol, [9]), 'proxy callable');
    var getCount = 0;
    throws(Error, function () { count.apply(null, {length:2,get 0() { throw new Error('stop'); },get 1() { ++getCount; }}); });
    check(getCount === 0, 'abrupt element read');
    throws(TypeError, function () { new Function.prototype.apply(); });
    throws(TypeError, function () { new Function.prototype.call(); });
    check(Function.prototype.apply.length === 2 && Function.prototype.call.length === 1, 'arities');
    print('ES6-FUNCTION-INVOKE checks=' + checks + ' failures=0');
}());
