/* Original ES2015 Annex B prototype initializers bypass public setters. */
var checks = 0;
function check(value, label) { ++checks; if (!value) throw Error(label); }
var original = Object.getOwnPropertyDescriptor(Object.prototype, '__proto__');
var calls = 0, proto = { inherited: 7 };
function verify(make) {
    var object = make(proto);
    check(Object.getPrototypeOf(object) === proto, 'literal prototype identity');
    check(!Object.prototype.hasOwnProperty.call(object, '__proto__'), 'no own proto data field');
    check(object.inherited === 7, 'inherited field');
    var empty = make(null);
    check(Object.getPrototypeOf(empty) === null, 'null prototype');
    check(Object.getOwnPropertyNames(empty).length === 0, 'null initializer creates no member');
    for (var i = 0, values = [undefined, false, 1, 'text', Symbol('p')]; i < values.length; ++i) {
        object = make(values[i]);
        check(Object.getPrototypeOf(object) === Object.prototype, 'primitive ignored');
        check(!Object.prototype.hasOwnProperty.call(object, '__proto__'), 'primitive creates no property');
    }
}
try {
    Object.defineProperty(Object.prototype, '__proto__', {
        configurable: true,
        get: function() { ++calls; throw Error('getter invoked'); },
        set: function() { ++calls; throw Error('setter invoked'); }
    });
    verify(function(p) { return { __proto__: p }; });
    verify(function(p) { 'use strict'; return { '__proto__': p }; });
    var computed = { ['__proto__']: proto };
    check(Object.getPrototypeOf(computed) === Object.prototype, 'computed key is ordinary');
    check(computed.__proto__ === proto, 'computed own value');
    var __proto__ = proto, shorthand = { __proto__ };
    check(Object.getPrototypeOf(shorthand) === Object.prototype, 'shorthand is ordinary');
    check(shorthand.__proto__ === proto, 'shorthand own value');
    var method = { __proto__() { return 9; } };
    check(Object.getPrototypeOf(method) === Object.prototype && method.__proto__() === 9, 'method is ordinary');
    var getter = { get __proto__() { return 11; } };
    check(Object.getPrototypeOf(getter) === Object.prototype && getter.__proto__ === 11, 'accessor is ordinary');
    var events = [];
    var ordered = { before: (events.push('before'), 1), __proto__: (events.push('prototype'), gc(), proto), after: (events.push('after'), 2) };
    check(events.join(',') === 'before,prototype,after', 'initializer evaluation order');
    check(ordered.before === 1 && ordered.after === 2 && Object.getPrototypeOf(ordered) === proto, 'GC and own fields');
    var trapped = 0, proxy = new Proxy(proto, { getPrototypeOf: function() { ++trapped; throw Error('prototype trap invoked'); } });
    var proxyObject = { __proto__: proxy };
    check(Object.getPrototypeOf(proxyObject) === proxy && trapped === 0, 'prototype proxy need not expose its chain');
    var mixed = { ['__proto__']: 5, __proto__: proto };
    check(Object.getPrototypeOf(mixed) === proto && mixed.__proto__ === 5, 'own computed field survives prototype initialization');
    var ownCalls = 0;
    var mixedGetter = { get __proto__() { ++ownCalls; return 5; }, __proto__: proto };
    check(Object.getPrototypeOf(mixedGetter) === proto && ownCalls === 0, 'own accessor is not invoked while setting prototype');
    var foreign = createTest262Realm().global.Object.prototype;
    check(Object.getPrototypeOf({ __proto__: foreign }) === foreign, 'foreign realm prototype identity');
    check(calls === 0, 'no public prototype accessor dispatch');
    var source = (function(p) { return { __proto__: p }; }).toString();
    check(Object.getPrototypeOf(eval('('+source+')')(proto)) === proto, 'decompiled literal retains semantics');
    Object.defineProperty(Object.prototype, '__proto__', { value: 42, writable: false, configurable: true });
    check(Object.getPrototypeOf({ __proto__: proto }) === proto, 'inherited readonly data field is bypassed');
    delete Object.prototype.__proto__;
    check(Object.getPrototypeOf({ __proto__: null }) === null, 'prototype initializer does not require public property');
} finally {
    Object.defineProperty(Object.prototype, '__proto__', original);
}
print('LITERAL-PROTOTYPE checks='+checks+' failures=0');
