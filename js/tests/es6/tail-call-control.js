/* ES2015 regressions; MPL 1.1/GPL 2.0/LGPL 2.1. */
'use strict';
var checks=0;function check(v,m){checks++;if(!v)throw Error(m);}
function catchTail(n){try{throw n;}catch(e){return e?catchTail(e-1):true;}}
check(catchTail(100000),'catch completion');
function finallyTail(n){try{throw n;}finally{if(!n)return true;return finallyTail(n-1);}}
check(finallyTail(100000),'finally replaces pending exception');
function whileTail(n){while(n){return whileTail(n-1);}return true;}
check(whileTail(100000),'while body');
function forTail(n){for(;n;){return forTail(n-1);}return true;}
check(forTail(100000),'for body');
function switchTail(n){switch(n){case 0:return true;default:return switchTail(n-1);}}
check(switchTail(100000),'switch scope');
function doTail(n){do{if(!n)return true;return doTail(n-1);}while(false);}
check(doTail(100000),'do body');
function labels(n){outer:{inner:{return n?labels(n-1):true;}}}
check(labels(100000),'labeled block');
function tag(strings,n){return n?tag`x${n-1}`:strings[0];}
check(tag`x${100000}`==='x','tagged template tail');
var closed=0;
function forOf(n){for(var x of {[Symbol.iterator]:function(){return {next:function(){return {value:1,done:false};},return:function(){closed++;return {};}};}}){return n?forOf(n-1):true;}}
check(forOf(20)&&closed===21,'for of iterator close is retained');
function logical(n){return n&&logical(n-1);}
check(logical(100000)===0,'logical and tail');
function comma(n){return (0,n?comma(n-1):true);}
check(comma(100000),'comma tail');
function parens(n){return n?(((parens(n-1)))):true;}
check(parens(100000),'nested parentheses');
function either(n){return !n||either(n-1);}
check(either(100000),'logical or tail');
var padding='';for(var i=0;i<20000;i++)padding+='0,';
var wide=Function('n','"use strict";return n?wide(n-1):('+padding+'true);');
check(wide(10000),'extended jump and grouped comma result');
print('TAIL-CONTROL checks='+checks+' failures=0');
