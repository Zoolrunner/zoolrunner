/* Extended property operands and catch notes must execute and decompile.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
var entries=[], checks=0;
for(var i=0;i<66000;++i)entries.push('atom="reference'+i+'"');
var prefix='(function(){var atom;'+entries.join(';')+';';
function check(value,label){++checks;if(!value)throw Error('wide reference: '+label);}
function compile(source,edition){
    var saved=version();
    try{version(edition);return evaluate(source,'wide-reference');}
    finally{version(saved);}
}
var reference=prefix+
 'var order=[],o={},key={toString:function(){order.push("key");return "wide";}};'+
 'o[key]=(order.push("rhs"),7);'+
 'var rejected=false;try{null.lastProperty=(order.push("bad"),8);}catch(e){rejected=e instanceof TypeError;}'+
 'return rejected && o.wide===7 && order.join()==="key,rhs";})';
var original=compile(reference,2015);
check(original(),'reference execution');gc();
check(compile('('+original.toString()+')',2015)(),'reference decompilation');
var writes=prefix+
 'var calls=0,o={};function base(){++calls;return o;}'+
 'base().lastProperty=7;base().lastProperty+=3;'+
 'var result=o.lastProperty===10 && calls===2 && !o.hasOwnProperty("7");'+
 'try{throw 19;}catch(e){result=result && e===19;}return result;})';
var literals=prefix+
 'var o={lastData:13,get lastGetter(){return this.lastData;},set lastSetter(v){this.lastData=v;}};'+
 'var initial=o.lastGetter===13;o.lastSetter=17;return initial && o.lastGetter===17;})';
for(var e=0;e<2;++e){
    var edition=e?170:2015;
    original=compile(writes,edition);
    check(original(),'wide write execution '+edition);
    check(compile('('+original.toString()+')',edition)(),'wide write decompilation '+edition);
    original=compile(literals,edition);
    check(original(),'wide literal execution '+edition);
    check(compile('('+original.toString()+')',edition)(),'wide literal decompilation '+edition);
}
var accessors=prefix+
 'var o={};o.lastGetter getter = function(){return 11;};'+
 'o.lastSetter setter = function(v){this.saved=v;};o.lastSetter=17;'+
 'return o.lastGetter===11 && o.saved===17;})';
original=compile(accessors,170);
check(original(),'legacy accessor assignment');
check(compile('('+original.toString()+')',170)(),'legacy accessor decompilation');
print('ES6-ASSIGNMENT-REFERENCE-WIDE checks='+checks+' failures=0');
