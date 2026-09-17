/* Shared property-tree duplicate flags must not contaminate later functions. */
var checks=0;
function check(ok,message) {
    ++checks;
    if(!ok) throw Error('FAIL strict parameter history: '+message);
}
function compileIn(edition,source) {
    var saved=version();
    try {version(edition);return evaluate(source,'strict-parameter-history');}
    finally {version(saved);}
}
var editions=[0,170,2015];
for(var i=0;i<editions.length;i++) {
    var edition=editions[i];
    check(compileIn(edition,'function duplicateHistory(x,x){return x;}duplicateHistory(1,2)===2'), 'legacy duplicate values '+edition);
    check(compileIn(edition,'"use strict"; function validHistory(x){return x;}validHistory(7)===7'), 'strict function after duplicate '+edition);
    check(compileIn(edition,'"use strict"; function twoHistory(x,y){return x+y;}twoHistory(4,5)===9'), 'strict distinct parameters '+edition);
    var rejected=false;
    try {compileIn(edition,'"use strict"; function invalidHistory(x,x){return x;}');}
    catch(error) {rejected=error instanceof SyntaxError;}
    check(rejected,'actual duplicate rejected '+edition);
    check(compileIn(edition,'"use strict"; function validHistory(x){return x;}validHistory(8)===8'), 'strict function after failed compilation '+edition);
    check(compileIn(edition,'function lateDirective(x){"use strict";return x;}lateDirective(9)===9'), 'body directive after duplicate '+edition);
    check(compileIn(edition,'Function("x", "\\\"use strict\\\"; return x;")(10)===10'), 'Function constructor after duplicate '+edition);
    rejected=false;
    try {compileIn(edition,'Function("x", "x", "\\\"use strict\\\"; return x;")');}
    catch(error) {rejected=error instanceof SyntaxError;}
    check(rejected,'Function constructor duplicate rejected '+edition);
    check(compileIn(edition,'eval("("+validHistory.toString()+")")(11)===11'), 'strict decompilation after duplicate '+edition);
    gc();
    check(compileIn(edition,'"use strict"; function afterCollection(x){return x;}afterCollection(12)===12'), 'strict compilation after GC '+edition);
}
print('STRICT-PARAMETER-HISTORY checks='+checks+' failures=0');
