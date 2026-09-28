/* ES2015 Statement versus StatementListItem, including Annex B labels. */
var checks = 0;
function check(value) { ++checks; if (!value) throw Error('statement edge ' + checks); }
function syntax(source) {
    var threw = false;
    try { Function(source); } catch (e) { threw = e instanceof SyntaxError; }
    check(threw);
}
var bodies = ['while(false) BODY', 'do BODY while(false)',
              'for(;false;) BODY', 'for(var k in {}) BODY',
              'with({}) BODY'];
for (var i = 0; i < bodies.length; ++i) {
    syntax(bodies[i].replace('BODY', 'function f(){}'));
    syntax(bodies[i].replace('BODY', 'a: b: function f(){}'));
    check(typeof Function(bodies[i].replace('BODY', '{function f(){}}')) === 'function');
}
syntax('if(true) a: function f(){}');
syntax('if(false) {} else a: function f(){}');
syntax('"use strict"; a: function f(){}');
syntax('try{}catch([x,x]){}');
syntax('try{}catch({a:x,b:x}){}');
check(typeof Function('a: b: function f(){}') === 'function');
check(typeof Function('if(true) function f(){} else function g(){}') === 'function');
check(typeof Function('while(false) a: {function f(){}}') === 'function');
check(typeof Function('try{}catch([x,y]){}') === 'function');
print('STATEMENT-FUNCTION-EDGES checks=' + checks + ' failures=0');
