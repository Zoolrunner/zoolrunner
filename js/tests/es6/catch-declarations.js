/* ES2015 catch variable instantiation and block lexical early errors. */
(function(){var checks=0;
function check(v,label){++checks;if(!v)throw Error('CATCH-DECLARATIONS FAIL '+label)}
function syntax(source){var caught=false;try{Function(source)}catch(e){caught=e instanceof SyntaxError}check(caught,source)}
for(var strict=0;strict<2;strict++){
 var prefix=strict?'"use strict";':'';
 check(Function(prefix+'x=1;try{throw 2}catch(x){var x=3}return x')()===1,'function catch hoist '+strict);
 check(Function(prefix+'function f(){return x}x=1;try{throw 2}catch(x){var x=3}return f()')()===1,'captured hoist '+strict);
 check(eval(prefix+'var result;try{throw 2}catch(x){var x=3;result=x}result===3&&x===undefined'),'eval declaration '+strict);
 check(Function(prefix+'x=1;try{throw 2}catch(x){for(var x in {a:1}){}}return x')()===1,'for-in '+strict);
 check(Function(prefix+'x=1;try{throw 2}catch(x){for(var x=3;x<4;x++){}}return x')()===1,'for statement '+strict);
 syntax(prefix+'{function f(){}function f(){}}');
 check(Function(prefix+'{function f(){}}{function f(){}}return 7')()===7,'separate blocks '+strict);
 check(Function(prefix+'function f(){return 1}function f(){return 2}return f()')()===2,'body declarations '+strict);
}
var savedVersion=version();
try {
 version(170);
 check(evaluate('{function historicalDuplicate(){return 1} function historicalDuplicate(){return 2}} historicalDuplicate()', 'legacy-block-functions')===2,'explicit legacy duplicates');
 version(0);
 check(evaluate('{function defaultDuplicate(){return 1} function defaultDuplicate(){return 2}} defaultDuplicate()', 'default-block-functions')===2,'default edition duplicates');
} finally {version(savedVersion)}
print('CATCH-DECLARATIONS PASS checks='+checks);
})();
