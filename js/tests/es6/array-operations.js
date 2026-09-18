/* ES2015 array-like operations: sparse objects, callbacks and large indices. */
var checks = 0;
function check(ok, label) { ++checks; if (!ok) throw Error('FAIL array operations: ' + label); }
function throwsType(fn, label) {
    var caught = false;
    try { fn(); } catch (e) { caught = e instanceof TypeError; }
    check(caught, label);
}
var names = ['find', 'findIndex', 'fill', 'copyWithin'];
for (var n = 0; n < names.length; ++n) {
    (function(name) {
        var method = Array.prototype[name], d = Object.getOwnPropertyDescriptor(Array.prototype, name);
        check(typeof method === 'function' && method.length === (name === 'copyWithin' ? 2 : 1), name + ' arity');
        check(d.writable && d.configurable && !d.enumerable, name + ' descriptor');
        check(!method.hasOwnProperty('prototype'), name + ' no prototype');
        throwsType(function() { new method(); }, name + ' not constructor');
        throwsType(function() { method.call(null); }, name + ' null');
        throwsType(function() { method.call(undefined); }, name + ' undefined');
    })(names[n]);
}
var find = Array.prototype.find, findIndex = Array.prototype.findIndex;
var fill = Array.prototype.fill, copy = Array.prototype.copyWithin;
check([1, 2, 3].find(function(x) { return x > 1; }) === 2, 'find first');
check([1, 2, 3].findIndex(function(x) { return x > 1; }) === 1, 'findIndex first');
check([1].find(function() { return false; }) === undefined, 'find absent');
check([1].findIndex(function() { return false; }) === -1, 'findIndex absent');
throwsType(function() { [].find(); }, 'empty validates callback');
throwsType(function() { [].findIndex({}); }, 'empty validates index callback');
var order = '', sentinel = {}, caught = false;
try { find.call({get length() { order += 'l'; throw sentinel; }}, null); }
catch(e) { caught = e === sentinel; }
check(caught && order === 'l', 'length before callback validation');
var a = [, 2, , 4], seen = [];
a.find(function(value, index, object) { seen.push([value, index, object === a]); gc(); return false; });
check(seen.length === 4 && seen[0][0] === undefined && seen[2][0] === undefined && seen[3][1] === 3 && seen[3][2], 'find visits holes and supplies receiver');
a = [1, 2, 3]; seen = [];
a.find(function(v, i) { seen.push(v); if (i === 0) { delete a[1]; a.push(4); a[2] = 5; } return false; });
check(seen.length === 3 && seen[1] === undefined && seen[2] === 5, 'snapshot length live values');
var proto = {1: 'inherited'}, obj = Object.create(proto); obj.length = 3;
check(findIndex.call(obj, function(v) { return v === 'inherited'; }) === 1, 'inherited element');
var receiver = {};
check([7].find(function(v) { 'use strict'; check(this === receiver, 'explicit callback receiver'); return true; }, receiver) === 7, 'find receiver value');
[0].find(function() { 'use strict'; check(this === undefined, 'default strict receiver'); return true; });
[0].find(function() { 'use strict'; check(this === 3, 'primitive strict receiver'); return true; }, 3);
[0].find(function() { 'use strict'; check(this === null, 'null strict receiver'); return true; }, null);
check(find.call('abc', function(v, i, o) { check(Object.prototype.toString.call(o) === '[object String]', 'boxed callback string'); return i === 1; }) === 'b', 'generic string');
check(find.call({length: 4294967297, 0: 'wide'}, function(v) { return true; }) === 'wide', 'length is not uint32');
check(find.call({length: Infinity, 0: 'clamped'}, function(v) { return true; }) === 'clamped', 'infinite length clamped');
check(findIndex.call({length: -1}, function() { throw Error('must not call'); }) === -1, 'negative length zero');
check(findIndex.call({length: NaN}, function() { throw Error('must not call'); }) === -1, 'NaN length zero');
check(findIndex.call({length: 1.9, 1: 7}, function(v) { return v === 7; }) === -1, 'fractional length truncated');
caught = false; try { [1].find(function() { throw sentinel; }); } catch(e) { caught = e === sentinel; }
check(caught, 'callback exception');
obj = {length: 1, get 0() { gc(); return {value: 8}; }};
check(find.call(obj, function(v) { delete obj[0]; gc(); check([2].find(function() { return true; }) === 2, 'reentrant find'); return true; }).value === 8, 'found value rooted across callback');
a = [1, 2, 3, 4]; check(a.fill(9, 1, 3) === a && a.join() === '1,9,9,4', 'fill range identity');
a = [1, 2, 3, 4]; check(a.fill(8, -3, -1).join() === '1,8,8,4', 'fill negative range');
check([1, 2].fill(7, -Infinity, Infinity).join() === '7,7', 'fill infinity range');
check([1, 2].fill(7, 1.9, undefined).join() === '1,7', 'fill fractional undefined end');
check([1, 2].fill(7, 2, 0).join() === '1,2', 'fill reversed range');
a = new Array(2); a.fill(); check(a.hasOwnProperty(0) && a.hasOwnProperty(1) && a[0] === undefined, 'fill holes with undefined');
obj = {length: 4294967297}; fill.call(obj, 8, 4294967295); check(obj[4294967295] === 8 && obj[4294967296] === 8 && obj.length === 4294967297, 'fill above uint32');
obj = {length: Infinity}; fill.call(obj, 3, 9007199254740990); check(obj['9007199254740990'] === 3 && !obj.hasOwnProperty('9007199254740991'), 'fill maximum ToLength boundary');
order = ''; obj = {get length() { order += 'l'; gc(); return 2; }};
fill.call(obj, 7, {valueOf: function() { order += 's'; gc(); return 0; }}, {valueOf: function() { order += 'e'; gc(); return 1; }});
check(order === 'lse' && obj[0] === 7 && !obj.hasOwnProperty(1), 'fill coercion order');
throwsType(function() { fill.call('x', 'a'); }, 'fill immutable string index');
throwsType(function() { fill.call(Object.freeze([1]), 2); }, 'fill readonly property');
throwsType(function() { fill.call(Object.preventExtensions(new Array(1)), 2); }, 'fill nonextensible hole');
obj = Object.create(Object.defineProperty({}, '0', {value: 1, writable: false})); obj.length = 1;
throwsType(function() { fill.call(obj, 2); }, 'fill inherited readonly');
obj = {length: 1, get 0() { return 1; }};
throwsType(function() { fill.call(obj, 2); }, 'fill getter only');
obj = {length: 1, set 0(v) { 'use strict'; check(this === obj && v === 5, 'fill setter receiver'); gc(); }};
check(fill.call(obj, 5) === obj, 'fill setter succeeds');
var locked = Object.freeze({x: 1});
obj = {length: 1, set 0(v) { locked.x = 2; check(locked.x === 1, 'Throw does not leak into sloppy setter'); gc(); }};
fill.call(obj, 5);
a = [0, 1, 2]; Object.defineProperty(a, '1', {writable: false});
throwsType(function() { a.fill(8); }, 'fill stops on rejection'); check(a.join() === '8,1,2', 'fill preceding effects retained');
a = [0, 1, 2, 3, 4]; check(a.copyWithin(1, 0, 4) === a && a.join() === '0,0,1,2,3', 'copy overlap backward');
check([0, 1, 2, 3, 4].copyWithin(0, 1).join() === '1,2,3,4,4', 'copy overlap forward');
check([0, 1, 2, 3].copyWithin(-2, -4, -2).join() === '0,1,0,1', 'copy negative bounds');
check([0, 1].copyWithin(Infinity, 0).join() === '0,1', 'copy empty clamped target');
check([0, 1, 2].copyWithin(NaN, 1.8, undefined).join() === '1,2,2', 'copy fractional NaN undefined');
a = [0, , 2, 3]; a.copyWithin(2, 0, 2); check(a[2] === 0 && !a.hasOwnProperty(3), 'copy holes delete target');
obj = Object.create({0: 7}); obj.length = 2; copy.call(obj, 1, 0, 1); check(obj[1] === 7 && obj.hasOwnProperty(1), 'copy inherited source');
obj = {length: 4294967297, 4294967295: 8}; copy.call(obj, 4294967296, 4294967295); check(obj[4294967296] === 8, 'copy above uint32');
obj = {length: Infinity, '9007199254740989': 7}; copy.call(obj, 9007199254740990, 9007199254740989); check(obj['9007199254740990'] === 7, 'copy maximum ToLength boundary');
order = ''; obj = {get length() { order += 'l'; return 2; }, get 0() { order += 'g'; gc(); return 9; }, set 1(v) { order += 'w'; gc(); check(v === 9, 'copy setter value'); }};
function bound(letter, value) { return {valueOf: function() { order += letter; gc(); return value; }}; }
copy.call(obj, bound('t', 1), bound('s', 0), bound('e', 1)); check(order === 'ltsegw', 'copy coercion and access order');
throwsType(function() { copy.call('ab', 1, 0, 1); }, 'copy immutable string index');
throwsType(function() { Object.freeze([1, 2]).copyWithin(1, 0, 1); }, 'copy readonly target');
a = [, 1]; Object.defineProperty(a, '1', {configurable: false});
throwsType(function() { a.copyWithin(1, 0, 1); }, 'copy undeletable target');
a = [1, , 3]; Object.preventExtensions(a); throwsType(function() { a.copyWithin(1, 0, 1); }, 'copy nonextensible hole');
obj = {length: 2, get 0() { throw sentinel; }}; caught = false;
try { copy.call(obj, 1, 0, 1); } catch(e) { caught = e === sentinel; } check(caught, 'copy getter exception');
obj = {length: 2, 0: {x: 5}, set 1(v) { gc(); check([1, 2].copyWithin(1, 0).join() === '1,1', 'reentrant copy'); check(v.x === 5, 'copy value rooted'); }};
copy.call(obj, 1, 0, 1);
/* Existing mutating methods and ordinary assignments retain their old policy. */
var legacy = Object.freeze({x: 1}); legacy.x = 2; check(legacy.x === 1, 'ordinary sloppy assignment');
check([1, 2, 3].map(function(v) { return v * 2; }).join() === '2,4,6', 'legacy map');
print('ES6-ARRAY-OPERATIONS checks=' + checks + ' failures=0');
