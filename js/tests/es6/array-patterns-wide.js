/* ES2015 pattern regression. MPL 1.1/GPL 2.0/LGPL 2.1. */
(function(){var checks=0,calls=0,parts=[],closed=0;function tick(){++calls;gc();return 0}
for(var i=0;i<6000;++i)parts.push('step()');parts.push('7');
var f=eval('(function(v,d,step){[d.x=('+parts.join(',')+')]=v;return d.x})');
function step(){++calls;return 0}
if(f([],{},step)!==7||calls!==6000)throw Error('wide default');++checks;
calls=0;if(f([4],{},step)!==4||calls!==0)throw Error('wide default skipped');++checks;
var restored=eval('('+f.toString()+')');if(restored([],{},step)!==7||calls!==6000)throw Error('wide source roundtrip');++checks;
var source={next:function(){return {done:false,value:undefined}},return:function(){++closed;gc();return {}}};source[Symbol.iterator]=function(){return this};
var marker={},caught;try{restored(source,{},function(){throw marker})}catch(e){caught=e}if(caught!==marker||closed!==1)throw Error('wide cleanup');++checks;
print('ES6-ARRAY-PATTERNS-WIDE PASS checks='+checks);
})();
