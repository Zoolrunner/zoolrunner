/* ES2015 update-expression early ReferenceErrors. MPL 1.1/GPL 2.0/LGPL 2.1. */
(function () {
    var checks = 0, sideEffect = 0;
    function check(ok, label) { ++checks; if (!ok) throw new Error(label); }
    var invalid = ['1++','1--','++1','--1','++ ++x','-- --x','++x++','--x--',
                   '++(x+1)','(x+1)--','f()++','--f()', '++ -x', '-- !x'];
    invalid.forEach(function (expression) {
        var caught = false;
        try { eval('sideEffect=1;' + expression); }
        catch (error) { caught = error instanceof ReferenceError; }
        check(caught && sideEffect === 0, 'early invalid target ' + expression);
    });
    var caught = false;
    try { eval('var x=0,y=0; var z=x\n++\n++\ny'); }
    catch (error) { caught = error instanceof ReferenceError; }
    check(caught, 'ASI unary expression target');
    var x = 1;
    check(++x === 2 && x++ === 2 && --x === 2 && x-- === 2 && x === 1, 'valid name updates');
    var reads = 0, writes = 0, backing = 3, o = {};
    Object.defineProperty(o, 'x', {get:function(){++reads;return backing;},set:function(v){++writes;backing=v;}});
    check(++o.x === 4 && o.x-- === 4 && reads === 2 && writes === 2 && backing === 3, 'property updates');
    var bases=0, keys=0;
    function base(){++bases;return o}
    function key(){++keys;return 'x'}
    check(++base()[key()] === 4 && bases === 1 && keys === 1, 'computed receiver once');
    function roundtrip(o){return ++o.x + o.x--}
    var copy=eval('('+roundtrip.toString()+')');
    check(copy({x:1})===roundtrip({x:1}), 'update source roundtrip');
    print('ES6-UPDATE-TARGETS PASS checks=' + checks);
}());
