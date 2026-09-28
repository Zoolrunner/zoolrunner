/* ES2015 rest formals; MPL 1.1/GPL 2.0/LGPL 2.1. */
(function () {
    var checks = 0, failures = 0;
    function test(name, source) {
        ++checks;
        try { if (eval(source) === true) return; }
        catch (e) { print('DETAIL ' + name + ': ' + e); }
        ++failures; print('FAIL rest parameters: ' + name);
    }
    function syntax(name, source) {
        ++checks;
        try { eval(source); } catch (e) { if (e instanceof SyntaxError) return; }
        ++failures; print('FAIL rest syntax: ' + name);
    }
    test('arrow empty', '((...r)=>Array.isArray(r)&&r.length===0)()');
    test('arrow values', '((...r)=>r.join()=== "1,2,3")(1,2,3)');
    test('arrow fixed and rest', '((a,...r)=>a===1&&r.join()==="2,3")(1,2,3)');
    test('arrow missing normal arguments', '((a,b,...r)=>a===1&&b===undefined&&r.length===0)(1)');
    test('rest length metadata', '(function(){var f=(a,b,...r)=>0;return f.length===2})()');
    test('rest named arguments', '((...arguments)=>arguments[0]===7)(7)');
    test('rest var redeclaration', '((...r)=>{var r;return r[0]===7})(7)');
    test('rest overwritten by body function', '((...r)=>{function r(){return 7}return r()===7})()');
    test('rest array fresh each call', '(function(){var f=(...r)=>r;return f()!==f()})()');
    test('lexical arguments unchanged', '(function(a){return ((...r)=>r[0]===9&&arguments[0]===a)(9)})(7)');
    test('rest retains captured values after return', '(function(){var f=((...r)=>()=>r[0])(7);gc();return f()===7})()');
    test('rest source reconstruction', '(function(){var f=(a,...r)=>a+r[0];return eval("("+f.toString()+")")(3,4)===7})()');
    test('inherited strict source reconstruction', '(function(){"use strict";var f=(...r)=>{try{eval("undeclaredRestProbe=1");return false}catch(e){return e instanceof ReferenceError&&r[0]===7}};return eval("("+f.toString()+")")(7)})()');
    test('ordinary rest values', '(function(a,...r){return a===1&&r.join()==="2,3"})(1,2,3)');
    test('ordinary rest unmapped arguments', '(function(a,...r){a=9;return arguments[0]===7&&r[0]===8})(7,8)');
    test('ordinary rest argument index writes', '(function(a,...r){arguments[0]=9;return a===7&&r[0]===8})(7,8)');
    test('ordinary rest array writes', '(function(a,...r){r[0]=9;return arguments[1]===8})(7,8)');
    test('ordinary rest constructor', '(function(){function C(...r){this.value=r[0]}return new C(7).value===7})()');
    syntax('trailing comma', '(...r,)=>0');
    syntax('not last', '(...r,a)=>0');
    syntax('default on rest', '(...r=[])=>0');
    syntax('rest without parentheses', '...r=>0');
    syntax('rest member binding', '(...obj.r)=>0');
    syntax('rest duplicate', '(r,...r)=>0');
    syntax('rest lexical collision', '(...r)=>{let r}');
    syntax('rest nested parentheses', '((...r))=>0');
    syntax('rest ordinary expression', '(...r)');
    syntax('non-simple explicit strict directive', '(...r)=>{"use strict";}');
    test('rest after destructured formal', '(function([a],...r){return a===7&&r[0]===8})([7],8)');
    test('ordinary rest named arguments', '(function(...arguments){return arguments[0]===7})(7)');
    test('ordinary rest length', '(function(a,...r){}).length===1');
    test('strict inheritance', '(function(){"use strict";return ((...r)=>this)()===7}).call(7)');
    test('ordinary decompilation', '(function(){var f=function(a,...r){return a+r[0]};return eval("("+f.toString()+")")(3,4)===7})()');
    test('escaping unmapped arguments', '(function(a,...r){a=9;return ()=>arguments[0]})(7)()===7');
    test('rest argument property descriptor', '((...r)=>{var d=Object.getOwnPropertyDescriptor(r,"0");return d.value===7&&d.writable&&d.enumerable&&d.configurable})(7)');
    test('normal parentheses retain in grammar', '(function(){var found;for(var x=("p" in {p:1});!found;){found=x}return found===true})()');
    test('normal parentheses retain regexp grammar', '(/a/).test("a")');
    syntax('ordinary duplicate with rest', 'function f(a,a,...r){}');
    syntax('getter rest', '({get value(...r){}})');
    syntax('setter rest', '({set value(...r){}})');
    syntax('strict restricted rest binding', 'function f(...eval){"use strict";}');
    test("array constructor is intrinsic", "(function(){var saved=Array,result;try{Array=function(){throw Error('poison')};result=((...r)=>r)(7)}finally{Array=saved}return Object.getPrototypeOf(result)===saved.prototype&&result[0]===7})()");
    test("inherited indexed setter is bypassed", "(function(){var calls=0,result;Object.defineProperty(Array.prototype,'0',{set:function(){calls++},configurable:true});try{result=((...r)=>r)(7)}finally{delete Array.prototype[0]}return calls===0&&result.hasOwnProperty('0')&&result[0]===7})()");
    test("ordinary unmapped callee poison", "(function(...r){try{arguments.callee;return false}catch(e){return e instanceof TypeError}})()");
    test("arguments length remains actual count", "(function(a,...r){return arguments.length===3&&r.length===2})(1,2,3)");
    test("body function replaces rest binding", "(function(...r){function r(){return 7}return r()===7})(9)");
    test("nested rest shadowing", "((...r)=>((...r)=>r[0])(9)===9&&r[0]===7)(7)");
    test("function constructor rest formals", "Function('a','...r','return a+r[0]')(3,4)===7");
    test("rest function decompilation strict nesting", "(function(){var outer=function(){'use strict';return (...r)=>{return this===undefined&&r[0]===7}};return eval('('+outer.toString()+')')()(7)})()");
    test("Function constructor metadata", "(function(){var f=Function('a','...r','return r[0]');return f.length===1&&eval('('+f.toString()+')')(3,7)===7})()");
    syntax("Function constructor duplicate formals", "Function('a,a,...r','return r')");
    syntax("Function constructor rest last", "Function('...r,a','return r')");
    syntax("Function constructor strict directive", "Function('...r','\"use strict\";')");
    syntax("rest inherited strict restricted name", "(function(){'use strict';return function(...eval){}})");
    test("rest arguments key order", "(function(...r){return Object.getOwnPropertyNames(arguments).join()==='0,length,caller,callee'})(7)");
    test("strict arguments key order", "(function(){'use strict';return Object.getOwnPropertyNames(arguments).join()==='0,length,caller,callee'})(7)");
    print('ES6-REST-PARAMETERS checks=' + checks + ' failures=' + failures);
    if (failures) throw Error('rest parameter regressions');
})();
