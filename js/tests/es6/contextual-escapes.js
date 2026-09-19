/* Contextual modifier spellings differ from ordinary IdentifierNames. */
var checks=0;
function same(a,b,label){++checks;if(a!==b)throw Error(label+': '+a+' != '+b);}
function syntax(source){var caught=false;try{Function(source);}catch(e){caught=e instanceof SyntaxError;}same(caught,true,source);}
var bad=[
 'return ({get a b(){}})',
 'return ({set a b(x){}})',
 'return ({get a *b(){}})',
 'return ({set a *b(x){}})',
 'return ({get ["a"] b(){}})',
 'return ({set ["a"] b(x){}})',
 'return class {get a *b(){}}',
 'return class {set a *b(x){}}',
 'return class { a b(){} }',
 'return class { ["a"] b(){} }',
 'return class { a *b(){} }',
 'return class { static a b(){} }',
 'return class { get a b(){} }',
 'return class { set a b(x){} }',
 'return class { constructor name(){} }',
 'return class { *a b(){} }',
 'return class { st\\u0061tic f(){} }',
 'return class { \\u0073tatic f(){} }',
 'return class { stat\\u{69}c f(){} }',
 'return class { st\\u0061tic *f(){} }',
 'return class { st\\u0061tic get f(){} }',
 'return class { st\\u0061tic ["f"](){} }',
 'return class { g\\u0065t f(){} }',
 'return class { \\u0067et f(){} }',
 'return class { ge\\u{74} f(){} }',
 'return class { s\\u0065t f(x){} }',
 'return class { \\u0073et f(x){} }',
 'return class { static g\\u0065t f(){} }',
 'return class { static s\\u0065t f(x){} }',
 'return class { g\\u0065t ["f"](){} }',
 'return class { s\\u0065t ["f"](x){} }',
 'return ({g\\u0065t f(){}})',
 'return ({\\u0067et f(){}})',
 'return ({ge\\u{74} f(){}})',
 'return ({s\\u0065t f(x){}})',
 'return ({\\u0073et f(x){}})',
 'return ({g\\u0065t ["f"](){}})',
 'return ({s\\u0065t ["f"](x){}})'
];
for(var i=0;i<bad.length;++i){syntax(bad[i]);syntax('"use strict";'+bad[i]);}
var C=Function('return class { st\\u0061tic(){return 1;} g\\u0065t(){return 2;} s\\u0065t(){return 3;} static g\\u0065t(){return 4;} get g\\u0065tter(){return 5;} set s\\u0065tter(x){this.x=x;} }')();
var c=new C;
same(c.static(),1,'escaped ordinary static method');
same(c.get(),2,'escaped ordinary get method');
same(c.set(),3,'escaped ordinary set method');
same(C.get(),4,'escaped static method name');
same(c.getter,5,'escaped accessor property name');
c.setter=6;same(c.x,6,'escaped setter property name');
var o=Function('return ({g\\u0065t(){return 7;}, s\\u0065t:8, get g\\u0065tter(){return 9;}, set s\\u0065tter(x){this.x=x;}})')();
same(o.get(),7,'escaped object method name');same(o.set,8,'escaped data property');
same(o.getter,9,'escaped object getter name');o.setter=10;same(o.x,10,'escaped object setter name');
var D=eval('('+C.toString()+')');same(new D().static(),1,'class source roundtrip');
var getter=Object.getOwnPropertyDescriptor(C.prototype,'getter').get;
same(getter.name,'get getter','accessor function name');
print('CONTEXTUAL-ESCAPES checks='+checks+' failures=0');
