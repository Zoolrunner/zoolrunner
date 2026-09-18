/* IsRegExp callbacks must precede search-string and position conversions. */
var checks = 0;
function check(ok, label) {
    ++checks;
    if (!ok) throw Error('FAIL IsRegExp: ' + label);
}
function throwsSame(fn, expected, label) {
    var caught;
    try { fn(); } catch (e) { caught = e; }
    check(caught === expected, label);
}
function throwsType(fn, label) {
    var caught = false;
    try { fn(); } catch (e) { caught = e instanceof TypeError; }
    check(caught, label);
}
var methods = ['includes', 'startsWith', 'endsWith'];
for (var i = 0; i < methods.length; ++i) {
    var method = methods[i], calls = [], marker = {};
    var receiver = {toString: function() { calls.push('receiver'); gc(); return '/x/'; }};
    var search = {toString: function() { calls.push('search'); gc(); return '/x/'; }};
    var position = {valueOf: function() { calls.push('position'); gc(); return method === 'endsWith' ? 3 : 0; }};
    Object.defineProperty(search, Symbol.match, {
        configurable: true,
        get: function() { calls.push('match'); gc(); return false; }
    });
    check(String.prototype[method].call(receiver, search, position), method + ' result');
    check(calls.join() === 'receiver,match,search,position', method + ' conversion order');
    Object.defineProperty(search, Symbol.match, {get: function() { throw marker; }});
    throwsSame(function() { '/x/'[method](search); }, marker, method + ' getter exception');
    var rx = new RegExp('x');
    rx[Symbol.match] = false;
    check('/x/'[method](rx), method + ' RegExp opt out');
    rx[Symbol.match] = null;
    check('/x/'[method](rx), method + ' null opt out');
    rx[Symbol.match] = undefined;
    throwsType(function() { '/x/'[method](rx); }, method + ' undefined uses internal type');
    var fake = {};
    fake[Symbol.match] = {valueOf: function() { throw marker; }};
    throwsType(function() { ''[method](fake); }, method + ' object truth without conversion');
    var inherited = {};
    inherited[Symbol.match] = true;
    throwsType(function() { ''[method](Object.create(inherited)); }, method + ' inherited classification');
    check('undefined'[method](undefined), method + ' primitive undefined');
    throwsType(function() { ''[method](Symbol('search')); }, method + ' symbol ToString');
    var count = 0;
    Object.defineProperty(rx, Symbol.match, {get: function() {
        ++count;
        gc();
        return false;
    }});
    check('/x/'[method](rx) && count === 1, method + ' one lookup');
    receiver.toString = function() { throw marker; };
    count = 0;
    throwsSame(function() { String.prototype[method].call(receiver, rx); }, marker,
               method + ' receiver exception first');
    check(count === 0, method + ' classification skipped after receiver exception');
}
print('ES6-STRING-MATCH-CLASSIFICATION checks=' + checks + ' failures=0');
