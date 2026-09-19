/* ES2015 regressions; MPL 1.1/GPL 2.0/LGPL 2.1. */
var checks=0;function check(v,label){checks++;if(!v)throw Error(label);}
function make(){'use strict';var f=n=>n?f(n-1):this;return f;}
var receiver={};var f=make.call(receiver);check(f(100000)===receiver,'captured this');
check(f.call({wrong:true},100000)===receiver,'call receiver ignored');
function Make(){'use strict';var f=n=>n?f(n-1):new.target;return f;}
var arrow=new Make();check(arrow(100000)===Make,'captured new.target');
function args(a){'use strict';var f=n=>n?f(n-1):arguments[0];return f;}
check(args(29)(100000)===29,'captured arguments');
var list=[];
function cap(){'use strict';var f=n=>{var x=n;list.push(()=>x);return n?f(n-1):0;};return f;}
check(cap()(1000)===0,'arrow local capture recursion');gc();
check(list[0]()===1000&&list[1000]()===0,'arrow local captures survive');
print('TAIL-CALL-ARROWS checks='+checks+' failures=0');
