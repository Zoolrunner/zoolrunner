/* Original ES2015 block functions and function-only Annex B.3.3. */
var checks = 0, failures = 0;
function check(label, source, expected) {
    var realm = createTest262Realm(), actual;
    realm.global.gc = gc;
    try { actual = realm.evalScript(source); }
    catch (error) { actual = String(error); }
    ++checks;
    if (actual !== expected) {
        ++failures;
        print('FAIL ' + label + ': ' + actual + ' expected ' + expected);
    }
}
check('script block', 'var result;{result=typeof f;function f(){}}result+","+typeof f', 'function,undefined');
check('eval block', '(function(){eval("{function f(){}}");return typeof f})()', 'undefined');
check('function binding', '(function(){var r=typeof f;{r+=","+typeof f;function f(){}}return r+","+typeof f})()', 'undefined,function,function');
check('unexecuted block', '(function(){if(false){function f(){}}return f})()', undefined);
check('before-declaration assignment', '(function(){{f=23;function f(){}}return f})()', 23);
check('after-declaration assignment', '(function(){{function f(){} f=23;}return typeof f})()', 'function');
check('parameter conflict', '(function(f){{function f(){}}return f})(42)', 42);
check('default parameter conflict', '(function(f=42){{function f(){}}return f})()', 42);
check('destructured parameter conflict', '(function({f}){{function f(){}}return f})({f:42})', 42);
check('rest parameter conflict', '(function(...f){{function f(){}}return f[0]})(42)', 42);
check('later let conflict', '(function(){{function f(){}}let f=42;return f})()', 42);
check('earlier let conflict', '(function(){let f=42;{function f(){}}return f})()', 42);
check('outer block let conflict', '(function(){{{function f(){}}let f=42;}return typeof f})()', 'undefined');
check('sibling let does not conflict', '(function(){{let f=42;}{function f(){return 9}}return f()})()', 9);
check('existing var', '(function(){var f=42;{function f(){return 9}}return f()})()', 9);
check('existing body function', '(function(){function f(){return 42}if(false){function f(){return 9}}return f()})()', 42);
check('implicit arguments before bridge', '(function(){var before=arguments[0];{function arguments(){return 9}}return before+arguments()})(42)', 51);
check('arrow arguments var', '(()=>{var before=arguments;{function arguments(){return 9}}return String(before)+","+arguments()})()', 'undefined,9');
check('generator declaration no bridge', '(function(){{function* f(){}}return typeof f})()', 'undefined');
check('strict function no bridge', '(function(){"use strict";{function f(){}}return typeof f})()', 'undefined');
check('switch entry', '(function(){var inside;switch(0){case 0:inside=typeof f;break;default:function f(){}}return inside+","+typeof f})()', 'function,undefined');
check('switch declaration bridge', '(function(){switch(0){case 0:function f(){return 9}}return f()})()', 9);
check('if arm true', '(function(){if(true)function f(){return 9}return f()})()', 9);
check('if arm false', '(function(){if(false)function f(){}return f})()', undefined);
check('both arms', '(function(){if(false)function f(){return 1}else function f(){return 2}return f()})()', 2);
check('script if has no bridge', 'if(true)function f(){}typeof f', 'undefined');
check('capture lexical versus var', '(function(){var inner;{function f(){return 1}inner=()=>f;f=2;}return inner()+","+f()})()', '2,1');
check('loop binding identities', '(function(){var a=[];for(var i=0;i<2;i++){a.push(f);function f(){}}return a[0]!==a[1]&&f===a[1]})()', true);
check('with does not redirect bridge', '(function(){var o={f:42};with(o){function f(){return 9}}return o.f+f()})()', 51);
check('direct eval reads var', '(function(){{function f(){return 9}}return eval("f()")})()', 9);
check('decompiled source', '(function(){var f=function(){var before=g;{function g(){return 9}}return before===undefined&&g()===9};return eval("("+f.toString()+")")()})()', true);
check('collection and capture', '(function(){var get=()=>f;{function f(){return 9}}gc();return get()()})()', 9);
check('nested function isolated', '(function(){var g=function(){{function f(){}}};return typeof f})()', 'undefined');
check('catch replacement allowed', '(function(){try{throw 42}catch(f){{function f(){return 9}}}return f()})()', 9);
check('labelled declaration hoisted', '(function(){var before=typeof f;label:function f(){return 9}return before+","+f()})()', 'function,9');
check('non-simple implicit arguments', '(function(x=1){var before=arguments[0];{function arguments(){return 9}}return before+arguments()})(42)', 51);
check('generator containing bridge', '(function*(){yield typeof f;{function f(){return 9}}yield f()})().next().value', 'undefined');
check('generator resumed bridge', '(function(){var it=(function*(){yield typeof f;{function f(){return 9}}yield f()})();it.next();return it.next().value})()', 9);
check('nested labelled declaration', '(function(){var before=typeof f;{label:function f(){return 9}}return before+","+f()})()', 'function,9');
check('if labelled declaration early error', '(function(){try{eval("if(false)label:function f(){}")}catch(e){return e instanceof SyntaxError}return false})()', true);
check('catch pattern blocks bridge', '(function(){try{throw {f:42}}catch({f}){{function f(){return 9}}}return typeof f})()', 'undefined');
check('named expression binding', '(function f(){{function f(){return 9}}return f()})()', 9);
check('multiple blocks current value', '(function(){{function f(){return 1}}var first=f;{function f(){return 2}}return first()+f()})()', 3);
check('const outer blocks bridge', '(function(){const f=42;{function f(){return 9}}return f})()', 42);
check('class outer blocks bridge', '(function(){class f{}var original=f;{function f(){}}return f===original})()', true);
check('lexical for initializer blocks bridge', '(function(){for(let f=0;f<1;f++){function f(){}}return typeof f})()', 'undefined');
check('parameter capture ignores bridge', '(function(f,get=()=>f){{function f(){return 9}}return get()})(42)', 42);
check('body var in with', '(function(){var o={f:42};with(o){if(true)function f(){return 9}}return o.f+f()})()', 51);
check('duplicate block declarations reject', '(function(){try{eval("{function f(){}function f(){}}")}catch(e){return e instanceof SyntaxError}return false})()', true);
print('SLOPPY-BLOCK-SEMANTICS checks='+checks+' failures='+failures);
if (failures) throw Error('block function failures');
