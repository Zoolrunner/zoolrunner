/* Numeric boundaries shared by modern and historical application globals. */
var checks = 0;
function check(ok, label) { ++checks; if (!ok) throw Error('FAIL Math numeric: ' + label); }
function same(a, b, label) {
    check(a === b ? a !== 0 || 1/a === 1/b : a !== a && b !== b, label);
}
function close(a, b, label) {
    check(a === b || a !== a && b !== b || isFinite(a) && Math.abs(a-b) <= Math.max(Number.MIN_VALUE, Math.abs(b)*Number.EPSILON*4), label);
}
var names = ['expm1','log1p','cbrt','asinh','tanh','acosh','atanh','cosh','sinh','log10','log2','fround','hypot'];
for (var i=0;i<names.length;++i) {
    var name=names[i], fn=Math[name], d=Object.getOwnPropertyDescriptor(Math,name);
    check(typeof fn==='function',name+' exists');
    check(fn.length===(name==='hypot'?2:1),name+' arity');
    check(d.writable && !d.enumerable && d.configurable,name+' descriptor');
    check(!fn.hasOwnProperty('prototype'),name+' no prototype');
    var rejected=false;try{new fn();}catch(e){rejected=e instanceof TypeError;}
    check(rejected,name+' not constructible');
    var calls=0;
    close(fn({valueOf:function(){++calls;gc();return 2;}}),fn(2),name+' coercion');
    check(calls===1,name+' converts once');
    var sentinel={},caught=false;
    try{fn({valueOf:function(){throw sentinel;}});}catch(e){caught=e===sentinel;}
    check(caught,name+' exception');
}
var odd=['expm1','log1p','cbrt','asinh','tanh','atanh','sinh','fround'];
for(i=0;i<odd.length;++i){same(Math[odd[i]](0),0,odd[i]+' +zero');same(Math[odd[i]](-0),-0,odd[i]+' -zero');same(Math[odd[i]](NaN),NaN,odd[i]+' NaN');}
var cases=[['acosh',1,0],['acosh',0,NaN],['acosh',Infinity,Infinity],
 ['atanh',1,Infinity],['atanh',-1,-Infinity],['atanh',2,NaN],
 ['asinh',Infinity,Infinity],['asinh',-Infinity,-Infinity],
 ['tanh',Infinity,1],['tanh',-Infinity,-1],['cosh',0,1],['cosh',-Infinity,Infinity],
 ['sinh',Infinity,Infinity],['sinh',-Infinity,-Infinity],
 ['expm1',-Infinity,-1],['expm1',Infinity,Infinity],
 ['log1p',-1,-Infinity],['log1p',-2,NaN],['log1p',Infinity,Infinity],
 ['cbrt',Infinity,Infinity],['cbrt',-Infinity,-Infinity],
 ['log10',0,-Infinity],['log10',-0,-Infinity],['log10',1,0],['log10',-1,NaN],['log10',Infinity,Infinity],
 ['log2',0,-Infinity],['log2',-0,-Infinity],['log2',1,0],['log2',-1,NaN],['log2',Infinity,Infinity],
 ['fround',Infinity,Infinity],['fround',-Infinity,-Infinity],['fround',Number.MAX_VALUE,Infinity],
 ['fround',-Number.MAX_VALUE,-Infinity],['fround',Number.MIN_VALUE,0],['fround',-Number.MIN_VALUE,-0]];
for(i=0;i<cases.length;++i)same(Math[cases[i][0]](cases[i][1]),cases[i][2],cases[i][0]+' special '+i);
for(i=-1074;i<=1023;++i){
 var power=Math.pow(2,i);
 same(Math.log2(power),i,'log2 power '+i);
 if(i%3===0){close(Math.cbrt(power),Math.pow(2,i/3),'cbrt power '+i);close(Math.cbrt(-power),-Math.pow(2,i/3),'negative cbrt power '+i);}
}
for(i=-1074;i<=-54;i+=17){var tiny=Math.pow(2,i);same(Math.expm1(tiny),tiny,'expm1 tiny '+i);same(Math.log1p(tiny),tiny,'log1p tiny '+i);same(Math.expm1(-tiny),-tiny,'expm1 negative tiny '+i);same(Math.log1p(-tiny),-tiny,'log1p negative tiny '+i);}
for(i=-149;i<=127;++i)same(Math.fround(Math.pow(2,i)),Math.pow(2,i),'fround power '+i);
var unit=Math.pow(2,-149);
same(Math.fround(unit/2),0,'subnormal even zero');same(Math.fround(-unit/2),-0,'negative subnormal even zero');
same(Math.fround(unit*1.5),unit*2,'subnormal odd ties up');same(Math.fround(unit*2.5),unit*2,'subnormal even ties down');
same(Math.fround(1+Math.pow(2,-24)),1,'normal even tie');same(Math.fround(1+3*Math.pow(2,-24)),1+Math.pow(2,-22),'normal odd tie');
same(Math.fround(Math.pow(2,128)-Math.pow(2,103)),Infinity,'overflow tie');
same(Math.fround(Math.pow(2,128)-Math.pow(2,104)),Math.pow(2,128)-Math.pow(2,104),'largest binary32');
same(Math.hypot(),0,'hypot empty');same(Math.hypot(-0,-0),0,'hypot positive zero');
same(Math.hypot(3,4),5,'hypot 3 4');same(Math.hypot(2,3,6),7,'hypot three');
close(Math.hypot(3e200,4e200),5e200,'hypot large');close(Math.hypot(3e-200,4e-200),5e-200,'hypot tiny');
same(Math.hypot(Number.MAX_VALUE),Number.MAX_VALUE,'hypot max');same(Math.hypot(Number.MIN_VALUE),Number.MIN_VALUE,'hypot min');
same(Math.hypot(NaN,Infinity),Infinity,'infinity wins NaN');same(Math.hypot(-Infinity,NaN),Infinity,'negative infinity wins NaN');
var order='';same(Math.hypot({valueOf:function(){order+='a';gc();return Infinity;}},{valueOf:function(){order+='b';gc();return NaN;}},{valueOf:function(){order+='c';gc();return 2;}}),Infinity,'hypot all converted');check(order==='abc','hypot left to right');
caught=false;try{Math.hypot(Infinity,{valueOf:function(){throw sentinel;}});}catch(e){caught=e===sentinel;}check(caught,'hypot infinity does not swallow exceptions');
var many=[];for(i=0;i<1024;++i)many.push(1);same(Math.hypot.apply(null,many),32,'hypot many arguments');
print('ES6-MATH-NUMERIC checks='+checks+' failures=0');
