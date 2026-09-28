/* ES2015 pattern regression. MPL 1.1/GPL 2.0/LGPL 2.1. */
var entries=[],global=Function('return this')();
for(var i=0;i<66000;++i)entries.push('atom="pattern'+i+'"');
var filler='var atom;'+entries.join(';')+';';
var source='(function(){"use strict";'+filler+'var global=Function("return this")();try{({x:widePatternUnresolved}={get x(){global.widePatternUnresolved=1;return 2}})}catch(e){return e instanceof ReferenceError&&global.widePatternUnresolved===1}return false})';
var f=eval(source),checks=0;
try{delete global.widePatternUnresolved;if(!f())throw Error('wide binding');++checks;
var restored=eval('('+f.toString()+')');delete global.widePatternUnresolved;if(!restored())throw Error('wide decompile');++checks}
finally{delete global.widePatternUnresolved}
print('ES6-PATTERN-REFERENCES-WIDE PASS checks='+checks);
