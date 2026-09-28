/* Block lexical initialization; MPL 1.1/GPL 2.0/LGPL 2.1. */
(function () {
    var checks = 0, failures = 0;
    function test(name, source) {
        ++checks;
        try { if (eval(source) === true) return; }
        catch (e) { print('DETAIL ' + name + ': ' + e); }
        ++failures; print('FAIL lexical initialization: ' + name);
    }
    function tdz(name, source) {
        test(name, '(function(){try{' + source + ';return false}' +
             'catch(e){return e instanceof ReferenceError}})()');
    }
    tdz('discarded read', '{x;let x=7}');
    tdz('initializer read', '{let x=x}');
    tdz('discarded typeof', '{typeof x;let x}');
    tdz('typeof result', '{var result=typeof x;let x}');
    tdz('write', '{x=7;let x}');
    tdz('compound assignment', '{x+=7;let x}');
    tdz('prefix increment', '{++x;let x}');
    tdz('postfix increment', '{x++;let x}');
    tdz('prefix decrement', '{--x;let x}');
    tdz('postfix decrement', '{x--;let x}');
    tdz('closure read', '{var f=()=>x;f();let x}');
    tdz('closure write', '{var f=()=>x=7;f();let x}');
    tdz('ordinary closure read', '{var f=function(){return x};f();let x}');
    tdz('direct eval read', '{eval("x");let x}');
    tdz('direct eval write', '{eval("x=7");let x}');
    tdz('direct eval typeof', '{eval("typeof x");let x}');
    tdz('with fallback read', '{with({}){x}let x}');
    tdz('with fallback write', '{with({}){x=7}let x}');
    tdz('detached read', 'var f=(function(){return ()=>x;let x=7})();gc();f()');
    tdz('detached write', 'var f=(function(){return ()=>x=7;let x=7})();gc();f()');
    tdz('detached typeof', 'var f=(function(){return ()=>typeof x;let x=7})();gc();f()');
    tdz('detached increment', 'var f=(function(){return ()=>x++;let x=7})();gc();f()');
    tdz('destructuring assignment', '{[x]=[7];let x}');
    tdz('object destructuring assignment', '{({a:x}={a:7});let x}');
    tdz('function body closure declaration', 'var f=(function(){return inner;let x;function inner(){return x}})();gc();f()');
    tdz('switch discriminant enters before case expression', 'switch(1){case x:let x=1}');
    tdz('abrupt initialization leaves binding uninitialized', 'var f;try{let x=(f=()=>x,(function(){throw 1})())}catch(e){}f()');
    tdz('for-in assignment', '{for(x in {a:1}){}let x}');
    test('for-in declaration', '(function(){var result;for(let x in {a:1})result=x;return result==="a"})()');
    test('initialized undefined', '(function(){let x;return x===undefined})()');
    test('left to right declarations', '(function(){let a=3,b=a+4;return b===7})()');
    test('reassignment', '(function(){let x=7;x=8;return x===8})()');
    test('initialized arithmetic', '(function(){let x=7;return x++===7&&++x===9&&x--===9&&--x===7})()');
    test('initialized captured value', '(function(){var f=(function(){let x=7;return ()=>x})();gc();return f()===7})()');
    test('detached assignment', '(function(){var f=(function(){let x=7;return ()=>++x})();gc();return f()===8&&f()===9})()');
    test('shadow outer binding', '(function(){let x=7;{let x=9;if(x!==9)return false}return x===7})()');
    test('destructuring declaration', '(function(){let [x,y]=[3,4];return x+y===7})()');
    test('object destructuring declaration', '(function(){let {a:x,b:y}={a:3,b:4};return x+y===7})()');
    test('catch binding', '(function(){try{throw 7}catch(x){return x===7}})()');
    test('catch destructuring', '(function(){try{throw {a:7}}catch({a:x}){return x===7}})()');
    test('decompile uninitialized declaration', '(function(){var f=function(){let x;return x===undefined};return eval("("+f.toString()+")")()})()');
    test('decompile initialized declaration', '(function(){var f=function(){let x=7;return x};return eval("("+f.toString()+")")()===7})()');
    test('decompile destructuring declaration', '(function(){var f=function(){let [a,b]=[3,4];return a+b};return eval("("+f.toString()+")")()===7})()');
    test('with own property shadows TDZ', '(function(){with({x:7}){if(x!==7)return false}let x;return x===undefined})()');
    print('ES6-LEXICAL-INITIALIZATION checks=' + checks + ' failures=' + failures);
    if (failures) throw Error('lexical initialization regressions');
})();
