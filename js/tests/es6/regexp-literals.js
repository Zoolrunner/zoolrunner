/* Each modern evaluation gets independent identity and mutable state. */
var checks = 0;
function check(ok, label) { ++checks; if (!ok) throw Error('FAIL regexp literal: ' + label); }
function fresh() { return /x/g; }
var a = fresh(), b = fresh(), key = Symbol('regexp');
check(a !== b, 'function evaluation identity');
a.lastIndex = 9; a.extra = 1; a[key] = 2;
check(b.lastIndex === 0 && b.extra === undefined && b[key] === undefined, 'independent state');
check(a.source === 'x' && b.global && b.test('x'), 'shared compiled pattern semantics');
var values = [];
for (var i = 0; i < 4; ++i) values.push(/loop/g);
check(values[0] !== values[1] && values[1] !== values[2], 'global loop identity');
values[0].lastIndex = 10;
check(values[1].lastIndex === 0, 'global loop lastIndex');
var evaluated = eval('(function(){var a=[];for(var i=0;i<3;++i)a.push(/eval/);return a;})()');
check(evaluated[0] !== evaluated[1], 'eval loop identity');
var dynamic = Function('return /dynamic/;');
check(dynamic() !== dynamic(), 'Function constructor identity');
var recompiled = eval('(' + fresh.toString() + ')');
check(recompiled() !== recompiled() && recompiled().source === 'x', 'function decompilation');
var saved = RegExp, proto = RegExp.prototype, count = 0;
RegExp = function() { ++count; throw Error('global constructor used'); };
check(Object.getPrototypeOf(fresh()) === proto && count === 0, 'intrinsic prototype');
check((function(RegExp) { return /shadow/; })(null).source === 'shadow', 'lexical constructor shadow');
RegExp = saved;
for (i = 0; i < 20; ++i) { gc(); check(fresh() !== a && fresh().lastIndex === 0, 'GC ' + i); }
for (i = 0; i < 2; ++i) {
    version(i ? 170 : 0);
    var legacy = evaluate('(function(){return /legacy/g;})', 'legacy-regexp');
    version(2015);
    var first = legacy(); first.lastIndex = 5;
    check(legacy() === first && legacy().lastIndex === 5, 'legacy identity ' + i);
}
check(fresh() !== fresh(), 'modern edition restored after legacy call');
print('ES6-REGEXP-LITERALS checks=' + checks + ' failures=0');
