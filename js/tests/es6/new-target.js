/* Function new.target, eval boundaries and constructor identity.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
var checks = 0;
function check(value, label) {
    ++checks;
    if (!value) throw Error('FAIL new.target: ' + label);
}
function syntax(source) {
    var failed = false;
    try { (0, eval)(source); } catch (error) { failed = error instanceof SyntaxError; }
    check(failed, 'syntax ' + source);
}
function Target() { gc(); return {target: new.target}; }
check(Target().target === undefined, 'ordinary call');
check(Target.call({}).target === undefined, 'explicit receiver');
check(new Target().target === Target, 'constructor');
function Other() {}
check(Reflect.construct(Target, [], Other).target === Other, 'alternate constructor');
var bound = Target.bind({});
check(new bound().target === Target, 'bound constructor default target');
check(Reflect.construct(bound, [], Other).target === Other, 'bound alternate target');
var proxy = new Proxy(Target, {});
check(new proxy().target === proxy, 'proxy constructor identity');
check(Reflect.construct(proxy, [], Other).target === Other, 'proxy alternate target');
function Direct() { return {target: eval('new.target')}; }
check(Direct().target === undefined, 'direct eval call');
check(new Direct().target === Direct, 'direct eval construct');
function Strict() { 'use strict'; return {target: eval('new.target')}; }
check(Strict().target === undefined, 'strict direct eval call');
check(Reflect.construct(Strict, [], Other).target === Other, 'strict direct eval construct');
function Nested() { return {target: eval('eval("new.target")')}; }
check(new Nested().target === Nested, 'nested direct eval');
function StrictNested() { 'use strict'; return {target: eval('eval("new.target")')}; }
check(new StrictNested().target === StrictNested, 'nested strict eval');
function Inner() { return {target: (function () { return new.target; })()}; }
check(new Inner().target === undefined, 'inner function has independent binding');
function EvalInner() { return {target: eval('(function(){return new.target})()')}; }
check(new EvalInner().target === undefined, 'inner eval function has independent binding');
function StrictEval() { return {target: eval('"use strict";new.target')}; }
check(new StrictEval().target === StrictEval, 'eval directive retains binding');
function Indirect() {
    try { (0, eval)('new.target'); } catch (e) { return {ok: e instanceof SyntaxError}; }
    return {ok:false};
}
check(new Indirect().ok, 'indirect eval cannot inherit binding');
var dynamic = Function('return {target:new.target};');
check(dynamic().target === undefined && new dynamic().target === dynamic, 'Function constructor body');
check(({method(){return new.target;}}).method() === undefined, 'method binding');
check(({get x(){return new.target;}}).x === undefined, 'getter binding');
function With() { with ({target:13}) { return {target:new.target}; } }
check(new With().target === With, 'with cannot shadow binding');
function Suffix() { return {target:new.target.prototype}; }
check(new Suffix().target === Suffix.prototype, 'member suffix');
function Name() { return {target:new /* comment */ .
 target}; }
check(new Name().target === Name, 'whitespace and comments');
var copy = eval('(' + Target.toString() + ')');
check(Target.toString().indexOf('new.target') !== -1 && new copy().target === copy, 'decompilation');
syntax('new.target');
syntax('eval("new.target")');
syntax('function f(){new.other}');
syntax('function f(){new.\\u0074arget}');
syntax('function f(){new.target=1}');
syntax('function f(){++new.target}');
syntax('function f(){new.target++}');
syntax('function f(){[new.target]=[]}');
syntax('function f(){({x:new.target}={})}');
syntax('function f(){for(new.target in {}){}}');
check((function(){return delete new.target;})(), 'delete non-reference');
var invalidConstructor = false;
try { new /z/(); } catch (e) { invalidConstructor = e instanceof TypeError; }
check(invalidConstructor, 'regexp constructor operand remains a runtime error');
var saved = version();
try {
    version(170);
    var rejected = false;
    try { evaluate('function f(){return new.target;}','legacy-new-target'); }
    catch (e) { rejected = e instanceof SyntaxError; }
    check(rejected, 'legacy grammar unchanged');
} finally { version(saved); }
print('ES6-NEW-TARGET checks=' + checks + ' failures=0');
