/* Configurable String global helpers retain deletion across caller editions.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
var checks = 0;
function check(value, message) {
    ++checks;
    if (!value) throw new Error('Annex globals: ' + message);
}
var global = this, StringCtor = String;
var names = ['escape', 'unescape', 'encodeURI', 'decodeURI',
             'encodeURIComponent', 'decodeURIComponent', 'uneval'];
var saved = [], i, desc, oldVersion;
for (i = 0; i < names.length; ++i) {
    desc = Object.getOwnPropertyDescriptor(global, names[i]);
    saved.push(desc.value);
    check(desc.configurable && desc.writable && !desc.enumerable, names[i] + ' attributes');
    check(delete global[names[i]] && !(names[i] in global) &&
          !Object.prototype.hasOwnProperty.call(global, names[i]), names[i] + ' stays deleted');
}
check(delete global.String && !('String' in global), 'deleted constructor stays deleted');
gc();
check(saved[0]('a b') === 'a%20b' && saved[1]('a%20b') === 'a b', 'retained helpers remain usable');
check(StringCtor.fromCharCode(65) === 'A' && 'x'.charAt(0) === 'x', 'intrinsic remains usable');
oldVersion = version(170);
for (i = 0; i < names.length; ++i)
    check(typeof global[names[i]] === 'undefined', names[i] + ' deletion survives legacy caller');
check(typeof global.String === 'undefined', 'constructor deletion survives legacy caller');
version(oldVersion);
print('ES6-ANNEX-GLOBALS checks=' + checks + ' failures=0');
