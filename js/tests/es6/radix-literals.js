/* Modern numeric grammar must not change legacy source or parseInt grammar. */
var checks=0;
function check(ok, message) {
    ++checks;
    if(!ok) throw Error('FAIL radix literals: '+message);
}
function inEdition(edition, source) {
    var saved=version();
    try { version(edition); return evaluate(source, 'radix-literals'); }
    finally { version(saved); }
}
function syntaxError(edition, source) {
    try { inEdition(edition, source); }
    catch(error) { return error instanceof SyntaxError; }
    return false;
}
var valid=[['0b0',0],['0B1',1],['0b101010',42],['0o0',0],['0O17',15],['0o755',493],
           ['0b100000000000000000000000000000000000000000000000000001',9007199254740992],
           ['0b100000000000000000000000000000000000000000000000000011',9007199254740996],
           ['0o400000000000000001',9007199254740992],
           ['0o400000000000000003',9007199254740996]];
for(var i=0;i<valid.length;i++) {
    var spelling=valid[i][0],expected=valid[i][1];
    check(inEdition(2015,spelling)===expected, 'literal '+spelling);
    check(inEdition(2015,'"use strict"; '+spelling)===expected, 'strict literal '+spelling);
    check(inEdition(2015,'Number("'+spelling+'")')===expected, 'numeric string '+spelling);
    check(inEdition(2015,'Number(" \\uFEFF'+spelling+'\\n ")')===expected, 'surrounding whitespace '+spelling);
    check(syntaxError(0,spelling), 'ES5 source unchanged '+spelling);
    check(syntaxError(170,spelling), 'JS1.7 source unchanged '+spelling);
    check(inEdition(0,'isNaN(Number("'+spelling+'"))'), 'ES5 conversion unchanged '+spelling);
    check(inEdition(170,'isNaN(Number("'+spelling+'"))'), 'JS1.7 conversion unchanged '+spelling);
}
var invalid=['0b','0B','0o','0O','0b2','0b102','0o8','0o128','00b1','00o1',
             '0b1e2','0o1e2','0b1_0','0o1_0','0b 1','0o 1','0b+1','0o-1'];
for(i=0;i<invalid.length;i++) {
    check(syntaxError(2015,invalid[i]), 'invalid literal '+invalid[i]);
    check(inEdition(2015,'isNaN(Number("'+invalid[i]+'"))'), 'invalid string '+invalid[i]);
}
check(inEdition(2015,'isNaN(Number("+0b1")) && isNaN(Number("-0b1")) && isNaN(Number("+0o1")) && isNaN(Number("-0o1"))'), 'signed string prefixes');
check(inEdition(2015,'isNaN(Number("-0x10")) && isNaN(Number("+0X10"))'), 'signed hex strings');
check(inEdition(0,'Number("-0x10")===-16'), 'default signed hex extension retained');
check(inEdition(170,'Number("+0X10")===16'), 'legacy signed hex extension retained');
check(inEdition(2015,'-0b11===-3 && +0o11===9'), 'unary operators on literals');
check(inEdition(2015,'1/-0b0===-Infinity && 1/-0o0===-Infinity'), 'negative zero');
check(inEdition(2015,'({0b11:7,0o11:8})[3]===7 && ({0o11:8})[9]===8'), 'property names');
check(inEdition(2015,'0b11.toString()==="3" && 0o11.toString()==="9"'), 'property access');
check(inEdition(2015,'parseInt("0b11")===0 && parseInt("0o11")===0 && parseFloat("0b11")===0 && parseFloat("0o11")===0'), 'parsers retain their grammar');
check(inEdition(2015,'+"0b101"===5 && "0o12"*2===20 && isFinite("0b1")'), 'abstract numeric conversion');
check(inEdition(2015,'var text="0b"+Array(1101).join("1"); Number(text)===Infinity'), 'overflow');
check(inEdition(2015,'Number("0b"+Array(1101).join("0"))===0'), 'long zero');
inEdition(170,'function legacyRadixValue(){return "0b101";}');
check(inEdition(2015,'Number({valueOf:legacyRadixValue})===5 && version()===2015'), 'coercion callback restores caller edition');
check(inEdition(2015,'function radixRoundTrip(){return 0b101+0o17;} eval("("+radixRoundTrip.toString()+")")()===20'), 'function decompilation');
print('ES6-RADIX-LITERALS checks='+checks+' failures=0');
