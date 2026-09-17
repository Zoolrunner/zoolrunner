/* A diagnostic for a temporary stack value must not swallow its TypeError. */
var checks = 0;
function check(ok, message) {
    ++checks;
    if (!ok) throw Error('FAIL destructuring errors: ' + message);
}
function compileIn(edition, source) {
    var saved = version();
    try {
        version(edition);
        return evaluate(source, 'destructuring-errors');
    } finally {
        version(saved);
    }
}
var bodies = [
    'var x; [[x]]=[null];',
    'var x; [[x]]=[undefined];',
    'var x; [[x]]=[,];',
    'var x; [[[x]]]=[[null]];',
    'var x; [[{x:x}]]=[[null]];',
    'var x; [{x:{y:x}}]=[{x:null}];',
    'var x,y; [x,[y]]=[1,null];',
    'var x; [[x]]=[null]; return 1;'
];
var editions = [0, 170, 2015];
for (var e=0; e<editions.length; ++e) {
    for (var i=0; i<bodies.length; ++i) {
        compileIn(editions[e], 'function destructuringFailure(){'+bodies[i]+'}');
        var caught = false, finalized = false;
        try { destructuringFailure(); }
        catch(error) { caught = error instanceof TypeError; }
        finally { finalized = true; }
        check(caught && finalized, 'catch and finally '+editions[e]+'/'+i);
        var text = destructuringFailure.toString();
        var copy = compileIn(editions[e], '('+text+')');
        caught = false;
        try { copy(); } catch(error) { caught = error instanceof TypeError; }
        check(caught, 'decompilation round trip '+editions[e]+'/'+i);
    }
    check(compileIn(editions[e],
        'var order=""; function diagnosticGetter(){var x;' +
        '[[x]]=[{get 0(){order+="g";throw new RangeError("original");}}];}' +
        'var caught=false;try{diagnosticGetter();}catch(e){caught=e instanceof RangeError && e.message==="original";}' +
        'caught && order==="g"'), 'preserve getter exception '+editions[e]);
    check(compileIn(editions[e],
        'function ordinaryGroup(){var x,y;[x,y]=[4,5];return x+y;}' +
        'ordinaryGroup()===9 && eval("("+ordinaryGroup.toString()+")")()===9'),
        'successful group assignment '+editions[e]);
}
print('DESTRUCTURING-ERRORS checks='+checks+' failures=0');
