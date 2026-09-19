/* Wide module prolog operands. MPL 1.1/GPL 2.0/LGPL 2.1. */
var words=[];for(var i=0;i<66000;i++)words.push("'moduleAtom"+i+"'");
var m=compileModule('['+words.join(',')+'];export var late=7;export function read(){return late}');
var ns=namespaceModule(m);gc();evaluateModule(m);if(ns.late!==7||ns.read()!==7)throw Error('wide module declarations');gc();print('MODULE-WIDE PASS');
