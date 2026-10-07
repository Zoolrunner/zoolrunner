/* Run in default ES5 and with -E -v 2015. */
var checks = 0;
function same(actual, expected, label) {
    ++checks;
    if (actual !== expected)
        throw Error(label + ': expected ' + expected + ', got ' + actual);
}
var names = ['hasOwnProperty', 'propertyIsEnumerable'];
for (var n = 0; n < names.length; ++n) {
    var query = Object.prototype[names[n]];
    var o = Object.create({inherited: 1});
    o.loaded = 1;
    o['0'] = 2;
    o['-0'] = 3;
    o.undefined = 4;
    same(query.call(o, ['loaded']), true, names[n] + ' array key');
    same(query.call(o, ['inherited']), false, 'inherited key');
    same(query.call(o, ['missing']), false, 'absent key');
    same(query.call(o, -0), true, 'numeric zero');
    delete o['0'];
    same(query.call(o, -0), false, 'numeric zero differs from string -0');
    same(query.call(o, new String('-0')), true, 'boxed string');
    same(query.call(o), true, 'omitted key');
    var calls = '';
    var key = {
        toString: function () { calls += 's'; return {}; },
        valueOf: function () { calls += 'v'; return 'loaded'; }
    };
    same(query.call(o, key), true, 'fallback conversion');
    same(calls, 'sv', 'string hint and one conversion');
    key.toString = function () {
        delete o.loaded;
        gc();
        return 'loaded';
    };
    same(query.call(o, key), false, 'lookup follows conversion and collection');
    key.toString = function () {
        o.created = 1;
        gc();
        return ['cre', 'ated'].join('');
    };
    same(query.call(o, key), true, 'new key survives collection');
    var sentinel = {};
    key.toString = function () { throw sentinel; };
    var caught;
    try { query.call(o, key); } catch (e) { caught = e; }
    same(caught, sentinel, 'conversion exception');
    key.toString = function () { return {}; };
    key.valueOf = function () { return {}; };
    caught = null;
    try { query.call(o, key); } catch (e) { caught = e; }
    same(caught instanceof TypeError, true, 'nonprimitive conversion rejected');
    Object.defineProperty(o, 'hidden', {value: 1});
    same(query.call(o, ['hidden']), n === 0, 'nonenumerable own key');
    var reads = 0;
    Object.defineProperty(o, 'accessor', {
        enumerable: true, get: function () { ++reads; return 1; }
    });
    same(query.call(o, ['accessor']), true, 'accessor ownership');
    same(reads, 0, 'query does not invoke getter');
}
print('PASS own-query-keys ' + checks);
