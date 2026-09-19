var checks=0;
function same(a,b,label){++checks;if(a!==b)throw Error(label+': '+a+' != '+b);}
function syntax(fn,label){++checks;try{fn();}catch(e){if(e instanceof SyntaxError)return;throw e;}throw Error(label);}
var prefixes=['','0'], bodies=['\n','text\nmore','\r','\r\n','\u2028','\u2029'], tails=['',' /**/',' /* first */ /* second */'];
for(var strict=0;strict<2;++strict)
 for(var p=0;p<prefixes.length;++p)
  for(var b=0;b<bodies.length;++b)
   for(var t=0;t<tails.length;++t){
    var source=(strict?'"use strict";':'')+'var count=0;\n'+prefixes[p]+'/*'+bodies[b]+'*/'+tails[t]+'--> ignored words\ncount++;count';
    same(eval(source),1,'multiline HTML close '+strict+'/'+p+'/'+b+'/'+t);
   }
same(eval('var x=1;var result=x/*same line*/-->0;x===0&&result'),true,'same-line decrement');
same(eval('var x=1;\n/*\n*/x-->0'),true,'token after comment restores dirty line');
same(eval('function f(){return/*\n*/--> ignored\n9;}f()'),undefined,'return ASI');
syntax(function(){eval('throw/*\n*/--> ignored\n9');},'throw newline');
same(Function('0/*\n*/--> ignored\nreturn 9;')(),9,'Function constructor');
var realm=createTest262Realm();
var rejected=false;try{realm.compileModule('0/*\n*/--> ignored\nexport var x=1;','comment-module');}catch(e){rejected=e instanceof realm.global.SyntaxError;}same(rejected,true,'module rejects HTML close');
gc();same(eval('0/*\n*/--> ignored\n8'),8,'after collection');
print('HTML-CLOSE-COMMENTS checks='+checks+' failures=0');
