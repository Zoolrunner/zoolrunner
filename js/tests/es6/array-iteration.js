/* ES2015 callback array methods, ToLength, species and reentrancy.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
var checks=0;
function check(v,name){++checks;if(!v)throw Error('Array iteration: '+name);}
function caught(f){try{f();}catch(e){return e;}return null;}
var names=['forEach','map','filter','some','every','reduce','reduceRight'],i,n,calls,obj,result,marker={},log;
for(i=0;i<names.length;i++){
 n=names[i];calls=0;obj={0:'x',length:-1};
 result=Array.prototype[n].call(obj,function(){++calls;return true;},'initial');
 check(calls===0,'negative ToLength '+n);
 check(caught((function(name){return function(){Array.prototype[name].call(null,function(){});};})(n)) instanceof TypeError,'null receiver '+n);
}
check([1,2,3].map(function(v){gc();return v*2;}).join()==='2,4,6','map values');
check([1,,3].filter(function(v){gc();return v>1;}).join()==='3','filter holes');
check([1,2].reduce(function(a,v){gc();return a+v;},0)===3&&[1,2].reduceRight(function(a,v){return a-v;})===1,'reduce directions');
check(caught(function(){[].reduce(function(){});}) instanceof TypeError&&[].reduce(function(){},undefined)===undefined,'reduce initial presence');
check(caught(function(){Array.prototype.map.call({length:4294967296},function(){});}) instanceof RangeError,'default map length bound');
obj={length:4294967297,0:'x'};calls=0;
check(Array.prototype.some.call(obj,function(v,k,o){++calls;return v==='x'&&k===0&&o===obj;})&&calls===1,'some length does not wrap');
check(!Array.prototype.every.call(obj,function(){return false;}),'every early stop at large length');
obj={length:4294967297,4294967296:7,4294967295:5};
check(caught(function(){Array.prototype.reduceRight.call(obj,function(a,v,k,o){check(a===7&&v===5&&k===4294967295&&o===obj,'large reduceRight arguments');gc();throw marker;});})===marker,'large reduceRight index');
var a=[1,,3],output={};a.constructor={};a.constructor[Symbol.species]=function(len){check(len===3,'map species length');return output;};
result=a.map(function(v,k,o){'use strict';check(this===undefined&&o===a,'map raw callback');gc();return v+k;});
check(result===output&&result[0]===1&&!(1 in result)&&result[2]===5&&!result.hasOwnProperty('length'),'map custom species without final length Set');
output={};a.constructor[Symbol.species]=function(len){check(len===0,'filter species length');return output;};
result=a.filter(function(v){gc();return true;});check(result===output&&result[0]===1&&result[1]===3&&!result.hasOwnProperty('length'),'filter custom species');
log=[];a=[1,2];Object.defineProperty(a,'constructor',{get:function(){log.push('constructor');return {[Symbol.species]:function(){log.push('species-call');return {};}};}});
a.map(function(v){log.push('callback');return v;});check(log.join()==='constructor,species-call,callback,callback','species before callbacks');
a=[1];Object.defineProperty(a,'constructor',{get:function(){throw marker;}});
check(a.some(function(){return true;})&&a.every(function(){return true;})&&a.reduce(function(){})===1,'noncreating methods skip species');
check(caught(function(){a.map(3);}) instanceof TypeError,'callback validation before species');
a=[1,,3];calls=[];result=a.map(function(v,k){calls.push(k);if(k===0){a[1]=2;delete a[2];a.push(4);}return v;});
check(calls.join()==='0,1'&&result.length===3&&result[1]===2&&!(2 in result),'snapshot length and live property checks');
a=[{x:1}];var original=a[0];result=a.filter(function(){a[0]={x:2};gc();return true;});check(result[0]===original,'filter preserves value before callback mutation');
var thisArg={};check([1].map(function(){'use strict';return this===thisArg;},thisArg)[0],'explicit callback receiver');
check([1].reduce(function(){'use strict';return this===undefined;},0),'reduce callback undefined receiver');
log=[];obj=new Proxy({0:1,length:2},{get:function(t,k){log.push('get:'+k);gc();return t[k];},has:function(t,k){log.push('has:'+k);gc();return k in t;}});
result=Array.prototype.map.call(obj,function(v){log.push('callback');return v;});check(log.join()==='get:length,has:0,get:0,callback,has:1'&&result.length===2&&!(1 in result),'Proxy access order');
print('ES6-ARRAY-ITERATION checks='+checks+' failures=0');
