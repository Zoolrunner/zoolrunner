var checks=0,hits=0;
function same(a,b,label){++checks;if(a!==b)throw Error(label+': '+a+' != '+b);}
function f(a){
 var args=arguments;
 delete args.length;delete args.callee;delete args[0];
 Object.setPrototypeOf(args,new Proxy({}, {get:function(){++hits;throw Error('get');},has:function(){++hits;throw Error('has');},getOwnPropertyDescriptor:function(){++hits;throw Error('descriptor');}}));
 return args;
}
var args=f(1);same(hits,0,'no prototype access at exit');
same(Object.getOwnPropertyDescriptor(args,'length'),undefined,'no revived length');
same(Object.getOwnPropertyDescriptor(args,'callee'),undefined,'no revived callee');
same(Object.getOwnPropertyDescriptor(args,'0'),undefined,'no revived index');
var calls=0;
function custom(x){x instanceof { [Symbol.hasInstance]:function(v){++calls;return true;}};}
custom({});same(calls,1,'discarded hasInstance protocol');
print('ARGUMENTS-EXIT-PROTOTYPE checks='+checks+' failures=0');
