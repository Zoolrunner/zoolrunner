/* ES2015 pattern regression. MPL 1.1/GPL 2.0/LGPL 2.1. */
(function(){var checks=0;function check(x,s){++checks;if(!x)throw Error(s)}
function rejects(s){var e;try{Function(s)}catch(x){e=x}check(e instanceof SyntaxError,s)}
var [a,...r]=[1,2,3];check(a===1&&r.join()==='2,3','rest values');
var [,,, ...empty]=[1];check(empty.length===0,'rest after exhaustion');
var [...characters]='a\uD83D\uDE00b';check(characters.length===3&&characters[1].length===2,'string code points');
var [...holes]=Array(2);check(holes.length===2&&holes.hasOwnProperty('0')&&holes.hasOwnProperty('1'),'holes become own values');
var calls=0,done=0,closed=0,iter={next:function(){++calls;gc();return calls<3?{done:false,value:calls}:{done:true}},return:function(){++closed;return {}}};iter[Symbol.iterator]=function(){return this};
var [...all]=iter;check(all.join()==='1,2'&&calls===3&&closed===0,'rest consumes without close');
var obj={};[...obj.items]=[4,5];check(obj.items.join()==='4,5','member rest target');
var first,second;[...[first,second]]=[6,7];check(first===6&&second===7,'nested array assignment');
var length;[...{length}]=[1,2,3];check(length===3,'nested object assignment');
var calls=0;Object.defineProperty(Array.prototype,'0',{configurable:true,set:function(){++calls}});
try{var [...own]='x';check(own[0]==='x'&&own.hasOwnProperty('0')&&calls===0,'rest defines own elements')}
finally{delete Array.prototype[0]}
rejects('var [...x,]=[]');rejects('[...x,y]=[]');rejects('[...x=1]=[]');rejects('var [...[x]]=[]');rejects('var [...obj.x]=[]');
function f(v){var [a,...r]=v;return r.join()}
function g(v,d){[...d.items]=v;return d.items.join()}
function h([...r]){return r.join()}
[f,g,h].forEach(function(f){check(eval('('+f.toString()+')')([1,2,3],{})===f([1,2,3],{}),'rest roundtrip '+f.name)});
var closes=0,source={next:function(){return {done:false,value:undefined}},return:function(){++closes;gc();return {}}};source[Symbol.iterator]=function(){return this};
function* suspended(v){var [a=yield 3]=v;return a}
var iterator=suspended(source);check(iterator.next().value===3&&iterator.return(9).value===9&&closes===1,'generator return closes');
var copy=eval('('+suspended.toString()+')');iterator=copy(source);check(iterator.next().value===3&&iterator.return(8).value===8&&closes===2,'generator source closes');
var marker={},error;source={next:function(){return {get done(){throw marker}}},return:function(){++closes;return {}}};source[Symbol.iterator]=function(){return this};
try{[a]=source}catch(e){error=e}check(error===marker&&closes===2,'done failure does not close');
print('ES6-ARRAY-REST PASS checks='+checks);
})();
