/* Symbol identity, property keys, coercion and collection regressions. */
var checks=0;
function check(ok,label){++checks;if(!ok)throw Error('FAIL Symbol: '+label);}
function throwsType(fn,label){var caught=false;try{fn();}catch(e){caught=e instanceof TypeError;}check(caught,label);}
var a=Symbol('same'),b=Symbol('same');
check(typeof a==='symbol'&&typeof b==='symbol','primitive type');
check(a===a&&a!==b&&a!=b&&a!='Symbol(same)'&&a!=1&&a!=true,'identity equality');
check(Object.is(a,a)&&!Object.is(a,b),'SameValue');
check(String(a)==='Symbol(same)'&&String(Symbol())==='Symbol()'&&String(Symbol(undefined))==='Symbol()','display strings');
check(String(Symbol('\u0000\ud800\ud83d\ude00'))==='Symbol(\u0000\ud800\ud83d\ude00)','Unicode and lone surrogate description');
var calls=0,desc={toString:function(){++calls;return 'description';}};
check(String(Symbol(desc))==='Symbol(description)'&&calls===1,'description conversion once');
throwsType(function(){Symbol(a);},'Symbol description rejected');
throwsType(function(){new Symbol();},'not constructible');
throwsType(function(){Number(a);},'numeric conversion');
throwsType(function(){+a;},'unary plus');
throwsType(function(){a+'';},'string concatenation');
throwsType(function(){a<1;},'relational conversion');
throwsType(function(){new String(a);},'String construction');
check(Boolean(a)&&!!a,'truthy');
var box=Object(a);
check(typeof box==='object'&&box.valueOf()===a&&box==a&&box!==a,'wrapper identity');
check(box.toString()==='Symbol(same)'&&a.valueOf()===a&&a.toString()==='Symbol(same)','primitive and wrapper methods');
throwsType(function(){String(box);},'implicit wrapper primitive conversion');
throwsType(function(){Symbol.prototype.valueOf.call({});},'valueOf brand');
throwsType(function(){Symbol.prototype.toString.call('x');},'toString brand');
throwsType(function(){Symbol.prototype.valueOf();},'prototype has no SymbolData');
var names=['hasInstance','isConcatSpreadable','iterator','match','replace','search','species','split','toPrimitive','toStringTag','unscopables'];
for(var i=0;i<names.length;++i){var name=names[i],d=Object.getOwnPropertyDescriptor(Symbol,name);check(typeof d.value==='symbol'&&!d.writable&&!d.enumerable&&!d.configurable&&String(d.value)==='Symbol(Symbol.'+name+')','well-known '+name);}
var prim=Object.getOwnPropertyDescriptor(Symbol.prototype,Symbol.toPrimitive);
check(prim.value.name==='[Symbol.toPrimitive]'&&prim.value.length===1&&!prim.writable&&!prim.enumerable&&prim.configurable,'primitive hook metadata');
check(prim.value.call(a,'anything')===a,'primitive hook ignores hint');
var target={plain:1};target[a]=2;target[b]=3;
check(target[a]===2&&target[b]===3&&a in target&&target.hasOwnProperty(a)&&target.propertyIsEnumerable(a),'symbol key lookup');
check(target[box]===2&&target.hasOwnProperty(box),'boxed symbol key');
check(Object.keys(target).join()==='plain'&&Object.getOwnPropertyNames(target).join()==='plain','string enumeration excludes symbols');
var enumerated=[];for(var key in target)enumerated.push(key);
check(enumerated.join()==='plain','for-in excludes symbols');
var symbols=Object.getOwnPropertySymbols(target);
check(symbols.length===2&&symbols[0]===a&&symbols[1]===b,'own symbol creation order');
Object.defineProperty(target,a,{value:4,writable:false,enumerable:false,configurable:true});
d=Object.getOwnPropertyDescriptor(target,box);
check(d.value===4&&!d.writable&&!d.enumerable&&d.configurable,'symbol descriptor');
var copied=Object.assign({},target);
check(copied.plain===1&&copied[b]===3&&!copied.hasOwnProperty(a),'assign enumerable symbols');
check(delete target[a]&&!(a in target),'symbol deletion');
var descriptors={};descriptors[a]={value:9,enumerable:true};
check(Object.defineProperties({},descriptors)[a]===9,'descriptor map symbol keys');
check(Object.create(null,descriptors)[a]===9,'create descriptor symbols');
check(Object.getOwnPropertySymbols('text').length===0&&Object.getOwnPropertySymbols(a).length===0,'primitive reflection');
throwsType(function(){Object.getOwnPropertySymbols(null);},'null symbol reflection');
check(JSON.stringify(a)===undefined&&JSON.stringify([a])==='[null]'&&JSON.stringify(box)==='{}','JSON primitive handling');
check(JSON.stringify(target)==='{"plain":1}','JSON excludes symbol keys');
var hints=[],coercible={};coercible[Symbol.toPrimitive]=function(h){hints.push(h);gc();return a;};
check(target[coercible]===undefined&&hints.join()==='string','property key primitive hook');
coercible[Symbol.toPrimitive]=function(){return {};};
throwsType(function(){Number(coercible);},'primitive hook must return primitive');
coercible[Symbol.toPrimitive]=3;
throwsType(function(){String(coercible);},'primitive hook must be callable');
var tagged={};tagged[Symbol.toStringTag]='\u03bb\u0000tag';
check(Object.prototype.toString.call(tagged)==='[object \u03bb\u0000tag]','Unicode custom tag');
check(Object.prototype.toString.call(a)==='[object Symbol]'&&Object.prototype.toString.call(Symbol.prototype)==='[object Symbol]','Symbol tag');
var registered=Symbol.for('registry\u0000\ud800');
check(registered===Symbol.for('registry\u0000\ud800')&&Symbol.keyFor(registered)==='registry\u0000\ud800','registry key identity');
check(Symbol.keyFor(a)===undefined&&Symbol.keyFor(Symbol.iterator)===undefined,'unregistered symbols');
throwsType(function(){Symbol.keyFor(Object(registered));},'keyFor rejects wrapper');
for(i=0;i<30;++i){var transient=Symbol('same');target[transient]=i;gc();check(target[transient]===i&&transient!==a&&Symbol.for('registry\u0000\ud800')===registered,'collection '+i);delete target[transient];}
var ephemeralTarget={},keyConversions=0,descriptorReads=0,keyObject={};
keyObject[Symbol.toPrimitive]=function(h){check(h==='string','descriptor key hint');++keyConversions;return Symbol('ephemeral');};
Object.defineProperty(ephemeralTarget,keyObject,{
    get enumerable(){gc();++descriptorReads;return true;},
    get configurable(){gc();++descriptorReads;return true;},
    get value(){gc();++descriptorReads;return 17;},
    get writable(){gc();++descriptorReads;return true;}
});
var ephemeralKeys=Object.getOwnPropertySymbols(ephemeralTarget);
check(ephemeralKeys.length===1&&ephemeralTarget[ephemeralKeys[0]]===17&&keyConversions===1&&descriptorReads===4,'ephemeral key rooted through descriptor getters');
var inherited={};inherited[a]=11;var child=Object.create(inherited);child[b]=12;
check(child[a]===11&&Object.getOwnPropertySymbols(child).length===1&&Object.getOwnPropertySymbols(child)[0]===b,'own symbol reflection excludes prototype keys');
var frozen={};frozen[a]=1;Object.freeze(frozen);
check(Object.isFrozen(frozen)&&!Object.getOwnPropertyDescriptor(frozen,a).writable&&!Object.getOwnPropertyDescriptor(frozen,a).configurable,'freeze includes symbol descriptors');
throwsType(function(){Object.defineProperty(frozen,a,{value:2});},'frozen symbol write');
var order=[],ordered={};Object.defineProperty(ordered,a,{enumerable:true,get:function(){order.push('symbol');return 1;}});Object.defineProperty(ordered,'text',{enumerable:true,get:function(){order.push('text');return 2;}});Object.defineProperty(ordered,'2',{enumerable:true,get:function(){order.push('index');return 3;}});
Object.assign({},ordered);
check(order.join()==='index,text,symbol','assign orders integer/string/symbol keys');
var savedVersion=version();version(170);
check(evaluate('var ns=new Namespace("urn:legacy"),q=new QName(ns,"node"),x=<node xmlns="urn:legacy">text</node>;String(ns)==="urn:legacy"&&String(q)==="urn:legacy::node"&&x.toXMLString().indexOf("text")>=0','legacy-symbol-boundary'),'legacy E4X conversion');
version(savedVersion);
print('ES6-SYMBOL-PRIMITIVES checks='+checks+' failures=0');
