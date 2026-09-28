/* Assignment values survive scripted setters and their callbacks. */
var setterChecks=0;
function verify(v,label){setterChecks++;if(!v)throw Error(label)}
var written, object={get x(){return written},set x(v){written=v;return 99}};
verify((object.x=3)===3&&written===3,'named assignment');
verify((object['x']=4)===4&&written===4,'computed assignment');
verify((object.x+=2)===6&&written===6,'compound assignment');
verify(object.x++===6&&written===7,'postfix update');
verify(++object.x===8&&written===8,'prefix update');
var value={id:12};verify((object.x=value)===value&&written===value,'object identity');
var inherited=Object.create(object);verify((inherited.x=13)===13&&written===13,'inherited setter');
Object.defineProperty(Number.prototype,'setterResultProbe',{set:function(v){'use strict';written=[this,v];return 91},configurable:true});
verify(((3).setterResultProbe=14)===14&&written[0]===3&&written[1]===14,'primitive receiver');
delete Number.prototype.setterResultProbe;
var noReturn={set x(v){written=v}};verify((noReturn.x=15)===15&&written===15,'implicit return');
var returnedObject={set x(v){return {different:true}}};verify((returnedObject.x=value)===value,'different returned object');
var thrown={set x(v){throw value}},caught=false;try{thrown.x=1}catch(e){caught=e===value}verify(caught,'throw propagation');
var changed={set x(v){Object.defineProperty(this,'x',{value:22,writable:true,configurable:true});return 23}};verify((changed.x=21)===21&&changed.x===22,'descriptor replacement');
var nested={set x(v){object.x=31;return 32}};verify((nested.x=30)===30&&written===31,'reentrant setter');
(function(){with(object){verify((x=34)===34&&written===34,'unqualified assignment')}})();
verify((object.x=-0)===0&&1/written===-Infinity,'negative zero');
verify((object.x=undefined)===undefined&&written===undefined,'undefined assignment');
print('SETTER-RESULT PASS checks='+setterChecks);
