var checks=0;
function same(a,b,label){++checks;if(a!==b)throw Error(label+': '+a+' != '+b);}
function throws(fn,C,label){++checks;try{fn();}catch(e){if(e instanceof C)return;throw e;}throw Error(label);}
var modes=['toExponential','toPrecision'], values=[NaN,Infinity,-Infinity], digits=[undefined,NaN,Infinity,-Infinity,-1000,1000,0,21];
for(var m=0;m<modes.length;++m)
 for(var v=0;v<values.length;++v)
  for(var d=0;d<digits.length;++d){
   same(values[v][modes[m]](digits[d]),String(values[v]),'nonfinite primitive');
   same(new Number(values[v])[modes[m]](digits[d]),String(values[v]),'nonfinite boxed');
  }
var spellings=[[0,'0e+0'],[-0,'0e+0'],[1,'1e+0'],[10,'1e+1'],[100,'1e+2'],[1200,'1.2e+3'],[-100,'-1e+2'],[1e20,'1e+20'],[1e-20,'1e-20'],[1.23e30,'1.23e+30']];
for(var i=0;i<spellings.length;++i){same(spellings[i][0].toExponential(),spellings[i][1],'omitted precision');same(spellings[i][0].toExponential(undefined),spellings[i][1],'undefined precision');}
same((100).toExponential(2),'1.00e+2','explicit precision retains zeros');
same((1200).toExponential(3),'1.200e+3','explicit precision trailing zeros');
var invalid=[-Infinity,Infinity,NaN,-1,0,1,1.9,37,4294967298,-4294967294,1e100];
for(var i=0;i<invalid.length;++i){var r=invalid[i];throws(function(){return (10).toString(r);},RangeError,'invalid radix');}
same((10).toString(2.9),'1010','truncated radix');same((35).toString(36.9),'z','upper radix');
var count=0;
same((10).toString({valueOf:function(){++count;gc();return 2;}}),'1010','radix callback');same(count,1,'radix callback count');
for(var m=0;m<modes.length;++m){
 count=0;same(Infinity[modes[m]]({valueOf:function(){++count;gc();return Infinity;}}),'Infinity','precision conversion before special value');same(count,1,'precision callback count');
 var sentinel={};try{NaN[modes[m]]({valueOf:function(){throw sentinel;}});}catch(e){same(e,sentinel,'conversion exception');}
 count=0;throws(function(){Number.prototype[modes[m]].call({}, {valueOf:function(){++count;return 1;}});},TypeError,'receiver before conversion');same(count,0,'receiver conversion order');
 throws(function(){return (1)[modes[m]](Infinity);},RangeError,'finite precision range');
}
throws(function(){return Infinity.toFixed(1000);},RangeError,'fixed retains range-first order');
print('NUMBER-FORMATTING checks='+checks+' failures=0');
