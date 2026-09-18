/* ES2015 Error prototypes, descriptors, construction and reentrancy.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
var checks=0;
function check(v,n){++checks;if(!v)throw Error('Modern Error: '+n);}
function caught(f){try{f();}catch(e){return e;}return null;}
var ctors=[Error,EvalError,RangeError,ReferenceError,SyntaxError,TypeError,URIError],i,C,e,d,marker={},log;
for(i=0;i<ctors.length;i++){
 C=ctors[i];e=new C('message');d=Object.getOwnPropertyDescriptor(e,'message');
 check(d.value==='message'&&d.writable&&d.configurable&&!d.enumerable,'message descriptor '+i);
 check(delete e.message&&!e.hasOwnProperty('message')&&e.message==='','message stays deleted '+i);
 check(Object.prototype.toString.call(C.prototype)==='[object Object]','ordinary prototype '+i);
 check(Object.getPrototypeOf(C)===(i===0?Function.prototype:Error),'constructor inheritance '+i);
 check(caught((function(P){return function(){return new P;};})(C.prototype)) instanceof TypeError,'prototype not constructor '+i);
 check(caught((function(x){return function(){return new x;};})(new C)) instanceof TypeError,'instance not constructor '+i);
 check(Object.prototype.toString.call(new C)==='[object Error]'&&new C instanceof Error,'native Error brand '+i);
 check(Object.prototype.hasOwnProperty.call(C.prototype,'message'),'own prototype message '+i);
}
check(!new Error().hasOwnProperty('message')&&!new Error(undefined).hasOwnProperty('message'),'omitted message');
check(Error('call') instanceof Error&&Error('call').message==='call','function call creates error');
check(new Error({toString:function(){gc();return new Error('nested').message;}}).message==='nested','nested conversion construction');
check(caught(function(){new Error({toString:function(){gc();throw marker;}});})===marker,'message conversion error');
check(caught(function(){new Error(Symbol());}) instanceof TypeError,'symbol message throws');
check(new Error('x',{toString:function(){throw marker;}},{valueOf:function(){throw marker;}}).message==='x','modern ignores legacy arguments');
var text=Error.prototype.toString;
check(text.call({})==='Error'&&text.call({name:'',message:'x'})==='x'&&text.call({name:'N'})==='N','generic defaults');
check(caught(function(){text.call(1);}) instanceof TypeError&&caught(function(){text.call(null);}) instanceof TypeError,'generic rejects primitives');
log=[];e={get name(){log.push('name');return {toString:function(){gc();log.push('name-string');return 'N';}};},get message(){log.push('message');return {toString:function(){gc();log.push('message-string');return 'M';}};}};
check(text.call(e)==='N: M'&&log.join()==='name,name-string,message,message-string','generic conversion order');
check(caught(function(){text.call({name:{toString:function(){throw marker;}},get message(){throw Error('too late');}});})===marker,'name error before message');
check(caught(function(){return new text;}) instanceof TypeError,'toString nonconstructor');
function Target(){}Target.prototype={custom:true};e=Reflect.construct(TypeError,['x'],Target);
check(Object.getPrototypeOf(e)===Target.prototype&&e.message==='x'&&Object.prototype.toString.call(e)==='[object Error]','newTarget custom prototype');
Target.prototype=null;e=Reflect.construct(RangeError,['x'],Target);check(Object.getPrototypeOf(e)===RangeError.prototype,'newTarget fallback type');
e=caught(function(){null.property;});check(e instanceof TypeError&&Object.prototype.toString.call(e)==='[object Error]','engine generated error');
check(delete e.message&&!e.hasOwnProperty('message')&&e.message==='','engine message remains deleted');
e=new Error('x');gc();check(typeof e.stack==='string'&&typeof e.fileName==='string'&&typeof e.lineNumber==='number','native stack extensions retained');
print('ES6-ERROR-MODERN checks='+checks+' failures=0');
