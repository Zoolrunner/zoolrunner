'use strict';
var checks=0;function check(v,m){checks++;if(!v)throw Error(m);}
check('\u{0}'.length===1&&'\u{0}'.charCodeAt(0)===0,'nul');
check('\u{00000065}'==='e','leading zeroes');
check('\u{5c}'==='\\'&&'\u{27}'==="'"&&'\u{22}'==='"','escaped source delimiters');
check('\u{d800}'.charCodeAt(0)===0xd800&&'\u{dfff}'.charCodeAt(0)===0xdfff,'surrogate code points');
check('\u{10000}'==='\ud800\udc00','first supplementary point');
check('\u{10FFFF}'==='\udbff\udfff','last supplementary point');
check('a\u{1F600}b'.length===4&&'a\u{1F600}b'.codePointAt(1)===0x1f600,'supplementary UTF16');
var C=class{get 'unicod\u{65}Escape'(){return 7;}set 'def\u{61}ult'(v){this.value=v;}};var c=new C();c.default=8;check(c.unicodeEscape===7&&c.value===8,'class accessor names');
var object={'\u{1f600}':9};check(object['\ud83d\ude00']===9,'object property names');
check(eval("'\\u{000000000000000000000000000000000000000000000041}'")==='A','unbounded zero prefix');
var invalid=['','110000','ffffffffffffffffffffffff','1_0','-1','+1',' 1','1 ','g','1\n','1\\\n'];
for(var i=0;i<invalid.length;i++){var caught=false;try{eval("'\\u{"+invalid[i]+"}'");}catch(e){caught=e instanceof SyntaxError;}check(caught,'invalid escape '+invalid[i]);}
var truncated=false;try{eval("'\\u{123");}catch(e){truncated=e instanceof SyntaxError;}check(truncated,'unterminated escape');
check(Function('return "\\u{1f600}"')()==='\ud83d\ude00','dynamic compilation');
var f=function(){return '\u{1f600}';};check(Function('return ('+f.toString()+')')()()===f(),'decompilation round trip');
var realm=createTest262Realm();check(realm.evalScript('"use strict"; "\\u{41}"')==='A','realm Unicode compiler');
var saved=version();try{for(var n=0;n<2;n++){version(n?170:0);var rejected=false;try{evaluate("'\\u{41}'",'legacy-string-escape');}catch(e){rejected=true;}check(rejected,'earlier edition retains escape grammar '+n);}}finally{version(saved);}
print('STRING-CODEPOINT-ESCAPES checks='+checks+' failures=0');
