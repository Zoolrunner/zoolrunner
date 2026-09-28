/* Run with -E: native and interpreted metadata in an ES2015 global. */
var checks = 0;
function check(value, message) {
    checks++;
    if (!value) throw Error('FAIL function metadata: ' + message);
}
function metadata(fn, key, value) {
    var d = Object.getOwnPropertyDescriptor(fn, key);
    check(d && d.value === value && !d.writable && !d.enumerable &&
          d.configurable, key + ' descriptor');
}
function target(a, b, c) { return this.offset + a + b + c; }
metadata(target, 'length', 3);
metadata(target, 'name', 'target');
metadata(Number, 'length', 1);
metadata(Math.abs, 'name', 'abs');
metadata(Function.prototype, 'length', 0);
metadata(Function.prototype, 'name', '');
var bound = target.bind({offset: 10}, 1);
metadata(bound, 'length', 2);
metadata(bound, 'name', 'bound target');
check(bound(2, 3) === 16, 'bound receiver and arguments');
metadata(bound.bind(null, 2), 'name', 'bound bound target');
check(delete target.length && !target.hasOwnProperty('length') &&
      target.length === 0, 'deleted length inherits prototype data');
check(delete target.name && !target.hasOwnProperty('name') &&
      target.name === '', 'deleted name inherits prototype data');
metadata(target.bind(null), 'length', 0);
metadata(target.bind(null), 'name', 'bound ');
var sequence = '';
Object.defineProperty(target, 'length', {configurable: true, get: function () {
    sequence += 'L'; gc(); return 5.9;
}});
Object.defineProperty(target, 'name', {configurable: true, get: function () {
    sequence += 'N'; gc(); return 'renamed';
}});
bound = target.bind(null, 1, 2);
check(sequence === 'LN', 'metadata getter order and collection');
metadata(bound, 'length', 3);
metadata(bound, 'name', 'bound renamed');
var lengths = [undefined, null, true, '4', NaN, -3, -Infinity, Infinity,
               {valueOf: function () { throw Error('must not coerce'); }}];
for (var i = 0; i < lengths.length; i++) {
    Object.defineProperty(target, 'length', {value: lengths[i], configurable: true});
    metadata(target.bind(null, 1), 'length', lengths[i] === Infinity ? Infinity : 0);
}
Object.defineProperty(target, 'name', {value: {}, configurable: true});
metadata(target.bind(null), 'name', 'bound ');
var sentinel = {};
Object.defineProperty(target, 'length', {get: function () { throw sentinel; }});
var caught;
try { target.bind(null); } catch (error) { caught = error; }
check(caught === sentinel, 'length getter exception');
function Constructor(value) { this.value = value; }
var BoundConstructor = Constructor.bind(null, 42);
var instance = new BoundConstructor();
check(instance instanceof Constructor && instance.value === 42, 'bound construction');
var closures = [];
function factory(n) { return function named(a) { return n + a; }; }
for (i = 0; i < 5; i++) closures.push(factory(i));
gc();
for (i = 0; i < closures.length; i++) {
    metadata(closures[i], 'length', 1);
    metadata(closures[i], 'name', 'named');
    check(closures[i](10) === i + 10, 'cloned closure');
    check(Object.getPrototypeOf(closures[i].prototype) === Object.prototype,
          'constructor prototype does not inherit compiler template');
    check(delete closures[i].name && closures[i].name === '', 'no template name');
}
var strictFunction = function () { 'use strict'; };
var restricted = ['caller', 'arguments'];
for (i = 0; i < restricted.length; i++) {
    var key = restricted[i];
    var descriptor = Object.getOwnPropertyDescriptor(Function.prototype, key);
    check(descriptor && typeof descriptor.get === 'function' &&
          descriptor.get === descriptor.set && descriptor.configurable &&
          !descriptor.enumerable, 'prototype restricted accessor ' + key);
    check(!strictFunction.hasOwnProperty(key), 'strict function inherits ' + key);
    check(!bound.hasOwnProperty(key), 'bound function inherits ' + key);
    caught = null;
    try { strictFunction[key]; } catch (error) { caught = error; }
    check(caught instanceof TypeError, 'strict restricted read ' + key);
    check(!Object.isExtensible(descriptor.get), 'thrower is non-extensible');
    check(Object.getOwnPropertyDescriptor(descriptor.get, 'length').configurable === false,
          'thrower length is permanent');
}
version(170);
evaluate("function legacyMetadata(a){return legacyMetadata.arguments[0];}" +
         "function legacyStrict(a){'use strict';return a;}" +
         "var legacyBound=legacyMetadata.bind(null,42);");
version(2015);
check(legacyMetadata(42) === 42, 'legacy arguments in modern global');
check(legacyMetadata.length === 1 && legacyMetadata.name === 'legacyMetadata',
      'legacy metadata in modern global');
check(!Object.getOwnPropertyDescriptor(legacyMetadata, 'length').configurable,
      'legacy length stays permanent');
check(legacyBound() === 42 && legacyBound.length === 0,
      'legacy bound function in modern global');
caught = null;
try { legacyStrict.arguments; } catch (error) { caught = error; }
check(caught instanceof TypeError, 'legacy strict arguments remain restricted');
metadata(legacyMetadata.bind(null), 'length', 1);
metadata(legacyMetadata.bind(null), 'name', 'bound legacyMetadata');
function regexpMetadata(value) {
    var local = 42;
    return typeof /first/ === 'object' && /second/.test('second') &&
           eval('local') === 42 && value === 17;
}
check(regexpMetadata(17), 'regexp slots and direct eval locals');
gc();
check(regexpMetadata(17), 'regexp slots survive collection');
metadata(regexpMetadata, 'length', 1);
metadata(regexpMetadata, 'name', 'regexpMetadata');
check(evaluate('(' + regexpMetadata.toString() + ')(17)'),
      'regexp and eval decompilation');
function nestedEvalFactory(value) {
    return function nestedEval(argument) {
        var local = value + argument;
        return eval('local');
    };
}
check(nestedEvalFactory(20)(22) === 42, 'cloned activation local bindings');
function nonStrictCaller() { return nonStrictCaller.caller; }
function outerCaller() { return nonStrictCaller(); }
check(outerCaller() === outerCaller, 'non-strict caller extension');
check(!Object.prototype.hasOwnProperty.call((function () {}), 'name'),
      'uninferred anonymous function has no own name');
Object.defineProperty(regexpMetadata, 'length', {value: -0});
check(1 / regexpMetadata.bind(null).length === Infinity,
      'bound negative zero length becomes positive zero');
print('ES6-FUNCTION-METADATA checks=' + checks + ' failures=0');
