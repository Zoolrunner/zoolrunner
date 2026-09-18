/* Prototype mutation must preserve property caches and embedding semantics. */
var checks = 0;
function check(ok, label) { ++checks; if (!ok) throw Error('FAIL prototype mutation: ' + label); }
function throwsType(fn, label) { var caught = false; try { fn(); } catch(e) { caught = e instanceof TypeError; } check(caught, label); }
var method = Object.setPrototypeOf, desc = Object.getOwnPropertyDescriptor(Object, 'setPrototypeOf');
check(typeof method === 'function' && method.length === 2 && method.name === 'setPrototypeOf', 'method metadata');
check(desc.writable && desc.configurable && !desc.enumerable, 'method descriptor');
check(!method.hasOwnProperty('prototype'), 'no constructor prototype');
throwsType(function() { new method({}, null); }, 'not constructible');
throwsType(function() { method(); }, 'missing target');
throwsType(function() { method(null, {}); }, 'null target');
throwsType(function() { method(undefined, {}); }, 'undefined target');
var invalid = [undefined, false, true, 0, 1, '', 'x', NaN];
for (var i = 0; i < invalid.length; ++i) (function(proto) {
    throwsType(function() { method({}, proto); }, 'invalid prototype ' + i);
    throwsType(function() { method(1, proto); }, 'validate prototype before primitive target ' + i);
})(invalid[i]);
var primitives = [1, -0, NaN, true, false, 'x'];
for (i = 0; i < primitives.length; ++i) {
    check(Object.is(method(primitives[i], null), primitives[i]), 'primitive returned ' + i);
}
var first = {value: 1, call: function() { return this.own + 1; }};
var second = {value: 2, call: function() { return this.own + 2; }};
var target = Object.create(first); target.own = 10;
function readValue() { return target.value; }
function callMethod() { return target.call(); }
for (i = 0; i < 50; ++i) { readValue(); callMethod(); }
check(readValue() === 1 && callMethod() === 11, 'initial inherited reads');
check(method(target, second) === target && Object.getPrototypeOf(target) === second, 'prototype and return identity');
check(readValue() === 2 && callMethod() === 12 && target.own === 10, 'cached reads reflect new prototype');
check(!target.hasOwnProperty('value'), 'inherited value stays inherited');
check(method(target, null) === target && Object.getPrototypeOf(target) === null && target.value === undefined, 'null prototype');
check(method(target, first) === target && readValue() === 1, 'restore prototype');
var frozen = Object.freeze(Object.create(first));
check(method(frozen, first) === frozen, 'same frozen prototype allowed');
throwsType(function() { method(frozen, second); }, 'different frozen prototype rejected');
var nonextensible = Object.preventExtensions(Object.create(null));
check(method(nonextensible, null) === nonextensible, 'same null nonextensible prototype');
throwsType(function() { method(nonextensible, {}); }, 'nonextensible change rejected');
throwsType(function() { method(target, target); }, 'direct cycle');
var child = Object.create(target), grandchild = Object.create(child);
throwsType(function() { method(target, grandchild); }, 'indirect cycle');
check(Object.getPrototypeOf(target) === first && Object.getPrototypeOf(child) === target, 'failed cycle leaves chain intact');
var poison = {get __proto__() { throw Error('getter must not run'); }, set __proto__(v) { throw Error('setter must not run'); }, valueOf: function() { throw Error('conversion must not run'); }};
check(method(poison, second) === poison && Object.getPrototypeOf(poison) === second, 'no property access or conversion');
var fn = function() { return 9; }, array = [1, 2];
check(method(fn, second) === fn && fn() === 9 && fn.value === 2, 'function remains callable');
check(method(array, second) === array && array[1] === 2 && array.length === 2 && array.value === 2, 'array retains indexed storage');
var box = new String('ab'); method(box, second); check(box[0] === 'a' && box.length === 2 && box.value === 2, 'string wrapper retains private data');
var regexp = /a/g; regexp.lastIndex = 1; method(regexp, second);
check(RegExp.prototype.exec.call(regexp, 'ba').index === 1 && regexp.lastIndex === 2, 'regexp private index remains live');
regexp.lastIndex = 0;
check(RegExp.prototype.exec.call(regexp, 'a').index === 0 && regexp.lastIndex === 1, 'regexp native setter retained');
/* ES2015 source/global are inherited accessors; replacing the prototype
 * removes their lookup path without changing the internal matcher. */
check(regexp.source === undefined && regexp.global === undefined && regexp.value === 2 &&
      Object.getOwnPropertyDescriptor(RegExp.prototype, 'source').get.call(regexp) === 'a' &&
      Object.getOwnPropertyDescriptor(RegExp.prototype, 'global').get.call(regexp) === true,
      'regexp matcher preserved after removing inherited accessors');
var nullTarget = Object.create(null), nullProto = Object.create(null); nullProto.x = 7;
check(method(nullTarget, nullProto) === nullTarget && nullTarget.x === 7, 'empty scopes and null-origin prototype');
for (i = 0; i < 50; ++i) {
    var transient = {id: i}; method(target, transient); gc();
    check(target.id === i && target.own === 10, 'new prototype rooted ' + i);
}
print('ES6-PROTOTYPE-MUTATION checks=' + checks + ' failures=0');
