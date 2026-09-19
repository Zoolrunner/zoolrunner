/* ES2015 regressions; MPL 1.1/GPL 2.0/LGPL 2.1. */
'use strict';
var checks=0;function check(v,m){checks++;if(!v)throw Error(m);}
function byCall(n){return n?byCall.call(null,n-1):true;}
function byApply(n){return n?byApply.apply(null,[n-1]):true;}
function byReflect(n){return n?Reflect.apply(byReflect,null,[n-1]):true;}
check(byCall(100000),'Function call forwarding');check(byApply(100000),'Function apply forwarding');check(byReflect(100000),'Reflect apply forwarding');
var proxy=new Proxy(function(n){return n?proxy(n-1):true;},{});check(proxy(100000),'proxy default forwarding');
var trapCount=0;
var trapped=new Proxy(function(n){return n?trapped(n-1):true;},{apply:function(target,receiver,args){trapCount++;return Reflect.apply(target,receiver,args);}});
check(trapped(100000)&&trapCount===100001,'proxy trap forwarding');
var getCount=0;
function getterApply(n){if(!n)return true;return getterApply.apply(null,{length:1,get 0(){getCount++;return n-1;}});}
check(getterApply(10000)&&getCount===10000,'apply getters before dispatch');
var revoked=Proxy.revocable(function(n){return n;},{get apply(){revoked.revoke();return function(target,receiver,args){return target.apply(receiver,args);};}});
function callRevoked(){return revoked.proxy(19);}
check(callRevoked()===19,'capture proxy state before trap lookup revokes it');
var oldEval=eval, global=this;
function aliasEval(){var local=17;return oldEval('typeof local');}
check(aliasEval()==='undefined','tail alias eval stays indirect');
function directEval(){var local=18;return eval('local');}
check(directEval()===18,'direct eval retains activation');
global.eval=function(n){return n?eval(n-1):true;};
try{check(global.eval(100000),'overridden eval tail call');}finally{global.eval=oldEval;}
global.eval=function(){return oldEval('typeof local');};
try{check(Function('var local=99;return eval();')()==='undefined','tail eval cannot inherit a retired call site');}finally{global.eval=oldEval;}
print('TAIL-CALL-FORWARDING checks='+checks+' failures=0');
