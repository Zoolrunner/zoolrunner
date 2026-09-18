/* ES2015 String additions with UTF-16 and callback/GC regressions. */
var checks = 0;
function check(ok, label) { ++checks; if (!ok) throw Error('FAIL String: ' + label); }
function throwsType(fn, type, label) { var caught=false; try { fn(); } catch(e) { caught=e instanceof type; } check(caught,label); }
var names=['codePointAt','repeat','startsWith','endsWith','includes'];
for(var i=0;i<names.length;++i){
 var name=names[i],fn=String.prototype[name],d=Object.getOwnPropertyDescriptor(String.prototype,name);
 check(typeof fn==='function' && fn.length===1,name+' method/arity');
 check(d.writable && d.configurable && !d.enumerable,name+' property');
 check(!fn.hasOwnProperty('prototype'),name+' no prototype');
 throwsType(function(){new fn();},TypeError,name+' not constructible');
 throwsType(function(){fn.call(null);},TypeError,name+' null receiver');
 throwsType(function(){fn.call(undefined);},TypeError,name+' undefined receiver');
}
check(String.fromCodePoint.length===1,'fromCodePoint arity');
d=Object.getOwnPropertyDescriptor(String,'fromCodePoint');check(d.writable && d.configurable && !d.enumerable,'fromCodePoint descriptor');
throwsType(function(){new String.fromCodePoint(1);},TypeError,'fromCodePoint not constructible');
var codes=[0,1,0x7f,0x80,0x7ff,0x800,0xd7ff,0xd800,0xdbff,0xdc00,0xdfff,0xe000,0xffff,0x10000,0x1f600,0x10ffff];
for(i=0;i<codes.length;++i){
 var cp=codes[i],str=String.fromCodePoint(cp);
 check(str.codePointAt(0)===cp,'code point roundtrip '+cp);
 check(str.length===(cp<=0xffff?1:2),'UTF-16 length '+cp);
 if(cp>0xffff)check(str.charCodeAt(0)===0xd800+((cp-0x10000)>>10) && str.charCodeAt(1)===0xdc00+((cp-0x10000)&1023),'surrogate encoding '+cp);
}
check(String.fromCodePoint()==='','empty code point list');
check(String.fromCodePoint(null,false,true,'65')==='\x00\x00\x01A','code point coercions');
check(String.fromCodePoint(-0)==='\x00','negative zero code point');
var invalid=[undefined,NaN,Infinity,-Infinity,-1,0x110000,1.5];
for(i=0;i<invalid.length;++i)(function(v){throwsType(function(){String.fromCodePoint(v);},RangeError,'invalid code point '+v);})(invalid[i]);
str='A\ud83d\ude00B\ud800X\udc00';
var at=[[undefined,65],[NaN,65],[-0.5,65],[1,0x1f600],[2,0xde00],[3,66],[4,0xd800],[5,88],[6,0xdc00],[-1,undefined],[7,undefined],[Infinity,undefined],[-Infinity,undefined]];
for(i=0;i<at.length;++i)check(str.codePointAt(at[i][0])===at[i][1],'codePointAt '+i);
check(''.codePointAt()===undefined,'empty codePointAt');
check(String.prototype.codePointAt.call(123,1)===50,'generic codePointAt');
var order='',sentinel={},caught=false;
check(String.fromCodePoint({valueOf:function(){order+='a';gc();return 65;}},{valueOf:function(){order+='b';gc();return 0x1f600;}})==='A\ud83d\ude00' && order==='ab','fromCodePoint conversion order and GC');
order='';try{String.fromCodePoint({valueOf:function(){order+='a';throw sentinel;}},{valueOf:function(){order+='b';return 1;}});}catch(e){caught=e===sentinel;}check(caught && order==='a','fromCodePoint exception stops conversions');
var repeats=[[undefined,''],[NaN,''],[null,''],[-0.5,''],[0,''],[1,'ab'],[2.9,'abab'],['3','ababab']];
for(i=0;i<repeats.length;++i)check('ab'.repeat(repeats[i][0])===repeats[i][1],'repeat '+i);
check(''.repeat(1e300)==='','empty repeat with large finite count');
check('a\x00\ud800'.repeat(3)==='a\x00\ud800a\x00\ud800a\x00\ud800','repeat preserves code units');
check('xyz'.repeat(7)==='xyzxyzxyzxyzxyzxyzxyz','repeat uneven doubling');
check(String.prototype.repeat.call(12,2)==='1212','generic repeat');
throwsType(function(){''.repeat(Infinity);},RangeError,'infinite empty repeat');
throwsType(function(){'a'.repeat(-1);},RangeError,'negative repeat');
throwsType(function(){'a'.repeat(-Infinity);},RangeError,'negative infinite repeat');
throwsType(function(){'a'.repeat(1e300);},RangeError,'repeat representation limit');
order='';check(String.prototype.repeat.call({toString:function(){order+='r';gc();return 'xy';}},{valueOf:function(){order+='n';gc();return 2;}})==='xyxy' && order==='rn','repeat order/GC');
var searchCases=[
 ['startsWith','abc','a',undefined,true],['startsWith','abc','b',1,true],['startsWith','abc','bc',1.9,true],['startsWith','abc','a',-Infinity,true],['startsWith','abc','a',Infinity,false],['startsWith','abc','',Infinity,true],
 ['endsWith','abc','c',undefined,true],['endsWith','abc','b',2,true],['endsWith','abc','a',1.9,true],['endsWith','abc','',-Infinity,true],['endsWith','abc','a',NaN,false],['endsWith','abc','c',Infinity,true],
 ['includes','abc','bc',undefined,true],['includes','abc','ab',1,false],['includes','abc','bc',1.9,true],['includes','abc','a',-Infinity,true],['includes','abc','',Infinity,true],['includes','abc','c',Infinity,false],
 ['includes','a\x00b','\x00',0,true],['startsWith','\ud83d\ude00','\ud83d',0,true],['endsWith','\ud83d\ude00','\ude00',undefined,true]];
