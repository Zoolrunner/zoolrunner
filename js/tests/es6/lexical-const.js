/* Modern lexical const; MPL 1.1/GPL 2.0/LGPL 2.1. */
(function () {
    var checks=0, failures=0;
    function test(label,source) {
        ++checks;
        try {if(eval(source)===true)return;}catch(e){print('DETAIL '+label+': '+e);}
        ++failures;print('FAIL block const: '+label);
    }
    function throws(label,type,source) {
        test(label,'(function(){try{'+source+';return false}catch(e){return e instanceof '+type+'}})()');
    }
    throws('prior discarded read','ReferenceError','{x;const x=7}');
    throws('self initializer','ReferenceError','{const x=x}');
    throws('prior typeof','ReferenceError','{typeof x;const x=7}');
    throws('prior write','ReferenceError','{x=7;const x=8}');
    throws('prior compound write','ReferenceError','{x+=7;const x=8}');
    throws('prior increment','ReferenceError','{++x;const x=8}');
    throws('prior closure read','ReferenceError','{var f=()=>x;f();const x=8}');
    throws('prior closure write','ReferenceError','{var f=()=>x=7;f();const x=8}');
    throws('detached prior read','ReferenceError','var f=(function(){return ()=>x;const x=8})();gc();f()');
    throws('detached prior write','ReferenceError','var f=(function(){return ()=>x=7;const x=8})();gc();f()');
    throws('prior eval write','ReferenceError','{eval("x=7");const x=8}');
    throws('prior destructuring write','ReferenceError','{[x]=[7];const x=8}');
    throws('prior for-in write','ReferenceError','{for(x in {a:1}){}const x=8}');
    throws('initialized write','TypeError','{const x=7;x=8}');
    throws('initialized compound write','TypeError','{const x=7;x+=8}');
    throws('initialized prefix increment','TypeError','{const x=7;++x}');
    throws('initialized postfix increment','TypeError','{const x=7;x++}');
    throws('initialized prefix decrement','TypeError','{const x=7;--x}');
    throws('initialized postfix decrement','TypeError','{const x=7;x--}');
    throws('initialized closure write','TypeError','var f;(function(){const x=7;f=()=>x=8})();gc();f()');
    throws('initialized eval write','TypeError','{const x=7;eval("x=8")}');
    throws('initialized destructuring write','TypeError','{const x=7;[x]=[8]}');
    test('RHS precedes TDZ assignment error','(function(){var calls=0;try{{x=(calls++,7);const x=8}}catch(e){return calls===1&&e instanceof ReferenceError}return false})()');
    test('TDZ compound read precedes RHS','(function(){var calls=0;try{{x+=(calls++,7);const x=8}}catch(e){return calls===0&&e instanceof ReferenceError}return false})()');
    test('block const shadows outer const','(function(){const x=7;{const x=9;if(x!==9)return false}return x===7})()');
    test('block let shadows outer const','(function(){const x=7;{let x=9;if(x!==9)return false}return x===7})()');
    test('block const shadows outer let','(function(){let x=7;{const x=9;if(x!==9)return false}return x===7})()');
    test('destructuring declaration','(function(){const [x,y]=[3,4];return x+y===7})()');
    test('object destructuring declaration','(function(){const {a:x,b:y}={a:3,b:4};return x+y===7})()');
    test('detached read','(function(){var f=(function(){const x=7;return ()=>x})();gc();return f()===7})()');
    test('decompile binding remains immutable','(function(){var f=function(){const x=7;try{x=8;return false}catch(e){return e instanceof TypeError}};return eval("("+f.toString()+")")()})()');
    test('for-in declaration','(function(){var result;for(const x in {a:1})result=x;return result==="a"})()');
    test('for-in source retains immutability','(function(){var f=function(){for(const x in {a:1}){try{x="b";return false}catch(e){return e instanceof TypeError}}};return eval("("+f.toString()+")")()})()');
    test('increment source roundtrip','(function(){var f=function(){const x=7;try{x++;return false}catch(e){return e instanceof TypeError}};return eval("("+f.toString()+")")()})()');
    test('destructured const source roundtrip','(function(){var f=function(){const [x,y]=[3,4];try{x=9;return false}catch(e){return x+y===7&&e instanceof TypeError}};return eval("("+f.toString()+")")()})()');
    throws('missing initializer','SyntaxError','eval("{const x;}")');
    throws('missing for initializer','SyntaxError','eval("for(const x;;){}")');
    throws('for-in initializer prohibited','SyntaxError','eval("for(const x=1 in {}){}")');
    throws('parenthesized const head prohibited','SyntaxError','eval("const(x=1)x")');
    throws('parenthesized const loop head prohibited','SyntaxError','eval("for(const(x=1)x;;){}")');
    if (failures) throw Error('const regressions: '+failures);
    print('ES6-LEXICAL-CONST checks='+checks+' failures='+failures);
})();
