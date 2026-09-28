var callSpreadChecks=0;
function check(v){callSpreadChecks++;if(!v)throw Error('call spread '+callSpreadChecks)}
function collect(){return Array.prototype.join.call(arguments,',')}
check(collect(...[1,2],3,...[4])==='1,2,3,4');
var obj={tag:7,method:function(){'use strict';return this.tag+arguments[0]}};
check(obj.method(...[2])===9);
check((function(){'use strict';return this===undefined})(...[]));
check((function(){var x=7;return eval(...['x'])})()===7);
check((function(){var x=7;return (0,eval)(...['typeof x'])})()==='undefined');
check((function(){'use strict';eval(...['var local=3']);return typeof local})()==='undefined');
function C(a,b){this.total=a+b;this.target=new.target}var a=new C(...[3,4]);check(a.total===7 && a.target===C && a instanceof C);
check(new (C.bind(null,3))(...[4]).total===7);
check(new Uint8Array(...[[1,2]]).join()==='1,2');
check(new RegExp(...['a','i']).test('A'));
check(new Proxy(...[C,{}]) instanceof Function);
check(new (new Proxy(C,{}))(...[3,4]).total===7);
var events=[], source={};source[Symbol.iterator]=function(){events.push('iterator');var n=0;return {next:function(){events.push('next');return n++?{done:true}:{value:3}}}};
var holder={get call(){events.push('callee');return function(v){events.push('call');return v}}};
check(holder.call(...source)===3 && events.join()==='callee,iterator,next,next,call');
var hit=false;try{(0)(...{[Symbol.iterator]:function(){hit=true;return {next:function(){return {done:true}}}}})}catch(e){check(hit && e instanceof TypeError)}
var f=function(){return collect(0,...(1,[2,3]),4)};check(eval('('+f.toString()+')')()==='0,2,3,4');
var ctor=function(){return new C(...[3,4])};check(eval('('+ctor.toString()+')')().total===7);
var g=(function*(){return collect(...(yield [1]),yield 2)})();check(g.next().value[0]===1);check(g.next([3,4]).value===2);check(g.next(5).value==='3,4,5');
var boundSource=function(){return new (C.bind(null,3))(...[4])};check(eval('('+boundSource.toString()+')')().total===7);
print('CALL-SPREAD PASS checks='+callSpreadChecks);