for(i=0;i<searchCases.length;++i){var row=searchCases[i];check(String.prototype[row[0]].call(row[1],row[2],row[3])===row[4],row[0]+' case '+i);}
for(i=2;i<names.length;++i){
 fn=String.prototype[names[i]];
 check(fn.call('undefined',undefined),names[i]+' undefined search');
 check(fn.call(123,'123'),names[i]+' generic receiver');
 check(fn.call('', ''),names[i]+' empty strings');
 throwsType(function(){fn.call('abc',/a/);},TypeError,names[i]+' rejects RegExp');
 order='';var result=fn.call({toString:function(){order+='r';gc();return 'abc';}},{toString:function(){order+='s';gc();return 'b';}},{valueOf:function(){order+='p';gc();return 2;}});
 check(order==='rsp',names[i]+' conversion order and GC');
 order='';var regexp=/a/;regexp.toString=function(){order+='s';return 'a';};caught=false;
 try{fn.call({toString:function(){order+='r';return 'abc';}},regexp,{valueOf:function(){order+='p';return 0;}});}catch(e){caught=e instanceof TypeError;}
 check(caught && order==='r',names[i]+' rejects RegExp before search/position conversions');
}
check('/a/'.indexOf(/a/)===0,'historical indexOf RegExp conversion unchanged');
check('abc'.substr(-1)==='c' && 'abc'.substring(2,1)==='b','historical string methods unchanged');

check(String.raw.length===1 && String.raw.name==='raw','raw metadata');
d=Object.getOwnPropertyDescriptor(String,'raw');check(d.writable && d.configurable && !d.enumerable,'raw descriptor');
throwsType(function(){new String.raw();},TypeError,'raw not constructible');
throwsType(function(){String.raw(null);},TypeError,'raw null template');
throwsType(function(){String.raw(undefined);},TypeError,'raw undefined template');
throwsType(function(){String.raw({raw:null});},TypeError,'raw null property');
throwsType(function(){String.raw({});},TypeError,'raw missing property');
check(String.raw({raw:['a','b','c']},1,2)==='a1b2c','raw substitutions');
check(String.raw({raw:['a','b','c']},1)==='a1bc','raw missing substitutions');
check(String.raw({raw:['a','b']},undefined)==='aundefinedb','raw explicit undefined substitution');
check(String.raw({raw:'xy'},'z')==='xzy','raw boxes a string');
check(String.raw({raw:{length:2.9,0:'a',1:'b',2:'c'}},'!')==='a!b','raw length ToLength');
check(String.raw({raw:{length:0,get 0(){throw Error('unreachable');}}})==='','raw zero length avoids elements');
check(String.raw({raw:['a']},{toString:function(){throw Error('unreachable');}})==='a','raw excess substitutions ignored');
check(String.raw({raw:['\x00','\ud800']},'\udc00')==='\x00\udc00\ud800','raw preserves code units');
order='';var rawObject={};
Object.defineProperty(rawObject,'length',{get:function(){order+='l';gc();return 2;}});
Object.defineProperty(rawObject,'0',{get:function(){order+='0';gc();return {toString:function(){order+='a';gc();return 'A';}};}});
Object.defineProperty(rawObject,'1',{get:function(){order+='1';gc();return 'B';}});
var template={};Object.defineProperty(template,'raw',{get:function(){order+='r';gc();return rawObject;}});
check(String.raw(template,{toString:function(){order+='s';gc();return 'S';}})==='ASB' && order==='rl0as1','raw access/coercion order and GC');
var mutable={length:2,get 0(){this.length=0;gc();return String.raw({raw:['x','y']},'!');},1:'z'};
check(String.raw({raw:mutable},'?')==='x!y?z','raw reentrancy and length snapshot');
var inherited=Object.create({0:'a',1:'b'});inherited.length=2;
check(String.raw({raw:inherited},'-')==='a-b','raw inherited elements');
caught=false;try{String.raw({raw:{get length(){throw sentinel;}}});}catch(e){caught=e===sentinel;}check(caught,'raw length exception');
caught=false;try{String.raw({raw:{length:1,get 0(){throw sentinel;}}});}catch(e){caught=e===sentinel;}check(caught,'raw element exception');
order='';caught=false;try{String.raw({raw:{length:2,0:'a',get 1(){order+='1';return 'b';}}},{toString:function(){throw sentinel;}});}catch(e){caught=e===sentinel;}check(caught && order==='','raw substitution exception order');
var many=[];for(i=0;i<100;++i)many.push('abc');check(String.raw({raw:many})==='abc'.repeat(100),'raw buffer growth');
print('ES6-STRING-ADDITIONS checks='+checks+' failures=0');
