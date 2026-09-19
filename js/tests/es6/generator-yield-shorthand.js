/* ES2015 regressions; MPL 1.1/GPL 2.0/LGPL 2.1. */
var checks=0;function check(v,m){checks++;if(!v)throw Error(m);}
function invalid(s){var ok=false;try{Function(s);}catch(e){ok=e instanceof SyntaxError;}check(ok,s);}
invalid('return function*(){ for({yield} of [{}]); };');
invalid('return function*(){ for({yield} in {}); };');
invalid('return function*(){ return {yield}; };');
invalid('return function*(){ ({yield}={}); };');
invalid('return function*(){ ({yield=1}={}); };');
var g=Function('return function*(){return {yield:2};}')();check(g().next().value.yield===2,'ordinary key');
var nested=Function('return function*(){function f(){var yield=3;return {yield};}return f();}')();check(nested().next().value.yield===3,'nested ordinary function resets context');
var arrow=Function('return function*(){return (()=>{var yield=4;return {yield};})();}')();check(arrow().next().value.yield===4,'arrow body resets context');
check(Function('var yield=5;return {yield}.yield;')()===5,'sloppy ordinary yield identifier');
print('YIELD-SHORTHAND checks='+checks+' failures=0');
