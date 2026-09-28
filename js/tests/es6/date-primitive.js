/* ES2015 Date primitive conversion and legacy isolation.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
var checks=0;
function check(v,name){++checks;if(!v)throw Error('Date primitive: '+name);}
function caught(f){try{f();}catch(e){return e;}return null;}
var hook=Date.prototype[Symbol.toPrimitive],d=Object.getOwnPropertyDescriptor(Date.prototype,Symbol.toPrimitive),log=[],obj;
check(typeof hook==='function'&&hook.length===1&&hook.name==='[Symbol.toPrimitive]','metadata');
check(!d.writable&&!d.enumerable&&d.configurable,'descriptor');
check(!hook.hasOwnProperty('prototype')&&caught(function(){new hook();}) instanceof TypeError,'nonconstructor');
check(caught(function(){hook.call(null,'default');}) instanceof TypeError,'null receiver');
check(caught(function(){hook.call(1,'default');}) instanceof TypeError,'primitive receiver');
obj={toString:function(){log.push('string:'+arguments.length);gc();return 'text';},valueOf:function(){log.push('value:'+arguments.length);gc();return 42;}};
check(hook.call(obj,'default')==='text'&&log.join()==='string:0','default string preference');
log=[];check(hook.call(obj,'string')==='text'&&log.join()==='string:0','string preference');
log=[];check(hook.call(obj,'number')===42&&log.join()==='value:0','number preference');
obj.toString=function(){return {};};check(hook.call(obj,'string')===42,'string object falls back');
obj.valueOf=function(){return {};};check(caught(function(){hook.call(obj,'number');}) instanceof TypeError,'both methods return objects');
obj.toString=3;obj.valueOf=function(){return Symbol.for('date-value');};check(hook.call(obj,'string')===Symbol.for('date-value'),'skip noncallable and preserve symbol');
var marker={};Object.defineProperty(obj,'toString',{get:function(){gc();throw marker;},configurable:true});
check(caught(function(){hook.call(obj,'string');})===marker,'getter error preserved');
check(caught(function(){hook.call(obj,{toString:function(){throw marker;}});}) instanceof TypeError,'hint is not converted');
check(caught(function(){hook.call(obj,new String('number'));}) instanceof TypeError,'boxed hint rejected');
check(caught(function(){hook.call(obj,'Number');}) instanceof TypeError,'hint case sensitive');
obj=new Date(1234);check(hook.call(obj,'number')===1234&&+obj===1234,'real Date numeric conversion');
check(hook.call(obj,'default')===obj.toString()&&obj+''===obj.toString(),'real Date default conversion');
check(obj.valueOf('string')===1234&&obj.valueOf({toString:function(){throw marker;}})===1234,'modern valueOf ignores extra arguments');
Object.defineProperty(obj,Symbol.toPrimitive,{value:undefined,configurable:true});
log=[];obj.valueOf=function(){log.push('value:'+arguments.length);gc();return 7;};obj.toString=function(){log.push('string:'+arguments.length);return 'seven';};
check(obj+1===8&&log.join()==='value:0','removed hook uses ordinary numeric default');
log=[];check(String(obj)==='seven'&&log.join()==='string:0','removed hook string conversion');
Object.defineProperty(obj,'valueOf',{get:function(){throw marker;},configurable:true});check(caught(function(){return +obj;})===marker,'removed hook getter error preserved');
obj=new Date(0);Object.defineProperty(obj,Symbol.toPrimitive,{value:null});check(obj+1===1,'null hook uses ordinary conversion');
var count=0,p=new Proxy({toString:function(){check(this===p&&arguments.length===0,'Proxy method receiver');return 'proxy';}},{get:function(t,k){++count;gc();return t[k];}});
check(hook.call(p,'default')==='proxy'&&count===1,'generic Proxy conversion');
obj=new Date(1234);obj.valueOf=obj.toString=function(){throw marker;};
check(new Date(obj).getTime()===1234,'copy internal Date value without conversion');
obj=new Date(NaN);obj.valueOf=function(){throw marker;};check(isNaN(new Date(obj).getTime()),'copy invalid Date value');
log=[];obj={};obj[Symbol.toPrimitive]=function(h){log.push(h);gc();return '2000-01-01T00:00:00.000Z';};
check(new Date(obj).getTime()===946684800000&&log.join()==='default','Date constructor default hint and string parse');
check(caught(function(){Date.prototype.getTime();}) instanceof TypeError&&caught(function(){Date.prototype.setFullYear(2012);}) instanceof TypeError,'modern prototype has no Date value');
check(Object.prototype.toString.call(Date.prototype)==='[object Object]'&&Object.prototype.toString.call(new Date(0))==='[object Date]','Date prototype and instance brands');
print('ES6-DATE-PRIMITIVE checks='+checks+' failures=0');
