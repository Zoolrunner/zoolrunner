/* Strict declaration positions and selected legacy grammar.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
(function () {
    var checks = 0;
    function check(ok, label) {
        ++checks;
        if (!ok) throw Error('FAIL function statement grammar: ' + label);
    }
    var bodies = [
        'if (false) function f() {}',
        'if (true) {} else function f() {}',
        'while (false) function f() {}',
        'do function f() {} while (false);',
        'for (;false;) function f() {}',
        'for (var k in {}) function f() {}',
        'label: function f() {}'
    ];
    for (var i = 0; i < bodies.length; ++i) {
        var threw = false;
        try { Function('"use strict"; ' + bodies[i]); }
        catch (e) { threw = e instanceof SyntaxError; }
        check(threw, 'strict Function ' + i);
        threw = false;
        try { eval('"use strict"; ' + bodies[i]); }
        catch (e) { threw = e instanceof SyntaxError; }
        check(threw, 'strict eval ' + i);
        threw = false;
        try { Function('"use strict"; return function () {' + bodies[i] + '};'); }
        catch (e) { threw = e instanceof SyntaxError; }
        check(threw, 'inherited strict ' + i);
        check(typeof Function(bodies[i]) === 'function', 'sloppy extension ' + i);
    }
    check(Function('"use strict"; function f(){return 7;} return f();')() === 7,
          'function body declaration');
    check(Function('"use strict"; if (true) { function f(){return 8;} return f(); }')() === 8,
          'block declaration');
    check(Function('"use strict"; switch (1) {case 1: function f(){return 9;} return f();}')() === 9,
          'switch declaration');
    var saved = version();
    try {
        version(170);
        check(evaluate('if(true) function historical(){return 10;} historical()',
                       'legacy-function-statement') === 10, 'legacy statement');
    } finally { version(saved); }
    print('ES6-FUNCTION-STATEMENT-GRAMMAR checks=' + checks + ' failures=0');
})();
