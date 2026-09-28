/* ES2015 concat, species, spreadability and callback GC.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
var checks=0;
function check(v,name){++checks;if(!v)throw Error('Array concat: '+name);}
function caught(f){try{f();}catch(e){return e;}return null;}
var concat=Array.prototype.concat, d=Object.getOwnPropertyDescriptor(Array,Symbol.species),a,b,log,marker={};
check(d.configurable&&!d.enumerable&&d.set===undefined&&d.get.name==='get [Symbol.species]'&&d.get.length===0,'species metadata');
check(d.get.call(marker)===marker&&d.get.call(null)===null&&Array[Symbol.species]===Array,'species raw receiver');
check(!d.get.hasOwnProperty('prototype')&&caught(function(){new d.get();}) instanceof TypeError,'species nonconstructor');
check(caught(function(){concat.call(null);}) instanceof TypeError,'null receiver');
check([1].concat([2],3).join()==='1,2,3','ordinary concat');
a=[,2];b=a.concat([,4]);check(b.length===4&&!(0 in b)&&!(2 in b)&&b[1]===2&&b[3]===4,'holes preserved');
a=[1];a[Symbol.isConcatSpreadable]=false;check([].concat(a)[0]===a,'array opt out');
a={0:'a',2:'c',length:3};a[Symbol.isConcatSpreadable]=true;b=[].concat(a);check(b.length===3&&b[0]==='a'&&!(1 in b)&&b[2]==='c','array-like opt in');
a.length=-1;check([].concat(a).length===0,'negative length clamps');
a.length=1.9;check([].concat(a).join()==='a','fractional length floors');
a.length=Infinity;check(caught(function(){[1].concat(a);}) instanceof TypeError,'safe integer bound before iteration');
a={constructor:{get prototype(){throw marker;}}};check(concat.call(a)[0]===a,'nonarray skips constructor');
a=[];a.constructor=null;check(caught(function(){a.concat();}) instanceof TypeError,'null constructor rejected');
a.constructor={};a.constructor[Symbol.species]=3;check(caught(function(){a.concat();}) instanceof TypeError,'nonconstructor species rejected');
a.constructor[Symbol.species]=null;check(Object.getPrototypeOf(a.concat())===Array.prototype,'null species defaults');
log=[];var output={};a=[1,2];var c={};Object.defineProperty(c,Symbol.species,{get:function(){log.push('species');return function(length){check(length===0,'species initial length');gc();log.push('construct');return output;};}});
Object.defineProperty(a,'constructor',{get:function(){log.push('constructor');gc();return c;}});
Object.defineProperty(a,Symbol.isConcatSpreadable,{get:function(){log.push('spread');gc();return true;}});
b=a.concat(3);check(b===output&&b.length===3&&b[0]===1&&b[2]===3&&log.join()==='constructor,species,construct,spread','species before spreading and custom result');
a=[];a.constructor={};a.constructor[Symbol.species]=function(){return Object.preventExtensions({});};check(caught(function(){a.concat(1);}) instanceof TypeError,'creation failure throws');
a.constructor[Symbol.species]=function(){return Object.defineProperty({},'length',{value:0,writable:false});};check(caught(function(){a.concat();}) instanceof TypeError,'length assignment throws even empty');
log=[];output=new Proxy({}, {defineProperty:function(t,k,d){log.push('define:'+k);gc();Object.defineProperty(t,k,d);return true;},set:function(t,k,v){log.push('set:'+k);t[k]=v;return true;}});
a=[1];a.constructor={};a.constructor[Symbol.species]=function(){return output;};check(a.concat(2)===output&&log.join()==='define:0,define:1,set:length','Proxy result definitions and final Set');
log=[];a=new Proxy({0:'x',length:2},{get:function(t,k){log.push('get:'+String(k));gc();return k===Symbol.isConcatSpreadable?true:t[k];},has:function(t,k){log.push('has:'+k);gc();return k in t;}});
b=[].concat(a);check(b.length===2&&b[0]==='x'&&!(1 in b)&&log.join()==='get:Symbol(Symbol.isConcatSpreadable),get:length,has:0,get:0,has:1','Proxy HasProperty and Get order');
a={};Object.defineProperty(a,Symbol.isConcatSpreadable,{get:function(){throw marker;}});check(caught(function(){[].concat(a);})===marker,'spread getter error preserved');
a=[];Object.defineProperty(a,'constructor',{get:function(){throw marker;}});check(caught(function(){a.concat();})===marker,'constructor getter error preserved');
a={length:1};a[Symbol.isConcatSpreadable]=true;Object.defineProperty(a,'0',{get:function(){gc();throw marker;}});check(caught(function(){[].concat(a);})===marker,'element getter error preserved');
var primitiveSymbol=Symbol('box');b=concat.call(primitiveSymbol);check(b.length===1&&b[0].valueOf()===primitiveSymbol,'Symbol receiver box');
b=concat.call('ab');check(b.length===1&&Object.getPrototypeOf(b[0])===String.prototype&&b[0].valueOf()==='ab','String receiver box');
check([].concat('ab')[0]==='ab','primitive argument remains primitive');
var rev=Proxy.revocable([],{});rev.revoke();check(caught(function(){concat.call(rev.proxy);}) instanceof TypeError,'revoked array receiver');
a=[1];var seen=[];a.constructor={};a.constructor[Symbol.species]=function(){return {set length(v){gc();seen.push(v);}};};b=a.concat(2);check(b[0]===1&&b[1]===2&&seen.join()==='2','result length setter');
print('ES6-ARRAY-CONCAT checks='+checks+' failures=0');
