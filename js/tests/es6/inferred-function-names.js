/* ES2015 inferred names are metadata, never new lexical self-bindings. */
var checks=0;
function check(value,message){checks++;if(!value)throw Error('FAIL inferred name: '+message);}
function modern(source){var old=version();try{version(2015);return evaluate(source,'inferred-function-names');}finally{version(old);}}
function metadata(fn,name){
    var d=Object.getOwnPropertyDescriptor(fn,'name');
    check(d && d.value===name,'name '+name);
    check(d && !d.writable && !d.enumerable && d.configurable,'descriptor '+name);
}
var bindings=['var','let','const'];
for(var i=0;i<bindings.length;i++){
    metadata(modern('(function(){'+bindings[i]+' inferred=function(){};return inferred;})()'),'inferred');
    metadata(modern('(function(){'+bindings[i]+' inferred=(function(){});return inferred;})()'),'inferred');
    metadata(modern('(function(){'+bindings[i]+' inferred=function explicit(){};return inferred;})()'),'explicit');
    check(!Object.prototype.hasOwnProperty.call(modern('(function(){'+bindings[i]+' inferred=(0,function(){});return inferred;})()'),'name'),'comma has no inferred name '+bindings[i]);
}
metadata(modern('(function(){var assigned;assigned=function(){};return assigned;})()'),'assigned');
check(!Object.prototype.hasOwnProperty.call(modern('(function(){var assigned;(assigned)=(function(){});return assigned;})()'),'name'),'parenthesized target has no inferred name');
check(!Object.prototype.hasOwnProperty.call(modern('({__proto__:function(){}}).__proto__'),'name'),'prototype setter has no inferred name');
var properties=modern('({plain:function(){},"with space":function(){},0:function(){},2.5:function(){},"":function(){},explicit:function own(){},get value(){return 1;},set value(v){}})');
metadata(modern('({\"\\u00e9\\u6f22\":function(){}})')[String.fromCharCode(0xe9,0x6f22)],String.fromCharCode(0xe9,0x6f22));
metadata(properties.plain,'plain');metadata(properties['with space'],'with space');metadata(properties[0],'0');metadata(properties[2.5],'2.5');metadata(properties[''],'');metadata(properties.explicit,'own');
var d=Object.getOwnPropertyDescriptor(properties,'value');metadata(d.get,'get value');metadata(d.set,'set value');
check(!Object.prototype.hasOwnProperty.call(d.get,'prototype'),'getter has no prototype');
check(!Object.prototype.hasOwnProperty.call(d.set,'prototype'),'setter has no prototype');
var constructRejected=false;try{new d.get();}catch(e){constructRejected=e instanceof TypeError;}check(constructRejected,'getter not constructible');
var noInference=modern('(function(){var o={};o.x=function(){};o["y"]=function(){};return [o.x,o.y,true?function(){}:null,(function(){})||null];})()');
for(i=0;i<noInference.length;i++)check(!Object.prototype.hasOwnProperty.call(noInference[i],'name'),'no inference '+i);
var uninferredInitializers=['true ? function(){} : null', 'false || function(){}', '(0,function(){})'];
for(i=0;i<uninferredInitializers.length;i++){
 check(!Object.prototype.hasOwnProperty.call(modern('(function(){var target='+uninferredInitializers[i]+';return target;})()'),'name'),'compound initializer '+i);
 check(!Object.prototype.hasOwnProperty.call(modern('(function(){var target;target='+uninferredInitializers[i]+';return target;})()'),'name'),'compound assignment RHS '+i);
 var anonymousFactory=modern('(function(){var target='+uninferredInitializers[i]+';return target;})');
 check(!Object.prototype.hasOwnProperty.call(modern('('+anonymousFactory.toString()+')')(),'name'),'compound initializer decompilation '+i);

}
check(modern('(function(){var inferred=function(){return inferred;};var original=inferred;inferred=42;return original()===42;})()'),'inferred name does not capture itself');
check(modern('(function(){var outer=17;var o={outer:function(){return outer;}};return o.outer()===17;})()'),'property name is not a binding');
check(modern('(function(){var eval=function(){"use strict";return 1;};return eval();})()')===1,'inferred eval is not a strict binding');
var objectFactory=modern('(function(){return {get "odd key"(){return 42;},get default(){return 7;},set 12(v){}};})');
var roundObject=modern('('+objectFactory.toString()+')')();
check(roundObject['odd key']===42 && roundObject.default===7,'quoted/keyword accessor decompilation');
metadata(Object.getOwnPropertyDescriptor(roundObject,'12').set,'set 12');
var parenFactory=modern('(function(){var target;(target)=function(){};return target;})');
check(!Object.prototype.hasOwnProperty.call(modern('('+parenFactory.toString()+')')(),'name'),'parenthesized target decompilation');
var protoFactory=modern('(function(){return {__proto__:function(){}};})');
check(!Object.prototype.hasOwnProperty.call(modern('('+protoFactory.toString()+')')().__proto__,'name'),'prototype setter decompilation');
var factory=modern('(function(){var local=function(){return /x/.test("x");};return local;})');
var first=factory(),second=factory();metadata(first,'local');metadata(second,'local');
check(first!==second && first() && second(),'clones and regexp slots');
delete first.name;check(!Object.prototype.hasOwnProperty.call(first,'name') && second.name==='local','metadata deletion isolated');
gc();metadata(factory(),'local');metadata(d.get,'get value');metadata(d.set,'set value');
metadata(modern('('+factory.toString()+')')(),'local');
check(!/function\s+local\s*\(/.test(second.toString()),'decompilation preserves anonymous syntax');
check(modern('(function(){var o={get "odd key"(){return 1;},set "odd key"(x){}};return Object.getOwnPropertyDescriptor(o,"odd key").get.name;})()')==='get odd key','quoted accessor name');
for(i=0;i<2;i++){
 var old=version();version(i?170:0);
 var legacy=evaluate('(function(){var inferred=function(){};return inferred;})()','legacy-inferred-name');version(old);
 check(legacy.name==='','legacy name unchanged '+i);
 version(i?170:0);
 var legacyGetter=evaluate('Object.getOwnPropertyDescriptor({get value(){return 1;}},"value").get','legacy-accessor');version(old);
 check(Object.prototype.hasOwnProperty.call(legacyGetter,'prototype'),'legacy getter prototype '+i);
 check(typeof new legacyGetter()==='object','legacy getter construction '+i);
}
print('ES6-INFERRED-NAMES checks='+checks+' failures=0');
