'use strict';
var checks=0;function check(v,m){checks++;if(!v)throw Error(m);}
var invalid=['({*})','({a:1,*})','({*get x(){return 1;}})','({*set x(v){}})','({*get [1](){return 1;}})','({*set [1](v){}})','({*get "x"(){}})','({*set 1(v){}})','({*x:1})','({*[1]:2})','({*x})'];
for(var i=0;i<invalid.length;i++){var caught=false;try{Function('return '+invalid[i]);}catch(e){caught=e instanceof SyntaxError;}check(caught,invalid[i]);}
var o={*get(){yield 1;},*set(){yield 2;},*['name'](){yield 3;},*'quoted'(){yield 4;},*5(){yield 5;}};
check(o.get().next().value===1&&o.set().next().value===2,'generator methods named get and set');
check(o.name().next().value===3&&o.quoted().next().value===4&&o[5]().next().value===5,'computed string and numeric generator names');
var value=0,accessors={get x(){return value;},set x(v){value=v;}};accessors.x=7;check(accessors.x===7,'ordinary accessors');
var C=class{*get(){yield 8;}static *set(){yield 9;}};check(new C().get().next().value===8&&C.set().next().value===9,'class generator methods');
var source=String(function(){return {*get(){yield 10;}};});check(Function('return ('+source+')')()().get().next().value===10,'generator method decompilation round trip');
print('GENERATOR-METHOD-GRAMMAR checks='+checks+' failures=0');
