/* ES2015 destructuring defaults. MPL 1.1/GPL 2.0/LGPL 2.1. */
(function(){
var checks=0;
function check(x,s){++checks;if(!x)throw Error(s)}
function rejects(s){var e;try{Function(s)}catch(x){e=x}check(e instanceof SyntaxError,s)}
var x, y, calls=0;
function init(){++calls;gc();return 17}
({x=init()}={});check(x===17&&calls===1,'undefined default');
({x=init()}={x:null});check(x===null&&calls===1,'null no default');
({x=init()}={x:0});check(x===0&&calls===1,'zero no default');
var {x:a=1,y:b=a+1}={};check(a===1&&b===2,'earlier binding');
var {x:{y:z=9}={}}={};check(z===9,'nested default');
var {[Symbol.iterator]:method=function(){}}={};check(method.name==='method','inferred name');
var {fn=function(){}}={};check(fn.name==='fn','shorthand inferred name');
var [entry=8]=[];check(entry===8,'array default');
rejects('({x=3})');rejects('if(false)({x=3})');rejects('function f(){({x=3})}');
rejects('var {x+=3}={}');rejects('var {x: obj.x=3}={}');
function f(o){var {x=3,y:x2=x+1}=o;return [x,x2].join()}
function g(o){var [x=2]=o;return x}
function h(o){var {x:{y=4}={}}=o;return y}
[f,g,h].forEach(function(f){var restored=eval('('+f.toString()+')');check(restored([])===f([]),'roundtrip '+f.name)});
var values=[];for(var {x=3} of [{},{x:4}])values.push(x);check(values.join()==='3,4','for of defaults');
function* gen(o){var {x=yield 3}=o;return x}
var it=gen({});check(it.next().value===3&&it.next(7).value===7,'generator default');
var restore=eval('('+gen.toString()+')');it=restore({});check(it.next().value===3&&it.next(9).value===9,'generator roundtrip');
function parameter({x=3}){return x}
function arrayParameter([x=2]){return x}
[parameter,arrayParameter].forEach(function(f){check(eval('('+f.toString()+')')([])===f([]),'parameter roundtrip')});
function lexical(){let {x=2,y=x+1}={};return y}
check(lexical()===3,'lexical earlier binding');
function tdz(){let {x=x}={};return x}
var error;try{tdz()}catch(e){error=e}check(error instanceof ReferenceError,'lexical TDZ');
(function(){
var calls=0,parts=[];function tick(){++calls;return 0}
for(var n=0;n<6000;++n)parts.push('tick()');parts.push('7');
var f=eval('(function(o){var {x=('+parts.join(',')+')}=o;return x})');
if(f({})!==7||calls!==6000)throw Error('wide default');++checks;
calls=0;if(f({x:9})!==9||calls!==0)throw Error('wide skip');++checks;
var restored=eval('('+f.toString()+')');if(restored({})!==7||calls!==6000)throw Error('wide decompile');++checks;
function lexical(o){let {x=2,y=x+1}=o;return y}
if(eval('('+lexical.toString()+')')({})!==3)throw Error('lexical roundtrip');++checks;
function tdz(){let {x=x}={};return x}
try{tdz();throw Error('missed TDZ')}catch(e){if(!(e instanceof ReferenceError))throw e}++checks;

})();
print('ES6-PATTERN-DEFAULTS PASS checks='+checks);
})();
