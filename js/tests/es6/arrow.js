/* Arrow grammar, lexical bindings and classic function interoperability.
 * MPL 1.1/GPL 2.0/LGPL 2.1.
 */
(function () {
    var checks = 0, failures = 0;
    function test(label, source) {
        checks++;
        try {
            if (eval(source) !== true) throw Error('incorrect result');
        } catch (error) {
            failures++;
            print('FAIL arrow ' + label + ': ' + error);
        }
    }
    function syntax(label, source) {
        checks++;
        try { eval(source); } catch (error) {
            if (error instanceof SyntaxError) return;
        }
        failures++;
        print('FAIL arrow syntax: ' + label);
    }
    test('empty parameters', '(function(){var f=()=>7;return f()===7})()');
    test('single parameter', '(x=>x+1)(4)===5');
    test('multiple parameters', '((a,b)=>a+b)(3,4)===7');
    test('block body', '((x)=>{var y=x+2;return y;})(5)===7');
    test('object expression body', '(()=>({x:7}))().x===7');
    test('lexical object receiver', '(function(){var o={};return (function(){return (()=>this).call(null)===o}).call(o)})()');
    test('lexical strict undefined', '(function(){"use strict";return (()=>this).call({})===undefined})()');
    test('lexical strict primitive', '(function(){"use strict";return (()=>this).bind({})()===23}).call(23)');
    test('nested lexical receiver', '(function(){"use strict";return (()=>()=>this)()()===23}).call(23)');
    test('lexical eval receiver', '(function(){"use strict";return (()=>eval("this"))()===23}).call(23)');
    test('arguments survive return', '(function(a){return ()=>arguments[0]})(7)()===7');
    test('strict arguments snapshot', '(function(a){"use strict";var f=()=>arguments[0];a=9;return f})(7)()===7');
    test('parameter named arguments', '((arguments)=>arguments)(7)===7');
    test('local arguments binding', '(function(){return (()=>{var arguments=7;return arguments;})()===7})(3)');
    test('own metadata', '(function(){var f=(a,b)=>a;return f.length===2&&f.name==="f"&&!f.hasOwnProperty("prototype")})()');
    test('constructor rejection', '(function(){var f=()=>0;try{new f();return false}catch(e){return e instanceof TypeError}})()');
    test('decompilation', '(function(){var f=x=>x+1,s=f.toString();return s.indexOf("=>")>=0&&eval("("+s+")")(6)===7})()');
    test('independent closure instances', '(function(){function outer(x){return ()=>x}var a=outer(1),b=outer(2);gc();return a()===1&&b()===2})()');
    test('lexical new.target', '(function(){function C(){return ()=>new.target}var f=new C;return f()===C&&C()()===undefined})()');
    syntax('duplicate parameters', '(a,a)=>0');
    syntax('line break before arrow', 'a\n=>0');
    syntax('extra parameter parentheses', '((a))=>0');
    syntax('binary expression parameters', 'a+b=>0');
    syntax('call expression parameters', 'f()=>0');
    syntax('strict restricted parameter', '(eval)=>{"use strict";return 0}');
    test('arguments read before mutation', '(function(a){var f=()=>arguments[0];f();a=9;return f})(7)()===9');
    test('explicit outer arguments binding', '(function(){var arguments=7;return ()=>arguments})()()===7');
    test('outer parameter named arguments', '(function(arguments){return ()=>arguments})(7)()===7');
    test('with scope survives creation', '(function(){var f;with({arguments:7}){f=()=>arguments}return f()===7})()');
    test('ordinary function inside arrow', '(()=>function(){return this.value})().call({value:7})===7');
    test('direct eval creates escaping arrow', '(function(a){return eval("()=>arguments[0]")})(7)()===7');
    test("strict eval preserves boxed receiver", "(function(){var receiver=this;return eval('\"use strict\"; (()=>this)()')===receiver}).call(7)");
    test("arrow toString invocation round trip", "(function(){var f=function(){return (()=>7)()};return eval('('+f.toString()+')')()===7})()");
    syntax('trailing comma before arrow', '(a,)=>0');
    syntax('parenthesized individual parameter', '(a,(b))=>0');
    syntax('member parameter', '(a.b)=>0');
    syntax('newline in comment before arrow', '(a)/*\n*/=>0');
    print('ES6-ARROW checks=' + checks + ' failures=' + failures);
    if (failures) throw Error('arrow regressions');
})();
