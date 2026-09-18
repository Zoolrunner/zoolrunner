/* Array.of uses constructors and own descriptors without invoking setters. */
var checks = 0;
function check(ok, label) { ++checks; if (!ok) throw Error('FAIL Array.of: ' + label); }
function throwsType(fn, label) { var caught=false; try { fn(); } catch(e) { caught=e instanceof TypeError; } check(caught,label); }
var of=Array.of, d=Object.getOwnPropertyDescriptor(Array,'of');
check(typeof of==='function' && of.length===0 && of.name==='of','metadata');
check(d.writable && d.configurable && !d.enumerable,'descriptor');
check(!of.hasOwnProperty('prototype'),'no constructor prototype');
throwsType(function(){new of();},'not constructible');
check(of().length===0,'empty');
check(of(3).length===1 && of(3)[0]===3,'single numeric value');
var marker={}, result=of(undefined,null,marker,-0,NaN);
check(result.length===5 && result[0]===undefined && result.hasOwnProperty(0) && result[1]===null && result[2]===marker && 1/result[3]===-Infinity && isNaN(result[4]),'argument preservation');
var count=0, requested=-1;
function C(length){++count;requested=length;}
result=of.call(C,1,2);
check(result instanceof C && result[0]===1 && result[1]===2 && result.length===2 && count===1 && requested===2,'custom constructor');
d=Object.getOwnPropertyDescriptor(result,'0');
check(d.value===1 && d.writable && d.enumerable && d.configurable,'own index descriptor');
var returned={}, calls=0;
function Returning(){return returned;}
Object.defineProperty(returned,'0',{configurable:true,get:function(){throw 'get';},set:function(){++calls;}});
result=of.call(Returning,7);
check(result===returned && result[0]===7 && calls===0,'replace configurable own accessor');
Object.defineProperty(C.prototype,'0',{configurable:true,set:function(){++calls;}});
result=of.call(C,9);
check(result.hasOwnProperty(0) && result[0]===9 && calls===0,'bypass inherited setter');
Object.defineProperty(returned,'length',{configurable:true,set:function(v){check(this===returned && v===2,'length setter receiver'); gc(); ++calls;}});
result=of.call(Returning,4,5);
check(result===returned && result[0]===4 && result[1]===5 && calls===1,'length setter after element creation');
function Frozen(){return Object.freeze({});}
throwsType(function(){of.call(Frozen,1);},'nonextensible result');
function ReadOnly(){return Object.defineProperty({},'0',{value:1});}
throwsType(function(){of.call(ReadOnly,1);},'nonconfigurable index even same value');
function LockedLength(){return Object.defineProperty({},'length',{value:0});}
throwsType(function(){of.call(LockedLength);},'throwing length write even zero');
var sentinel={};
try {of.call(function(){throw sentinel;},1);check(false,'constructor exception');} catch(e) {check(e===sentinel,'constructor exception identity');}
var partial=Object.defineProperty({},'1',{value:'locked'});
throwsType(function(){of.call(function(){return partial;},'first','second','third');},'partial definition throws');
check(partial[0]==='first' && partial[1]==='locked' && !partial.hasOwnProperty(2) && !partial.hasOwnProperty('length'),'stop at first failed definition');
var lockedArray=[];Object.defineProperty(lockedArray,'length',{writable:false});
throwsType(function(){of.call(function(){return lockedArray;},1);},'array length blocks element creation');
check(lockedArray.length===0 && !lockedArray.hasOwnProperty(0),'blocked array remains empty');
throwsType(function(){of.call(function(){return new String('x');},'y');},'string exotic index is immutable');
var lengthTarget={};Object.defineProperty(lengthTarget,'length',{set:function(){throw sentinel;}});
try {of.call(function(){return lengthTarget;},8);check(false,'length exception');} catch(e) {check(e===sentinel && lengthTarget[0]===8,'length exception after elements');}
var bound=C.bind(null);
result=of.call(bound,11);
check(result instanceof C && result[0]===11 && requested===1,'bound constructor');
var nonconstructors=[undefined,null,0,'x',true,{},Math.max,Math.max.bind(null),of];
for(var i=0;i<nonconstructors.length;++i){result=of.call(nonconstructors[i],marker);check(Array.isArray(result)&&result[0]===marker,'fallback '+i);}
var saved=Array, proto=Array.prototype;
Array=function(){throw 'global Array replacement';};
check(Object.getPrototypeOf(of.call(null,1))===proto,'fallback ignores global replacement');
version(0);
check(Object.getPrototypeOf(of.call(null,1))===proto,'legacy caller uses intrinsic fallback');
version(2015);Array=saved;
for(i=0;i<15;++i){result=of.call(function(n){gc();this.requested=n;},marker);check(result[0]===marker&&result.requested===1,'reentrant collection '+i);}
print('ES6-ARRAY-OF checks='+checks+' failures=0');
