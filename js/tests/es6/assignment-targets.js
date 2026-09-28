/* Original ES2015 assignment early errors and historical grammar isolation. */
var checks=0, assignmentEffects=0;
function check(ok,label){++checks;if(!ok)throw Error(label);}
function inEdition(edition,source){var saved=version();try{version(edition);return evaluate(source,'assignment-targets');}finally{version(saved);}}
var referenceErrors=[
 '({}) = 1;', '([]) = [];', '(({})) = 1;', '(([a])) = [1];',
 '(x - y) = 1;', '(!x) = 1;', '(x = y) = 1;',
 '() => ({}) = 1;', '() => ([]) = [];',
 '(x ? y : z) = 1;',
 'function f(){}; f() = 1;', 'function f(){}; (f()) += 1;',
 'function tag(){}; tag`` = 1;', 'function tag(){}; (tag``) *= 2;'
];
var syntaxErrors=[
 'x - y = 1;', 'x + y = 1;', '-x = 1;', '!x = 1;',
 '1 + 2 = 1;', '-1 = 1;', 'true && false = 1;',
 '++x = 1;', 'x++ = 1;', 'x && y = 1;', 'delete x.y = 1;',
 'function f(){}; for(f() in {}){}', 'function f(){}; for((f()) in {}){}',
 'function f(){}; for(f() of []){}', 'for(({}) in {}){}', 'for(([]) of []){}',
 '[({})]=[{}];', '[([])]=[[]];', '({x:({})}={x:{}});',
 '({x:([])}={x:[]});', '[...([a])]=[];', '[...({})]=[];',
 '[(a=1)]=[];', '({x:(a=1)}={});'
];
function reject(source,kind){
 for(var strict=0;strict<2;++strict){
  assignmentEffects=0;var error;
  try{inEdition(2015,(strict?'"use strict";':'')+'assignmentEffects++;'+source);}catch(e){error=e;}
  check(error&&error.name===kind,'early '+kind+' '+strict+' '+source);
  check(assignmentEffects===0,'no evaluation '+source);
 }
}
for(var i=0;i<referenceErrors.length;++i)reject(referenceErrors[i],'ReferenceError');
for(i=0;i<syntaxErrors.length;++i)reject(syntaxErrors[i],'SyntaxError');
var valid=[
 'var a;(a)=3;a===3', 'var a;[(a)]=[4];a===4',
 'var a;({x:(a)}={x:5});a===5', 'var a;[(a)=6]=[];a===6',
 'var a;({x:(a)=7}={});a===7', 'var a;[...[a]]=[8];a===8',
 'var a;for((a) in {key:1}){};a==="key"',
 'var a;for((a) of [9]){};a===9',
 'var o={};[(o.x)]=[10];o.x===10',
 'var a;({x:[a]}={x:[11]});a===11',
 'for(({});false;){};true'
];
for(i=0;i<valid.length;++i){
 check(inEdition(2015,'(function(){'+valid[i].replace(/;([^;]*)$/, ';return $1')+'})()'),'valid '+valid[i]);
}
var nameCases=[
 '(function(){var a;[(a)=function(){}]=[];return a.name==="";})',
 '(function(){var a;({x:(a)=function(){}}={});return a.name==="";})',
 '(function(){var a;function capture(){return a;}[(a)=function(){}]=[];return capture().name==="";})',
 '(function(){let a;[(a)=function(){}]=[];return a.name==="";})',
 '(function(){var a;[a=function(){}]=[];return a.name==="a";})',
 '(function(){var a;({x:a=function(){}}={});return a.name==="a";})',
 '(function(){var a;[(a)=class {}]=[];return a.name==="";})',
 '(function(){var a;[(a)=()=>1]=[];return a.name===""&&a()===1;})'
];
for(i=0;i<nameCases.length;++i){
 var fn=inEdition(2015,nameCases[i]);gc();check(fn(),'default name '+i);
 var roundtrip=inEdition(2015,'('+fn.toString()+')');gc();check(roundtrip(),'roundtrip default name '+i);
}
var globFn=inEdition(2015,'(function(){[(assignmentGlobalName)=function(){}]=[];return assignmentGlobalName.name==="";})');
check(globFn(),'global default name');
check(inEdition(2015,'('+globFn.toString()+')')(),'global default source name');
for(var ed=0;ed<2;++ed){
 var edition=ed?170:0;
 check(inEdition(edition,'(function(){var a;([a])=[3];return a===3;})()'),'legacy parenthesized pattern '+edition);
 check(inEdition(edition,'(function(){var calls=0;function f(){calls++;}try{f()=1;}catch(e){return e instanceof ReferenceError&&calls===1;}return false;})()'),'legacy call target runtime '+edition);
 check(inEdition(edition,'(function(){function f(){};for(f() in {}){}return true;})()'),'legacy empty for-in '+edition);
}
gc();
print('ES6-ASSIGNMENT-TARGETS checks='+checks+' failures=0');
