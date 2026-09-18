/* ES2015 RegExp prototype fields, generic flags/stringification and legacy data.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
var checks = 0;
function check(value, message) {
    ++checks;
    if (!value) throw new Error('RegExp fields: ' + message);
}
function caught(callback) {
    try { callback(); } catch (error) { return error; }
    return null;
}
var names = ['source', 'global', 'ignoreCase', 'multiline', 'flags'];
var i, d, method, r = /a/gim, marker = {}, source, recompiled;
for (i = 0; i < names.length; ++i) {
    d = Object.getOwnPropertyDescriptor(RegExp.prototype, names[i]);
    check(typeof d.get === 'function' && d.set === undefined &&
          !('value' in d) && !('writable' in d) && d.configurable && !d.enumerable, names[i] + ' accessor descriptor');
    check(d.get.length === 0 && d.get.name === 'get ' + names[i], names[i] + ' getter metadata');
    check(!Object.prototype.hasOwnProperty.call(r, names[i]), names[i] + ' inherited accessor');
    method = d.get;
    check(caught(function() { new method(); }) instanceof TypeError, names[i] + ' nonconstructor');
    check(caught(function() { method.call(null); }) instanceof TypeError, names[i] + ' null receiver');
}
check(r.source === 'a' && r.global && r.ignoreCase && r.multiline && r.flags === 'gim', 'instance values');
check(caught(function() { return RegExp.prototype.source; }) instanceof TypeError &&
      caught(function() { return RegExp.prototype.global; }) instanceof TypeError &&
      caught(function() { return RegExp.prototype.ignoreCase; }) instanceof TypeError &&
      caught(function() { return RegExp.prototype.multiline; }) instanceof TypeError &&
      caught(function() { return RegExp.prototype.flags; }) instanceof TypeError,
      'ES2015 prototype has no matcher slots');
check(Object.prototype.toString.call(RegExp.prototype) === '[object Object]', 'ordinary prototype tag');
check(caught(function() { RegExp.prototype.exec(''); }) instanceof TypeError, 'prototype cannot execute');
check(!Object.prototype.hasOwnProperty.call(RegExp.prototype, 'lastIndex'), 'prototype has no lastIndex');
d = Object.getOwnPropertyDescriptor(r, 'lastIndex');
check(d.value === 0 && d.writable && !d.configurable && !d.enumerable, 'own lastIndex');
check(r.exec('a')[0] === 'a' && r.lastIndex === 1, 'exec updates own lastIndex');
check(Object.getOwnPropertyDescriptor(r, 'lastIndex').value === 1, 'lastIndex descriptor stays live');
method = Object.getOwnPropertyDescriptor(RegExp.prototype, 'global').get;
check(caught(function() { method.call(new Proxy(r, {})); }) instanceof TypeError, 'proxy does not inherit matcher slot');
check(caught(function() { method.call(Object.create(RegExp.prototype)); }) instanceof TypeError, 'derived ordinary object has no matcher');
var patterns = ['', '/', '\\/', '\\\\/', 'a\nb', 'a\rb', '\u2028', '\u2029', '\ud800', '\u0000'];
for (i = 0; i < patterns.length; ++i) {
    r = new RegExp(patterns[i]); source = r.source;
    check(source.indexOf('\n') < 0 && source.indexOf('\r') < 0 &&
          source.indexOf('\u2028') < 0 && source.indexOf('\u2029') < 0, 'escaped source ' + i);
    recompiled = evaluate(r.toString(), 'regexp-source-roundtrip');
    check(recompiled.source === source, 'source roundtrip ' + i);
}
var log = [], receiver = {}, values = [true, {}, 1, 'x', Symbol()];
for (i = 0; i < 5; ++i) (function(index) {
    Object.defineProperty(receiver, ['global', 'ignoreCase', 'multiline', 'unicode', 'sticky'][index], {
        get: function() { log.push(index); gc(); return values[index]; }
    });
})(i);
method = Object.getOwnPropertyDescriptor(RegExp.prototype, 'flags').get;
check(method.call(receiver) === 'gimuy' && log.join() === '0,1,2,3,4', 'generic flags order and truthiness');
check(method.call({global: NaN, ignoreCase: 0, multiline: null, unicode: undefined, sticky: ''}) === '', 'falsy flags');
log = [];
receiver = {get source() { log.push('source'); return {toString: function() { gc(); log.push('source-string'); return 'x'; }}; },
            get flags() { gc(); log.push('flags'); return {toString: function() { log.push('flags-string'); return 'y'; }}; }};
check(RegExp.prototype.toString.call(receiver) === '/x/y' && log.join() === 'source,source-string,flags,flags-string', 'generic toString order and GC');
check(RegExp.prototype.toString.call({}) === '/undefined/undefined', 'generic missing fields');
check(caught(function() { RegExp.prototype.toString.call(1); }) instanceof TypeError, 'toString rejects primitive');
check(caught(function() { RegExp.prototype.toString.call({get source() { throw marker; }}); }) === marker, 'source exception');
check(caught(function() { RegExp.prototype.toString.call({source: 'x', get flags() { throw marker; }}); }) === marker, 'flags exception');
r = /a/g;
Object.defineProperty(r, 'source', {value: 'custom'});
Object.defineProperty(r, 'flags', {value: 'z'});
check(r.toString() === '/custom/z' && r.exec('a')[0] === 'a', 'overrides affect stringification but not matcher');
function Alternate() {}
Alternate.prototype = {lastIndex: 90};
r = Reflect.construct(RegExp, ['a', 'g'], Alternate);
check(Object.getPrototypeOf(r) === Alternate.prototype && Object.getOwnPropertyDescriptor(r, 'lastIndex').value === 0,
      'alternate prototype retains own lastIndex');
Object.defineProperty(RegExp.prototype, 'lastIndex', {value: 99, configurable: true});
try {
    r = new RegExp('x', 'g');
    check(Object.getOwnPropertyDescriptor(r, 'lastIndex').value === 0 && r.exec('x')[0] === 'x',
          'inherited readonly lastIndex does not block construction');
    r = evaluate('/y/g', 'regexp-inherited-index');
    check(r.lastIndex === 0 && r.exec('y')[0] === 'y', 'literal owns index despite inherited readonly property');
} finally { delete RegExp.prototype.lastIndex; }
print('ES6-REGEXP-FIELDS checks=' + checks + ' failures=0');
