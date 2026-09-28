/* Contextual let in a single Statement uses the expression lookahead rule. */
var checks=0;
function check(ok,label){++checks;if(!ok)throw Error(label);}
function inEdition(ed,source){var old=version();try{version(ed);return evaluate(source,'let-statement-newline');}finally{version(old);}}
var bodies=[
 'for(;false;)let\nx=1;', 'for(var k in {})let\nx=1;',
 'for(var k of [])let\nx=1;', 'while(false)let\nx=1;',
 'if(false)let\nx=1;', 'with({})let\nx=1;',
 'label:let\nx=1;', 'for(;false;)let\n{}x=1;',
 'for(var k of [])let\n{}x=1;', 'if(false)let\n{}x=1;'
];
for(var i=0;i<bodies.length;++i){
 for(var j=0;j<3;++j){
  var body=bodies[i].replace(/\n/g,['\n','/*\n*/','\u2028'][j]);
  var fn=inEdition(2015,'(function(){var let=7,x=0;'+body+'return x===1;})');
  check(fn(),'ASI '+i+'/'+j);
  check(inEdition(2015,'('+fn.toString()+')')(),'source ASI '+i+'/'+j);
 }
}
var invalid=[
 'for(;false;)let x=1;', 'for(;false;)let{}={};',
 'for(var k of [])let x=1;', 'if(false)let[x]=[];',
 'if(false)let\n[x]=[];', 'while(false)let\n[0];',
 '"use strict";for(;false;)let\nx=1;',
 'function* g(){let\nyield 0;}', 'function* g(){for(let\nyield of []);}'
];
for(i=0;i<invalid.length;++i){var caught=false;try{inEdition(2015,'(function(){'+invalid[i]+'})');}catch(e){caught=e instanceof SyntaxError;}check(caught,'invalid '+i);}
check(inEdition(2015,'(function(){let\nx=3;return x===3;})()'),'statement-list declaration');
check(inEdition(2015,'(function(){let\n{x}={x:4};return x===4;})()'),'statement-list pattern declaration');
check(inEdition(170,'(function(){let(x=5){return x===5;}})()'),'legacy let block');
gc();print('ES6-LET-STATEMENT-NEWLINE checks='+checks+' failures=0');
