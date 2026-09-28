/* ES2015 regressions; MPL 1.1/GPL 2.0/LGPL 2.1. */
var checks=0;
function check(value,label){checks++;if(!value)throw Error(label);}
function simple(n,a){'use strict';return n?simple(n-1,a+1):a;}
check(simple(100000,0)===100000,'deep recursion');
var captures=[];
function capture(n){'use strict';var local=n;captures.push(function(){return local;});if(!n)return 0;return capture(n-1);}
check(capture(1000)===0,'capture recursion');gc();
check(captures[0]()===1000&&captures[500]()===500&&captures[1000]()===0,'captured activation values');
var args=[];
function argCapture(n){'use strict';args.push(arguments);if(!n)return 0;return argCapture(n-1);}
argCapture(1000);gc();check(args[0][0]===1000&&args[500][0]===500&&args[1000][0]===0,'arguments snapshots');
var blocks=[];
function blockCapture(n){'use strict';{let x=n;blocks.push(function(){return x;});if(!n)return 0;return blockCapture(n-1);}}
blockCapture(1000);gc();check(blocks[0]()===1000&&blocks[500]()===500&&blocks[1000]()===0,'block snapshots');
function receiver(n){'use strict';if(!n)return this;if(n%100===0)gc();return holder.receiver(n-1);}
var holder={receiver:receiver};check(holder.receiver(10000)===holder,'receiver and collection');
function primitive(n){'use strict';if(!n)return this;return primitive(n-1);}
check(primitive(10000)===undefined,'undefined strict receiver');
function caught(n){'use strict';try{if(!n)throw 13;return caught(n-1);}catch(e){return e+n;}}
check(caught(10)===13,'retained catch');
var exits=[];
function finalizer(n){'use strict';try{if(!n)return 1;return finalizer(n-1);}finally{exits.push(n);}}
check(finalizer(10)===1&&exits.join(',')==='0,1,2,3,4,5,6,7,8,9,10','retained finally');
function notTail(n){'use strict';return n?1+notTail(n-1):0;}
check(notTail(30)===30,'non tail result');
function evalCall(n){'use strict';if(!n)return 7;return eval('evalCall(n-1)');}
check(evalCall(30)===7,'direct eval keeps its own execution frame');
var overrideArgs=[];
function overrides(n){'use strict';var a=arguments;overrideArgs.push(a);if(n===100){Object.defineProperty(a,'length',{value:77});}if(!n)return a.length;return overrides(n-1);}
check(overrides(100)===1&&overrideArgs[0].length===77&&overrideArgs[1].length===1,'arguments overrides do not survive frame reuse');
print('TAIL-CALL-SELF checks='+checks+' failures=0');
