/* Modern iterators: live generic arrays, Unicode, callbacks and collection. */
var checks=0;
function check(ok,label){++checks;if(!ok)throw Error('FAIL iterator: '+label);}
function throwsType(fn,label){var caught=false;try{fn();}catch(e){caught=e instanceof TypeError;}check(caught,label);}
var methods=['keys','values','entries'];
for(var m=0;m<methods.length;++m){
 var name=methods[m],fn=Array.prototype[name],d=Object.getOwnPropertyDescriptor(Array.prototype,name);
 check(fn.name===name&&fn.length===0&&!fn.hasOwnProperty('prototype'),'metadata '+name);
 check(d.writable&&!d.enumerable&&d.configurable,'descriptor '+name);
 throwsType(function(){fn.call(null);},'null receiver '+name);
 throwsType(function(){new fn();},'construction '+name);
}
check(Array.prototype[Symbol.iterator]===Array.prototype.values,'default values identity');
var it=[10,20].entries(),proto=Object.getPrototypeOf(it),base=Object.getPrototypeOf(proto);
check(Object.getPrototypeOf(base)===Object.prototype,'iterator prototype chain');
check(Object.getOwnPropertyNames(it).length===0,'private iterator state');
check(Object.getOwnPropertyNames(proto).join()==='next','next prototype property');
check(proto.next.name==='next'&&proto.next.length===0&&!proto.next.hasOwnProperty('prototype'),'next metadata');
check(base[Symbol.iterator].call(null)===null&&base[Symbol.iterator].call(7)===7,'generic iterator identity');
check(it[Symbol.iterator]()===it,'iterator is iterable');
check(Object.prototype.toString.call(it)==='[object Array Iterator]','array iterator tag');
var first=it.next();
check(first.value.join()==='0,10'&&!first.done&&Object.keys(first).join()==='value,done','entry and result');
check(it.next().value.join()==='1,20','second entry');
var end=it.next();
check(end.done&&end.value===undefined&&end.hasOwnProperty('value'),'completed result');
check(it.next()!==end&&it.next().done,'fresh completed results');
throwsType(function(){proto.next.call(proto);},'prototype lacks iterator state');
throwsType(function(){proto.next.call(Object.create(proto));},'inherited state rejected');
throwsType(function(){new proto.next();},'next not constructible');
var data=[,2],values=data.values();
check(values.next().value===undefined&&!values.next().done,'holes visited');
data.push(3);check(values.next().value===3,'live growth');
check(values.next().done,'exhaustion');data.push(4);check(values.next().done,'exhaustion remains final');
var reads=0,generic={0:'zero',1:'one'};
Object.defineProperty(generic,'length',{get:function(){++reads;gc();return 2;}});
it=Array.prototype.values.call(generic);
check(reads===0,'creation does not read length');
check(it.next().value==='zero'&&it.next().value==='one'&&reads===2,'live generic length');
var marker={},fail=true;
Object.defineProperty(generic,'0',{configurable:true,get:function(){gc();throw marker;}});
it=Array.prototype.values.call(generic);var caught;
try{it.next();}catch(e){caught=e;}
check(caught===marker&&it.next().value==='one','element exception advances index');
it=Array.prototype.keys.call(generic);
check(it.next().value===0&&it.next().value===1,'keys do not access elements');
generic={0:'zero'};
Object.defineProperty(generic,'length',{get:function(){gc();if(fail){fail=false;throw marker;}return 1;}});
it=Array.prototype.values.call(generic);caught=null;try{it.next();}catch(e){caught=e;}
check(caught===marker&&it.next().value==='zero','length exception preserves index');
var nested,once=true;
generic={0:'zero',1:'one'};
Object.defineProperty(generic,'length',{get:function(){if(once){once=false;nested=it.next();}gc();return 2;}});
it=Array.prototype.values.call(generic);
check(it.next().value==='zero'&&nested.value==='zero'&&it.next().value==='one','reentrant length');
generic={length:2,1:'one'};
Object.defineProperty(generic,'0',{get:function(){nested=it.next();gc();return 'zero';}});
it=Array.prototype.values.call(generic);
check(it.next().value==='zero'&&nested.value==='one'&&it.next().done,'reentrant element');
check(!Array.prototype.keys.call({length:4294967296}).next().done,'length exceeds uint32');
check(Array.prototype.values.call({length:-1}).next().done,'negative length');
check(Array.prototype.values.call(Symbol()).next().done,'symbol array-like receiver');
it=[3].entries();var setters=0;
Object.defineProperty(Array.prototype,'0',{configurable:true,set:function(){++setters;}});
try{var pair=it.next().value;check(pair[0]===0&&pair[1]===3&&pair.length===2&&setters===0,'entry own properties bypass setters');}
finally{delete Array.prototype[0];}
var stringMethod=String.prototype[Symbol.iterator];
check(stringMethod.name==='[Symbol.iterator]'&&stringMethod.length===0,'string method metadata');
throwsType(function(){stringMethod.call(null);},'string null receiver');
throwsType(function(){stringMethod.call(Symbol());},'string symbol receiver');
throwsType(function(){new stringMethod();},'string iterator not constructible');
var text='a\ud83d\ude00\ud800z\udc00\u0000',pieces=['a','\ud83d\ude00','\ud800','z','\udc00','\u0000'];
it=stringMethod.call(text);
check(Object.getPrototypeOf(Object.getPrototypeOf(it))===base,'shared iterator prototype');
check(Object.prototype.toString.call(it)==='[object String Iterator]','string iterator tag');
for(var i=0;i<pieces.length;++i){gc();var step=it.next();check(!step.done&&step.value===pieces[i],'code point '+i);}
check(it.next().done&&it.next().value===undefined,'string exhaustion');
var conversions=0;it=stringMethod.call({toString:function(){++conversions;gc();return 'xy';}});
check(conversions===1&&it.next().value==='x'&&it.next().value==='y'&&conversions===1,'string conversion once');
throwsType(function(){Object.getPrototypeOf(it).next.call([].values());},'string next rejects array iterator');
check(typeof Iterator==='function'&&typeof StopIteration==='object','classic interfaces remain');
var originalValues=Array.prototype.values;
Array.prototype.values=function(){throw Error('replaced values must not be used');};
try {
 var args=(function(a){return arguments;})(9);
 var argDescriptor=Object.getOwnPropertyDescriptor(args,Symbol.iterator);
 check(argDescriptor.value===originalValues&&argDescriptor.writable&&!argDescriptor.enumerable&&argDescriptor.configurable,'arguments intrinsic descriptor');
 check(args[Symbol.iterator]().next().value===9,'arguments ignore replacement values');
 check((function(a){var iterator=arguments[Symbol.iterator]();a=11;return iterator.next().value===11;})(9),'mapped arguments remain live');
 check((function(a){'use strict';var iterator=arguments[Symbol.iterator]();a=11;return iterator.next().value===9;})(9),'strict arguments remain unmapped');
} finally {Array.prototype.values=originalValues;}
print('ES6-MODERN-ITERATORS checks='+checks+' failures=0');
