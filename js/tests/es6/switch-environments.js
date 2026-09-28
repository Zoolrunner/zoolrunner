var checks=0;
function check(v,m){checks++;if(!v)throw Error(m);}
let outer='outer';
var discrim,selector,body;
switch ((discrim=()=>outer,gc(),3)) {
 case (selector=()=>outer,3):
  let outer='inner';body=()=>outer;break;
}
check(discrim()==='outer','discriminant closure');
check(selector()==='inner'&&body()==='inner','selector and body closure');
function table(v){let x=20;switch((gc(),v+x-20)){case 1:let x=40;return ()=>x;case 2:return ()=>x;default:return ()=>x;}}
check(table(1)()===40,'table switch');
var threw=false;try{table(2)();}catch(e){threw=e instanceof ReferenceError;}check(threw,'uninitialized case binding');
function lookup(v){let x=2;switch(v){case 'one':let x=4;return ()=>x;default:let y=6;return ()=>y;}}
check(lookup('one')()===4&&lookup('other')()===6,'lookup switch');
function roundtrip(){let x=3;var f;switch((f=()=>x,1)){case 1:let x=9;gc();return [f(),x];}}
check(roundtrip().join(',')==='3,9','function discriminant closure');
var restored=Function('return ('+String(roundtrip)+')')();
check(restored().join(',')==='3,9','decompilation roundtrip');
var sum=0;for(var i=0;i<3;i++){switch(i){case 0:let x=4;sum+=x;continue;case 1:break;default:sum+=2;}}
check(sum===6,'break and continue unwinding');
var marker={};threw=false;try{switch((()=>{throw marker;})()){case 0:let x;}}catch(e){threw=e===marker;}check(threw,'abrupt discriminant');
let visible=7;switch(eval('visible')){case 7:let visible=8;check(visible===8,'direct eval discriminant');break;default:check(false,'eval selected case');}
var saved=version();
try {
 version(170);
 var legacy=evaluate('(function(){var x="outside", f;switch((f=function(){return x;},1)){case 1:let x="inside";}return f();})','legacy-switch-scope');
 check(legacy()==='inside','explicit legacy switch scope preserved');
} finally { version(saved); }
function clonedOuter(){
 let getter;
 {let x='enclosing';var keep=()=>x;keep();
  switch((getter=()=>x,gc(),{})){
   default:let x='body';gc();check(getter()==='enclosing','cloned enclosing block');check(x==='body','cloned body');
  }
 }
 gc();return getter;
}
check(clonedOuter()()==='enclosing','retired enclosing environment');
function multi(){let x=2;switch(x){case 2:let x=3,y=4,z=5;return [x,y,z];}}
check(Function('return ('+String(multi)+')')()().join(',')==='3,4,5','multiple slots and source roundtrip');
function onlyDefault(v){switch(v){default:let x=6,y=7;return x+y;}}
check(Function('return ('+String(onlyDefault)+')')()(0)===13,'default-only body source roundtrip');
function emptySwitch(v){switch(v){}}
check(Function('return ('+String(emptySwitch)+')')()(0)===undefined,'empty switch source roundtrip');
try {version(170);var oldDefault=evaluate('(function(v){switch(v){default:return 19;}})','legacy-default-switch');var oldCopy=evaluate('('+String(oldDefault)+')','legacy-default-copy');check(oldCopy(0)===19,'legacy default-only body preserved');}finally{version(saved);}
print('SWITCH-SCOPE checks='+checks+' failures=0');
