/* Unicode identifier tokens and source reconstruction. MPL 1.1/GPL 2.0/LGPL 2.1. */
(function () {
    var checks=0, failures=0, slash=String.fromCharCode(92);
    function check(label, value) {checks++;if(!value){failures++;print('FAIL '+label);}}
    function rejects(source) {try{Function(source);return false;}catch(e){return e instanceof SyntaxError;}}
    function escaped(hex) {return slash+'u{'+hex+'}';}
    function binding(name) {return Function('var '+name+'=7;return '+name)()===7;}
    var names=['61','24','5f','e9','3b1','10400','20000','2118'];
    names.forEach(function(hex){check('escaped '+hex,binding(escaped(hex)));});
    check('part digit',binding('x'+escaped('30')));
    check('joiner',binding('x'+escaped('200d')));
    check('nonjoiner',binding('x'+escaped('200c')));
    check('combining mark',binding('x'+escaped('301')));
    check('leading zeroes',binding(escaped('000000000061')));
    var astral=String.fromCharCode(0xd801,0xdc00);
    check('raw supplementary',binding(astral));
    check('mixed raw and escaped',Function('var '+astral+'=9;return '+escaped('10400'))()===9);
    ['', '110000','ffffffffffffffff','d800','dc00','1f600','30','301','200c','200d','20','a','0'].forEach(function(hex){check('invalid start '+hex,rejects('var '+escaped(hex)+'=1'));});
    ['g','+61','61 ','61\n','61_'].forEach(function(hex){check('invalid syntax '+hex,rejects('var '+escaped(hex)+'=1'));});
    check('unterminated',rejects('var '+slash+'u{61'));
    check('keyword',rejects('var '+escaped('69')+'f=1'));
    check('contextual separator',rejects('for(var x '+escaped('6f')+'f []){}'));
    check('lone surrogate',rejects('var '+String.fromCharCode(0xd801)+'=1'));
    check('two surrogate escapes',rejects('var '+slash+'ud801'+slash+'udc00=1'));
    names.forEach(function(hex){var n=escaped(hex),f=Function('return function '+n+'('+n+'x){var '+n+'y='+n+'x+1;return '+n+'y}')();check('decompile '+hex,eval('('+f.toString()+')')(6)===7);});
    check('property name',Function('var o={'+escaped('10400')+':8};return o.'+astral)()===8);
    check('string surrogate escapes unchanged',Function('return "'+slash+'ud801'+slash+'udc00"')()===astral);
    check('long zero escape',binding(escaped(Array(600).join('0')+'61')));
    check('large invalid escape',rejects('var '+escaped(Array(600).join('f'))+'=1'));
    check('raw pair across line buffer',Function(Array(256).join(' ')+'var '+astral+'=9;return '+astral)()===9);
    check('escape across line buffer',Function(Array(256).join(' ')+'var '+escaped('10400')+'=9;return '+astral)()===9);
    var old=version();
    try {
        version(170);
        check('legacy BMP',evaluate('var '+slash+'u00e9=3;'+slash+'u00e9')===3);
        try {evaluate('var '+escaped('61')+'=3');check('legacy brace rejected',false);}
        catch(e){check('legacy brace rejected',e instanceof SyntaxError);}
        check('legacy XML',evaluate('var x=<r><a>1</a></r>;x.*.length()')===1);
    } finally {version(old);}
    if(failures)throw Error('identifier codepoints failures='+failures);
    print('ES6-IDENTIFIER-CODEPOINTS checks='+checks+' failures=0');
})();
