/* Function/catch parameters and body lexical declarations share early errors. */
var checks=0;
function check(ok,message) {
    ++checks;
    if(!ok) throw Error('FAIL lexical parameters: '+message);
}
function inEdition(edition,source) {
    var saved=version();
    try {version(edition);return evaluate(source,'lexical-parameters');}
    finally {version(saved);}
}
var invalid=[
    'function f(x){let x;}',
    'function f(x){let x=1;}',
    'function f(x,x){let x;}',
    'function f([x]){let x;}',
    'function f({x:x}){let x;}',
    'function f({x:{y:y}}){let y;}',
    'try{}catch(x){let x;}',
    'try{}catch(x){let x=1;}',
    'try{}catch([x]){let x;}',
    'try{}catch({x:x}){let x;}'
];
for(var i=0;i<invalid.length;i++) {
    for(var strict=0;strict<2;strict++) {
        var rejected=false;
        try {inEdition(2015,(strict?'"use strict"; ':'')+invalid[i]);}
        catch(error) {rejected=error instanceof SyntaxError;}
        check(rejected,'conflict '+i+'/'+strict);
    }
}
var valid=[
    'function f(x){{let x=2;return x;}} f(1)===2',
    'function f([x]){{let x=2;return x;}} f([1])===2',
    'function f({x:x}){{let x=2;return x;}} f({x:1})===2',
    'function f(x){var x;return x;} f(1)===1',
    'function f(x){let y=2;return x+y;} f(1)===3',
    'var value=0;try{throw 1;}catch(x){{let x=2;value=x;}}value===2',
    'var value=0;try{throw 1;}catch(x){var x;value=x;}value===1'
];
for(i=0;i<valid.length;i++) {
    check(inEdition(2015,valid[i]),'valid scope '+i);
    check(inEdition(2015,'"use strict"; '+valid[i]),'valid strict scope '+i);
}
check(inEdition(2015,'var rejected=false;try{Function("x","let x;");}catch(e){rejected=e instanceof SyntaxError;}rejected'), 'Function constructor parameters');
check(inEdition(170,'function f(x){let x=2;return x;}f(1)===2'), 'legacy parameter shadowing retained');
print('ES6-LEXICAL-PARAMETERS checks='+checks+' failures=0');
