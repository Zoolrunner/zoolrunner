/* Computed data property ordering, naming, GC and decompilation. */
var checks=0;
function check(ok,label){++checks;if(!ok)throw Error('FAIL computed property: '+label);}
function caught(fn){try{fn();}catch(e){return e;}return null;}
var key='field',o=({[key]:7});check(o.field===7,'string key');
o=({[3]:9,[-0]:2});check(o[3]===9&&o[0]===2,'numeric keys');
var symbol=Symbol('key');o=({[symbol]:11});check(o[symbol]===11&&Object.getOwnPropertySymbols(o)[0]===symbol,'symbol key');
var order=[],marker={},count=0;
key={};key[Symbol.toPrimitive]=function(hint){order.push('key:'+hint);gc();return 'answer';};
o=({[key]:(order.push('value'),gc(),42)});
check(o.answer===42&&order.join()==='key:string,value','key conversion precedes value');
key[Symbol.toPrimitive]=function(){throw marker;};
check(caught(function(){return {[key]:++count};})===marker&&count===0,'key failure skips value');
key[Symbol.toPrimitive]=function(){return symbol;};o=({[key]:12});check(o[symbol]===12,'symbol conversion');
var prototype=Object.getPrototypeOf({});o=({['__proto__']:9});
check(Object.getPrototypeOf(o)===prototype&&o.hasOwnProperty('__proto__')&&o.__proto__===9,'computed proto is data');
o=({__proto__:null,['__proto__']:10});check(Object.getPrototypeOf(o)===null&&o.__proto__===10,'prototype setter plus computed data');
o=({['__proto__']:1,['__proto__']:2});check(o.__proto__===2,'duplicate computed proto allowed');
var setters=0;
Object.defineProperty(Object.prototype,'computedInherited',{configurable:true,set:function(){++setters;}});
try{o=({['computedInherited']:5});check(o.computedInherited===5&&setters===0,'own definition bypasses setter');}
finally{delete Object.prototype.computedInherited;}
o=({get value(){return 1;},['value']:2});var d=Object.getOwnPropertyDescriptor(o,'value');
check(d.value===2&&d.writable&&d.enumerable&&d.configurable&&!d.hasOwnProperty('get'),'replaces accessor');
function make(k){return {[k]:function(){return 3;}};}
var a=make('a').a,b=make('b').b;
check(a.name==='a'&&b.name==='b'&&a.name==='a'&&a!==b,'names belong to closures');
d=Object.getOwnPropertyDescriptor(a,'name');check(!d.writable&&!d.enumerable&&d.configurable,'name descriptor');
check((new a()) instanceof a,'ordinary functions remain constructors');
o=({[Symbol()]:function(){}});check(o[Object.getOwnPropertySymbols(o)[0]].name==='','absent symbol description');
o=({[Symbol('')]:function(){}});check(o[Object.getOwnPropertySymbols(o)[0]].name==='[]','empty symbol description');
o=({[Symbol('a\u0000\ud83d\ude00')]:function(){}});check(o[Object.getOwnPropertySymbols(o)[0]].name==='[a\u0000\ud83d\ude00]','Unicode symbol name');
o=({[Symbol.iterator]:function(){}});check(o[Symbol.iterator].name==='[Symbol.iterator]','well-known symbol name');
o=({[17]:function(){}});check(o[17].name==='17','numeric function name');
o=({['k']:function named(){return named;}});check(o.k.name==='named'&&o.k()===o.k,'explicit name preserved');
var original=function(){};o=({['newName']:original});check(o.newName===original&&original.name==='original','references not renamed');
o=({['k']:(function(){})});check(o.k.name==='k','parenthesized anonymous function');
function commaKey(a,b){return {[(a,b)]:function(){return 4;}};}
var restored=eval('('+make.toString()+')');check(restored('x').x.name==='x','decompile ordinary computed key');
restored=eval('('+commaKey.toString()+')');check(restored('a','b').b()===4,'decompile comma key');
check(caught(function(){eval('({["x"]:x}={x:1})');}) instanceof SyntaxError,'unsupported computed destructuring fails safely');
o=({method(a){return a+1;},['computed'](){return this;},get(){return 8;},set(){return 9;},17(){return 10;}});
check(o.method(2)===3&&o.computed()===o&&o.get()===8&&o.set()===9&&o[17]()===10,'concise method calls');
check(o.method.name==='method'&&o.method.length===1&&o.computed.name==='computed'&&o[17].name==='17','method metadata');
check(!o.method.hasOwnProperty('prototype')&&!o.computed.hasOwnProperty('prototype'),'methods lack prototype');
check(caught(function(){new o.method();}) instanceof TypeError,'method is not constructor');
d=Object.getOwnPropertyDescriptor(o,'method');check(d.writable&&d.enumerable&&d.configurable,'method descriptor');
check(caught(function(){eval('({method(a,a){}})');}) instanceof SyntaxError,'unique method parameters');
check(caught(function(){eval('({["method"](a,a){}})');}) instanceof SyntaxError,'unique computed method parameters');
o=({method(eval,arguments){return eval+arguments;}});check(o.method(1,2)===3,'nonstrict method binding names');
o=({__proto__(){return 11;},['__proto__'](){return 12;}});check(o.__proto__()===12&&Object.getPrototypeOf(o)===prototype,'method proto is ordinary data');
o=({__proto__:null,__proto__(){return 13;}});check(Object.getPrototypeOf(o)===null&&o.__proto__()===13,'prototype setter and named method');
function methodFactory(k){return {[k](){return this;}};}
a=methodFactory('a');b=methodFactory('b');check(a.a.name==='a'&&b.b.name==='b'&&a.a()===a,'computed method closures');
restored=eval('('+methodFactory.toString()+')');o=restored('again');check(o.again()===o&&caught(function(){new o.again();}) instanceof TypeError,'method decompilation preserves construction restriction');
o=({[symbol](){return 14;}});check(o[symbol].name==='[key]'&&o[symbol]()===14,'symbol method name');
function namedMethodFactory(){return {plain(){return 15;},'string name'(){return 16;},12(){return 17;}};}
restored=eval('('+namedMethodFactory.toString()+')');o=restored();check(o.plain()===15&&o['string name']()===16&&o[12]()===17,'named method decompilation');
var stored=0;
o=({get [symbol](){gc();return stored;},set [symbol](v){gc();stored=v;}});
o[symbol]=21;d=Object.getOwnPropertyDescriptor(o,symbol);
check(o[symbol]===21&&d.enumerable&&d.configurable&&!d.hasOwnProperty('value'),'computed accessors');
check(d.get.name==='get [key]'&&d.set.name==='set [key]'&&d.get.length===0&&d.set.length===1,'accessor metadata');
check(!d.get.hasOwnProperty('prototype')&&caught(function(){new d.get();}) instanceof TypeError,'accessor nonconstructor');
var unnamed=Symbol();o=({get [unnamed](){return 1;},set [unnamed](v){}});d=Object.getOwnPropertyDescriptor(o,unnamed);
check(d.get.name==='get '&&d.set.name==='set ','absent description accessor names');
check(caught(function(){eval('({get ["x"](a){}})');}) instanceof SyntaxError,'getter arity');
check(caught(function(){eval('({set ["x"](){}})');}) instanceof SyntaxError,'setter arity');
o=({x:4,get ['x'](){return 5;}});d=Object.getOwnPropertyDescriptor(o,'x');check(o.x===5&&!d.hasOwnProperty('value'),'computed getter replaces data');
function accessorFactory(k){var saved=1;return {get [k](){return saved;},set [k](v){saved=v;}};}
restored=eval('('+accessorFactory.toString()+')');o=restored('x');o.x=23;check(o.x===23,'accessor decompilation');
var shortValue=19;check(({shortValue}).shortValue===19,'shorthand value');
function shorthandFactory(value){return {value};}
check(shorthandFactory(20).value===20,'shorthand local');
restored=eval('('+shorthandFactory.toString()+')');check(restored(24).value===24,'shorthand decompilation');
check((function(__proto__){var result={__proto__};return result.hasOwnProperty('__proto__')&&result.__proto__===7&&Object.getPrototypeOf(result)===prototype;})(7),'shorthand proto is data');
check(caught(function(){eval('({return})');}) instanceof SyntaxError,'keyword shorthand rejected');
check(caught(function(){eval('({class})');}) instanceof SyntaxError,'reserved shorthand rejected');
check(caught(function(){eval('"use strict"; var yield=1;({yield});');}) instanceof SyntaxError,'strict contextual name rejected');
var class\u0100=31;
check(({class\u0100}).class\u0100===31,'Unicode shorthand is not an ASCII keyword');
var discardedEffects=0;
({[{toString:function(){++discardedEffects;gc();return 'discarded';}}]:1});
check(discardedEffects===1,'discarded object keeps key conversion effects');
check(caught(function(){eval('var af = (x, {x}) => 1;');}) instanceof SyntaxError,'duplicate arrow binding is a syntax error');
check(caught(function(){eval('var af = ({x}, {y:x}) => 1;');}) instanceof SyntaxError,'duplicate arrow object binding is a syntax error');
print('ES6-COMPUTED-PROPERTIES checks='+checks+' failures=0');
