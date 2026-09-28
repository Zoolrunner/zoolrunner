/* Per-iteration lexical environments; MPL 1.1/GPL 2.0/LGPL 2.1. */
(function () {
    var checks=0, failures=0;
    function test(label,source) {
        ++checks;
        try {if(eval(source)===true)return;}catch(e){print('DETAIL '+label+': '+e);}
        ++failures;print('FAIL lexical loop: '+label);
    }
    test('body closures','(function(){var f=[];for(let i=0;i<3;i++)f.push(()=>i);gc();return f[0]()===0&&f[1]()===1&&f[2]()===2})()');
    test('initializer closure','(function(){var f;for(let i=(f=()=>i,0);i<3;i++){}return f()===0})()');
    test('condition closures','(function(){var f=[];for(let i=0;(f.push(()=>i),i<3);i++){}return f.map(function(x){return x()}).join()==="0,1,2,3"})()');
    test('update closures','(function(){var f=[];for(let i=0;i<3;f.push(()=>i),i++){}return f.map(function(x){return x()}).join()==="1,2,3"})()');
    test('multiple bindings','(function(){var f=[];for(let i=0,j=9;i<3;i++,j--)f.push(()=>i+":"+j);return f.map(function(x){return x()}).join()==="0:9,1:8,2:7"})()');
    test('continue runs freshening','(function(){var f=[];for(let i=0;i<3;i++){f.push(()=>i);continue}return f.map(function(x){return x()}).join()==="0,1,2"})()');
    test('continue without update expression','(function(){var f=[];for(let i=0;i<3;){f.push(()=>i);i++;continue}return f.map(function(x){return x()}).join()==="1,2,3"})()');
    test('labeled continue','(function(){var f=[];outer:for(let i=0;i<3;i++){for(let j=0;j<2;j++){f.push(()=>i+":"+j);continue outer}}return f.map(function(x){return x()}).join()==="0:0,1:0,2:0"})()');
    test('finally before next binding','(function(){var f=[];for(let i=0;i<3;i++){try{f.push(()=>i);continue}finally{i+=0}}return f.map(function(x){return x()}).join()==="0,1,2"})()');
    test('break retains final binding','(function(){var f;for(let i=0;i<3;i++){f=()=>i;if(i===1)break}return f()===1})()');
    test('return retains binding','(function(){for(let i=0;i<3;i++){if(i===1)return ()=>i}})()()===1');
    test('throw retains binding','(function(){var f;try{for(let i=0;i<3;i++){f=()=>i;if(i===1)throw 7}}catch(e){}gc();return f()===1})()');
    test('initializer TDZ','(function(){let i=7;try{for(let i=i;;)break;return false}catch(e){return e instanceof ReferenceError}})()');
    test('initializer typeof TDZ','(function(){try{for(let i=typeof i;;)break;return false}catch(e){return e instanceof ReferenceError}})()');
    test('for-in body let closures','(function(){var f=[];for(let x in {a:1,b:2})f.push(()=>x);gc();return f[0]()!==f[1]()&&f[0]()+f[1]()==="ab"})()');
    test('for-in body const closures','(function(){var f=[];for(const x in {a:1,b:2})f.push(()=>x);return f[0]()+f[1]()==="ab"})()');
    test('for-in head stays uninitialized','(function(){var f;for(let x in (f=()=>x,{a:1})){}try{f();return false}catch(e){return e instanceof ReferenceError}})()');
    test('const for-in head stays uninitialized','(function(){var f;for(const x in (f=()=>x,{a:1})){}try{f();return false}catch(e){return e instanceof ReferenceError}})()');
    test('const loop rejects writes','(function(){for(const x in {a:1}){try{x="b";return false}catch(e){return e instanceof TypeError}}})()');
    test('outer bindings retain identity','(function(){var f=[];let outer=0;for(let i=0;i<3;i++)f.push(()=>++outer+i);return f[0]()===1&&f[1]()===3&&f[2]()===5&&outer===3})()');
    test('body-only binding scopes','(function(){var f=[];for(let i=0;i<3;i++){let j=i+4;f.push(()=>i+j)}return f[0]()===4&&f[1]()===6&&f[2]()===8})()');
    test('var loops retain shared binding','(function(){var f=[];for(var i=0;i<3;i++)f.push(()=>i);return f[0]()===3&&f[1]()===3&&f[2]()===3})()');
    test('var through catch retains shared binding','(function(){var f=[];try{throw 7}catch(x){for(var x in {a:1,b:2})f.push(()=>x)}return f[0]()===f[1]()})()');
    test('source roundtrip C-style loop','(function(){var f=function(){var r=[];for(let i=0;i<3;i++)r.push(()=>i);return r.map(function(g){return g()}).join()};return eval("("+f.toString()+")")()==="0,1,2"})()');
    test('source roundtrip for-in','(function(){var f=function(){var r=[];for(const x in {a:1,b:2})r.push(()=>x);return r[0]()+r[1]()};return eval("("+f.toString()+")")()==="ab"})()');
    test('source roundtrip without update','(function(){var f=function(){var r=[];for(let i=0;i<2;){r.push(()=>i);++i;}return r[0]()+r[1]()};return eval("("+f.toString()+")")()===3})()');
    print('ES6-LEXICAL-LOOPS checks='+checks+' failures='+failures);
    if(failures)throw Error('lexical loop regressions');
})();
