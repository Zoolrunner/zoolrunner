var checks=0;
function check(v,m){checks++;if(!v)throw Error(m);}
function caught(fn){try{fn();}catch(e){return e;}return null;}
var log=[],target={binding:7}, proxy=new Proxy(target,{
 has(t,k){log.push('has:'+String(k));return Reflect.has(t,k);},
 get(t,k,r){log.push('get:'+String(k));return Reflect.get(t,k,r);},
 set(t,k,v,r){log.push('set:'+String(k));return Reflect.set(t,k,v,r);}
});
function read(){with(proxy){return binding;}}
check(read()===7,'with binding value');
check(log.join('|')==='has:binding|get:Symbol(Symbol.unscopables)|has:binding|get:binding','HasBinding then GetBindingValue');
log=[];function increment(){with(proxy){binding+=2;}}
increment();check(target.binding===9,'compound assignment');
check(log.join('|')==='has:binding|get:Symbol(Symbol.unscopables)|has:binding|get:binding|set:binding','one read existence check; original ES2015 Set does not recheck');
log=[];target.method=function(){'use strict';return this;};
function invoke(){with(proxy){return method();}}
check(invoke()===proxy,'implicit object receiver');
check(log.join('|')==='has:method|get:Symbol(Symbol.unscopables)|has:method|get:method','method lookup order');
var calls=0,env={binding:1};Object.defineProperty(env,Symbol.unscopables,{get:function(){calls++;delete env.binding;gc();return null;}});
function deleted(){with(env){return binding;}}
check(deleted()===undefined&&calls===1,'deleted non-strict binding');
env.binding=1;var strictRead;with(env){strictRead=function(){'use strict';return binding;};}
check(caught(strictRead) instanceof ReferenceError&&calls===2,'deleted strict binding');
var hasCalls=0,getCalls=0;
proxy=new Proxy({binding:1},{has(t,k){if(k==='binding')return ++hasCalls===1;return Reflect.has(t,k);},get(t,k,r){if(k==='binding')getCalls++;return Reflect.get(t,k,r);}});
check(read()===undefined&&hasCalls===2&&getCalls===0,'second has false prevents get');
var marker={};hasCalls=0;
proxy=new Proxy({binding:1},{has(t,k){if(k==='binding'&&++hasCalls===2){gc();throw marker;}return Reflect.has(t,k);}});
check(caught(read)===marker&&hasCalls===2,'second has abrupt completion');
log=[];proxy=new Proxy({binding:8},{has(t,k){log.push('has');return Reflect.has(t,k);},get(t,k,r){log.push('get');return Reflect.get(t,k,r);}});
var saved=version();try{version(170);var legacy=evaluate('(function(proxy){with(proxy){return binding;}})','legacy-with-read');check(legacy(proxy)===8,'legacy value');check(log.join() === 'has,get','legacy lookup count');}finally{version(saved);}
env.binding=1;var strictTypeof;
with(env){strictTypeof=function(){'use strict';return typeof binding;};}
check(caught(strictTypeof) instanceof ReferenceError,'typeof cannot suppress failure of a resolved binding');
print('WITH-BINDING-VALUE checks='+checks+' failures=0');
