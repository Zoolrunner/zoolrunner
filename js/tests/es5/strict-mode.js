/* Strict-mode environments, receiver values, and early-error regressions. */
var strictChecks = 0;
function strictSame(actual, expected, label) {
    ++strictChecks;
    if (actual !== expected) throw Error(label + ': got ' + actual);
}
function strictThrows(source, kind, label) {
    ++strictChecks;
    try { eval(source); } catch (e) {
        if (e instanceof kind) return;
        throw Error(label + ': wrong exception ' + e);
    }
    throw Error(label + ': missing exception');
}
function strictReceiver() { 'use strict'; return this; }
strictSame(strictReceiver(), undefined, 'unqualified receiver');
strictSame(strictReceiver.call(null), null, 'explicit null');
strictSame(strictReceiver.call(5), 5, 'number receiver');
strictSame(strictReceiver.apply('x', []), 'x', 'string receiver');
strictSame(strictReceiver.bind(false)(), false, 'bound boolean');
strictSame((function () { 'use strict'; return eval('this'); }).call(null), null, 'direct eval receiver');
strictSame((function () { 'use strict'; return eval('this'); }).call(5), 5, 'direct eval primitive');
strictSame((function () { 'use strict'; eval('var localOnly = 1'); return typeof localOnly; })(), 'undefined', 'strict eval variables');
strictSame((function () { eval('var localOnly = 2'); return localOnly; })(), 2, 'ordinary direct eval variables');
strictSame((function () { 'use strict'; return (0, eval)('this'); })(), this, 'indirect eval global');
strictSame((function () { 'use strict'; return eval('function f(){return this;} f()'); })(), undefined, 'eval inherits strictness');
strictSame((function (x) { 'use strict'; x = 9; return arguments[0]; })(1), 1, 'strict arguments snapshot');
strictSame((function (x) { 'use strict'; arguments[0] = 9; return x; })(1), 1, 'strict arguments unmapping');
strictSame((function (x) { x = 9; return arguments[0]; })(1), 9, 'ordinary arguments mapping');
var strictArgs = (function (x) { 'use strict'; return arguments; })(7);
if (typeof gc === 'function') gc();
strictSame(strictArgs[0], 7, 'escaped arguments after collection');
strictSame(strictArgs.length, 1, 'escaped arguments length');
var poison = Object.getOwnPropertyDescriptor(strictArgs, 'callee');
strictSame(poison.configurable, false, 'callee poison permanent');
strictSame(poison.get, poison.set, 'callee shared thrower');
strictSame(Object.getOwnPropertyDescriptor(strictReceiver, 'caller').get, poison.get, 'realm thrower shared');
strictSame(Object.isExtensible(poison.get), false, 'thrower nonextensible');
strictThrows('strictArgs.callee', TypeError, 'strict arguments callee');
strictThrows('strictReceiver.caller', TypeError, 'strict caller');
strictThrows('"use strict"; strictMissingBinding = 1', ReferenceError, 'unresolvable assignment');
strictThrows('"use strict"\nvar public = 1', SyntaxError, 'directive without semicolon');
strictThrows('"use strict"; function f(a,a) {}', SyntaxError, 'duplicate formals');
strictThrows('function f(eval) { "use strict"; }', SyntaxError, 'restricted formal');
strictThrows('"use strict"; with ({}) {}', SyntaxError, 'with early error');
strictThrows('"use strict"; delete missingName', SyntaxError, 'delete binding early error');
strictThrows('"use strict"; 010', SyntaxError, 'octal number');
strictThrows('"\\1"; "use strict";', SyntaxError, 'earlier octal directive');
strictSame(eval('"use\\x20strict"; (function(){ return this; })()'), this, 'escaped directive remains ordinary');
strictThrows('"use strict"; ({a:1,"a":2})', SyntaxError, 'duplicate strict data properties');
strictThrows('({1:1, get "1"(){return 2;}})', SyntaxError, 'numeric accessor collision');
strictSame(eval('({get a(){return 1;},set a(v){}}).a'), 1, 'accessor pair accepted');
var strictReadonly = Object.defineProperty({}, 'x', {value: 1});
strictThrows('"use strict"; strictReadonly.x = 2', TypeError, 'readonly assignment');
strictThrows('"use strict"; delete strictReadonly.x', TypeError, 'permanent deletion');
var strictGetter = {get x() { return 3; }};
strictThrows('"use strict"; strictGetter.x *= 2', TypeError, 'getter-only compound assignment');
strictSame(strictGetter.x, 3, 'getter unaffected');
Object.defineProperty(Number.prototype, 'strictProbe', {get: strictReceiver, configurable: true});
strictSame((5).strictProbe, 5, 'primitive getter');
strictSame((5)['strictProbe'], 5, 'computed primitive getter');
delete Number.prototype.strictProbe;
var callbackThis;
[1].forEach(function () { 'use strict'; callbackThis = this; });
strictSame(callbackThis, undefined, 'array default callback receiver');
[1].forEach(function () { 'use strict'; callbackThis = this; }, 5);
strictSame(callbackThis, 5, 'array primitive callback receiver');
'x'.replace(/x/, function () { 'use strict'; callbackThis = this; return 'y'; });
strictSame(callbackThis, undefined, 'replace callback receiver');
[2,1].sort(function () { 'use strict'; callbackThis = this; return 0; });
strictSame(callbackThis, undefined, 'sort callback receiver');
strictSame(Object.prototype.toString.call(undefined), '[object Undefined]', 'undefined class');
strictSame(Object.prototype.toString.call(null), '[object Null]', 'null class');
strictSame(eval('("use strict"); (function(){return this;})()'), this, 'parenthesized string is not a directive');
Number.prototype.strictMethodProbe = strictReceiver;
strictSame((5).strictMethodProbe(), 5, 'primitive method receiver');
strictSame((5)['strictMethodProbe'](), 5, 'computed primitive method receiver');
delete Number.prototype.strictMethodProbe;
Object.defineProperty(Number.prototype, 'strictSetterProbe', {
    set: function (v) { 'use strict'; callbackThis = this; }, configurable: true
});
(5).strictSetterProbe = 1;
strictSame(callbackThis, 5, 'primitive setter receiver');
(6)['strictSetterProbe'] = 1;
strictSame(callbackThis, 6, 'computed primitive setter receiver');
delete Number.prototype.strictSetterProbe;
strictThrows('"use strict"; (5).absentProperty = 1', TypeError, 'primitive cannot gain a property');
strictSame(eval('(' + strictReceiver.toString() + ')')(), undefined, 'decompiled strict function');
strictSame(eval('(' + (function () {'use strict'; return function () {return this;};})().toString() + ')')(), undefined, 'decompiled inherited strictness');
strictSame(eval('("first"); "use strict"; (function(){return this;})()'), this, 'parenthesized expression ends directive prologue');
print('ES5-STRICT-MODE checks=' + strictChecks + ' failures=0');
