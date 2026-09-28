/* ES2015 regressions; MPL 1.1/GPL 2.0/LGPL 2.1. */
'use strict';
var checks=0;function check(v,m){checks++;if(!v)throw Error(m);}
function even(n){return n?odd(n-1):true;}function odd(n){return n?even(n-1):false;}
check(even(100000)===true&&odd(100000)===false,'mutual recursion');
function many(n,a,b,c,d,e,f,g){if(!n)return arguments.length;return few(n-1);}
function few(n){if(!n)return arguments.length;return many(n-1,1,2,3,4,5,6,7);}
check(many(100000,1,2,3,4,5,6,7)===8,'growing and shrinking arity');
function d1(n,a=7){return n?d2(n-1,a+1):a;}
function d2(n,a){return n?d1(n-1,a+1):a;}
check(d1(100000)===100007,'default parameter frame');
function rest1(n,...xs){return n?rest2(n-1,xs[0]+1):xs[0];}
function rest2(n,a){return n?rest1(n-1,a+1):a;}
check(rest1(100000,3)===100003,'rest parameter frame');
function pattern1({n,v}){return n?pattern2(n-1,v+1):v;}
function pattern2(n,v){return n?pattern1({n:n-1,v:v+1}):v;}
check(pattern1({n:100000,v:2})===100002,'destructured parameters');
var captured=[];
function capA(n){let a=n;captured.push(()=>a);return n?capB(n-1):0;}
function capB(n){var b=n;captured.push(function(){return b;});return n?capA(n-1):0;}
check(capA(1000)===0,'mutual captured activations');gc();
check(captured[0]()===1000&&captured[1]()===999&&captured[1000]()===0,'mutual captures survive');
var token={};function errA(n){return errB(n);}function errB(n){if(!n)throw token;return errA(n-1);}
var caught;try{errA(100000);}catch(e){caught=e;}check(caught===token,'tail target exception reaches live caller');
class Base{step(n){return n?this.other(n-1):this;}}
class Derived extends Base{other(n){return super.step(n);}}
var instance=new Derived();check(instance.step(100000)===instance,'super and receiver');
function spreadA(n,a){return n?spreadB(...[n-1,a+1]):a;}
function spreadB(n,a){return n?spreadA(...[n-1,a+1]):a;}
check(spreadA(100000,1)===100001,'spread mutual recursion');
var iterableCalls=0;
function spreadEffects(n){if(!n)return true;return spreadEffects(...{[Symbol.iterator]:function(){iterableCalls++;var done=false;return {next:function(){if(done)return {done:true};done=true;return {value:n-1,done:false};}};}});}
check(spreadEffects(10000)&&iterableCalls===10000,'spread iterator evaluated before frame retirement');
print('TAIL-CALL-GENERAL checks='+checks+' failures=0');
