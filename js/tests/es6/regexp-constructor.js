/* ES2015 RegExp construction, sticky flags, ordering and legacy syntax.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
var checks=0;
function check(v,name){++checks;if(!v)throw Error('RegExp constructor: '+name);}
function caught(f){try{f();}catch(e){return e;}return null;}
var r=/a/y, d=Object.getOwnPropertyDescriptor(RegExp.prototype,'sticky'), copy, log, pattern;
check(d.configurable&&!d.enumerable&&d.set===undefined&&typeof d.get==='function','sticky descriptor');
check(d.get.name==='get sticky'&&d.get.length===0&&!d.get.hasOwnProperty('prototype'),'sticky getter metadata');
check(caught(function(){return RegExp.prototype.sticky;}) instanceof TypeError,'2015 prototype rejects sticky getter');
check(caught(function(){return d.get.call(new Proxy(r,{}));}) instanceof TypeError,'sticky getter rejects Proxy matcher');
check(r.sticky&&!r.global&&r.flags==='y'&&r.toString()==='/a/y','sticky literal flags');
check(r.exec('ba')===null&&r.lastIndex===0,'sticky does not scan');
r.lastIndex=1;check(r.exec('ba')[0]==='a'&&r.lastIndex===2,'sticky at requested index');
check(r.exec('ba')===null&&r.lastIndex===0,'sticky exhaustion resets index');
r=/(?:a|b)c/y;check(r.exec('xbc')===null,'compound sticky does not scan');
r.lastIndex=1;check(r.exec('xbc')[0]==='bc','sticky alternatives backtrack');
r=/a+/y;check(r.exec('xaa')===null,'sticky quantifier does not scan');
r.lastIndex=1;check(r.exec('xaa')[0]==='aa','sticky quantifier matches at index');
r=/(?:)/y;check(r.exec('x')[0]===''&&r.lastIndex===0,'empty sticky exec does not advance itself');
check('ba'.search(/a/y)===-1&&'a'.search(/a/y)===0,'String search honors sticky');
check('aa'.match(/a/gy).join()==='a,a'&&'ba'.match(/a/gy)===null,'global sticky match stops at first gap');
check(caught(function(){new RegExp('a','yy');}) instanceof SyntaxError,'duplicate sticky flag');
check(caught(function(){eval('/a/yy');}) instanceof SyntaxError,'duplicate sticky literal flag');
check(new RegExp('a','yig').flags==='giy','canonical flags order');
r=/a/gy;copy=new RegExp(r);check(copy!==r&&copy.source==='a'&&copy.flags==='gy'&&copy.lastIndex===0,'copy matcher flags');
check(RegExp(r)===r,'matching constructor identity');
copy=new RegExp(r,'i');check(copy.flags==='i'&&copy.source==='a','flags override');
Object.defineProperty(r,'source',{get:function(){throw Error('public source');}});
Object.defineProperty(r,'flags',{get:function(){throw Error('public flags');}});
check(new RegExp(r,'m').source==='a','internal matcher source bypasses public fields');
r[Symbol.match]=false;check(RegExp(r)!==r&&new RegExp(r).global,'false match marker still copies actual matcher');
log=[];pattern={};
Object.defineProperty(pattern,Symbol.match,{get:function(){log.push('match');return true;}});
Object.defineProperty(pattern,'constructor',{get:function(){log.push('constructor');return RegExp;}});
check(RegExp(pattern)===pattern&&log.join()==='match,constructor','regexp-like constructor identity short circuit');
log=[];pattern={};
Object.defineProperty(pattern,Symbol.match,{get:function(){log.push('match');return true;}});
Object.defineProperty(pattern,'source',{get:function(){log.push('source');return {toString:function(){gc();log.push('source-string');return 'a';}};}});
Object.defineProperty(pattern,'flags',{get:function(){log.push('flags');return {toString:function(){gc();log.push('flags-string');return 'y';}};}});
var alternate={};
var target=new Proxy(function(){},{get:function(t,k){if(k==='prototype'){log.push('prototype');gc();return alternate;}return t[k];}});
copy=Reflect.construct(RegExp,[pattern],target);
check(Object.getPrototypeOf(copy)===alternate&&log.join()==='match,source,flags,prototype,source-string,flags-string','constructor ordering before allocation');
check(Object.getOwnPropertyDescriptor(copy,'lastIndex').value===0&&d.get.call(copy),'alternate prototype has matcher and own index');
log=[];copy=new RegExp(pattern,'i');
check(copy.ignoreCase&&!copy.sticky&&log.join()==='match,source,source-string','explicit flags skip flags getter');
var marker={};pattern={};Object.defineProperty(pattern,Symbol.match,{get:function(){throw marker;}});
log=[];check(caught(function(){Reflect.construct(RegExp,[pattern],target);})===marker&&log.length===0,'match error precedes newTarget prototype');
var revocable=Proxy.revocable(function(){},{get:function(t,k){if(k==='prototype'){revocable.revoke();gc();return alternate;}return t[k];}});
copy=Reflect.construct(RegExp,['a','y'],revocable.proxy);
check(Object.getPrototypeOf(copy)===alternate&&d.get.call(copy),'prototype object avoids querying revoked constructor realm');
d=Object.getOwnPropertyDescriptor(RegExp,Symbol.species);
check(d.configurable&&!d.enumerable&&d.set===undefined&&d.get.name==='get [Symbol.species]','species descriptor');
check(d.get.call(marker)===marker&&d.get.call(null)===null&&RegExp[Symbol.species]===RegExp,'species preserves raw receiver');
function literal(){return /a\/b/y;}
check(eval('('+literal.toString()+')')().flags==='y','function decompilation preserves sticky flag');
r=/a/y;check(evaluate(r.toString(),'sticky-roundtrip').sticky,'literal text roundtrip');
var old=version(170), rejected=false;
try{eval('/a/y');}catch(e){rejected=e instanceof SyntaxError;}
check(rejected&&new RegExp('a','y').sticky,'legacy grammar and borrowed modern constructor');
version(old);
/* String fallback uses RegExpCreate, without constructor IsRegExp/slot-copy. */
r=/a/;r[Symbol.match]=null;
check('/a/'.match(r)[0]==='/a/'&&'a'.match(r)===null,'String fallback initializes from regexp string value');
print('ES6-REGEXP-CONSTRUCTOR checks='+checks+' failures=0');
