var spreadChecks=0;
function check(v){spreadChecks++;if(!v)throw Error('array spread check '+spreadChecks)}
function raises(f){var caught=false;try{f()}catch(e){caught=e instanceof TypeError}check(caught)}
check([...[]].length===0);
check([0,...[1,2],3,...[4]].join()==='0,1,2,3,4');
var a=[,...[,],,4];check(a.length===4 && !(0 in a) && 1 in a && !(2 in a) && a[3]===4);
check([...'a\uD83D\uDE00b'].join('|')==='a|\uD83D\uDE00|b');
var log=[],source={};
source[Symbol.iterator]=function(){var n=0;log.push('iterator');return {next:function(){log.push('next');return {get done(){log.push('done');return n===2},get value(){log.push('value');return ++n}}}}};
a=[(log.push('before'),0),...source,(log.push('after'),3)];
check(a.join()==='0,1,2,3');check(log.join()==='before,iterator,next,done,value,next,done,value,next,done,after');
raises(function(){return [...null]});raises(function(){return [...{}]});raises(function(){var o={};o[Symbol.iterator]=function(){return 1};return [...o]});
var closed=0,bad={};bad[Symbol.iterator]=function(){return {next:function(){throw TypeError('next')},return:function(){closed++;return {}}}};
raises(function(){return [...bad]});check(closed===0);
bad[Symbol.iterator]=function(){return {next:function(){return {done:false,get value(){throw TypeError('value')}}},return:function(){closed++;return {}}}};
raises(function(){return [...bad]});check(closed===0);
var setter=0;Object.defineProperty(Array.prototype,'0',{set:function(){setter++},configurable:true});
try{a=[...[7]];check(a[0]===7 && Object.prototype.hasOwnProperty.call(a,'0') && setter===0)}finally{delete Array.prototype[0]}
var f=function(){return [,,...[1,2],,3,...[],]};var copy=eval('('+f.toString()+')');
a=copy();check(a.length===6 && !(0 in a) && !(1 in a) && a[2]===1 && a[3]===2 && !(4 in a) && a[5]===3);
var g=(function*(){return [0,...(yield [1,2]),yield 3]})();check(g.next().value.join()==='1,2');check(g.next([4,5]).value===3);check(g.next(6).value.join()==='0,4,5,6');
var nested=[...[...[1,2]],...[3]];check(nested.join()==='1,2,3');
check([...(function*(){yield 1;yield 2})()].join()==='1,2');
var comma=function(){return [...(1,[2,3])]};check(eval('('+comma.toString()+')')().join()==='2,3');
print('ARRAY-SPREAD PASS checks='+spreadChecks);
