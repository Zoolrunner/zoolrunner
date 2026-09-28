/* Exercise ES2015 contextual keywords alongside the legacy JS 1.7 grammar. */
var checks = 0;
function check(ok, message) {
    ++checks;
    if (!ok) throw Error('FAIL contextual keywords: ' + message);
}
function compileIn(edition, source) {
    var saved = version();
    try {
        version(edition);
        return evaluate(source, 'contextual-keywords');
    } finally {
        version(saved);
    }
}
function rejects(source) {
    try { compileIn(2015, source); }
    catch (error) { return error instanceof SyntaxError; }
    return false;
}
check(compileIn(2015, 'var let=7, yield=9; let+yield===16'), 'ordinary identifiers');
check(compileIn(2015, 'var let; for(let in {answer:1}){} let==="answer"'), 'for-in identifier');
check(compileIn(2015, 'var yield; ({x:yield}={x:12}); yield===12'), 'destructuring identifier');
check(compileIn(2015, 'yield: {break yield;} true'), 'yield label');
check(compileIn(2015, 'let: {break let;} true'), 'let label');
check(compileIn(2015, 'function let(x){return x+1;} let(4)===5'), 'let call');
check(compileIn(2015, 'var answer=0; {let scoped=7; answer=scoped;} answer===7'), 'block declaration');
check(compileIn(2015, 'var answer=""; for(let key in {a:1}){answer+=key;} answer==="a"'), 'for-in declaration');
check(compileIn(2015, 'var answer=0; for(let i=0;i<3;i++){answer+=i;} answer===3'), 'for declaration');
check(compileIn(2015, 'var l\\u0065t=4; l\\u0065t===4'), 'escaped identifier');
check(rejects('l\\u0065t x=1;'), 'escaped spelling cannot declare');
check(rejects('"use strict"; var let;'), 'strict let binding');
check(rejects('"use strict"; var yield;'), 'strict yield binding');
check(rejects('"use strict"; yield: {}'), 'strict yield label');
check(rejects('for(let key=1 in {}){}'), 'for-in initializer');
var bodies = ['if(false) BODY', 'if(false){}else BODY', 'while(false) BODY',
              'do BODY while(false);', 'for(;false;) BODY',
              'for(var key in {}) BODY', 'label: BODY', 'with({}) BODY'];
for(var i=0;i<bodies.length;i++) {
    check(rejects(bodies[i].replace('BODY','let x;')), 'bare lexical declaration '+i);
    check(rejects(bodies[i].replace('BODY','let x=1;')), 'bare initialized declaration '+i);
}
check(compileIn(170, 'let (legacy=4) legacy===4'), 'legacy let expression');
check(compileIn(170, 'function legacyGenerator(){yield 7;} legacyGenerator().next()===7'), 'legacy generator');
check(compileIn(170, 'var result=0; if(true) let legacy=3; true'), 'legacy bare let');
check(compileIn(0, 'var let=2,yield=3;let+yield===5'), 'ES5 identifiers');
check(rejects('let let;'), 'let cannot bind let');
check(rejects('let \nlet=1;'), 'newline cannot permit let binding');
check(rejects('const \nlet=1;'), 'const cannot bind let');
check(compileIn(2015, 'try{throw 7;}catch(let){if(let!==7)throw Error();}true'), 'catch may bind let');
check(compileIn(2015, 'try{throw 7;}catch(e){var e;}true'), 'simple catch var redeclaration');
var conflicts = [
    'let duplicate; let duplicate;',
    'let duplicate; var duplicate;',
    'var duplicate; let duplicate;',
    'let duplicate; function duplicate(){}',
    'function duplicate(){} let duplicate;',
    '{let duplicate; var duplicate;}',
    '{var duplicate; let duplicate;}',
    '{let duplicate; function duplicate(){}}',
    '{function duplicate(){} let duplicate;}',
    '{let duplicate; {var duplicate;}}',
    '{{var duplicate;} let duplicate;}',
    'function outer(){let duplicate;var duplicate;}',
    'function outer(){var duplicate;let duplicate;}'
];
for(i=0;i<conflicts.length;i++) {
    check(rejects(conflicts[i]), 'declaration conflict '+i);
    check(rejects('"use strict"; '+conflicts[i]), 'strict declaration conflict '+i);
}
var shadowing = [
    'var shadow; {let shadow;}',
    '{var shadow;} {let shadow;}',
    '{let shadow;} {var shadow;}',
    '{let shadow;} {let shadow;}',
    '{let shadow; {let shadow;}}',
    'let shadow; function nested(){var shadow;}',
    'function nested(shadow){{let shadow;}}',
    'function shadow(){} {let shadow;}',
    '{function shadow(){}} let shadow;'
];
for(i=0;i<shadowing.length;i++) {
    /* These are independent scripts. Persistent global lexical declarations
     * must not collide with a prior case's global var or lexical binding. */
    var ordinaryCase = shadowing[i].replace(/shadow/g, 'shadowCase' + i + 'Plain');
    var strictCase = shadowing[i].replace(/shadow/g, 'shadowCase' + i + 'Strict');
    check(compileIn(2015, ordinaryCase+' true'), 'independent scope '+i);
    check(compileIn(2015, '"use strict"; '+strictCase+' true'), 'strict independent scope '+i);
}
print('ES6-CONTEXTUAL-KEYWORDS checks='+checks+' failures=0');
