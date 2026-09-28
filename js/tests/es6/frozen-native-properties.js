var checks=0;
function same(a,b,label){++checks;if(a!==b)throw Error(label);}
function desc(o,k){return Object.getOwnPropertyDescriptor(o,k);}
function f(){return [desc(f,'arguments').value,desc(f,'caller').value];}
Object.defineProperty(f,'arguments',{writable:false,configurable:false});
Object.defineProperty(f,'caller',{writable:false,configurable:false});
var args=desc(f,'arguments').value,caller=desc(f,'caller').value;
function callF(){return f(7);}
var got=callF();same(got[0],args,'frozen arguments descriptor');same(got[1],caller,'frozen caller descriptor');
same(f.arguments,args,'frozen arguments read');same(f.caller,caller,'frozen caller read');
function live(a){if(!Object.prototype.hasOwnProperty.call(live,"arguments"))Object.defineProperty(live,"arguments",{value:live.arguments,writable:true,configurable:false});Object.freeze(live);return live.arguments;}
var saved=live(11);same(saved[0],11,'active arguments snapshot');gc();same(live.arguments,saved,'snapshot survives return/GC');same(live(22),saved,'subsequent activation cannot change value');same(saved[0],11,'detached argument value');
function configurable(){return configurable.arguments;}
Object.defineProperty(configurable,'arguments',{value:null,writable:false});
same(configurable(),null,'permanent function field becomes immutable');
/(x)(y)/.exec('xy');Object.defineProperty(RegExp,'$1',{configurable:false});
var capture=RegExp.$1;same(capture,'x','capture snapshot');
/(a)(b)/.exec('ab');gc();same(RegExp.$1,capture,'frozen capture read');same(desc(RegExp,'$1').value,capture,'frozen capture descriptor');same(RegExp.$2,'b','other captures stay live');
Object.defineProperty(RegExp,'$2',{enumerable:false});/(c)(d)/.exec('cd');same(RegExp.$2,'d','configurable readonly capture stays live');
Object.defineProperty(RegExp,'$2',{configurable:false});/(e)(f)/.exec('ef');same(RegExp.$2,'d','second capture becomes immutable');
var callback=0,obj={};Object.defineProperty(obj,'x',{get:function(){++callback;return callback;},configurable:false});Object.freeze(obj);same(obj.x,1,'accessor remains live');same(obj.x,2,'frozen accessor not snapshotted');
var array=[1,2];Object.defineProperty(array,'length',{writable:false});same(array.length,2,'array length hook retained');
print('FROZEN-NATIVE-PROPERTIES checks='+checks+' failures=0');
