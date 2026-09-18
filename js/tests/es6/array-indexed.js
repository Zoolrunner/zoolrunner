/* ES2015 large indexed operations and observable mutation ordering.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
var checks=0;
function check(v,n){++checks;if(!v)throw Error('Array indexed: '+n);}
function caught(f){try{f();}catch(e){return e;}return null;}
var names=['push','pop','shift','unshift','reverse','slice','indexOf','lastIndexOf','splice'],i,obj,a,r,log,marker={};
for(i=0;i<names.length;i++){
 check(caught((function(n){return function(){Array.prototype[n].call(null);};})(names[i])) instanceof TypeError,'null '+names[i]);
 obj={length:-1};r=Array.prototype[names[i]].call(obj);
 check((names[i]==='slice'||names[i]==='splice')?r.length===0:(names[i]==='indexOf'||names[i]==='lastIndexOf')?r===-1:names[i]==='reverse'?r===obj:obj.length===0,'negative length '+names[i]);
}
obj={length:4294967296};check(Array.prototype.push.call(obj,'x')===4294967297&&obj[4294967296]==='x','push large');
check(Array.prototype.pop.call(obj)==='x'&&obj.length===4294967296&&!(4294967296 in obj),'pop large');
obj={length:9007199254740991};check(caught(function(){Array.prototype.push.call(obj,1);}) instanceof TypeError&&obj.length===9007199254740991,'push safe limit');
check(caught(function(){Array.prototype.unshift.call(obj,1);}) instanceof TypeError,'unshift safe limit');
obj={length:4294967297,4294967296:'x'};check(Array.prototype.indexOf.call(obj,'x',4294967296)===4294967296,'indexOf large');
check(Array.prototype.lastIndexOf.call(obj,'x')===4294967296,'lastIndexOf large');
check(Array.prototype.indexOf.call(obj,'x',Infinity)===-1&&Array.prototype.lastIndexOf.call(obj,'x',-Infinity)===-1,'infinite starts');
check([NaN].indexOf(NaN)===-1&&[0].lastIndexOf(-0)===0&&[,].indexOf(undefined)===-1,'strict equality and holes');
check([1,2,1].lastIndexOf(1,undefined)===0&&[1,2,1].lastIndexOf(1)===2,'omitted last start');
obj={length:0};check(Array.prototype.indexOf.call(obj,1,{valueOf:function(){throw marker;}})===-1,'zero length skips start');
obj=Object.create({0:3});obj.length=1;check(Array.prototype.indexOf.call(obj,3)===0,'inherited search value');
log=[];obj=new Proxy({length:1},{get:function(t,k){log.push('get:'+k);gc();return t[k];},deleteProperty:function(t,k){log.push('delete:'+k);return true;},set:function(t,k,v){log.push('set:'+k);t[k]=v;return true;},has:function(){throw marker;}});
check(Array.prototype.pop.call(obj)===undefined&&log.join()==='get:length,get:0,delete:0,set:length','pop gets and deletes holes without has');
obj={length:1};Object.defineProperty(obj,'0',{value:4,configurable:false});check(caught(function(){Array.prototype.pop.call(obj);}) instanceof TypeError&&obj.length===1,'pop deletion failure');
obj={};Object.defineProperty(obj,'length',{value:0,writable:false});check(caught(function(){Array.prototype.pop.call(obj);}) instanceof TypeError,'empty pop throws on length');
check(caught(function(){Array.prototype.push.call(obj);}) instanceof TypeError,'empty push throws on length');
a=[1,,3];check(a.shift()===1&&a.length===2&&!(0 in a)&&a[1]===3,'shift holes');
a=[1,,3];check(a.unshift('a','b')===5&&a[0]==='a'&&a[1]==='b'&&a[2]===1&&!(3 in a)&&a[4]===3,'unshift holes');
a=[1,,3,,];check(a.reverse()===a&&a.length===4&&!(0 in a)&&a[1]===3&&!(2 in a)&&a[3]===1,'reverse holes');
log=[];obj=new Proxy({length:2,0:'a'},{get:function(t,k){log.push('get:'+k);gc();return t[k];},has:function(t,k){log.push('has:'+k);return k in t;},set:function(t,k,v){log.push('set:'+k);t[k]=v;return true;},deleteProperty:function(t,k){log.push('delete:'+k);delete t[k];return true;}});
check(Array.prototype.reverse.call(obj)===obj&&log.join()==='get:length,has:0,get:0,has:1,delete:0,set:1','reverse trap order');
log=[];obj=new Proxy({length:2,1:'b'},{get:function(t,k){log.push('get:'+k);return t[k];},has:function(t,k){log.push('has:'+k);return k in t;},set:function(t,k,v){log.push('set:'+k);return true;},deleteProperty:function(t,k){log.push('delete:'+k);return true;}});
Array.prototype.reverse.call(obj);check(log.join()==='get:length,has:0,has:1,get:1,set:0,delete:1','reverse upper-only order');
var output={};a=[1,,3];a.constructor={};a.constructor[Symbol.species]=function(len){check(len===3,'slice species length');gc();return output;};
r=a.slice();check(r===output&&r.length===3&&r[0]===1&&!(1 in r)&&r[2]===3,'slice custom species');
check(Array.prototype.slice.call({length:4294967297,4294967296:'x'},4294967296)[0]==='x','slice large start');
check(caught(function(){Array.prototype.slice.call({length:4294967296});}) instanceof RangeError,'slice default length bound');
a=[1];a.constructor={};a.constructor[Symbol.species]=function(){return Object.preventExtensions({});};check(caught(function(){a.slice();}) instanceof TypeError,'slice nonextensible result');
a=[1,2,3];log=[];Object.defineProperty(a,'constructor',{get:function(){log.push('constructor');return Array;}});
r=a.slice({valueOf:function(){log.push('start');return 1;}},{valueOf:function(){log.push('end');return 3;}});check(log.join()==='start,end,constructor'&&r.join()==='2,3','slice conversion order');
log=[];obj=new Proxy({length:2,1:'x'},{get:function(t,k){log.push('get:'+k);gc();return t[k];},has:function(t,k){log.push('has:'+k);return k in t;}});
check(Array.prototype.indexOf.call(obj,'x')===1&&log.join()==='get:length,has:0,has:1,get:1','search proxy ordering');
obj={length:0};Object.defineProperty(obj,'0',{set:function(){gc();throw marker;}});check(caught(function(){Array.prototype.push.call(obj,{});})===marker&&obj.length===0,'push setter error');
obj={length:-0};Array.prototype.pop.call(obj);check(1/obj.length===Infinity,'pop positive zero length');
obj={length:-0};Array.prototype.shift.call(obj);check(1/obj.length===Infinity,'shift positive zero length');
check(1/[true].indexOf(true,-0)===Infinity&&1/[true].lastIndexOf(true,-0)===Infinity,'search positive zero index');
a=[1,2,3];r=a.splice();check(r.length===0&&a.join()==='1,2,3','splice no arguments');
r=a.splice(1);check(r.join()==='2,3'&&a.join()==='1','splice omitted delete count');
a=[1,2,3];r=a.splice(1,undefined,4);check(r.length===0&&a.join()==='1,4,2,3','splice explicit undefined count');
a=[1,,3,4];r=a.splice(1,2,'x');check(r.length===2&&!(0 in r)&&r[1]===3&&a.join()==='1,x,4','splice left movement holes');
a=[1,,3];r=a.splice(0,1,'x','y');check(r[0]===1&&a.length===4&&a[0]==='x'&&a[1]==='y'&&!(2 in a)&&a[3]===3,'splice right movement holes');
a=[1,2];output={};a.constructor={};a.constructor[Symbol.species]=function(len){check(len===1,'splice species length');gc();return output;};r=a.splice(0,1);check(r===output&&r[0]===1&&r.length===1&&a.join()==='2','splice custom species');
obj={length:4294967297,4294967296:'x'};r=Array.prototype.splice.call(obj,4294967296,1,'y');check(r[0]==='x'&&obj[4294967296]==='y'&&obj.length===4294967297,'splice large index');
obj={length:9007199254740991};check(caught(function(){Array.prototype.splice.call(obj,0,0,1);}) instanceof TypeError,'splice safe length limit');
a=[1];output={};Object.defineProperty(output,'length',{set:function(){throw marker;}});a.constructor={};a.constructor[Symbol.species]=function(){return output;};check(caught(function(){a.splice(0,1);})===marker&&a.length===1&&a[0]===1,'splice result length before mutation');
print('ES6-ARRAY-INDEXED checks='+checks+' failures=0');
