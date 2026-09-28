/* ES2015 RegExpExec, @@match and @@search observability and GC regressions.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
var checks = 0;
function check(value, name) { ++checks; if (!value) throw Error('RegExp protocols: ' + name); }
function caught(f) { try { f(); } catch (e) { return e; } return null; }
var match = RegExp.prototype[Symbol.match], search = RegExp.prototype[Symbol.search];
var test = RegExp.prototype.test, r, d, marker = {}, log, index, result;
for (var i = 0; i < 2; ++i) {
    var method = i ? search : match, key = i ? Symbol.search : Symbol.match;
    d = Object.getOwnPropertyDescriptor(RegExp.prototype, key);
    check(d.value === method && d.writable && d.configurable && !d.enumerable, 'method descriptor ' + i);
    check(method.name === (i ? '[Symbol.search]' : '[Symbol.match]') && method.length === 1 &&
          !method.hasOwnProperty('prototype'), 'method metadata ' + i);
    check(caught(function() { new method('x'); }) instanceof TypeError, 'not constructible ' + i);
    check(caught(function() { method.call(null, 'x'); }) instanceof TypeError, 'raw null receiver ' + i);
}
check(match.call(/a/, 'ba')[0] === 'a', 'nonglobal native match');
check(match.call(/a/g, 'aba').join() === 'a,a', 'global native match');
check(match.call(/z/g, 'aba') === null, 'global no matches');
check(match.call(/(?:)/g, 'ab').length === 3, 'empty native matches progress');
check(search.call(/a/, 'ba') === 1 && search.call(/z/, 'ba') === -1, 'native search');
r = /a/g; r.lastIndex = 9;
check(search.call(r, 'ba') === 1 && r.lastIndex === 9, 'search restores index');
r = /a/; r.exec = 7;
check(match.call(r, 'a')[0] === 'a' && test.call(r, 'a'), 'noncallable exec falls back');
check(caught(function() { match.call({exec: 7}, 'a'); }) instanceof TypeError, 'fallback requires matcher');
result = {marker: marker};
r = {exec: function(s) { check(this === r && s === 'value', 'exec receiver and input'); gc(); return result; }};
check(match.call(r, {toString:function(){return 'value';}}) === result, 'nonglobal result preserved');
check(test.call({exec:function(){gc();return {};}}, 'x') === true, 'generic test true');
check(test.call({exec:function(){return null;}}, 'x') === false, 'generic test false');
check(caught(function() { test.call({exec:function(){return 1;}}, 'x'); }) instanceof TypeError, 'exec primitive result rejected');
check(caught(function() { test.call({get exec(){gc();throw marker;}}, 'x'); }) === marker, 'exec getter abrupt');
log = [];
r = {get global(){log.push('global');return false;}, get exec(){log.push('exec');return function(s){log.push(s);return null;};}};
check(match.call(r, {toString:function(){log.push('string');return 'input';}}) === null &&
      log.join() === 'string,global,exec,input', 'match conversion order');
log = []; index = -0;
r = {get lastIndex(){log.push('get');return index;}, set lastIndex(v){log.push('set:'+v);index=v;},
     exec:function(s){gc();log.push('exec');return {get index(){log.push('result');return marker;}};}};
check(search.call(r, 'x') === marker && 1/index === -Infinity &&
      log.join() === 'get,set:0,exec,set:0,result', 'search writes zero and restores saved negative zero');
log = []; index = 0;
r = {get lastIndex(){log.push('get');return index;},set lastIndex(v){log.push('set:'+v);index=v;},
     exec:function(){log.push('exec');return null;}};
check(search.call(r, 'x') === -1 && log.join() === 'get,set:0,exec,set:0', 'search writes zero and restores unchanged lastIndex');
log = []; index = -0;
r = {get lastIndex(){return index;},set lastIndex(v){log.push('set:'+v);index=v;},
     exec:function(){return null;}};
check(search.call(r, 'x') === -1 && 1/index === -Infinity && log.join() === 'set:0,set:0', 'search distinguishes negative zero');
log = []; index = 7;
r = {get lastIndex(){return index;}, set lastIndex(v){log.push(v);index=v;}, exec:function(){throw marker;}};
check(caught(function(){search.call(r,'x');}) === marker && index === 0 && log.join() === '0', 'search does not restore on exec throw');
r = {lastIndex:3,exec:function(){return null;}};
Object.defineProperty(r,'lastIndex',{writable:false});
check(caught(function(){search.call(r,'x');}) instanceof TypeError, 'search checks write rejection');
log = []; index = 0;
r = {global:true,unicode:true,get lastIndex(){return index;},set lastIndex(v){index=v;log.push(v);},
     exec:function(s){gc();return index<=s.length ? [''] : null;}};
check(match.call(r,'\ud83d\ude00x').length === 3 && log.join() === '0,2,3,4', 'Unicode progress over surrogate pair');
log = []; index = 0;
r.unicode = false;
check(match.call(r,'\ud83d\ude00').length === 3 && log.join() === '0,1,2,3', 'code-unit progress without Unicode');
var calls=0;
r = {global:true,exec:function(){return calls++ ? null : {0:{toString:function(){gc();return 'converted';}}};}};
check(match.call(r,'x')[0] === 'converted', 'global matches stringify values');
var raw={exec:function(){return null;}};
var proxy=new Proxy(raw,{get:function(t,k,recv){check(recv===proxy,'proxy getter receiver');gc();return t[k];}});
check(match.call(proxy,'x')===null,'generic Proxy receiver');
r = /a/;
Object.defineProperty(r,'global',{value:true});
check(match.call(r,'aa').length===2 && r.lastIndex===0,'global failure resets lastIndex');
r = /a/;
Object.defineProperty(r,'sticky',{value:true});
check(r.exec('ba')===null && r.lastIndex===0,'observable sticky flag anchors matcher');
r.lastIndex=1;
check(r.exec('ba')[0]==='a' && r.lastIndex===2,'sticky match updates index');
r = /(?:a|b)/;
Object.defineProperty(r,'sticky',{value:true});
check(r.exec('xb')===null,'sticky compound pattern cannot scan forward');
r = /a/g; r.lastIndex=-1;
check(r.test('a') && r.lastIndex===1,'exec ToLength clamps negative index');
log=[]; r=/a/;
r.lastIndex={valueOf:function(){log.push('index');gc();return 0;}};
Object.defineProperty(r,'global',{get:function(){log.push('global');return false;}});
Object.defineProperty(r,'sticky',{get:function(){log.push('sticky');return false;}});
check(r.exec({toString:function(){log.push('string');return 'a';}})[0]==='a' &&
      log.join()==='string,index,global,sticky','exec observable conversion and getter order');
r=/a/;
Object.defineProperty(r,'sticky',{get:function(){r.compile('b');gc();return false;}});
check(r.exec('b')[0]==='b','matcher selected after reentrant flag getter');
r=/z/;r.lastIndex=5;
Object.defineProperty(r,'lastIndex',{writable:false});
check(caught(function(){r.exec('abc');}) instanceof TypeError && r.lastIndex===5,'nonglobal failure resets lastIndex');
log=[];
check(caught(function(){RegExp.prototype.exec.call({}, {toString:function(){log.push('string');return 'x';}});}) instanceof TypeError &&
      log.length===0,'exec validates matcher before string conversion');
log=[];
check(test.call({get exec(){log.push('exec');return function(){return null;};}},
                {toString:function(){log.push('string');return 'x';}})===false &&
      log.join()==='string,exec','generic test converts input before exec getter');
check('aba'.match(/a/g).join()==='a,a' && 'ba'.search(/a/)===1,'String entry points dispatch');
check('ab'.match('b')[0]==='b' && 'ab'.search('b')===1,'String fallback constructs intrinsic RegExp');
check('x'.match()[0]==='' && 'x'.search()===0,'String missing pattern creates empty regexp');
var stringReceiver={toString:function(){throw marker;}};
for (i=0;i<2;++i) {
    var skey=i?Symbol.search:Symbol.match, smethod=i?String.prototype.search:String.prototype.match;
    var pattern={};
    pattern[skey]=function(value){gc();check(this===pattern && value===stringReceiver,'String hook raw receiver '+i);return marker;};
    check(smethod.call(stringReceiver,pattern)===marker,'custom String hook skips coercion '+i);
    pattern[skey]=3;
    check(caught(function(){smethod.call(stringReceiver,pattern);}) instanceof TypeError,'noncallable String hook before coercion '+i);
}
log=[];
var pattern={toString:function(){log.push('pattern');return 'b';}};
Object.defineProperty(pattern,Symbol.match,{get:function(){log.push('method');return null;}});
check(String.prototype.match.call({toString:function(){log.push('string');return 'ab';}},pattern)[0]==='b' &&
      log.join()==='method,string,pattern','String fallback conversion order');
Object.defineProperty(Number.prototype,Symbol.search,{configurable:true,get:function(){'use strict';check(this===7,'primitive pattern getter receiver');return function(value){'use strict';check(this===7 && value===stringReceiver,'primitive pattern method receiver');return 91;};}});
try { check(String.prototype.search.call(stringReceiver,7)===91,'primitive pattern protocol'); }
finally { delete Number.prototype[Symbol.search]; }
var savedRegExp=RegExp;
try { RegExp=function(){throw marker;};check('ab'.search('b')===1,'String fallback uses cached intrinsic'); }
finally { RegExp=savedRegExp; }
print('ES6-REGEXP-PROTOCOLS checks='+checks+' failures=0');
