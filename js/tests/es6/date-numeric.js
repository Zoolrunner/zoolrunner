/* Original ES2015 Date arithmetic and callback semantics. */
var checks=0;
function same(actual,expected,label){++checks;if(!Object.is(actual,expected))throw Error(label+': '+actual+' != '+expected);}
function collect(){if(typeof gc==='function')gc();}
same(1/new Date(-0.1).getTime(),Infinity,'constructor truncates to positive zero');
same(1/new Date(-0).getTime(),Infinity,'constructor clips negative zero');
same(1/new Date(1).setTime(-0.1),Infinity,'setTime positive zero');
same(Date.UTC(1970,0,0),-86400000,'explicit zero day');
same(Date.UTC(1970,0,-1),-172800000,'explicit negative day');
same(Date.UTC(1970,0),0,'omitted day');
same(Date.UTC(99.9,0),Date.UTC(1999,0),'fractional year offset');
same(Date.UTC(-0.9,0),Date.UTC(1900,0),'negative fractional year offset');
same(Date.UTC(1970,-0.9,1.9,0.9,0.9,0.9,0.9),0,'truncate fields');
var fields=[1970,0,1,0,0,0,0];
for(var i=0;i<7;++i){
 for(var j=0;j<3;++j){var args=fields.slice();args[i]=[Infinity,-Infinity,NaN][j];same(Date.UTC.apply(null,args),NaN,'nonfinite UTC '+i);}
}
var methods=['setMilliseconds','setSeconds','setMinutes','setHours','setUTCMilliseconds','setUTCSeconds','setUTCMinutes','setUTCHours'];
for(var i=0;i<methods.length;++i){
 var name=methods[i], count=i%4+1, d=new Date(NaN), seen=[], args=[];
 for(var j=0;j<count;++j)(function(n){args.push({valueOf:function(){seen.push(n);collect();d.setTime(0);return n;}});})(j);
 same(d[name].apply(d,args),NaN,name+' invalid result');
 same(d.getTime(),NaN,name+' original ES2015 final store');
 same(seen.join(','),[0,1,2,3].slice(0,count).join(','),name+' converts all invalid-date arguments');
 d=new Date(0);seen=[];
 args=[{valueOf:function(){seen.push(0);return NaN;}}];
 for(var j=1;j<count;++j)(function(n){args.push({valueOf:function(){seen.push(n);collect();return 0;}});})(j);
 same(d[name].apply(d,args),NaN,name+' nonfinite result');
 same(seen.length,count,name+' does not skip later coercions');
 var sentinel={};d=new Date(0);
 try{d[name]({valueOf:function(){d.setTime(1234);collect();throw sentinel;}});}catch(e){same(e,sentinel,name+' propagates callback exception');}
 same(d.getTime(),1234,name+' abrupt completion retains callback mutation');
 d=new Date(0);var expected=new Date(0)[name](5);
 same(d[name]({valueOf:function(){d.setTime(86400000);collect();return 5;}}),expected,name+' snapshots date before coercion');
}
var events=[],args=[];
for(var i=0;i<7;++i)(function(n){args.push({valueOf:function(){events.push(n);collect();return n===0?NaN:0;}});})(i);
same(Date.UTC.apply(null,args),NaN,'UTC invalid');
same(events.join(','),'0,1,2,3,4,5,6','UTC converts all fields');
events=[];
same(new Date(args[0],args[1],args[2],args[3],args[4],args[5],args[6]).getTime(),NaN,'constructor invalid');
same(events.join(','),'0,1,2,3,4,5,6','constructor converts all fields');
same(Date.UTC(1970,0,100000001),8640000000000000,'maximum time');
same(Date.UTC(1970,0,100000002),NaN,'outside maximum time');
same(Date.UTC(1e100,0),NaN,'large year');
same(new Date(1e100,0).getTime(),NaN,'large local year');
var calendarMethods=['setDate','setMonth','setFullYear','setUTCDate','setUTCMonth','setUTCFullYear','setYear'];
for(var i=0;i<calendarMethods.length;++i){
 var name=calendarMethods[i], count=i===6?1:i%3+1;
 var d=new Date(0), seen=[],args=[];
 for(var j=0;j<count;++j)(function(n){args.push({valueOf:function(){seen.push(n);collect();return n===0?NaN:1;}});})(j);
 same(d[name].apply(d,args),NaN,name+' invalid field');
 same(seen.length,count,name+' converts after nonfinite');
 d=new Date(0);var expected=new Date(0)[name](5);
 same(d[name]({valueOf:function(){d.setTime(123456789);collect();return 5;}}),expected,name+' date snapshot');
 var sentinel={};d=new Date(0);var caught;
 try{d[name]({valueOf:function(){d.setTime(1234);collect();throw sentinel;}});}catch(e){caught=e;}
 same(caught,sentinel,name+' callback exception');
 same(d.getTime(),1234,name+' abrupt mutation');
 if(i%3!==2&&i!==6){
  d=new Date(NaN);
  same(d[name]({valueOf:function(){d.setTime(0);return 1;}}),NaN,name+' invalid snapshot');
  same(d.getTime(),NaN,name+' original final store');
 }else{
  d=new Date(NaN);expected=new Date(NaN)[name](2000);
  same(d[name]({valueOf:function(){d.setTime(123456789);collect();return 2000;}}),expected,name+' invalid snapshot defaults');
 }
 same(new Date(0)[name](1e100),NaN,name+' extreme field');
}
var d=new Date(0);d.setYear(-0.9);same(d.getFullYear(),1900,'setYear truncates before offset');
same(Date.parse('1970-01-01T00:00:00'),new Date(1970,0,1).getTime(),'offsetless local datetime');
same(new Date('1970-01-01T00:00:00').getTime(),new Date(1970,0,1).getTime(),'constructor local datetime');
same(Date.parse('1970-01-01'),0,'date-only UTC compatibility');
same(Date.parse('1970-01-01T00:00:00Z'),0,'explicit UTC');
same(Date.parse('1970-01-01T00:00:00+01:00'),-3600000,'explicit offset');
same(Date.parse('2000-07-01T12:34:56.789'),new Date(2000,6,1,12,34,56,789).getTime(),'local summer datetime');
same(Date.parse('+999999-01-01T00:00:00'),NaN,'out of range local parse');
same(Date.parse('1970-01-01T24:00'),new Date(1970,0,2).getTime(),'local midnight normalization');
print('DATE-NUMERIC checks='+checks+' failures=0');
