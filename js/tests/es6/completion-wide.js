var checks=0;
function check(v){checks++;if(!v)throw Error('wide completion '+checks)}
var repeated=[];
for(var i=0;i<16000;i++)repeated.push('x++;');
var normal=Function('var x=0;try{return 7;'+repeated.join('')+'}finally{x++}');
check(normal()===7);
var copy=eval('('+normal.toString()+')');check(copy()===7);
var abrupt=Function('var x=0;try{return 7;'+repeated.join('')+'}finally{if(true)return 9}');
check(abrupt()===9);
copy=eval('('+abrupt.toString()+')');check(copy()===9);
print('COMPLETION-WIDE PASS checks='+checks);
