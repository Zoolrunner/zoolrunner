/* ES2015 context regression. MPL 1.1/GPL 2.0/LGPL 2.1. */
var checks=0,failures=0;function invalid(s){checks++;try{compileModule(s);failures++;print('FAIL accepted: '+s)}catch(e){if(!(e instanceof SyntaxError)){failures++;print('FAIL exception: '+e)}}}
invalid('import x fr\\u006fm "dep"');invalid('import * a\\u0073 x from "dep"');invalid('import {x a\\u0073 y} from "dep"');invalid('export {x a\\u0073 y} from "dep"');
invalid('export let f=()=>new.target');invalid('export let f=()=>super.x');invalid('export const {await:x}= {await:1};await');
checks++;compileModule('let x=1;export {x}\nfoo()');checks++;compileModule('let x=1;export {x}\nfr\\u006fm()');
print('MODULE-CONTEXTUAL checks='+checks+' failures='+failures);if(failures)throw Error('module contextual words');
