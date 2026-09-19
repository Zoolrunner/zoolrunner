var checks=0;
function check(value,label){checks++;if(!value)throw Error(label);}
function throws(kind,fn,label){var seen=false;try{fn();}catch(e){seen=e instanceof kind;}check(seen,label);}
check(!Object.prototype.hasOwnProperty.call(this,'lexicalA'),'absent before');
throws(ReferenceError,function(){return lexicalA},'read before declaration');
throws(ReferenceError,function(){return typeof lexicalA},'typeof before declaration');
throws(ReferenceError,function(){lexicalA=4},'write before declaration');
let lexicalA=3;
const lexicalB=4;
check(lexicalA===3&&lexicalB===4,'initialized');
check(!Object.prototype.hasOwnProperty.call(this,'lexicalA'),'no global property');
lexicalA++;
check(lexicalA===4,'mutable');
throws(TypeError,function(){lexicalB=5},'immutable');
let lexicalEmpty;
check(lexicalEmpty===undefined,'empty initialized');
let [lexicalC,lexicalD=9]=[5];
const {x:lexicalE}= {x:6};
check(lexicalC===5&&lexicalD===9&&lexicalE===6,'patterns');
throws(TypeError,function(){lexicalE=7},'pattern immutable');
check(eval('let lexicalA=22; lexicalA')===22 && lexicalA===4,'eval shadow');
check(eval('let undefined=8;undefined')===8,'eval restricted shadow');
check(eval('let evalOnly=8;evalOnly')===8 && typeof evalOnly==='undefined','eval local');
var conflict=false;try{eval('var lexicalA')}catch(e){conflict=e instanceof SyntaxError}check(conflict,'eval var conflict');
check(eval('const localCapture=31;(function(){return localCapture})')()===31,'eval capture');
check((0,eval)('let indirectOnly=19;indirectOnly')===19&&typeof indirectOnly==='undefined','indirect local');
print('GLOBAL-LEXICAL-COMPILER PASS checks='+checks);
