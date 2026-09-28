/* Canonical collation without imposing compatibility normalization. */
var checks=0;
function same(a,b,label){checks++;if(a!==b)throw Error(label+': '+a+' != '+b);}
function sign(x){return x<0?-1:x>0?1:0;}
function collect(){if(typeof gc==='function')gc();}
var pairs=[['o\u0308','\u00f6'],['\u00e4\u0323','a\u0323\u0308'],
 ['\u1111\u1171\u11b6','\ud4db'],['\u212b','\u00c5'],['\u2126','\u03a9'],
 ['\u1e0b\u0323','\u1e0d\u0307'],['\ud834\udd5e','\ud834\udd57\ud834\udd65']];
for(var i=0;i<pairs.length;i++){
 var a=pairs[i][0],b=pairs[i][1];
 same(a.localeCompare(b),0,'canonical equivalence');
 same(b.localeCompare(a),0,'symmetric equivalence');
 same(sign(a.localeCompare('z')),sign(b.localeCompare('z')),'consistent third-string ordering');
}
same('\ufb01'.localeCompare('fi')===0,false,'compatibility ligature remains distinct');
same('\uff21'.localeCompare('A')===0,false,'compatibility width remains distinct');
var events=[];
same(String.prototype.localeCompare.call({toString:function(){events.push('this');collect();return 'o\u0308';}},
 {toString:function(){events.push('that');collect();return '\u00f6';}}),0,'coercion and collection');
same(events.join(','),'this,that','coercion order');
var sentinel={},caught;
try{String.prototype.localeCompare.call({toString:function(){throw sentinel;}},{toString:function(){throw Error('unreached');}});}catch(e){caught=e;}
same(caught,sentinel,'receiver exception');
same('undefined'.localeCompare(),0,'absent argument conversion');
same('\ud800'.localeCompare('\ud800'),0,'lone surrogate retained');
same('\ud800'.localeCompare('\ud801')<0,true,'lone surrogate ordering');
same('a'.localeCompare('Z')<0,true,'case-folded primary ordering');
same('B'.localeCompare('c')<0,true,'case-folded mixed case');
same('A'.localeCompare('a')<0,true,'stable secondary ordering');
same('\ud801\udc00'.localeCompare('\ud801\udc29')<0,true,'supplementary primary ordering');
print('LOCALE-COMPARE checks='+checks+' failures=0');
