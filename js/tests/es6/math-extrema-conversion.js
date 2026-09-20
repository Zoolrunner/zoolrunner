/* Math extrema must convert every argument, including those after NaN. */
var checks=0;
function check(ok,label){++checks;if(!ok)throw Error(label);}
function same(a,b,label){check(a===b ? a!==0 || 1/a===1/b : a!==a && b!==b,label);}
function inEdition(edition,source){var old=version();try{version(edition);return evaluate(source,'math-extrema');}finally{version(old);}}
var sentinel={};
for(var ei=0;ei<3;++ei){
 var edition=[0,170,2015][ei], standard=edition!==170;
 var invoke=inEdition(edition,'(function(fn,args){return fn.apply(null,args);})');
 for(var ni=0;ni<2;++ni){
  var name=['max','min'][ni], fn=Math[name], label=name+' '+edition;
  same(invoke(fn,[]),ni?Infinity:-Infinity,label+' empty');
  same(invoke(fn,[3,9,-2]),ni?-2:9,label+' ordinary');
  same(invoke(fn,[-0,+0]),ni?-0:+0,label+' signed zero');
  same(invoke(fn,[+0,-0]),ni?-0:+0,label+' reversed zero');
  for(var pos=0;pos<4;++pos){
   var order='',args=[];
   for(var j=0;j<4;++j)(function(index){args.push({valueOf:function(){order+=index;gc();same(Math.max(4,5),5,'reentrant max');same(Math.min(4,5),4,'reentrant min');return index===pos?NaN:index;}});})(j);
   same(invoke(fn,args),NaN,label+' NaN '+pos);
   check(order===(standard?'0123':'0123'.slice(0,pos+1)),label+' conversion order '+pos);
  }
  var called=0,thrower={valueOf:function(){++called;gc();throw sentinel;}};
  var caught=false;
  try{invoke(fn,[NaN,thrower]);}catch(e){caught=e===sentinel;}
  check(caught===standard && called===(standard?1:0),label+' exception after NaN');
  caught=false;called=0;
  try{invoke(fn,[thrower,NaN,{valueOf:function(){called+=10;return 1;}}]);}catch(e){caught=e===sentinel;}
  check(caught && called===1,label+' exception stops conversion');
  caught=false;try{invoke(fn,[NaN,Symbol('tail')]);}catch(e){caught=e instanceof TypeError;}
  check(caught===standard,label+' symbol after NaN');
  var foreign=inEdition(edition===170?2015:170,'({valueOf:function(){gc();return 7;}})');
  same(invoke(fn,[foreign,2]),ni?2:7,label+' cross-edition conversion');
 }
}
print('ES6-MATH-EXTREMA checks='+checks+' failures=0');
