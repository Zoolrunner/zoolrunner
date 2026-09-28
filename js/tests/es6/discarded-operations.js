var checks=0,hits=0;
function same(a,b,label){++checks;if(a!==b)throw Error(label+': '+a+' != '+b);}
var operations=['x+1','x-1','x*2','x/2','x%2','x<<1','x>>1','x>>>1','x|1','x^1','x&1','x<2','x<=2','x>2','x>=2','x==1','x!=1','+x','-x','~x','void(x+1)','delete(x+1)','(x+1,0)','true ? x+1 : 0'];
for(var strict=0;strict<2;++strict){
 for(var i=0;i<operations.length;++i){
  hits=0;var o={valueOf:function(){++hits;return 1;}};
  Function('x',(strict?'"use strict";':'')+operations[i]+';')(o);
  same(hits,1,(strict?'strict ':'')+operations[i]);
 }
 hits=0;Function('x',(strict?'"use strict";':'')+'for(x+1;false;){}')({valueOf:function(){++hits;return 1;}});same(hits,1,'for initializer');
 hits=0;Function('x',(strict?'"use strict";':'')+'x===1;x!==1;!x;void x;typeof x;')({valueOf:function(){++hits;return 1;}});same(hits,0,'non-coercive operations');
 var object={};hits=0;Function('x','y',(strict?'"use strict";':'')+'x in y;')({toString:function(){++hits;return 'p';}},object);same(hits,1,'in property coercion');
 for(var j=0;j<2;++j){
  var threw=false;try{Function('x',(strict?'"use strict";':'')+(j?'x instanceof 1;':'"p" in x;'))(null);}catch(e){threw=e instanceof TypeError;}
  same(threw,true,'discarded protocol exception');
 }
}
function lengthRead(){Object.defineProperty(arguments,'length',{get:function(){++hits;return 1;}});arguments.length;}
function indexRead(x){Object.defineProperty(arguments,'0',{get:function(){++hits;return 1;}});arguments[0];}
function dynamicIndex(x){arguments[x];}
hits=0;lengthRead(1);same(hits,1,'arguments length getter');
hits=0;indexRead(1);same(hits,1,'arguments index getter');
hits=0;dynamicIndex({toString:function(){++hits;return '0';}});same(hits,1,'arguments key coercion');
var sentinel={},seen=[];
try{(function(x){try{+x;}finally{seen.push('finally');}})({valueOf:function(){seen.push('convert');throw sentinel;}});}catch(e){same(e,sentinel,'original thrown value');}
same(seen.join(','),'convert,finally','coercion and finally order');
print('DISCARDED-OPERATIONS checks='+checks+' failures=0');
