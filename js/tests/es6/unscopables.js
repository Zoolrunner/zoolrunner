/* ES2015 with filtering, mutation during lookup and classic coexistence. */
var checks=0;
function check(ok,label){++checks;if(!ok)throw Error('FAIL unscopables: '+label);}
function caught(fn){try{fn();}catch(e){return e;}return null;}
var d=Object.getOwnPropertyDescriptor(Array.prototype,Symbol.unscopables),table=d.value;
check(!d.writable&&!d.enumerable&&d.configurable,'array descriptor');
check(Object.getPrototypeOf(table)===null,'null prototype');
check(Object.keys(table).join()==='copyWithin,entries,fill,find,findIndex,keys,values','seven ES2015 names');
for(var i=0,names=Object.keys(table);i<names.length;++i){d=Object.getOwnPropertyDescriptor(table,names[i]);check(d.value===true&&d.writable&&d.enumerable&&d.configurable,'entry '+names[i]);}
var x=1,env={x:2},seen;
env[Symbol.unscopables]={x:true};with(env){seen=x;}check(seen===1,'blocked read');
env[Symbol.unscopables].x=false;with(env){seen=x;}check(seen===2,'unblocked read');
env[Symbol.unscopables].x=true;with(env){x=3;}check(x===3&&env.x===2,'blocked assignment');
with(env){++x;}check(x===4&&env.x===2,'blocked increment');
env[Symbol.unscopables].x=false;with(env){++x;}check(x===4&&env.x===3,'unblocked increment');
var inherited={x:true};env[Symbol.unscopables]=Object.create(inherited);with(env){seen=x;}check(seen===4,'inherited block');
var values=[null,undefined,false,7,'x',Symbol()];
for(i=0;i<values.length;++i){env[Symbol.unscopables]=values[i];with(env){seen=x;}check(seen===3,'nonobject table '+i);}
var calls=0,marker={};env={};Object.defineProperty(env,Symbol.unscopables,{get:function(){++calls;throw marker;}});
with(env){seen=x;}check(seen===4&&calls===0,'missing binding skips getter');
env.x=8;check(caught(function(){with(env){return x;}})===marker&&calls===1,'table getter failure');
env={x:8};table={};env[Symbol.unscopables]=table;
Object.defineProperty(table,'x',{get:function(){gc();throw marker;}});
check(caught(function(){with(env){return x;}})===marker,'flag getter failure');
env={x:8};Object.defineProperty(env,Symbol.unscopables,{get:function(){delete env.x;gc();return null;}});
with(env){seen=x;}check(seen===undefined&&x===4,'deletion after HasProperty keeps binding');
env={x:8};Object.defineProperty(env,Symbol.unscopables,{get:function(){delete env.x;gc();return {x:true};}});
with(env){seen=x;}check(seen===4,'deleted but blocked binding falls outward');
env={x:8,method:function(){'use strict';gc();return this;}};
env[Symbol.unscopables]={x:true};with(env){seen=method();}check(seen===env,'implicit method receiver');
var closure;with(env){closure=function(){'use strict';return x;};}
check(closure()===4,'captured with environment');
env[Symbol.unscopables].x=false;check(closure()===8,'captured environment remains live');
var values='outer';with([]){seen=values;}check(seen==='outer','array values excluded');
var outer={x:11},inner={x:12};inner[Symbol.unscopables]={x:true};with(outer){with(inner){seen=x;}}check(seen===11,'nested filtering');
env={x:9};table={x:true};env[Symbol.unscopables]=table;
with(env){seen=delete x;}check(seen===false&&env.x===9,'blocked delete targets global var');
table.x=false;with(env){seen=delete x;}check(seen===true&&!env.hasOwnProperty('x'),'unblocked delete');
this[Symbol.unscopables]={x:true};check(x===4,'global environment ignored');delete this[Symbol.unscopables];
var once=true,nested;
env={x:7};table={};env[Symbol.unscopables]=table;
function resolveAgain(){with(env){return x;}}
Object.defineProperty(table,'x',{get:function(){if(once){once=false;nested=resolveAgain();}gc();return false;}});
check(resolveAgain()===7&&nested===7,'reentrant lookup');
env={x:6};Object.defineProperty(env,Symbol.unscopables,{get:function(){delete env.x;gc();return null;}});
with(env){x=10;}check(env.x===10&&x===4,'assignment after deletion retains binding');
print('ES6-UNSCOPABLES checks='+checks+' failures=0');
