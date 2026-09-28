var checks=0,hits=0;
function same(a,b,label){++checks;if(a!==b)throw Error(label+': '+a+' != '+b);}
for(var strict=0;strict<2;++strict){
 for(var n=0;n<2;++n){
  var name=n?'callee':'length';
  hits=0;
  var f=Function('name',(strict?'"use strict";':'')+'Object.defineProperty(arguments,name,{configurable:true,get:function(){++hits;gc();return 7;},set:function(){++hits;gc();}});return arguments;');
  if(strict&&name==='callee')continue;
  var args=f(name);
  same(hits,0,'no accessor at exit '+name+' '+strict);
  same(args[name],7,'escaped getter');same(hits,1,'getter only on read');
  args[name]=4;same(hits,2,'setter only on write');
 }
}
function mapped(a){var args=arguments;var initial=args[0];a=9;return args;}
same(mapped(1)[0],9,'mapped value snapshot');
function deleted(a){delete arguments[0];delete arguments.length;delete arguments.callee;return arguments;}
var args=deleted(1);
same(Object.prototype.hasOwnProperty.call(args,'0'),false,'deleted index remains absent');
same(Object.prototype.hasOwnProperty.call(args,'length'),false,'deleted length remains absent');
same(Object.prototype.hasOwnProperty.call(args,'callee'),false,'deleted callee remains absent');
function detached(a){Object.defineProperty(arguments,'0',{value:4,writable:false});a=9;return arguments;}
same(detached(1)[0],4,'detached mapping');
function assigned(a){arguments.length=8;arguments.callee=3;return arguments;}
args=assigned(1);same(args.length,8,'assigned length');same(args.callee,3,'assigned callee');
function ordinary(a){return arguments;}
args=ordinary(1,2);same(args.length,2,'ordinary length');same(args.callee,ordinary,'ordinary callee');
var token={};
function throwing(){Object.defineProperty(arguments,'length',{get:function(){throw Error('unexpected getter');}});throw token;}
try{throwing();}catch(e){same(e,token,'exit preserves thrown value');}
function indexAccessor(a){Object.defineProperty(arguments,'0',{get:function(){++hits;return 6;}});a=9;return arguments;}
hits=0;args=indexAccessor(1);same(hits,0,'no index getter at exit');same(args[0],6,'escaped index getter');same(hits,1,'one index getter');
print('ARGUMENTS-EXIT checks='+checks+' failures=0');
