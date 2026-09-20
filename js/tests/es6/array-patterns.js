/* ES2015 pattern regression. MPL 1.1/GPL 2.0/LGPL 2.1. */
(function(){
var checks=0;function check(x,s){++checks;if(!x)throw Error(s)}
function throws(kind,fn,s){var e;try{fn()}catch(x){e=x}check(e instanceof kind,s)}
function iterable(next,close){var x={next:next,return:close||function(){return {}}};x[Symbol.iterator]=function(){return this};return x}
var steps=0,closes=0,values=0;
var i=iterable(function(){++steps;return {done:false,get value(){++values;gc();return 9}}},function(){++closes;return {}});
[]=i;check(steps===0&&closes===1,'empty closes without next');
closes=0;[,]=i;check(steps===1&&values===0&&closes===1,'elision does not read value');
closes=0;var [x]=i;check(x===9&&steps===2&&values===1&&closes===1,'value and early close');
steps=closes=0;i=iterable(function(){++steps;return {done:true,get value(){throw Error('value on done')}}},function(){++closes;return {}});
var [a=1,b=2,c=3]=i;check(a+b+c===6&&steps===1&&closes===0,'exhaustion retained');
throws(TypeError,function(){[]=3},'requires iterator');
var marker={},caught;steps=closes=0;i=iterable(function(){throw marker},function(){++closes;return {}});
try{[a]=i}catch(e){caught=e}check(caught===marker&&closes===0,'next failure does not close');
i=iterable(function(){return {done:false,get value(){throw marker}}},function(){++closes;return {}});caught=null;
try{[a]=i}catch(e){caught=e}check(caught===marker&&closes===0,'value failure does not close');
i=iterable(function(){++steps;return {done:false,value:undefined}},function(){++closes;return {}});caught=null;
try{[a=(function(){gc();throw marker})()]=i}catch(e){caught=e}check(caught===marker&&closes===1,'default failure closes');
steps=closes=0;i=iterable(function(){++steps;return {done:false,value:1}},function(){++closes;return {}});
throws(TypeError,function(){[null.x]=i},'target failure');check(steps===0&&closes===1,'target before next, closes');
var events=[],target={};i=iterable(function(){events.push('next');return {done:false,value:4}},function(){events.push('close');return {}});
function dest(){events.push('target');return target}[dest().x]=i;
check(events.join()==='target,next,close'&&target.x===4,'target order');
var out=[];for(const [n] of [[1],[2]])out.push(n);check(out.join()==='1,2','const for-of binding');
var nextGets=0,nextCalls=0,patternIterator={};
Object.defineProperty(patternIterator,'next',{get:function(){++nextGets;var method=function(){++nextCalls;return {done:nextCalls>2,value:nextCalls}};gc();return method}});
patternIterator[Symbol.iterator]=function(){return this};
var [patternFirst,...patternRest]=patternIterator;
check(patternFirst===1&&patternRest.length===1&&patternRest[0]===2&&nextGets===3&&nextCalls===3,'pattern reads next method for each step');
function f(v){var [a,,b=3]=v;return a+b}
function g(v,d){[d.x,d.y=4]=v;return d.x+d.y}
function h(v){var [[a],{x:b=4}]=v;return a+b}
check(eval('('+f.toString()+')')([1])===4,'elision roundtrip');
check(eval('('+g.toString()+')')([1],{})===5,'reference roundtrip');
check(eval('('+h.toString()+')')([[2],{}])===6,'nested roundtrip');
var old=Array.prototype[Symbol.iterator],calls=0;
try{Array.prototype[Symbol.iterator]=function(){++calls;return old.call(this)};var [z]=[8];check(z===8&&calls===1,'literal observes iterator')}
finally{Array.prototype[Symbol.iterator]=old}
var previous=version();try{version(170);check(evaluate('(function(){var [a]={0:7};return a})()','legacy-pattern')===7,'legacy indexed pattern')}
finally{version(previous)}
print('ES6-ARRAY-PATTERNS PASS checks='+checks);
})();
