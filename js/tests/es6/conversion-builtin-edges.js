/* ES2015 conversion and built-in metadata regressions. */
var checks=0;function check(b,s){checks++;if(!b)throw Error(s);}
function throws(f,expected,label){var thrown=false;try{f();}catch(e){thrown=e===expected;}check(thrown,label);}
check(Number.parseInt===parseInt&&Number.parseFloat===parseFloat,'parser identity');
check(Number.parseInt('10',2)===2&&Number.parseFloat('1.25x')===1.25,'parser behavior');
check(DataView.length===1,'DataView arity');
check(Object.getOwnPropertyDescriptor(Number,'parseInt').enumerable===false,'parser enumerability');
check('__null__'.indexOf({toString:function(){return null;}})===2,'ordinary null string');
check('__null__'.indexOf({[Symbol.toPrimitive]:function(){return null;}})===2,'symbol null string');
check('__foo__'.indexOf({toString:{},valueOf:function(){return 'foo';}})===2,'skip non callable toString');
check('aaaa'.indexOf('aa',{valueOf:{},toString:function(){return 1;}})===1,'skip non callable valueOf');
var sentinel={};
throws(function(){String({get toString(){throw sentinel;}});},sentinel,'propagate toString getter');
throws(function(){Number({get valueOf(){throw sentinel;}});},sentinel,'propagate valueOf getter');
var method=Object.prototype.isPrototypeOf;
check(method.call(null,1)===false&&method.call(undefined,null)===false,'primitive argument before receiver');
var type=false;try{method.call(null,{});}catch(e){type=e instanceof TypeError;}check(type,'null receiver object argument');
var proto={};check(method.call(proto,Object.create(proto)),'prototype identity');
var count=0;var proxy=new Proxy({}, {getPrototypeOf:function(){count++;gc();return proto;}});
check(method.call(proto,proxy)&&count===1,'proxy prototype trap and gc');
var countArgs=-1;
check(Number({valueOf:function(){countArgs=arguments.length;return 6;}})===6&&countArgs===0,'valueOf receives no arguments');
check(String({toString:null,valueOf:function(){countArgs=arguments.length;return 7;}})==='7'&&countArgs===0,'fallback valueOf receives no arguments');
check(Date.prototype.isPrototypeOf(new Date(2000,0)), 'Date prototype receiver is not converted');
var uncoerced={valueOf:function(){throw Error('receiver conversion');},toString:function(){throw Error('receiver conversion');}};
check(method.call(uncoerced,Object.create(uncoerced)), 'object receiver identity');
print('CONVERSION-BUILTIN-EDGES checks='+checks+' failures=0');
