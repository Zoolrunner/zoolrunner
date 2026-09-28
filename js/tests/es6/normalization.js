/* Unicode normalization, including historical UTF-16 strings and callbacks. */
var checks=0;
function check(ok,label){++checks;if(!ok)throw Error('FAIL normalize: '+label);}
function throwsType(fn,type,label){var caught=false;try{fn();}catch(e){caught=e instanceof type;}check(caught,label);}
var method=String.prototype.normalize;
check(typeof method==='function' && method.length===0,'method and arity');
var descriptor=Object.getOwnPropertyDescriptor(String.prototype,'normalize');
check(descriptor.writable && descriptor.configurable && !descriptor.enumerable,'descriptor');
check(!method.hasOwnProperty('prototype'),'no prototype');
throwsType(function(){new method();},TypeError,'not constructible');
throwsType(function(){method.call(null);},TypeError,'null receiver');
throwsType(function(){method.call(undefined);},TypeError,'undefined receiver');
var forms=['NFC','NFD','NFKC','NFKD'];
for(var i=0;i<forms.length;++i){
 check(''.normalize(forms[i])==='','empty '+forms[i]);
 check('a\x00z'.normalize(forms[i])==='a\x00z','NUL '+forms[i]);
 check('\ud800x\udc00'.normalize(forms[i])==='\ud800x\udc00','lone surrogates '+forms[i]);
 check('\ud83d\ude00'.normalize(forms[i])==='\ud83d\ude00','supplementary '+forms[i]);
}
check('e\u0301'.normalize()==='\u00e9','default NFC');
check('e\u0301'.normalize(undefined)==='\u00e9','undefined form');
check('\u00e9'.normalize('NFD')==='e\u0301','canonical decomposition');
check('\ufb03'.normalize('NFC')==='\ufb03','canonical keeps ligature');
check('\ufb03'.normalize('NFKC')==='ffi','compatibility ligature');
check('\u2460'.normalize('NFKD')==='1','compatibility circled digit');
check('\uac01'.normalize('NFD')==='\u1100\u1161\u11a8','Hangul decomposition');
check('\u1100\u1161\u11a8'.normalize()==='\uac01','Hangul composition');
check('A\u030a\u0301'.normalize()==='\u01fa','successive composition');
check('q\u0315\u0300'.normalize('NFD')==='q\u0300\u0315','canonical order');
check('q\u0301\u0300'.normalize('NFD')==='q\u0301\u0300','equal classes stay stable');
check('\u0344'.normalize()==='\u0308\u0301','composition exclusion');
check(method.call(123)==='123','generic receiver');
var invalid=['nfc','','NFK','NFCx','NFD\x00',null,0];
for(i=0;i<invalid.length;++i)(function(form){throwsType(function(){''.normalize(form);},RangeError,'invalid form '+form);})(invalid[i]);
var order='',sentinel={},caught=false;
check(method.call({toString:function(){order+='r';gc();return 'e\u0301';}},{toString:function(){order+='f';gc();return 'NFC';}})==='\u00e9' && order==='rf','coercion order and GC');
order='';try{method.call({toString:function(){order+='r';throw sentinel;}},{toString:function(){order+='f';return 'NFC';}});}catch(e){caught=e===sentinel;}check(caught && order==='r','receiver exception');
caught=false;try{'x'.normalize({toString:function(){throw sentinel;}});}catch(e){caught=e===sentinel;}check(caught,'form exception');
check('e\u0301'.normalize({toString:function(){check('\ufb03'.normalize('NFKC')==='ffi','reentrant inner');gc();return 'NFC';}})==='\u00e9','reentrant outer');
var source='';for(i=0;i<128;++i)source+='\u0315\u0300';
check(source.normalize('NFD')==='\u0300'.repeat(128)+'\u0315'.repeat(128),'long combining sequence');
check('x'.substring(0)==='x' && '\u00e9'.length===1,'legacy strings unchanged');
check(String.fromCodePoint(0x1ccd6).normalize('NFKC')==='A','supplementary compatibility data');
print('ES6-NORMALIZATION checks='+checks+' failures=0');
