/* ES2015 context regression. MPL 1.1/GPL 2.0/LGPL 2.1. */
var checks=0,failures=0;function check(n,f){checks++;try{if(f()!==true)throw Error('wrong result')}catch(e){failures++;print('FAIL '+n+': '+e)}}
function namespace(source){var m=compileModule(source);evaluateModule(m);return namespaceModule(m)}
check('named exported class',function(){var ns=namespace('export class C {m(){return 3}}');return new ns.C().m()===3});
check('exported generator',function(){return namespace('export function* g(){yield 3}').g().next().value===3});
check('default generator',function(){var f=namespace('export default function*(){yield 3}').default;return f.name==='default'&&f().next().value===3});
check('default named class live binding',function(){var ns=namespace('export default class C{};export function change(){C=3}');if(ns.default.name!=='C')return false;ns.change();return ns.default===3});
check('export binding patterns',function(){var ns=namespace('export const [a,b=3]=[1];export let {x:c}={x:4}');return ns.a===1&&ns.b===3&&ns.c===4});
check('star paths share origin',function(){var a=compileModule('export let x=3'),b=compileModule('export * from "a"'),c=compileModule('export * from "a";export * from "b"');linkModule(b,'a',a);linkModule(c,'a',a);linkModule(c,'b',b);evaluateModule(c);return namespaceModule(c).x===3});
check('local aliased default stays live',function(){var ns=namespace('let x=3;export {x as default};export function change(){x=4}');ns.change();return ns.default===4});
check('module arrow eval rejects new target',function(){var ns=namespace('export let f=()=>eval("new.target")');try{ns.f()}catch(e){return e instanceof SyntaxError}return false});
print('MODULE-PRODUCTIONS checks='+checks+' failures='+failures);if(failures)throw Error('module productions');
