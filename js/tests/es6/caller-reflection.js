var checks=0;function check(v,m){checks++;if(!v)throw Error(m);}
function probe(){return [probe.caller,Object.getOwnPropertyDescriptor(probe,'caller').value];}
function ordinary(){return probe();}
var r=ordinary();check(r[0]===ordinary&&r[1]===ordinary,'ordinary caller extension preserved');
function strictCaller(){'use strict';var r=probe();return r;}
r=strictCaller();check(r[0]===null&&r[1]===null,'strict caller represented as null');
var C=class{method(){var r=probe();return r;}static method(){var r=probe();return r;}get value(){var r=probe();return r;}constructor(){this.result=probe();}*generator(){yield probe();}};
var c=new C();check(c.result[0]===null&&c.result[1]===null,'class constructor');
r=c.method();check(r[0]===null&&r[1]===null,'class method');r=C.method();check(r[0]===null&&r[1]===null,'class static method');r=c.value;check(r[0]===null&&r[1]===null,'class accessor');r=c.generator().next().value;check(r[0]===null&&r[1]===null,'class generator');
var arrow=()=>{'use strict';var r=probe();return r;};r=arrow();check(r[0]===null&&r[1]===null,'strict arrow caller');
function* generator(){'use strict';yield probe();}r=generator().next().value;check(r[0]===null&&r[1]===null,'strict generator caller');
var threw=false;try{strictCaller.caller;}catch(e){threw=e instanceof TypeError;}check(threw,'strict function inherited caller accessor');
var bound=ordinary.bind(null);threw=false;try{bound.caller;}catch(e){threw=e instanceof TypeError;}check(threw,'bound function inherited caller accessor');
var saved=version();try{for(var i=0;i<2;i++){version(i?170:0);var legacy=evaluate('(function legacy(){return legacy.caller;})','legacy-caller-reflection');version(saved);threw=false;try{(function(){'use strict';var x=legacy();return x;})();}catch(e){threw=e instanceof TypeError;}check(threw,'legacy throwing getter '+i);}}finally{version(saved);}
gc();r=strictCaller();check(r[0]===null&&r[1]===null,'reflection after collection');
print('CALLER-REFLECTION checks='+checks+' failures=0');
