/* Annex B identity fallback must consume no partial hex digits. */
var checks=0;
function same(a,b,label){checks++;if(a!==b)throw Error(label+': '+a+' != '+b);}
var patterns=['\\x','\\xA','\\x0G','\\u','\\uA','\\uAB','\\uABC','\\uABCG'];
for(var i=0;i<patterns.length;i++){
 var p=patterns[i],text=p.slice(1),r=new RegExp('^(?:'+p+')$');
 same(r.test(text),true,'whole identity escape '+p);
 var end=new RegExp(p);
 same(end.exec(text)[0],text,'end-of-pattern escape '+p);
 var cls=new RegExp('['+p+']');
 if(typeof gc==='function')gc();
 same(cls.test(text[0]),true,'lazy character class '+p);
 same(cls.test('\\'),false,'identity class excludes backslash '+p);
 var caught=false;
 try{new RegExp(p,'u');}catch(e){caught=e instanceof SyntaxError;}
 same(caught,true,'unicode escape stays strict '+p);
 caught=false;
 try{new RegExp('['+p+']','u');}catch(e){caught=e instanceof SyntaxError;}
 same(caught,true,'unicode class stays strict '+p);
 same(eval(end.toString()).test(text),true,'regexp source roundtrip '+p);
}
same(/\x/.test('x'),true,'literal at end');
same(/[\u-\x]/.test('v'),true,'identity range endpoints');
same(/[\x41-\x43]/.test('B'),true,'complete hex range');
same(/[\uFFFF]/.test('\uffff'),true,'complete wide escape');
same(/[^\x]/.test('x'),false,'negated identity class');
same('x'.split(/\x/).join('|'),'|','split uses identity escape');
print('REGEXP-INCOMPLETE-HEX checks='+checks+' failures=0');
