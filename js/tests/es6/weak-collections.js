/* ES2015 weak collection API, callbacks and key handling. */
var checks=0;
function check(ok,label){++checks;if(!ok)throw Error('FAIL weak collection: '+label);}
function caught(fn){try{fn();}catch(e){return e;}return null;}
var w=new WeakMap(),s=new WeakSet(),key={},value={answer:42};
check(w.set(key,value)===w&&w.get(key)===value&&w.has(key),'weak map insertion');
check(w.set(key,7)===w&&w.get(key)===7,'weak map replacement');
check(w.get({})===undefined&&!w.has({}),'object identity');
check(w.delete(key)&&!w.delete(key)&&w.get(key)===undefined,'weak map deletion');
check(s.add(key)===s&&s.add(key)===s&&s.has(key),'weak set insertion');
check(s.delete(key)&&!s.delete(key)&&!s.has(key),'weak set deletion');
var primitives=[undefined,null,false,true,0,-1,NaN,'key',Symbol('key')];
for(var i=0;i<primitives.length;++i){
 var primitive=primitives[i];
 check(caught(function(){w.set(primitive,1);}) instanceof TypeError,'primitive map key '+i);
 check(caught(function(){s.add(primitive);}) instanceof TypeError,'primitive set key '+i);
 check(w.get(primitive)===undefined&&!w.has(primitive)&&!w.delete(primitive),'primitive map lookup '+i);
 check(!s.has(primitive)&&!s.delete(primitive),'primitive set lookup '+i);
}
check(caught(function(){WeakMap();}) instanceof TypeError&&caught(function(){WeakSet();}) instanceof TypeError,'constructors require new');
check(caught(function(){WeakMap.prototype.get.call(new Map(),{});}) instanceof TypeError,'map brand');
check(caught(function(){WeakSet.prototype.has.call(new Set(),{});}) instanceof TypeError,'set brand');
check(caught(function(){WeakMap.prototype.has.call(WeakMap.prototype,{});}) instanceof TypeError,'prototype lacks weak data');
check(!('size' in w)&&!('clear' in w)&&!('keys' in w)&&!('forEach' in w)&&!w[Symbol.iterator],'weak map exposes no enumeration');
check(!('size' in s)&&!('clear' in s)&&!('values' in s)&&!s[Symbol.iterator],'weak set exposes no enumeration');
key=Object.freeze({toString:function(){throw Error('must not coerce');}});
w=new WeakMap([[key,value]]);s=new WeakSet([key,key]);gc();
check(w.get(key)===value&&s.has(key),'frozen keys without coercion');
key=Object.create(null);w.set(key,19);s.add(key);gc();
check(w.get(key)===19&&s.has(key),'null prototype key');
check(Object.prototype.toString.call(w)==='[object WeakMap]'&&Object.prototype.toString.call(s)==='[object WeakSet]','weak collection tags');
check(WeakMap.prototype.set.length===2&&WeakMap.prototype.get.length===1&&WeakSet.prototype.add.length===1,'method arity');
check(!WeakMap.prototype.set.hasOwnProperty('prototype')&&caught(function(){new WeakSet.prototype.add();}) instanceof TypeError,'nonconstructor methods');
var marker={},closed=0,iterable={};
iterable[Symbol.iterator]=function(){return {next:function(){return {done:false,value:3};},return:function(){++closed;gc();return {};}};};
check(caught(function(){new WeakSet(iterable);}) instanceof TypeError&&closed===1,'invalid weak key closes iterator');
iterable[Symbol.iterator]=function(){return {next:function(){return {done:false,value:{get 0(){gc();throw marker;}}};},return:function(){++closed;return {};}};};
check(caught(function(){new WeakMap(iterable);})===marker&&closed===2,'entry getter throw closes iterator');
var original=WeakMap.prototype.set;
Object.defineProperty(WeakMap.prototype,'set',{configurable:true,get:function(){throw marker;}});
try{check(new WeakMap(null) instanceof WeakMap&&new WeakMap(undefined) instanceof WeakMap,'null skips adder');check(caught(function(){new WeakMap([]);})===marker,'adder acquired before iterator');}
finally{Object.defineProperty(WeakMap.prototype,'set',{value:original,writable:true,configurable:true});}
for(i=0;i<200;++i){w=new WeakMap();key={};value={back:key};w.set(key,value);key=value=null;gc();}
w=null;gc();check(true,'repeated cyclic-value collection');
var savedMap=WeakMap,savedSet=WeakSet;
check(delete this.WeakMap&&!Object.prototype.hasOwnProperty.call(this,'WeakMap'),'deleted weak map stays absent');
check(delete this.WeakSet&&!Object.prototype.hasOwnProperty.call(this,'WeakSet'),'deleted weak set stays absent');
this.WeakMap=savedMap;this.WeakSet=savedSet;
print('ES6-WEAK-COLLECTIONS checks='+checks+' failures=0');
