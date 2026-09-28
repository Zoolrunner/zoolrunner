var checks=0;function check(value){checks++;if(!value)throw Error('realm host check '+checks)}
var realm=createTest262Realm(),other=realm.createRealm();
check(realm.global!==this&&other.global!==realm.global);
check(realm.evalScript('this')===realm.global);
check(realm.evalScript('Array')!==Array&&realm.evalScript('Array')!==other.evalScript('Array'));
realm.evalScript('var foo=7;let bar=8');check(realm.global.foo===7&&realm.evalScript('bar')===8&&typeof foo==='undefined');
check(realm.evalScript('Object.getOwnPropertyDescriptor(Number,"length").configurable'));
var detached=realm.evalScript;check(detached.call(other,'this')===realm.global);
check(realm.evalScript('$262.global==this && $262.evalScript("this")==this'));
realm.evalScript('Object.preventExtensions(this)');check(realm.evalScript('foo+bar')===15);
gc();check(other.evalScript('Array.prototype.realmMarker=3;[].realmMarker')===3&&realm.evalScript('[].realmMarker')===undefined);
var m=other.compileModule('export let x=3');other.evaluateModule(m);check(namespaceModule(m).x===3);
var compiled=other.compileScript('var deferred=19; deferred');
check(other.evalScript('typeof deferred')==='undefined');
gc();check(other.executeScript(compiled)===19&&other.global.deferred===19);
var syntaxError;try{other.compileScript('var =')}catch(e){syntaxError=e}
check(syntaxError instanceof other.global.SyntaxError);
var runtimeThrow=other.compileScript('throw new SyntaxError("runtime")'),runtimeError;
try{other.executeScript(runtimeThrow)}catch(e){runtimeError=e}
check(runtimeError instanceof other.global.SyntaxError);
var wrongRealm=false;try{realm.executeScript(compiled)}catch(e){wrongRealm=true}check(wrongRealm);
other.evalScript('var outerValue="outside"');
check(other.evalScript('(function(){var eval=$262.createRealm().global.eval;eval("var outerValue=\\"inside\\"");return outerValue;})()')==='outside');
print('TEST262-REALM checks='+checks+' failures=0');
