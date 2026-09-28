/* Escaping parameter maps and eval receiver bindings; MPL 1.1/GPL 2.0/LGPL 2.1. */
(function () {
    var checks = 0, failures = 0;
    function test(name, body) {
        ++checks;
        try { if (body()) return; } catch (e) { print('DETAIL ' + name + ': ' + e); }
        ++failures; print('FAIL arguments lifetime: ' + name);
    }
    test('read before parameter write', function () {
        return (function (a) { var args = arguments; args[0]; a = 9; return args; })(7)[0] === 9;
    });
    test('enumerate before parameter write', function () {
        return (function (a) { Object.keys(arguments); a = 9; return arguments; })(7)[0] === 9;
    });
    test('mapped descriptor survives return', function () {
        var args = (function (a) {
            Object.defineProperty(arguments, '0', {enumerable:false}); a = 9; return arguments;
        })(7);
        return args[0] === 9 && !Object.getOwnPropertyDescriptor(args, '0').enumerable;
    });
    test('deleted mapping stays deleted', function () {
        var args = (function (a) { arguments[0]; delete arguments[0]; a = 9; return arguments; })(7);
        return !args.hasOwnProperty('0');
    });
    test('recreated index stays detached', function () {
        return (function (a) { delete arguments[0]; arguments[0] = 4; a = 9; return arguments; })(7)[0] === 4;
    });
    test('read only index stays detached', function () {
        return (function (a) { Object.defineProperty(arguments, '0', {writable:false}); a = 9; return arguments; })(7)[0] === 7;
    });
    test('accessor is not called at return', function () {
        var calls = 0;
        var args = (function (a) {
            Object.defineProperty(arguments, '0', {get:function () { ++calls; return 4; }});
            a = 9; return arguments;
        })(7);
        return calls === 0 && args[0] === 4 && calls === 1;
    });
    test('strict snapshot', function () {
        return (function (a) { 'use strict'; arguments[0]; a = 9; return arguments; })(7)[0] === 7;
    });
    test('duplicate parameter mapping', function () {
        var args = (function (a, a) { arguments[0]; arguments[1]; a = 9; return arguments; })(3, 7);
        return args[0] === 3 && args[1] === 9;
    });
    test('strict eval retains outer boxed receiver', function () {
        return (function () { var receiver = this; return eval('"use strict"; this') === receiver; }).call(7);
    });
    test('strict eval retains outer global receiver', function () {
        return (function () { var receiver = this; return eval('"use strict"; this') === receiver; }).call(null);
    });
    test('strict function retains primitive receiver', function () {
        return (function () { 'use strict'; return eval('this') === 7; }).call(7);
    });
    print('ARGUMENTS-LIFETIME checks=' + checks + ' failures=' + failures);
    if (failures) throw Error('arguments lifetime regressions');
})();
