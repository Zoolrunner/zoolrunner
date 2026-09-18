/* ES2015 keyed collections, callback mutation and lifetime checks. */
var checks=0;
function check(ok,label){++checks;if(!ok)throw Error('FAIL collection: '+label);}
function caught(fn){try{fn();}catch(e){return e;}return null;}
function values(it){var a=[],r;while(!(r=it.next()).done)a.push(r.value);return a;}
var m=new Map(),s=new Set(),object={},sym=Symbol('key');
check(m.size===0&&s.size===0,'empty constructors');
check(caught(function(){Map();}) instanceof TypeError&&caught(function(){Set();}) instanceof TypeError,'constructors require new');
check(m.set('a',1)===m&&m.get('a')===1&&m.has('a')&&m.size===1,'map insertion');
check(m.set('a',2)===m&&m.get('a')===2&&m.size===1,'map replacement');
m.set(object,3).set(sym,4).set(NaN,5).set(-0,6);
check(m.get(object)===3&&m.get({})===undefined&&m.get(sym)===4,'identity keys');
check(m.get(Number('bad'))===5&&m.get(+0)===6,'SameValueZero');
check(m.get('missing')===undefined&&!m.has('missing'),'missing map key');
check(m.delete(object)&&!m.delete(object)&&m.size===4,'delete map key');
check(m.set()===m&&m.has(undefined)&&m.get(undefined)===undefined,'missing set arguments');
check(s.add('a')===s&&s.add('a')===s&&s.size===1,'set deduplication');
s.add(NaN).add(Number('bad')).add(-0).add(+0);
check(s.size===3&&s.has(NaN)&&s.has(0),'set SameValueZero');
check(1/values(s.values())[2]===Infinity,'set canonical positive zero');
check(s.delete(NaN)&&!s.delete(NaN),'set deletion');
check(Map.prototype[Symbol.iterator]===Map.prototype.entries,'map iterator alias');
check(Set.prototype[Symbol.iterator]===Set.prototype.values&&Set.prototype.keys===Set.prototype.values,'set iterator aliases');
m=new Map([['a',1],['b',2],['a',3]]);
check(m.size===2&&m.get('a')===3&&values(m.keys()).join()==='a,b','map iterable constructor and order');
s=new Set('a\ud83d\ude00a');
check(s.size===2&&values(s.values()).join()==='a,\ud83d\ude00','set string codepoints');
var it=m.entries(),r=it.next();
check(r.value[0]==='a'&&r.value[1]===3&&!r.done,'map entry pair');
check(Object.getPrototypeOf(r)===Object.prototype&&Object.getPrototypeOf(r.value)===Array.prototype,'result prototypes');
check(it[Symbol.iterator]()===it,'iterator self');
check(Object.getPrototypeOf(Object.getPrototypeOf(it))===Object.getPrototypeOf(Object.getPrototypeOf([].values())),'shared IteratorPrototype');
check(Object.prototype.toString.call(m)==='[object Map]'&&Object.prototype.toString.call(s)==='[object Set]','collection tags');
check(Object.prototype.toString.call(it)==='[object Map Iterator]'&&Object.prototype.toString.call(s.values())==='[object Set Iterator]','iterator tags');
m=new Map([[1,10],[2,20],[3,30]]);it=m.keys();
check(it.next().value===1,'first key');
m.delete(2);m.delete(1);m.set(1,11);m.set(4,40);gc();
check(values(it).join()==='3,1,4','mutation preserves live order');
m.set(5,50);check(it.next().done,'exhaustion stays sticky');
it=m.keys();check(it.next().value===3,'new cursor starts first live row');
m.clear();m.set(9,90);gc();check(it.next().value===9&&it.next().done,'clear followed by insertion');
m=new Map([[1,10],[2,20]]);var seen=[],receiver={};
m.forEach(function(value,key,owner){'use strict';check(this===receiver&&owner===m,'forEach receiver and owner');seen.push(key+':'+value);if(key===1){m.delete(2);m.set(3,30);gc();}},receiver);
check(seen.join()==='1:10,3:30','forEach observes callback mutations');
s=new Set([1,2]);seen=[];s.forEach(function(value,key,owner){check(value===key&&owner===s,'set callback arguments');seen.push(value);if(value===1){s.clear();s.add(3);gc();}});
check(seen.join()==='1,3','set forEach clear and append');
check(caught(function(){Map.prototype.get.call({});}) instanceof TypeError,'map brand');
check(caught(function(){Set.prototype.add.call(m,1);}) instanceof TypeError,'set brand');
var sizeGetter=Object.getOwnPropertyDescriptor(Map.prototype,'size').get;
check(caught(function(){sizeGetter.call(Map.prototype);}) instanceof TypeError,'prototype has no map data');
check(caught(function(){m.forEach(null);}) instanceof TypeError,'callback validation');
var marker={};check(caught(function(){m.forEach(function(){gc();throw marker;});})===marker,'callback exception');
check(caught(function(){new Map([1]);}) instanceof TypeError,'entry must be object');
var closed=0, iterable={};
iterable[Symbol.iterator]=function(){return {next:function(){return {done:false,value:1};},return:function(){++closed;gc();return {};}};};
check(caught(function(){new Map(iterable);}) instanceof TypeError&&closed===1,'invalid entry closes iterator');
var order=[];var pair={get 0(){order.push(0);gc();return 'k';},get 1(){order.push(1);gc();return 7;}};
check(new Map([pair]).get('k')===7&&order.join()==='0,1','entry property order');
var originalAdd=Set.prototype.add;
Object.defineProperty(Set.prototype,'add',{configurable:true,get:function(){throw marker;}});
try{check(new Set(null).size===0&&new Set(undefined).size===0,'null iterable skips adder');check(caught(function(){new Set([]);})===marker,'adder acquired first');}
finally{Object.defineProperty(Set.prototype,'add',{value:originalAdd,writable:true,configurable:true});}
check(Object.getOwnPropertyDescriptor(Map.prototype,'size').enumerable===false&&sizeGetter.name==='get size'&&sizeGetter.length===0,'size getter metadata');
check(!Map.prototype.set.hasOwnProperty('prototype')&&caught(function(){new Map.prototype.set();}) instanceof TypeError,'methods nonconstructible');
check(Map.prototype.set.length===2&&Set.prototype.add.length===1&&Map.prototype.forEach.length===1,'method lengths');
check(Map[Symbol.species]===Map&&Set[Symbol.species]===Set,'species');
var species=Object.getOwnPropertyDescriptor(Map,Symbol.species).get;
check(species.call(null)===null&&species.call(3)===3,'species raw receiver');
(function(){var key={},value={answer:42},map=new Map([[key,value]]);it=map.entries();map=null;key=null;value=null;})();
gc();r=it.next();check(r.value[1].answer===42,'iterator retains collection values');
it=null;r=null;gc();
for(var n=0;n<200;++n){m=new Map([[{},{}]]);it=m.entries();if(n%2)m=null;else it=null;gc();}
m=null;it=null;gc();check(true,'both dead-owner and dead-iterator lifetime orders');
var savedMap=Map,savedSet=Set;
check(delete this.Map&&!Object.prototype.hasOwnProperty.call(this,'Map'),'deleted Map binding stays absent');
check(delete this.Set&&!Object.prototype.hasOwnProperty.call(this,'Set'),'deleted Set binding stays absent');
this.Map=savedMap;this.Set=savedSet;
print('ES6-COLLECTIONS checks='+checks+' failures=0');
