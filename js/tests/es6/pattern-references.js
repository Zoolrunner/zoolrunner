/* ES2015 pattern regression. MPL 1.1/GPL 2.0/LGPL 2.1. */
(function(){
var checks=0;function check(x,s){++checks;if(!x)throw Error(s)}
function throws(kind,fn,s){var e;try{fn()}catch(x){e=x}check(e instanceof kind,s)}
var events=[],target={},source={get x(){events.push('get');gc();return undefined}};
function destination(){events.push('target');gc();return target}
function key(){events.push('key');return 'y'}
function init(){events.push('init');gc();return 7}
({x:destination()[key()]=init()}=source);
check(events.join()==='target,key,get,init'&&target.y===7,'reference before getter/default');
events=[];({[(events.push('source-key'),'x')]:destination()[key()]=init()}=source);
check(events.join()==='source-key,target,key,get,init','computed source key first');
events=[];throws(TypeError,function(){({x:null.y}=source)},'null target');check(events.length===0,'target error precedes getter');
events=[];var primitiveKey={};primitiveKey[Symbol.toPrimitive]=function(){events.push('coerce');gc();return 'y'};
({x:target[primitiveKey]=init()}=source);check(events.join()==='coerce,get,init','target key conversion before getter');
throws(ReferenceError,function(){'use strict';({x:neverDeclaredPatternReference}={x:1})},'strict unresolvable');
var globalName='outer',environment={globalName:'inner'};
source={get x(){delete environment.globalName;return 9}};
with(environment){({x:globalName}=source)}
check(environment.globalName===9&&globalName==='outer','captured object binding');
var original={},replacement={},holder=original;source={get x(){holder=replacement;return 5}};
({x:holder.y}=source);check(original.y===5&&!('y' in replacement),'captured property base');
var receiver;Object.defineProperty(Number.prototype,'patternSetter',{configurable:true,set:function(v){'use strict';receiver=[typeof this,this,v]}});
try{({x:(3).patternSetter}={x:8});check(receiver.join()==='number,3,8','primitive setter receiver')}
finally{delete Number.prototype.patternSetter}
throws(TypeError,function(){'use strict';({x:(3).newProperty}={x:8})},'strict primitive write');
function f(o,d){({x:d.y=4}=o);return d.y}
function g(o,d,k){({[k]:d[k]=5}=o);return d[k]}
function h(o){({x:globalName=6}=o);return globalName}
[f,g,h].forEach(function(f){var restored=eval('('+f.toString()+')');check(restored({}, {},'x')===f({}, {},'x'),'roundtrip '+f.name)});
function* suspended(o,d){({x:d.y=yield 3}=o);return d.y}
var iterator=suspended({},target);check(iterator.next().value===3&&iterator.next(11).value===11,'yield default reference');
var restored=eval('('+suspended.toString()+')');iterator=restored({},target);check(iterator.next().value===3&&iterator.next(12).value===12,'yield source roundtrip');
var parts=[],calls=0;function tick(){++calls;return 0}
for(var n=0;n<6000;++n)parts.push('tick()');parts.push('"x"');
var wide=eval('(function(o,d,k){({[k?('+parts.join(',')+'):"y"]:d.z=7}=o);return d.z})');
check(wide({},target,true)===7&&calls===6000,'wide source key');calls=0;
check(eval('('+wide.toString()+')')({},target,true)===7&&calls===6000,'wide source roundtrip');
print('ES6-PATTERN-REFERENCES PASS checks='+checks);
})();
