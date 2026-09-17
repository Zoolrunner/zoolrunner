/* Immutable writes must throw after their operands run, only in modern mode. */
var checks = 0;
function check(value, message) {
    checks++;
    if (!value) throw Error('FAIL const writes: ' + message);
}
function inEdition(edition, source) {
    var saved = version();
    try { version(edition); return evaluate(source, 'const-writes'); }
    finally { version(saved); }
}
var operations = ['fixed = (++called, 2)', 'fixed += (++called, 2)',
    'fixed -= (++called, 2)', 'fixed *= (++called, 2)', 'fixed /= (++called, 2)',
    'fixed %= (++called, 2)', 'fixed <<= (++called, 2)', 'fixed >>= (++called, 2)',
    'fixed >>>= (++called, 2)', 'fixed |= (++called, 2)', 'fixed &= (++called, 2)',
    'fixed ^= (++called, 2)', '++fixed', '--fixed', 'fixed++', 'fixed--',
    '[fixed] = [(++called, 2)]', '({x:fixed} = {x:(++called,2)})',
    '[[fixed]] = [[(++called,2)]]'];
for (var i = 0; i < operations.length; i++) {
    var expected = i >= 12 && i < 16 ? 0 : 1;
    for (var strict = 0; strict < 2; strict++) {
        var source = '(function constWrite(){' + (strict ? '"use strict";' : '') +
            'const fixed=1;var called=0;try{' + operations[i] + ';}' +
            'catch(e){return e instanceof TypeError && fixed===1 && called===' + expected + ';}' +
            'return false;})';
        var fn = inEdition(2015, source);
        check(fn(), 'write ' + i + '/' + strict);
        check(inEdition(2015, '(' + fn.toString() + ')')(),
              'decompiled write ' + i + '/' + strict);
    }
}
var cases = [
    'const fixed=1;try{for(fixed in {x:1}){}}catch(e){return e instanceof TypeError;}return false;',
    'const fixed=1;for(fixed in {}){}return fixed===1;',
    'const fixed=1;try{for(const i=0;i<1;i++){}}catch(e){return e instanceof TypeError;}return false;',
    'const fixed=1;try{eval("fixed=2");}catch(e){return e instanceof TypeError && fixed===1;}return false;',
    'const fixed=1;return function(){try{fixed=2;}catch(e){return e instanceof TypeError;}return false;};',
    'var seen=0;const fixed={valueOf:function(){seen++;gc();return 1;}};try{fixed++;}catch(e){return e instanceof TypeError && seen===1;}return false;',
    'var sentinel={};const fixed={valueOf:function(){throw sentinel;}};try{++fixed;}catch(e){return e===sentinel;}return false;',
    'var sentinel={};const fixed=1;try{fixed=(function(){throw sentinel;})();}catch(e){return e===sentinel;}return false;',
    'var finallyRan=false;const fixed=1;try{try{fixed=2;}finally{finallyRan=true;}}catch(e){return e instanceof TypeError && finallyRan;}return false;',
    'var obj={};Object.defineProperty(obj,"x",{value:1});obj.x=2;return obj.x===1;',
    'return (function named(){named=1;return typeof named==="function";})();',
    'const [fixed]=[1];try{fixed=2;}catch(e){return e instanceof TypeError;}return false;'
];
for (i = 0; i < cases.length; i++) {
    var result = inEdition(2015, '(function(){' + cases[i] + '})()');
    if (typeof result === 'function') { gc(); result = result(); }
    check(result, 'evaluation/closure case ' + i);
}
for (i = 0; i < 2; i++) {
    var globalName = 'globalConstWrite' + i;
    check(inEdition(2015, (i ? '"use strict";' : '') + 'const ' + globalName + '=1;' +
        'var globalCaught=false;try{' + globalName + '=2;}catch(e){globalCaught=e instanceof TypeError;}' +
        'globalCaught && ' + globalName + '===1'), 'global write ' + i);
}
var invalid = ['const missing;', 'const a=1,b;', 'const a,b=1;',
    'if(true) const x=1;', 'if(false){}else const x=1;',
    'while(false) const x=1;', 'do const x=1;while(false);',
    'for(;;) const x=1;', 'label: const x=1;', 'switch(1){case 1:const x;}'];
for (i = 0; i < invalid.length; i++) {
    var rejected = false;
    try { inEdition(2015, '(function(){' + invalid[i] + '})'); }
    catch (error) { rejected = error instanceof SyntaxError; }
    check(rejected, 'declaration syntax ' + i);
}
var legacyEditions = [0, 170];
for (i = 0; i < legacyEditions.length; i++) {
    check(inEdition(legacyEditions[i], '(function(){const fixed=1;fixed=2;fixed++;return fixed===1;})()'),
          'legacy immutable write policy ' + legacyEditions[i]);
    check(inEdition(legacyEditions[i], '(function(){const missing;return missing===undefined;})()'),
          'legacy initializer omission ' + legacyEditions[i]);
}
print('ES6-CONST-WRITES checks=' + checks + ' failures=0');
