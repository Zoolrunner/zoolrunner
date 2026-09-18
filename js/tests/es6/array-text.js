/* Modern Array string conversion, sorting and callback/GC behavior.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
var checks=0;
function check(v,n){++checks;if(!v)throw Error('Array text/sort: '+n);}
function caught(f){try{f();}catch(e){return e;}return null;}
var names=['join','toLocaleString','toString','sort'],i,a,r,obj,log,marker={};
for(i=0;i<names.length;i++)check(caught((function(n){return function(){Array.prototype[n].call(null);};})(names[i])) instanceof TypeError,'null '+names[i]);
obj={length:-1,0:'x'};check(Array.prototype.join.call(obj)===''&&Array.prototype.toLocaleString.call(obj)===''&&Array.prototype.sort.call(obj)===obj&&obj[0]==='x','negative length');
log=[];obj={get length(){log.push('length');return 2;},get 0(){log.push('get0');gc();return 'a';},get 1(){log.push('get1');return 'b';}};
r=Array.prototype.join.call(obj,{toString:function(){log.push('separator');gc();return '|';}});check(r==='a|b'&&log.join()==='length,separator,get0,get1','length before separator and elements');
check(caught(function(){Array.prototype.join.call({length:0},{toString:function(){throw marker;}});})===marker,'empty join still converts separator');
log=[];obj=new Proxy({length:2,1:'x'},{get:function(t,k){log.push('get:'+k);gc();return t[k];},has:function(){throw marker;},ownKeys:function(){throw marker;}});
check(Array.prototype.join.call(obj)===',x'&&log.join()==='get:length,get:0,get:1','join only Get no enumeration');
check([null,undefined,,3].join('|')==='|||3','empty pieces');
a=[1];a.push(a,2);check(a.join()==='1,,2','join self cycle');
var b=[a];a[1]=b;check(a.join()==='1,,2','join indirect cycle');
a=[{toString:function(){gc();throw marker;}}];check(caught(function(){a.join();})===marker,'join conversion throw');a[0]='ok';check(a.join()==='ok','join guard unwound');
check([1,2].join('\u0000')==='1\u00002','embedded NUL separator');
check(caught(function(){[Symbol()].join();}) instanceof TypeError,'join Symbol conversion');
var value={};obj={join:function(){'use strict';check(this===obj&&arguments.length===0,'toString join call receiver');gc();return value;}};check(Array.prototype.toString.call(obj)===value,'toString returns raw join result');
obj={get join(){throw marker;}};check(caught(function(){Array.prototype.toString.call(obj);})===marker,'toString join getter error');
var old=Object.prototype.toString;Object.prototype.toString=function(){throw marker;};
try{check(Array.prototype.toString.call({join:3})==='[object Object]','toString intrinsic fallback');}finally{Object.prototype.toString=old;}
obj={join:null};obj[Symbol.toStringTag]='Tagged';check(Array.prototype.toString.call(obj)==='[object Tagged]','toString fallback tag');
check(caught(function(){[{toLocaleString:3}].toLocaleString();}) instanceof TypeError,'locale invalid method');
obj={toLocaleString:function(){'use strict';check(this===obj&&arguments.length===0,'locale raw receiver and zero args');gc();return {toString:function(){return 'X';}};}};check([obj,null,undefined].toLocaleString()==='X,,','locale return conversion');
var descriptor=Object.getOwnPropertyDescriptor(Number.prototype,'toLocaleString');log=[];
Object.defineProperty(Number.prototype,'toLocaleString',{configurable:true,get:function(){'use strict';log.push(typeof this+':'+this);return function(){'use strict';log.push(typeof this+':'+this);gc();return 'N';};}});
try{check([7].toLocaleString()==='N'&&log.join()==='number:7,number:7','locale primitive getter and call receivers');}finally{Object.defineProperty(Number.prototype,'toLocaleString',descriptor);}
a=[];a[0]=a;check(a.toLocaleString()==='','locale cycle');
log=[];obj={get length(){log.push('length');throw marker;}};check(caught(function(){Array.prototype.sort.call(obj,3);})===marker&&log.join()==='length','sort length before callback validation');
check(caught(function(){[].sort(3);}) instanceof TypeError,'sort rejects comparator');
a=[3,undefined,,1,2,,];r=a.sort(function(x,y){'use strict';if(this!==undefined)throw Error('sort callback receiver');gc();return x-y;});check(r===a&&a[0]===1&&a[1]===2&&a[2]===3&&a[3]===undefined&&(3 in a)&&!(4 in a)&&!(5 in a),'sort values undefined holes');
a=[];for(i=300;i>0;--i)a.push({n:i,toString:function(){gc();return ('000'+this.n).slice(-3);}});a.sort();check(a[0].n===1&&a[299].n===300,'sort vector growth and conversion GC');
a=[2,1];check(caught(function(){a.sort(function(){gc();throw marker;});})===marker,'sort callback throw');check(a.sort().join()==='1,2','sort recovery');
obj={length:2,0:2,1:1};Object.defineProperty(obj,'0',{value:2,writable:false});check(caught(function(){Array.prototype.sort.call(obj);}) instanceof TypeError,'sort throwing write');
obj={length:3,0:1};Object.defineProperty(obj,'2',{value:undefined,configurable:false});check(caught(function(){Array.prototype.sort.call(obj);}) instanceof TypeError,'sort throwing delete');
var conversions=0;obj={toString:function(){++conversions;gc();return '';}};[obj,obj].sort();check(conversions>=2,'sort identical objects still convert');
var booleanText=Object.getOwnPropertyDescriptor(Boolean.prototype,'toString');
Object.defineProperty(Boolean.prototype,'toString',{configurable:true,value:function(){'use strict';return typeof this;}});
try{check([true,false].toLocaleString()==='boolean,boolean','inherited Object locale raw call');}finally{Object.defineProperty(Boolean.prototype,'toString',booleanText);}
Object.defineProperty(Boolean.prototype,'toString',{configurable:true,get:function(){'use strict';var type=typeof this;return function(){return type;};}});
try{check([true,false].toLocaleString()==='boolean,boolean','inherited Object locale raw getter');}finally{Object.defineProperty(Boolean.prototype,'toString',booleanText);}
print('ES6-ARRAY-TEXT checks='+checks+' failures=0');
