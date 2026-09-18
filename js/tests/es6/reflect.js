/* Reflect receivers, descriptors, construction and enumeration. */
var checks=0;
function check(ok,label){++checks;if(!ok)throw Error('FAIL Reflect: '+label);}
function caught(fn){try{fn();}catch(e){return e;}return null;}
function drain(it){var a=[],r;while(!(r=it.next()).done)a.push(r.value);return a;}
check(typeof Reflect==='object'&&Object.getPrototypeOf(Reflect)===Object.prototype,'namespace');
check(Object.prototype.toString.call(Reflect)==='[object Reflect]','namespace tag');
var o={a:1},sym=Symbol('key'),proto={inherited:2};Object.setPrototypeOf(o,proto);o[sym]=3;
check(Reflect.get(o,'a')===1&&Reflect.get(o,sym)===3&&Reflect.get(o,'inherited')===2,'get own and inherited');
check(Reflect.has(o,'a')&&Reflect.has(o,'inherited')&&!Reflect.has(o,'missing'),'has without reading value');
var receiver={},getterThis,setterThis,marker={};
Object.defineProperty(proto,'access',{configurable:true,get:function(){'use strict';gc();getterThis=this;return 9;},set:function(v){'use strict';gc();setterThis=this;if(v===marker)throw marker;}});
check(Reflect.get(o,'access',receiver)===9&&getterThis===receiver,'get receiver');
check(Reflect.get(o,'access',undefined)===9&&getterThis===undefined,'primitive getter receiver');
check(Reflect.set(o,'access',17,null)&&setterThis===null,'primitive setter receiver');
check(caught(function(){Reflect.set(o,'access',marker,receiver);})===marker,'setter exception');
check(Reflect.set(o,'a',7,receiver)&&receiver.a===7&&o.a===1,'data write on distinct receiver');
check(!Reflect.set(o,'a',8,3)&&o.a===1,'primitive data receiver rejected');
var calls=0;receiver={};Object.defineProperty(receiver,'a',{set:function(){++calls;},configurable:true});
check(!Reflect.set(o,'a',8,receiver)&&calls===0,'receiver accessor not invoked for target data');
Object.defineProperty(o,'locked',{value:1,writable:false,configurable:false});
check(!Reflect.set(o,'locked',2)&&!Reflect.deleteProperty(o,'locked'),'boolean rejection');
check(!Reflect.defineProperty(o,'locked',{value:2})&&o.locked===1,'define rejection');
check(caught(function(){Reflect.defineProperty(o,'x',{get:3});}) instanceof TypeError,'invalid descriptor still throws');
check(caught(function(){Reflect.defineProperty(o,'x',{get value(){gc();throw marker;}});})===marker,'descriptor getter throw');
var key={};key[Symbol.toPrimitive]=function(hint){check(hint==='string','property hint');gc();return sym;};
check(Reflect.defineProperty(o,key,{value:5,writable:true,configurable:true})&&o[sym]===5,'symbol key conversion');
check(Reflect.deleteProperty(o,sym)&&!Reflect.has(o,sym),'symbol deletion');
check(Reflect.getOwnPropertyDescriptor(o,'a').value===1&&Reflect.getOwnPropertyDescriptor(o,'absent')===undefined,'own descriptor');
check(Reflect.getPrototypeOf(o)===proto,'prototype read');
check(!Reflect.setPrototypeOf(proto,o),'prototype cycle rejected');
var frozen=Object.preventExtensions({});check(!Reflect.isExtensible(frozen)&&Reflect.preventExtensions(frozen),'nonextensible query');
check(!Reflect.set(frozen,'new',1)&&!Reflect.defineProperty(frozen,'new',{value:1}),'new property rejection');
check(!Reflect.setPrototypeOf(frozen,null)&&Reflect.setPrototypeOf(frozen,Object.prototype),'nonextensible prototype result');
var array=[0,1,2];Object.defineProperty(array,'1',{configurable:false});
check(!Reflect.set(array,'length',0)&&array.length===2&&!array.hasOwnProperty('2'),'partial length write reports false');
array=[0,1,2];Object.defineProperty(array,'1',{configurable:false});
check(!Reflect.defineProperty(array,'length',{value:0,writable:false})&&array.length===2&&!Object.getOwnPropertyDescriptor(array,'length').writable,'partial define keeps readonly length');
var cleanDescriptor=Object.create(null);cleanDescriptor.value=8;
Object.defineProperty(Object.prototype,'get',{value:function(){return 1;},configurable:true});
try{check(Reflect.defineProperty(o,'a',cleanDescriptor)&&o.a===8,'internal descriptor ignores inherited accessor field');}
finally{delete Object.prototype.get;}
Object.defineProperty(Object.prototype,'value',{value:99,configurable:true});
try{check(Reflect.set(o,'access',1,receiver),'internal accessor descriptor ignores inherited data field');}
finally{delete Object.prototype.value;}
function strictCall(){'use strict';return {receiver:this,args:Array.prototype.slice.call(arguments)};}
var order=[],args={get length(){order.push('length');gc();return 2.9;},get 0(){order.push(0);gc();return 4;},get 1(){order.push(1);return 5;}};
var result=Reflect.apply(strictCall,17,args);
check(result.receiver===17&&result.args.join()==='4,5'&&order.join()==='length,0,1','apply ToLength and raw this');
check(caught(function(){Reflect.apply(strictCall,null,null);}) instanceof TypeError,'apply needs argument object');
check(caught(function(){Reflect.apply(null,null,{get length(){throw marker;}});}) instanceof TypeError,'callability checked before length');
function Target(a,b){this.sum=a+b;gc();}
function Other(){};Other.prototype={marker:31};
var instance=Reflect.construct(Target,[4,5],Other);
check(instance.sum===9&&Object.getPrototypeOf(instance)===Other.prototype,'alternate newTarget');
check(caught(function(){Reflect.construct(Target,[],Date.now);}) instanceof TypeError,'newTarget constructor validation');
var bound=Target.bind(null,10),nested=bound.bind(null,20);
instance=Reflect.construct(bound,[5],Other);check(instance.sum===15&&Object.getPrototypeOf(instance)===Other.prototype,'bound target with alternate newTarget');
instance=Reflect.construct(nested,[]);check(instance.sum===30&&Object.getPrototypeOf(instance)===Target.prototype,'nested bound default newTarget');
instance=Reflect.construct(Object,[o],Other);check(instance!==o&&Object.getPrototypeOf(instance)===Other.prototype,'Object derived path ignores argument');
instance=Reflect.construct(Array,[1,2],Other);check(Array.isArray(instance)&&instance.length===2&&instance[1]===2&&Object.getPrototypeOf(instance)===Other.prototype,'Array native allocation kind');
instance=Reflect.construct(String,['abc'],Other);var d=Object.getOwnPropertyDescriptor(instance,'length');
check(instance[1]==='b'&&d.value===3&&!d.writable&&!d.enumerable&&!d.configurable,'String owns length with alternate prototype');
instance=Reflect.construct(RegExp,['a','g'],Other);d=Object.getOwnPropertyDescriptor(instance,'lastIndex');
check(d.value===0&&d.writable&&!d.enumerable&&!d.configurable,'RegExp owns lastIndex with alternate prototype');
instance.lastIndex=1;check(RegExp.prototype.exec.call(instance,'ba')[0]==='a'&&instance.lastIndex===2,'RegExp native lastIndex state');
instance=Reflect.construct(Function,['return 23'],Other);check(instance()===23&&Object.getPrototypeOf(instance)===Other.prototype,'Function allocation');
instance=Reflect.construct(Map,[],Other);Map.prototype.set.call(instance,sym,44);check(Map.prototype.get.call(instance,sym)===44,'Map private allocation');
Other.prototype=3;instance=Reflect.construct(TypeError,['bad'],Other);
check(Object.getPrototypeOf(instance)===TypeError.prototype&&instance.message==='bad','native error fallback intrinsic');
var s1=Symbol('a'),s2=Symbol('b');o={b:1,2:2,a:3,1:4};o[s1]=5;o[s2]=6;
var keys=Reflect.ownKeys(o);check(keys.slice(0,4).join()==='1,2,b,a'&&keys[4]===s1&&keys[5]===s2,'own key order with symbols');
proto={hidden:1,late:2};o=Object.create(proto);o.a=1;o.b=2;o[sym]=3;Object.defineProperty(o,'hidden',{value:9,enumerable:false});
var it=Reflect.enumerate(o);check(it[Symbol.iterator]()===it,'enumerator self');
check(it.next().value==='a','first enumeration key');delete o.b;o.c=3;gc();
check(drain(it).join()==='late','live descriptors and nonenumerable shadowing');
o.after=4;check(it.next().done,'enumerator exhaustion');
it=Reflect.enumerate({a:1,b:2});check(caught(function(){it.next.call({});}) instanceof TypeError,'enumerator brand');
check(drain(Reflect.enumerate({['__proto__']:1,a:2})).join()==='__proto__,a','enumeration visited keys use data properties');
check(Reflect.enumerate.length===1&&Reflect.apply.length===3&&Reflect.construct.length===2,'method lengths');
check(!Reflect.get.hasOwnProperty('prototype')&&caught(function(){new Reflect.get();}) instanceof TypeError,'methods nonconstructible');
var savedReflect=Reflect;check(delete this.Reflect&&!Object.prototype.hasOwnProperty.call(this,'Reflect'),'deleted namespace stays absent');this.Reflect=savedReflect;
print('ES6-REFLECT checks='+checks+' failures=0');
