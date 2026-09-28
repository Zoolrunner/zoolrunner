/* Array.from ordering, raw receivers, iterable cleanup and GC. */
var checks=0;
function check(ok,label){++checks;if(!ok)throw Error('FAIL Array.from: '+label);}
function caught(fn){try{fn();}catch(e){return e;}return null;}
var descriptor=Object.getOwnPropertyDescriptor(Array,'from');
check(Array.from.length===1&&Array.from.name==='from'&&!Array.from.hasOwnProperty('prototype'),'metadata');
check(descriptor.writable&&!descriptor.enumerable&&descriptor.configurable,'descriptor');
check(caught(function(){new Array.from([]);}) instanceof TypeError,'nonconstructor');
check(caught(function(){Array.from(null);}) instanceof TypeError,'null input');
check(caught(function(){Array.from(undefined);}) instanceof TypeError,'undefined input');
check(Array.from([,2]).hasOwnProperty('0'),'holes copied');
check(Array.from('a\ud83d\ude00b').length===3,'Unicode iteration');
check(Array.from(Symbol()).length===0,'symbol array-like');
var log=[],items={length:2,0:'a',1:'b'};
Object.defineProperty(items,Symbol.iterator,{get:function(){log.push('method');gc();return null;}});
Object.defineProperty(items,'length',{get:function(){log.push('length');gc();return 2;}});
function C(n){log.push('construct:'+n+':'+arguments.length);this.initial=n;gc();}
var result=Array.from.call(C,items,function(v,k){'use strict';log.push('map:'+k);check(this===7,'raw mapper receiver');gc();return v+k;},7);
check(result instanceof C&&result.initial===2&&result.length===2&&result[1]==='b1','generic array-like construction');
check(log.join()==='method,length,construct:2:1,map:0,map:1','array-like order');
log=[];
check(caught(function(){Array.from(items,{});}) instanceof TypeError&&log.length===0,'mapper validated first');
var marker={},nextCalls=0,closeCalls=0,iter;
function iterable(next,ret){
 var input={};iter={next:next};if(ret!==undefined)iter['return']=ret;
 input[Symbol.iterator]=function(){'use strict';check(this===input,'iterator method receiver');gc();return iter;};
 return input;
}
items=iterable(function(){gc();return {value:++nextCalls,done:nextCalls>2};});
log=[];result=Array.from.call(C,items);
check(result[0]===1&&result[1]===2&&result.length===2&&log.join()==='construct:undefined:0','iterable constructor has no arguments');
items=iterable(function(){return {value:1,done:false};},function(){'use strict';check(this===iter,'return receiver');++closeCalls;gc();throw Error('ignored close');});
check(caught(function(){Array.from(items,function(){gc();throw marker;});})===marker&&closeCalls===1,'mapper failure closes preserving throw');
Object.defineProperty(iter,'return',{get:function(){gc();throw 'get-return';}});
check(caught(function(){Array.from(items,function(){throw marker;});})==='get-return','ES2015 return getter exception precedence');
items=iterable(function(){throw marker;},function(){++closeCalls;});closeCalls=0;
check(caught(function(){Array.from(items);})===marker&&closeCalls===0,'next failure does not close');
items=iterable(function(){return {get done(){gc();throw marker;}};},function(){++closeCalls;});
check(caught(function(){Array.from(items);})===marker&&closeCalls===0,'done failure does not close');
items=iterable(function(){return {done:false,get value(){gc();throw marker;}};},function(){++closeCalls;});
check(caught(function(){Array.from(items);})===marker&&closeCalls===0,'value failure does not close');
items=iterable(function(){return {done:false,value:3};},function(){++closeCalls;gc();return {};});
function Frozen(){Object.preventExtensions(this);}
check(caught(function(){Array.from.call(Frozen,items);}) instanceof TypeError&&closeCalls===1,'property failure closes');
items=iterable(function(){return {done:true};},function(){++closeCalls;});closeCalls=0;
function NoLength(){Object.defineProperty(this,'length',{value:0});}
check(caught(function(){Array.from.call(NoLength,items);}) instanceof TypeError&&closeCalls===0,'final length failure does not close');
var saved=Object.getOwnPropertyDescriptor(Number.prototype,Symbol.iterator),raw;
Object.defineProperty(Number.prototype,Symbol.iterator,{configurable:true,get:function(){'use strict';raw=this;return function(){'use strict';check(this===9,'primitive iterator call');return {next:function(){return {done:true};}};};}});
try{check(Array.from(9).length===0&&raw===9,'primitive iterator getter receiver');}
finally{if(saved)Object.defineProperty(Number.prototype,Symbol.iterator,saved);else delete Number.prototype[Symbol.iterator];}
var calls=0;
Object.defineProperty(Array.prototype,'0',{configurable:true,set:function(){++calls;}});
try{result=Array.from({0:4,length:1});check(result[0]===4&&calls===0,'own definition bypasses inherited setter');}
finally{delete Array.prototype[0];}
check(caught(function(){Array.from.call(null,{length:4294967296});}) instanceof RangeError,'intrinsic array length limit');
function Large(n){check(n===9007199254740991,'ToLength constructor bound');throw marker;}
check(caught(function(){Array.from.call(Large,{length:Infinity});})===marker,'large generic length');
print('ES6-ARRAY-FROM checks='+checks+' failures=0');
