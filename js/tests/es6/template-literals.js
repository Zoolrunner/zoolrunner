/* ES2015 template strings, evaluation and realm template registry.
 * MPL 1.1/GPL 2.0/LGPL 2.1.  */
(function(){
    var checks=0,failures=0;
    function test(label,body){
        ++checks;
        try{if(body()===true)return;}catch(e){print('DETAIL '+label+': '+e);}
        ++failures;print('FAIL template literal: '+label);
    }
    function source(label,text){test(label,function(){return eval(text);});}
    function rejected(label,text){test(label,function(){try{eval(text);}catch(e){return e instanceof SyntaxError;}return false;});}
    source('empty','`` === ""');
    source('plain','`hello` === "hello"');
    source('substitution','`a${1+2}b${null}c${undefined}` === "a3bnullcundefined"');
    source('comma expression','`${(1,2)}` === "2"');
    source('nested object and regexp','`${({a:/}/.test("}")}).a}` === "true"');
    source('nested templates','`a${`b${3}c`}d` === "ab3cd"');
    source('escaped delimiters','`\\`\\${x}` === "`'+'${x}"');
    source('line feeds','`a\nb` === "a\\nb"');
    source('CR normalization','`a\rb` === "a\\nb"');
    source('CRLF normalization','`a\r\nb` === "a\\nb"');
    source('Unicode line separator','`a\u2028b` === "a\\u2028b"');
    source('Unicode paragraph separator','`a\u2029b` === "a\\u2029b"');
    source('line continuation','`a\\\nb` === "ab"');
    source('code point escape','`\\u{1f600}` === "\\ud83d\\ude00"');
    source('hex and Unicode escapes','`\\x41\\u0042` === "AB"');
    source('NUL escape','`a\\0b`.charCodeAt(1) === 0');
    source('not a strict directive','(function(){`use strict`;with({x:3}){return x===3;}})()');
    source('string hint and callback order',
        '(function(){var order=[],x={toString:function(){order.push("str");gc();return "x";},'+
        'valueOf:function(){throw Error("wrong hint");}};var value=`${x}${(order.push("rhs"),2)}`;'+
        'return value==="x2" && order.join()==="str,rhs";})()');
    source('conversion exception stops later expression',
        '(function(){var ran=false,marker={},x={toString:function(){throw marker;}};'+
        'try{`${x}${(ran=true)}`;}catch(e){return e===marker && !ran;}return false;})()');
    source('Symbol conversion rejects',
        '(function(){try{`${Symbol()}`;}catch(e){return e instanceof TypeError;}return false;})()');
    source('tag receives uncoerced values',
        '(function(){var value={},seen;function tag(t,v){seen=v;return t[0]+t[1];}'+
        'return tag`a${value}b`==="ab" && seen===value;})()');
    source('tag property receiver',
        '(function(){var o={tag:function(t){"use strict";return this===o && t[0]==="x";}};return o.tag`x`;})()');
    source('tag strict receiver',
        '(function(){function tag(t){"use strict";return this===undefined;}return tag`x`;})()');
    source('tagged eval is indirect',
        '(function(){var eval=function(t){"use strict";return this===undefined;};return eval`x`;})()');
    source('tag cache matches raw segments across sites',
        '(function(){function tag(t){gc();return t;}return tag`a${1}b`===tag`a${2}b`;})()');
    source('tag cache distinguishes raw spelling',
        '(function(){function tag(t){return t;}return tag`a`!==tag`\\x61`;})()');
    source('tag raw and cooked',
        '(function(){function tag(t){return t;}var t=tag`a\\nb`;return t[0]==="a\\nb" && t.raw[0]==="a\\\\nb";})()');
    source('tag arrays frozen',
        '(function(){function tag(t){return t;}var t=tag`x`;var d=Object.getOwnPropertyDescriptor(t,"raw");'+
        'return Array.isArray(t) && Array.isArray(t.raw) && Object.isFrozen(t) && Object.isFrozen(t.raw) && '+
        '!d.writable && !d.enumerable && !d.configurable;})()');
    source('tag chaining',
        '(function(){function a(t){return function(u){return t[0]+u[0];};}return a`x``y`==="xy";})()');
    source('new tagged expression',
        '(function(){function tag(t){return function(v){this.value=t[0]+v;};}return new tag`x`("y").value==="xy";})()');
    source('String.raw','String.raw`a\\n${3}b` === "a\\\\n3b"');
    source('decompiled template and tag',
        '(function(){var f=function(x){function tag(t,v){return t.raw[0]+v+t.raw[1];}'+
        'return `${x}`+tag`a\\n${x}b`;};var g=eval("("+f.toString()+")");return f(3)===g(3);})()');
    var unicodeRaw=['\u2028','\u2029','\u00e9','\ud83d\ude00','\ud800','\0'];
    for(var u=0;u<unicodeRaw.length;++u)(function(raw){
        test('Unicode/raw decompilation '+raw.charCodeAt(0),function(){
            var text='(function(){function tag(t){return t.raw[0];}return tag`'+raw+'`;})';
            var original=eval(text),restored=eval('('+original.toString()+')');
            return original()===raw && restored()===raw;
        });
    })(unicodeRaw[u]);
    rejected('unterminated','`a');
    rejected('empty substitution','`${}`');
    rejected('unterminated substitution','`${1');
    rejected('octal escape','`\\1`');
    rejected('decimal escape','`\\8`');
    rejected('invalid Unicode escape','`\\uZZZZ`');
    rejected('invalid tagged escape in ES2015','String.raw`\\uZZZZ`');
    print('ES6-TEMPLATE-LITERALS checks='+checks+' failures='+failures);
    if(failures)throw Error('template literal failures: '+failures);
})();
