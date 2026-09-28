var checks=0;
function same(a,b,label){++checks;if(a!==b)throw Error(label+': '+a+' != '+b);}
function names(a){return Object.getOwnPropertyNames(a).join(',');}
var actions=['','arguments.callee;','arguments.length;','arguments.extra=1;','Object.getOwnPropertyDescriptor(arguments,"callee");','Object.defineProperty(arguments,"length",{value:7});','Object.defineProperty(arguments,"callee",{get:function(){gc();return 9;}});arguments.callee;'];
for(var i=0;i<actions.length;++i){
 var f=Function('x',actions[i]+'var keys=Object.getOwnPropertyNames(arguments).join(",");return {args:arguments,keys:keys};');
 var r=f(1),expected='0,length,callee'+(i===3?',extra':'');
 same(r.keys,expected,'live order '+i);same(names(r.args),expected,'detached order '+i);
 same(Reflect.ownKeys(r.args).map(String).join(','),expected+',Symbol(Symbol.iterator)','symbol grouping '+i);
}
function append(a){arguments.extra=1;delete arguments.length;arguments.length=3;return arguments;}
same(names(append(1)),'0,callee,extra,length','deleted/readded length');
function appendCallee(a){arguments.extra=1;delete arguments.callee;arguments.callee=3;return arguments;}
same(names(appendCallee(1)),'0,length,extra,callee','deleted/readded callee');
function mapped(a){arguments.callee;a=9;return arguments;}
var a=mapped(1);same(a[0],9,'mapping remains live');same(names(a),'0,length,callee','mapping order');
function frozen(a){arguments.callee;Object.freeze(arguments);a=9;return arguments;}
a=frozen(1);same(a[0],1,'freeze detaches mapping');same(names(a),'0,length,callee','frozen order');
function zero(){arguments.callee;return arguments;}
same(names(zero()),'length,callee','zero actual arguments');
print('ARGUMENTS-PROPERTY-ORDER checks='+checks+' failures=0');
