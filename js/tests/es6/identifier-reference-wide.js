/* Wide identifier operations and unresolvable references.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
var checks=0,failures=0,entries=[],global=Function('return this')();
for(var i=0;i<66000;++i)entries.push('atom="identifier'+i+'"');
var filler='var atom;'+entries.join(';')+';';
var cases=[
    ['read','try{referenceWideProbe;}catch(e){return e instanceof ReferenceError;}return false;'],
    ['typeof','return typeof referenceWideProbe==="undefined";'],
    ['typeof comma','try{typeof (0,referenceWideProbe);}catch(e){return e instanceof ReferenceError;}return false;'],
    ['delete','return delete referenceWideProbe;'],
    ['increment','try{referenceWideProbe++;}catch(e){return e instanceof ReferenceError;}return false;'],
    ['compound read before rhs','var called=false;try{referenceWideProbe+=(called=true,1);}catch(e){return e instanceof ReferenceError && !called;}return false;'],
    ['strict unresolved','var global=Function("return this")();try{referenceWideProbe=(global.referenceWideProbe=1,2);}catch(e){return e instanceof ReferenceError && global.referenceWideProbe===1;}return false;','"use strict";']
];
function check(name,fn){
    ++checks;delete global.referenceWideProbe;
    try{if(fn()===true)return;}catch(e){print('DETAIL '+name+': '+e);}
    finally{delete global.referenceWideProbe;}
    ++failures;print('FAIL wide identifier: '+name);
}
for(var c=0;c<cases.length;++c){
    var spec=cases[c],source='(function(){'+(spec[2]||'')+filler+spec[1]+'})';
    var original=eval(source);
    check(spec[0],original);
    var restored=eval('('+original.toString()+')');
    check(spec[0]+' decompiled',restored);
}
print('ES6-IDENTIFIER-REFERENCE-WIDE checks='+checks+' failures='+failures);
if(failures)throw Error('wide identifier reference failures: '+failures);
