'use strict';
var checks=0;function check(v,m){checks++;if(!v)throw Error(m);}
var probeParam,probeBlock;let x='outside';
try{throw [];}catch([_ = probeParam = function(){return x;}]){probeBlock=function(){return x;};let x='inside';}
gc();check(probeParam()==='outside'&&probeBlock()==='inside','catch body has a distinct environment');
function syntax(s){var threw=false;try{Function(s);}catch(e){threw=e instanceof SyntaxError;}check(threw,'catch binding conflict: '+s);}
syntax('try{}catch(x){let x;}');syntax('try{}catch([x]){const x=1;}');syntax('"use strict";try{}catch(x){function x(){}}');
var nested;try{throw 1;}catch(x){{let x=2;nested=x;}}check(nested===2,'nested block may shadow catch parameter');
var a,b;try{throw [7];}catch([v,w=function(){return v;}]){a=w;b=function(){return v;};let extra=3;}gc();check(a()===7&&b()===7,'parameter and body closures share catch binding');
var v=9;try{throw 3;}catch(v){var v=4;}check(v===9,'simple catch var retains Annex B binding behavior');
var text=String(function(){try{throw [];}catch([x=function(){return 1;}]){let y=2;return x()+y;}});
check(Function('return ('+text+')')()()===3,'catch scope decompilation round trip');
var tdz=false;try{try{throw [];}catch([x=y,y=1]){}}catch(e){tdz=e instanceof ReferenceError;}check(tdz,'later catch binding has TDZ');
tdz=false;try{try{throw {};}catch({x=x}){}}catch(e){tdz=e instanceof ReferenceError;}check(tdz,'self catch default has TDZ');
var outer='outer',read;try{throw {};}catch({x=(read=function(){return outer;})}){let outer='inner';}gc();check(read()==='outer','object pattern default captures outer environment');
var closed=false,iterator={};iterator[Symbol.iterator]=function(){return {next:function(){return {done:false,value:undefined};},return:function(){closed=true;return {};}};};
try{try{throw iterator;}catch([x=x]){}}catch(e){check(e instanceof ReferenceError&&closed,'default failure closes iterator');}
var p,q;try{throw [];}catch([x=(p=function(){return typeof body;})]){q=function(){return body;};function body(){return 3;}}check(p()==='undefined'&&q()()===3,'body function is absent during catch default');
var iteratorClosure,seen;try{throw {get x(){gc();return 4;}};}catch({x,y=(iteratorClosure=function(){return x;})}){seen=x;}gc();check(iteratorClosure()===4&&seen===4,'initialized earlier binding survives getter collection');
print('CATCH-ENVIRONMENTS checks='+checks+' failures=0');
