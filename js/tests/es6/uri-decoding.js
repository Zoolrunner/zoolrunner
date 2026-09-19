var checks=0;
function same(a,b,label){++checks;if(a!==b)throw Error(label+': '+a+' != '+b);}
function bad(fn,s){++checks;try{fn(s);}catch(e){if(e instanceof URIError)return;throw e;}throw Error('accepted '+s);}
function hex(n){return '%'+('0'+n.toString(16)).slice(-2);}
var decoders=[decodeURI,decodeURIComponent];
for(var d=0;d<decoders.length;++d){
 var fn=decoders[d];
 for(var c=0xd800;c<=0xdfff;++c)bad(fn,hex(0xe0|(c>>12))+hex(0x80|((c>>6)&63))+hex(0x80|(c&63)));
 var malformed=['%C0%80','%C0%AF','%C1%BF','%E0%80%80','%E0%9F%BF','%F0%80%80%80','%F0%8F%BF%BF','%F4%90%80%80','%F5%80%80%80','%F8%80%80%80%80','%FC%80%80%80%80%80','%FF','%80','%ED%7F%BF','%ED%BF','%','%0','%gg'];
 for(var i=0;i<malformed.length;++i)bad(fn,malformed[i]);
 var points=[0,0x7f,0x80,0x7ff,0x800,0xd7ff,0xe000,0xfffd,0xffff,0x10000,0x10ffff];
 for(var i=0;i<points.length;++i){var c=points[i],s=c<65536?String.fromCharCode(c):String.fromCharCode(0xd800+((c-65536)>>10),0xdc00+((c-65536)&1023));same(fn(encodeURIComponent(s)),s,'scalar roundtrip '+c);}
 same(fn('\ud800'),'\ud800','literal high surrogate');same(fn('\udfff'),'\udfff','literal low surrogate');
 var calls=0;bad(fn,{toString:function(){++calls;gc();return '%ED%BF%BF';}});same(calls,1,'conversion count');
 var sentinel={};try{fn({toString:function(){throw sentinel;}});}catch(e){same(e,sentinel,'conversion throw');}
}
same(decodeURI('%2f%3f%23'),'%2f%3f%23','URI reserved spelling');same(decodeURIComponent('%2f%3f%23'),'/?#','component reserved');
print('URI-DECODING checks='+checks+' failures=0');
