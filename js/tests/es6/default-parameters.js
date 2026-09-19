/* ES2015 parameter environments and historical function interoperability.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
var checks=0,failures=0;
function test(name,source){++checks;try{if(eval(source)!==true)throw Error('wrong result')}catch(e){++failures;print('FAIL '+name+': '+e)}}
function syntax(name,source){++checks;try{eval(source)}catch(e){if(e instanceof SyntaxError)return;++failures;print('FAIL '+name+': '+e);return}++failures;print('FAIL '+name+': accepted')}
function reference(name,source){++checks;try{eval(source)}catch(e){if(e instanceof ReferenceError)return;++failures;print('FAIL '+name+': '+e);return}++failures;print('FAIL '+name+': accepted')}
test('basic','(function(a=3,b=a+2){return a===3&&b===5})()');
test('length','(function(a,b=2,c){}).length===1');
test('supplied null','(function(a=3){return a===null})(null)');
test('order','(function(){var n=0;return (function(a=++n,b=++n){return a===1&&b===2&&n===2})()})()');
test('unmapped arguments','(function(a=3){a=8;return arguments[0]===2})(2)');
test('arguments mutation','(function(a=3){arguments[0]=8;return a===2})(2)');
test('arguments reassignment','(function(a=3){arguments=7;return arguments===7})()');
test('arguments formal','(function(arguments=7){return arguments===7})()');
test('arguments body var','(function(a=arguments){var arguments=2;return a.length===0&&arguments===2})()');
test('arguments closure','(function(a=()=>arguments){var arguments=2;return a().length===0&&arguments===2})()');
test('body shadow','(function(a=1,b=()=>a){var a=2;return a===2&&b()===1})()');
test('body uninitialized shadow','(function(a=1,b=()=>a){var a;return a===1&&b()===1})()');
test('pattern body shadow','(function([a]=[1],b=()=>a){var a=2;return a===2&&b()===1})()');
test('function body shadow','(function(a=1,b=()=>a){function a(){}return typeof a==="function"&&b()===1})()');
test('pattern function shadow','(function([a]=[1],b=()=>a){function a(){}return typeof a==="function"&&b()===1})()');
test('closure stores','(function(a=1,set=()=>a=9){set();return a===9})()');
test('after gc','(function(){var f=(function(a=7,b=()=>a){return b})();gc();return f()===7})()');
test('eval var isolation','(function(a=eval("var p=3;p"),b=typeof p){return a===3&&b==="undefined"&&typeof p==="undefined"})()');
test('eval retained vars','(function(a=eval("var p=3;()=>p")){gc();return a()===3&&typeof p==="undefined"})()');
test('eval reads params','(function(a=3,b=eval("a+1")){return b===4})()');
test('eval writes params','(function(a=3,b=eval("a=5")){return a===5&&b===5})()');
test('computed eval','(function({[eval("var p=3;\\\"x\\\"")]:a},b=typeof p){return a===7&&b==="undefined"&&typeof p==="undefined"})({x:7})');
test('array defaults','(function([a,b=4]=[3]){return a===3&&b===4})()');
test('object defaults','(function({a,b=4}={a:3}){return a===3&&b===4})()');
test('rest after default','(function(a=1,...b){return a===1&&b.join() ==="8,9"})(undefined,8,9)');
test('rest pattern','(function(...[a,b=4]){return a===3&&b===4})(3)');
test('nested rest pattern','(function(...[...[a]]){return a===3})(3)');
test('arrow default','((a=3,b=a+1)=>a===3&&b===4)()');
test('arrow object','(({a=3}={})=>a===3)()');
test('arrow arguments','(function(){return ((a=arguments[0])=>a===7)()})(7)');
test('arrow this','(function(){return ((a=this)=>a===this)()}).call({})');
test('arrow target','(function(){function C(){return (a=new.target)=>a}return new C()()===C})()');
test('ordinary target','(function(){function C(a=new.target){this.a=a}return new C().a===C})()');
test('strict this','(function(){"use strict";return (function(a=this){return a===undefined})()})()');
test('super property','(function(){var p={x:7},o={m(a=super.x){return a}};Object.setPrototypeOf(o,p);return o.m()===7})()');
test('super call','(function(){class B{constructor(){this.x=7}}class D extends B{constructor(a=super()){}}return new D().x===7})()');
test('generator before next','(function(){var n=0;function* g(a=++n){yield a}var i=g();return n===1&&i.next().value===1})()');
test('generator failure before next','(function(){function* g(a=missingParamName){}try{g()}catch(e){return e instanceof ReferenceError}return false})()');
test('generator dynamic','(function(){var G=(function*(){}).constructor,g=G("a=3","yield a");return g().next().value===3})()');
test('constructor dynamic','Function("a=3","b=a+2","return a===3&&b===5")()');
test('dynamic source','(function(){var f=Function("a=3","b=a+2","return a+b"),g=eval("("+f+")");return g()===8})()');
test('ordinary source','(function(){var f=function(a=3,b=()=>a){return b()},g=eval("("+f+")");return g()===3})()');
test('arrow source','(function(){var f=({a=3}={})=>a,g=eval("("+f+")");return g()===3})()');
test('multiline source','(function(){var f=eval("(function(a=/*one\\n two*/3){return a})");return eval("("+f+")")()===3})()');
test('iterator close on default throw','(function(){var n=0,i={[Symbol.iterator](){return this},next(){return {value:undefined,done:false}},return(){n++;return {}}};try{(function([a=missingParamName]){})(i)}catch(e){return e instanceof ReferenceError&&n===1}return false})()');
test('iterator close on completion','(function(){var n=0,i={[Symbol.iterator](){return this},next(){return {value:7,done:false}},return(){n++;return {}}};return (function([a]){return a})(i)===7&&n===1})()');
reference('later param','(function(a=b,b=3){})()');
reference('own param','(function(a=a){})()');
reference('arguments TDZ','(function(a=arguments,arguments=3){})()');
syntax('duplicate default','(function(a=3,a){})');
syntax('duplicate pattern','(function([a,a]){})');
syntax('duplicate mixed','(function(a,{a}){})');
syntax('duplicate arrow','(a=3,a)=>0');
syntax('strict directive','(function(a=3){"use strict";})');
syntax('arrow strict directive','(a=3)=>{"use strict";}');
syntax('yield default','(function* (a=yield){})');
syntax('dynamic delimiter','Function("a) { return 1; } //","return 2")');
syntax('dynamic comment','Function("a/*", "*/ return 2")');
syntax('body lexical collision','(function(a=3){let a})');
syntax('pattern lexical collision','(function([a]){let a})');
print('DEFAULT-PARAMETERS checks='+checks+' failures='+failures);

if(failures)throw Error("default parameter regressions");
