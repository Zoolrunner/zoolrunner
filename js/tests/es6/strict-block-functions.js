'use strict';
var checks=0;function check(value,label){checks++;if(!value)throw Error(label)}
check(typeof scoped==='undefined','before block');
{check(scoped()===7,'hoisted call');function scoped(){return 7;}check(scoped()===7,'after declaration')}
check(typeof scoped==='undefined','after block');
var saved;
{let x=8; saved=f; function f(){return x;} }
gc();check(saved()===8,'captured block');
function local(){'use strict';var results=[];for(var i=0;i<2;i++){let x=i;results.push(f);function f(){return x;}}return results;}
var functions=local();check(functions[0]()===0&&functions[1]()===1,'fresh functions');
var seen;
switch(1){case 1:seen=sw();break;case 2:function sw(){return 9}}
check(seen===9&&typeof sw==='undefined','switch hoist and scope');
check(typeof scoped==='undefined'&&!Object.prototype.hasOwnProperty.call(this,'scoped'),'no global property');
var roundtrip=eval('('+local.toString()+')')();check(roundtrip[0]()===0&&roundtrip[1]()===1,'source roundtrip');
print('STRICT-BLOCK-FUNCTIONS checks='+checks+' failures=0');
