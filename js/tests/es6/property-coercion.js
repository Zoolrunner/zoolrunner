/* MPL 1.1/GPL 2.0/LGPL 2.1. */
var checks=0;function check(v,m){checks++;if(!v)throw Error(m);}
var queries=[Object.prototype.hasOwnProperty,Object.prototype.propertyIsEnumerable];
queries.forEach(function(query){
  [null,undefined].forEach(function(receiver){
    var token={},calls=0,hint;
    var key={[Symbol.toPrimitive]:function(h){calls++;hint=h;gc();throw token;}};
    var error;try{query.call(receiver,key);}catch(e){error=e;}
    check(error===token&&calls===1&&hint==='string','key conversion before receiver');
    calls=0;key={[Symbol.toPrimitive]:function(){calls++;return 'x';}};
    error=null;try{query.call(receiver,key);}catch(e){error=e;}
    check(error instanceof TypeError&&calls===1,'receiver rejection after key conversion');
  });
  var symbol=Symbol(),target={},calls=0;target[symbol]=9;
  check(query.call(target,{[Symbol.toPrimitive]:function(){calls++;gc();return symbol;}})&&calls===1,'symbol key conversion once');
  check(query.call('abc','0'),'primitive string receiver');
  target={x:1,valueOf:function(){throw Error('unexpected receiver conversion');}};
  check(query.call(target,'x'),'preserve object receiver');
  var events=[],proxy=new Proxy({x:1},{getOwnPropertyDescriptor:function(t,k){events.push(k);return Object.getOwnPropertyDescriptor(t,k);}});
  check(query.call(proxy,{toString:function(){events.push('key');return 'x';}})&&events.join(',')==='key,x','key before proxy own descriptor');
});
var array,value,calls,error;
array=[1,2,3];calls=0;value={[Symbol.toPrimitive]:function(){calls++;Object.defineProperty(array,'length',{writable:false});return 0;}};
error=null;try{(function(){'use strict';array.length=value;})();}catch(e){error=e;}
check(error instanceof TypeError&&calls===2&&array.length===3&&array[2]===3,'strict shrink rechecks writable');
array=[1,2,3];calls=0;var assigned=(array.length=value);
check(assigned===value&&calls===2&&array.length===3&&array[2]===3,'sloppy shrink leaves array unchanged');
array=[1,2,3];calls=0;
check(!Reflect.set(array,'length',value)&&calls===2&&array.length===3,'Reflect set rejects after conversion');
array=[1,2];calls=0;value={valueOf:function(){calls++;if(calls===2)Object.defineProperty(array,'length',{writable:false});return 2;}};
error=null;try{Object.defineProperty(array,'length',{value:value,writable:true});}catch(e){error=e;}
check(error instanceof TypeError&&calls===2&&!Object.getOwnPropertyDescriptor(array,'length').writable,'define length uses refreshed descriptor');
array=[1,2];calls=0;
check(!Reflect.defineProperty(array,'length',{value:value,writable:true})&&calls===2,'Reflect define refreshed descriptor');
array=[1,2];calls=0;
check(Reflect.defineProperty(array,'length',{value:value})&&calls===2&&array.length===2,'same length may preserve readonly descriptor');
print('PROPERTY-COERCION checks='+checks+' failures=0');
