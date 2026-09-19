/* Basic ES2015 object patterns and source reconstruction. MPL 1.1/GPL 2.0/LGPL 2.1. */
(function () {
    var checks = 0;
    function check(value, label) { ++checks; if (!value) throw new Error(label); }
    function throws(kind, code, label) {
        var caught = false;
        try { eval(code); } catch (error) { caught = error instanceof kind; }
        check(caught, label);
    }
    var x, y, source = {x: 7, nested: {y: 8}};
    check(({x} = source) === source && x === 7, 'shorthand assignment');
    var {x: local} = source;
    check(local === 7, 'renamed binding');
    ({nested: {y}} = source);
    check(y === 8, 'nested shorthand');
    var {nested: {y: inner}} = source;
    check(inner === 8, 'nested binding');
    var value = {};
    check(({} = value) === value, 'empty pattern result identity');
    check(({} = 3) === 3 && ({} = false) === false, 'primitive object coercibility');
    var {length: length} = 'abc';
    check(length === 3, 'primitive property access');
    var events = [];
    var inherited = {get x() { events.push('x'); return 9; }};
    ({x} = Object.create(inherited));
    check(x === 9 && events.join() === 'x', 'inherited getter');
    ({missing: y} = source);
    check(y === undefined, 'missing property');
    throws(TypeError, '({}=null)', 'empty assignment null');
    throws(TypeError, '({}=undefined)', 'empty assignment undefined');
    throws(TypeError, 'var {}=null', 'empty binding null');
    throws(TypeError, '({x}=null)', 'nonempty assignment null');
    throws(TypeError, 'var {x}=undefined', 'nonempty binding undefined');
    throws(TypeError, '({nested:{}}={nested:null})', 'nested empty null');
    throws(SyntaxError, '({x: 0} = {})', 'literal assignment target');
    throws(SyntaxError, '({x: (function(){})()} = {})', 'call assignment target');
    throws(SyntaxError, '"use strict"; ({eval} = {})', 'strict eval shorthand');
    throws(SyntaxError, '"use strict"; ({x: arguments} = {})', 'strict arguments target');
    function empty(o) { ({} = o); return true; }
    function shorthand(o) { var {x} = o; return x; }
    function nested(o) { var {nested: {y}} = o; return y; }
    function emptyBinding(o) { var {} = o; return true; }
    [empty, shorthand, nested, emptyBinding].forEach(function (f) {
        var rebuilt = eval('(' + f.toString() + ')');
        check(rebuilt(source) === f(source), 'source roundtrip ' + f.name);
        var caught = false;
        try { rebuilt(null); } catch (error) { caught = error instanceof TypeError; }
        check(caught, 'source roundtrip coercibility ' + f.name);
    });
    print('ES6-OBJECT-PATTERNS PASS checks=' + checks);
}());
