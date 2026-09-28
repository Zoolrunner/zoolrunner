/* ES2015 module host regression. MPL 1.1/GPL 2.0/LGPL 2.1. */
var checks=0,failures=0;
function check(name,f){checks++;try{if(f()!==true)throw Error('wrong result')}catch(e){failures++;print('FAIL '+name+': '+e)}}
check('anonymous default names',function(){var a=compileModule('export default function(){}'),b=compileModule('export default class {}'),c=compileModule('export default ()=>1');evaluateModule(a);evaluateModule(b);evaluateModule(c);return namespaceModule(a).default.name==='default'&&namespaceModule(b).default.name==='default'&&namespaceModule(c).default.name==='default'});
check('named default binding mutable',function(){var a=compileModule('export default function f(){return f}; export function change(){f=3}');evaluateModule(a);var n=namespaceModule(a),f=n.default;n.change();return n.default===3&&f()===3});
check('namespace alone keeps graph alive',function(){var n=(function(){var a=compileModule('export let x=1;export function inc(){gc();x++}'),b=compileModule('export * from "a"');linkModule(b,'a',a);evaluateModule(b);return namespaceModule(b)})();gc();n.inc();gc();return n.x===2});
check('function alone keeps graph alive',function(){var f=(function(){var a=compileModule('export let x=1;export function inc(){gc();return ++x}');evaluateModule(a);return namespaceModule(a).inc})();gc();return f()===2&&f()===3});
check('retained module exception',function(){var error={sentinel:3};this.moduleError=error;var m=compileModule('throw moduleError'),a,b;try{evaluateModule(m)}catch(e){a=e}gc();try{evaluateModule(m)}catch(e){b=e}delete this.moduleError;return a===error&&b===error});
check('Unicode bindings and module specifiers',function(){var a=compileModule('export let \u03b1=7'),b=compileModule('import {\u03b1 as \u03b2} from "\u03b3";export {\u03b2}');linkModule(b,'\u03b3',a);evaluateModule(b);return namespaceModule(b)['\u03b2']===7});
check('module code remains strict',function(){var a=compileModule('export function f(){return this}');evaluateModule(a);return namespaceModule(a).f.call(undefined)===undefined});
print('MODULE-EXTRA checks='+checks+' failures='+failures);if(failures)throw Error('module extra failures');
