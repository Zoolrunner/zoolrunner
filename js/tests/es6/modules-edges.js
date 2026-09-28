var checks=0,failures=0;function check(name,f){checks++;try{if(f()!==true)throw Error('wrong result')}catch(e){failures++;print('FAIL '+name+': '+e)}}
function invalid(s){check(s,function(){try{compileModule(s)}catch(e){return e instanceof SyntaxError}return false})}
invalid('var {await}={}');invalid('({await})');invalid('var {aw\\u0061it}={}');invalid('({aw\\u0061it})');
check('import call receiver',function(){var a=compileModule('export function f(){return this}'),b=compileModule('import {f} from "a";export let x=f()');linkModule(b,'a',a);evaluateModule(b);return namespaceModule(b).x===undefined});
check('eval local isolation',function(){var m=compileModule('var x=1;eval("var x=2");export {x}');evaluateModule(m);return namespaceModule(m).x===1});
check('namespace setPrototype ES2015',function(){var n=namespaceModule(compileModule(''));return !Reflect.setPrototypeOf(n,null)&&!Reflect.setPrototypeOf(n,{})});
print('MODULE-EDGES checks='+checks+' failures='+failures);if(failures)throw Error('module edge failures');
