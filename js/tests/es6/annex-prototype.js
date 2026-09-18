/* ES2015 Annex B __proto__ accessors. MPL 1.1/GPL 2.0/LGPL 2.1. */
var checks = 0;
function check(value, message) {
    ++checks;
    if (!value) throw new Error('Annex prototype: ' + message);
}
function caught(callback) {
    try { callback(); } catch (error) { return error; }
    return null;
}
var desc = Object.getOwnPropertyDescriptor(Object.prototype, '__proto__');
var get = desc.get, set = desc.set, object = {}, proto = {}, marker = {};
check(typeof get === 'function' && typeof set === 'function', 'accessor descriptor');
check(desc.configurable && !desc.enumerable && !('value' in desc) && !('writable' in desc), 'descriptor attributes');
check(get.name === 'get __proto__' && get.length === 0, 'getter metadata');
check(set.name === 'set __proto__' && set.length === 1, 'setter metadata');
check(caught(function() { new get(); }) instanceof TypeError, 'getter cannot construct');
check(caught(function() { new set(); }) instanceof TypeError, 'setter cannot construct');
check(get.call(object) === Object.prototype, 'object prototype');
check(get.call(7) === Number.prototype, 'number receiver');
check(get.call('x') === String.prototype, 'string receiver');
check(get.call(true) === Boolean.prototype, 'boolean receiver');
check(get.call(Symbol()) === Symbol.prototype, 'symbol receiver');
check(caught(function() { get.call(null); }) instanceof TypeError, 'null getter receiver');
check(caught(function() { get.call(undefined); }) instanceof TypeError, 'undefined getter receiver');
check(caught(function() { set.call(null, 1); }) instanceof TypeError, 'null setter before invalid prototype');
check(caught(function() { set.call(undefined, {}); }) instanceof TypeError, 'undefined setter receiver');
check(set.call(object, proto) === undefined && get.call(object) === proto, 'set object prototype');
check(set.call(object, null) === undefined && get.call(object) === null, 'set null prototype');
check(set.call(object, 1) === undefined && get.call(object) === null, 'ignore numeric prototype');
check(set.call(object, Symbol()) === undefined && get.call(object) === null, 'ignore symbol prototype');
check(set.call(3, {}) === undefined && set.call('x', null) === undefined, 'ignore primitive receiver');
object = Object.preventExtensions({});
check(set.call(object, Object.prototype) === undefined, 'same prototype on nonextensible receiver');
check(caught(function() { set.call(object, {}); }) instanceof TypeError, 'reject changing nonextensible receiver');
object = {};
check(caught(function() { set.call(object, object); }) instanceof TypeError, 'reject prototype cycle');
var log = [], proxy = new Proxy({}, {
    getPrototypeOf: function() { gc(); log.push('get'); return proto; },
    setPrototypeOf: function(t, p) { gc(); log.push(p); return true; }
});
check(proxy.__proto__ === proto && log.join() === 'get', 'inherited getter preserves proxy receiver');
log = []; proxy.__proto__ = proto;
check(log.length === 1 && log[0] === proto, 'inherited setter preserves proxy receiver');
proxy = new Proxy({}, {setPrototypeOf: function() { return false; }});
check(caught(function() { set.call(proxy, proto); }) instanceof TypeError, 'false proxy setter throws');
proxy = new Proxy({}, {getPrototypeOf: function() { throw marker; }});
check(caught(function() { get.call(proxy); }) === marker, 'getter trap exception');
proxy = new Proxy({}, {setPrototypeOf: function() { throw marker; }});
check(caught(function() { set.call(proxy, proto); }) === marker, 'setter trap exception');
var revoked = Proxy.revocable({}, {}); revoked.revoke();
check(caught(function() { get.call(revoked.proxy); }) instanceof TypeError, 'revoked getter');
check(set.call(revoked.proxy, 1) === undefined, 'invalid prototype does not inspect revoked proxy');
check(caught(function() { set.call(revoked.proxy, null); }) instanceof TypeError, 'revoked setter');
var savedEdition = version();
try {
    version(170);
    check(evaluate("({}).hasOwnProperty('__proto__')", 'annex-legacy-owner'), 'legacy virtual ownership in modern global');
} finally { version(savedEdition); }
print('ES6-ANNEX-PROTOTYPE checks=' + checks + ' failures=0');
