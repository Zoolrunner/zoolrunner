/* Proxy invariants, receivers, revocation and callback lifetime regressions.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
var checks = 0;
function check(ok, label) { ++checks; if (!ok) throw Error('FAIL Proxy: ' + label); }
function caught(fn) { try { fn(); } catch (e) { return e; } return null; }
var target = {a: 1}, handler = {}, proxy = new Proxy(target, handler);
check(proxy !== target && proxy.a === 1, 'empty handler forwards get');
proxy.a = 2;
check(target.a === 2 && 'a' in proxy, 'empty handler forwards set and has');
check(Object.keys(proxy).join() === 'a', 'empty handler forwards own keys');
check(delete proxy.a && !('a' in target), 'empty handler forwards delete');
check(caught(function() { Proxy({}, {}); }) instanceof TypeError, 'requires new');
check(caught(function() { new Proxy(null, {}); }) instanceof TypeError, 'object target');
check(caught(function() { new Proxy({}, null); }) instanceof TypeError, 'object handler');
var receiver, observed, symbol = Symbol('key');
target = {};
handler = {get: function(t, k, r) { gc(); observed = [this, t, k]; receiver = r; return 17; }};
proxy = new Proxy(target, handler);
check(proxy[symbol] === 17 && observed[0] === handler && observed[1] === target && observed[2] === symbol && receiver === proxy, 'get trap arguments and receiver');
check(Reflect.get(proxy, symbol, null) === 17 && receiver === null, 'Reflect raw receiver');
var accessLog = [], inheritedProxy = new Proxy({}, {
    get: function(t, k, r) { accessLog.push('get'); return r; },
    has: function() { accessLog.push('has'); return false; },
    set: function(t, k, v, r) { receiver = r; accessLog.push('set'); return true; }
});
var child = Object.create(inheritedProxy);
check(child.field === child && accessLog.join() === 'get', 'prototype get preserves receiver without has trap');
accessLog = []; child.field = 3;
check(receiver === child && accessLog.join() === 'set', 'prototype set preserves receiver without has trap');
handler.get = undefined;
Object.defineProperty(target, 'access', {get: function() { 'use strict'; return this; }, configurable: true});
check(Reflect.get(proxy, 'access', 7) === 7, 'default get preserves receiver');
handler.set = function(t, k, v, r) { gc(); observed = [this, t, k, v]; receiver = r; return true; };
check(Reflect.set(proxy, symbol, 9, null) && receiver === null && observed[0] === handler && observed[1] === target && observed[2] === symbol && observed[3] === 9, 'set trap arguments');
handler.set = function() { return false; };
check(!Reflect.set(proxy, 'a', 1), 'set false result');
check(caught(function() { 'use strict'; proxy.a = 1; }) instanceof TypeError, 'strict assignment rejects false');
var locked = {};
Object.defineProperty(locked, 'value', {value: 3, writable: false, configurable: false});
proxy = new Proxy(locked, {get: function() { return 4; }});
check(caught(function() { return proxy.value; }) instanceof TypeError, 'get invariant');
proxy = new Proxy(locked, {set: function() { return true; }});
check(caught(function() { Reflect.set(proxy, 'value', 4); }) instanceof TypeError, 'set invariant');
check(Reflect.set(proxy, 'value', 3), 'SameValue set permitted');
proxy = new Proxy(locked, {has: function() { return false; }});
check(caught(function() { return 'value' in proxy; }) instanceof TypeError, 'has invariant');
proxy = new Proxy(locked, {deleteProperty: function() { return true; }});
check(caught(function() { return delete proxy.value; }) instanceof TypeError, 'delete invariant');
proxy = new Proxy(locked, {ownKeys: function() { return []; }});
check(caught(function() { Reflect.ownKeys(proxy); }) instanceof TypeError, 'ownKeys cannot omit permanent key');
proxy = new Proxy({}, {ownKeys: function() { return [3]; }});
check(caught(function() { Reflect.ownKeys(proxy); }) instanceof TypeError, 'ownKeys requires property keys');
proxy = new Proxy({}, {ownKeys: function() { return [symbol, 'x']; }});
check(Reflect.ownKeys(proxy)[0] === symbol, 'ownKeys preserves trap order');
target = Object.preventExtensions({});
proxy = new Proxy(target, {isExtensible: function() { return true; }});
check(caught(function() { Reflect.isExtensible(proxy); }) instanceof TypeError, 'extensibility invariant');
proxy = new Proxy({}, {preventExtensions: function() { return true; }});
check(caught(function() { Reflect.preventExtensions(proxy); }) instanceof TypeError, 'preventExtensions invariant');
proxy = new Proxy(target, {getPrototypeOf: function() { return null; }});
check(caught(function() { Reflect.getPrototypeOf(proxy); }) instanceof TypeError, 'prototype invariant');
proxy = new Proxy(target, {setPrototypeOf: function() { return true; }});
check(caught(function() { Reflect.setPrototypeOf(proxy, null); }) instanceof TypeError, 'prototype write invariant');
proxy = new Proxy({}, {getOwnPropertyDescriptor: function() { return {value: 1, configurable: false}; }});
check(caught(function() { Object.getOwnPropertyDescriptor(proxy, 'a'); }) instanceof TypeError, 'descriptor cannot invent permanent property');
proxy = new Proxy(target, {defineProperty: function() { return true; }});
check(caught(function() { Reflect.defineProperty(proxy, 'a', {value: 1}); }) instanceof TypeError, 'define invariant');
proxy = new Proxy(locked, {defineProperty: function(t, k, d) { d.value = 3; gc(); return true; }});
check(caught(function() { Reflect.defineProperty(proxy, 'value', {value: 4}); }) instanceof TypeError, 'trap cannot rewrite original descriptor record');
var marker = {};
proxy = new Proxy({}, {get get() { gc(); throw marker; }});
check(caught(function() { return proxy.a; }) === marker, 'trap getter exception identity');
proxy = new Proxy({}, {get: 3});
check(caught(function() { return proxy.a; }) instanceof TypeError, 'trap must be callable');
var revoke, pair = Proxy.revocable({a: 8}, {get: function(t, k) { revoke(); gc(); return t[k]; }});
revoke = pair.revoke;
check(pair.proxy.a === 8, 'operation retains target during reentrant revoke');
check(caught(function() { return pair.proxy.a; }) instanceof TypeError, 'revoked access throws');
revoke();
pair = Proxy.revocable({}, {});
check(!Object.prototype.hasOwnProperty.call(pair.revoke, 'name'), 'ES2015 revoker is anonymous');
pair.revoke();
check(caught(function() { Reflect.getPrototypeOf(pair.proxy); }) instanceof TypeError, 'revoked prototype throws');
var fn = function(a) { 'use strict'; return [this, a]; };
pair = Proxy.revocable(fn, {});
check(typeof pair.proxy === 'function' && Reflect.apply(pair.proxy, null, [4])[0] === null, 'callable forwarding');
check(Reflect.apply(pair.proxy, 17, [4])[0] === 17, 'callable forwarding preserves primitive receiver');
pair.revoke();
check(typeof pair.proxy === 'function', 'revocation retains callability');
check(caught(function() { pair.proxy(); }) instanceof TypeError, 'revoked call throws');
check(Array.isArray(new Proxy([], {})), 'IsArray follows target');
pair = Proxy.revocable([], {}); pair.revoke();
check(caught(function() { Array.isArray(pair.proxy); }) instanceof TypeError, 'IsArray revoked throws');
function Constructor(v) { this.value = v; }
function Alternate() {}
proxy = new Proxy(Constructor, {});
var instance = Reflect.construct(proxy, [42], Alternate);
check(instance.value === 42 && Object.getPrototypeOf(instance) === Alternate.prototype, 'constructor forwarding preserves newTarget');
handler = {construct: function(t, args, nt) { gc(); observed = [this, t, args, nt]; return marker; }};
proxy = new Proxy(Constructor, handler);
check(Reflect.construct(proxy, [6], Alternate) === marker && observed[0] === handler && observed[1] === Constructor && observed[2][0] === 6 && observed[3] === Alternate, 'construct trap arguments');
handler.construct = function() { return 3; };
check(caught(function() { new proxy(); }) instanceof TypeError, 'construct trap requires object');
var unusedPrototype = Constructor.bind(null);
Object.defineProperty(unusedPrototype, 'prototype', {get: function() { throw marker; }});
check(typeof Reflect.construct(Proxy, [{}, {}], unusedPrototype) === 'object', 'Proxy constructor ignores newTarget prototype');
check(Object.prototype.toString.call(new Proxy(new Proxy([], {}), {})) === '[object Array]', 'array tag follows nested targets');
check(Object.prototype.toString.call(new Proxy(new Date(), {})) === '[object Object]', 'non-array internal slots are not forwarded');
pair = Proxy.revocable([], {}); pair.revoke();
check(caught(function() { Object.prototype.toString.call(pair.proxy); }) instanceof TypeError, 'revoked array tag throws');
accessLog = [];
child = Object.create(new Proxy({}, {
    has: function() { accessLog.push('has'); throw marker; },
    get: function() { accessLog.push('get'); throw marker; }
}));
check(Object.getOwnPropertyDescriptor(child, 'missing') === undefined &&
      !Object.prototype.hasOwnProperty.call(child, 'missing') &&
      !Object.prototype.propertyIsEnumerable.call(child, 'missing') && accessLog.length === 0,
      'own queries do not inspect proxy prototype');
proxy = new Proxy({a: 1}, {has: function() { throw marker; }});
check(Object.prototype.hasOwnProperty.call(proxy, 'a') &&
      Object.prototype.propertyIsEnumerable.call(proxy, 'a'), 'own queries do not use has trap');
accessLog = [];
proxy = new Proxy({a: 5}, {
    ownKeys: function(t) { accessLog.push('keys'); return ['a']; },
    getOwnPropertyDescriptor: function(t, k) { accessLog.push('descriptor'); return Reflect.getOwnPropertyDescriptor(t, k); },
    get: function(t, k) { accessLog.push('get'); return t[k]; },
    has: function() { throw marker; }
});
check(Object.assign({}, proxy).a === 5 && accessLog.join() === 'keys,descriptor,get', 'assign trap order');
function integrityProxy(log) {
    return new Proxy({a: 1}, {
        preventExtensions: function(t) { log.push('prevent'); return Reflect.preventExtensions(t); },
        ownKeys: function(t) { log.push('keys'); return Reflect.ownKeys(t); },
        getOwnPropertyDescriptor: function(t, k) { log.push('descriptor'); return Reflect.getOwnPropertyDescriptor(t, k); },
        defineProperty: function(t, k, d) { log.push('define'); return Reflect.defineProperty(t, k, d); }
    });
}
accessLog = []; proxy = integrityProxy(accessLog); Object.freeze(proxy);
check(accessLog.join() === 'prevent,keys,descriptor,define' && Object.isFrozen(proxy), 'freeze order and state');
accessLog = []; proxy = integrityProxy(accessLog); Object.seal(proxy);
check(accessLog.join() === 'prevent,keys,define' && Object.isSealed(proxy) && !Object.isFrozen(proxy), 'seal order and state');
pair = Proxy.revocable({}, {});
check(pair.revoke.length === 0, 'revoker length is independent of captured state');
check(JSON.stringify(new Proxy([1, 2], {})) === '[1,2]', 'JSON recognizes proxy array');
check(JSON.stringify({a: 1, b: 2}, new Proxy(['b'], {})) === '{"b":2}', 'JSON proxy replacer array');
check([0].concat(new Proxy([1, 2], {})).join() === '0,1,2', 'concat recognizes proxy array');
pair = Proxy.revocable([], {}); pair.revoke();
check(caught(function() { JSON.stringify({}, pair.proxy); }) instanceof TypeError, 'JSON revoked replacer throws');
check(caught(function() { [].concat(pair.proxy); }) instanceof TypeError, 'concat revoked array throws');
function enumeratingPrototype() {
    return new Proxy({}, {
        enumerate: function() {
            var n = 0, names = ['hidden', 'own', 'inherited'];
            return {next: function() { gc(); return n < names.length ?
                {value: names[n++], done: false} : {done: true}; }};
        },
        has: function() { throw marker; },
        ownKeys: function() { throw marker; }
    });
}
child = Object.create(enumeratingPrototype());
Object.defineProperty(child, 'hidden', {value: 1}); child.own = 2;
var enumerated = [], key;
for (key in child) enumerated.push(key);
check(enumerated.join() === 'own,inherited', 'for-in delegates prototype enumeration and shadows names');
var iterator = Reflect.enumerate(child), step;
enumerated = [];
while (!(step = iterator.next()).done) enumerated.push(step.value);
check(enumerated.join() === 'own,inherited', 'Reflect delegates prototype enumeration and shadows names');
check(iterator.next().done, 'delegated enumeration remains finished');
accessLog = [];
target = new Proxy({}, {
    getOwnPropertyDescriptor: function() { accessLog.push('descriptor'); return undefined; },
    isExtensible: function() { throw marker; }
});
proxy = new Proxy(target, {getOwnPropertyDescriptor: function() { return undefined; }});
check(Reflect.getOwnPropertyDescriptor(proxy, 'missing') === undefined &&
      accessLog.join() === 'descriptor', 'absent descriptor skips extensibility query');
target = new Proxy(Object.defineProperty({}, 'a', {value: 1}), {
    isExtensible: function() { throw marker; }
});
proxy = new Proxy(target, {getOwnPropertyDescriptor: function() { return undefined; }});
check(caught(function() { Reflect.getOwnPropertyDescriptor(proxy, 'a'); }) instanceof TypeError,
      'permanent descriptor rejects before extensibility query');
accessLog = [];
target = new Proxy(Object.preventExtensions({}), {
    isExtensible: function() { accessLog.push('extensible'); return false; },
    getPrototypeOf: function(t) { accessLog.push('prototype'); return Object.getPrototypeOf(t); }
});
proxy = new Proxy(target, {setPrototypeOf: function() { return false; }});
check(!Reflect.setPrototypeOf(proxy, null) && accessLog.join() === 'extensible,prototype',
      'ES2015 rejected prototype trap still queries target invariants');
print('ES6-PROXY checks=' + checks + ' failures=0');
