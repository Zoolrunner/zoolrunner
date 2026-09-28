/* Reviver definitions must reject atomically and ignore a false result. */
var checks = 0;
function same(actual, expected, label) {
    ++checks;
    if (actual !== expected) throw Error(label + ': ' + actual + ' != ' + expected);
}
function collect() { if (typeof gc === 'function') gc(); }
function checkContainer(array, kind) {
    var first = array ? '0' : 'a', second = array ? '1' : 'b';
    var calls = [], getterCalls = 0, setterCalls = 0;
    var result = JSON.parse(array ? '[1,2]' : '{"a":1,"b":2}', function(key, value) {
        calls.push(key);
        if (key === first) {
            if (kind === 'fixed') Object.defineProperty(this, second, {configurable: false});
            if (kind === 'frozen') Object.freeze(this);
            if (kind === 'accessor') Object.defineProperty(this, second, {
                configurable: false,
                get: function() { getterCalls++; collect(); return 2; },
                set: function() { setterCalls++; }
            });
            if (kind === 'missing') {
                delete this[second];
                Object.preventExtensions(this);
            }
            if (kind === 'replace') Object.defineProperty(this, second, {
                configurable: true, enumerable: false, writable: false
            });
            if (kind === 'delete') Object.defineProperty(this, second, {configurable: false});
        }
        collect();
        if (key === second) {
            same(JSON.parse('3'), 3, 'reentrant parse');
            return kind === 'delete' ? undefined : 22;
        }
        return value;
    });
    same(result[first], 1, kind + ' preceding property');
    same(result[second], kind === 'missing' ? undefined : kind === 'replace' ? 22 : 2,
         kind + ' definition result');
    same(calls.join(','), first + ',' + second + ',', kind + ' snapshot visitation');
    same(setterCalls, 0, kind + ' does not invoke setter');
    same(getterCalls, kind === 'accessor' ? 2 : 0, kind + ' getter reads');
    var d = Object.getOwnPropertyDescriptor(result, second);
    if (kind === 'replace') {
        same(d.configurable && d.enumerable && d.writable, true, 'complete data descriptor');
    } else if (kind === 'missing') {
        same(d, undefined, 'nonextensible property remains absent');
    } else {
        same(d.configurable, false, 'rejected definition preserves attributes');
    }
}
var kinds = ['fixed', 'frozen', 'accessor', 'missing', 'replace', 'delete'];
for (var i = 0; i < kinds.length; i++) {
    checkContainer(false, kinds[i]);
    checkContainer(true, kinds[i]);
}
/* These traps are reached through a callback replacing a later sibling. */
if (typeof Proxy === 'function') {
    var trapCalls = 0, sentinel = {}, target = {child: 2};
    function parseProxy(throwing) {
        return JSON.parse('{"a":1,"b":0}', function(key, value) {
            if (key === 'a') this.b = new Proxy(target, {
                defineProperty: function(object, name, descriptor) {
                    trapCalls++;
                    collect();
                    same(name, 'child', 'proxy key');
                    same(descriptor.value, 22, 'proxy value');
                    same(descriptor.writable && descriptor.enumerable && descriptor.configurable,
                         true, 'proxy complete descriptor');
                    if (throwing) throw sentinel;
                    return false;
                }
            });
            return key === 'child' ? 22 : value;
        });
    }
    same(parseProxy(false).b.child, 2, 'false trap result ignored');
    var caught;
    try { parseProxy(true); } catch (e) { caught = e; }
    same(caught, sentinel, 'trap exception propagated');
    same(trapCalls, 2, 'each definition attempted once');
}
print('JSON-REVIVER checks=' + checks + ' failures=0');
