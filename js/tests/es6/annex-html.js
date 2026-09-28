/* ES2015 CreateHTML ordering, quoting and legacy edition preservation.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
var checks = 0;
function check(value, message) {
    ++checks;
    if (!value) throw new Error('Annex HTML: ' + message);
}
function caught(callback) {
    try { callback(); } catch (error) { return error; }
    return null;
}
var names = ['anchor', 'link', 'fontcolor', 'fontsize'];
var tags = ['a name', 'a href', 'font color', 'font size'];
var ends = ['a', 'a', 'font', 'font'];
var i, method, log, receiver, argument, marker = {}, savedVersion;
for (i = 0; i < names.length; ++i) {
    method = String.prototype[names[i]];
    check(method.call('x', '"\ud800"') === '<' + tags[i] + '="&quot;\ud800&quot;">x</' + ends[i] + '>', names[i] + ' quotes and lone surrogate');
    log = [];
    receiver = {toString: function() { log.push('receiver'); gc(); return '\ud83d\ude00'; }};
    argument = {toString: function() { log.push('argument'); gc(); return '"'; }};
    check(method.call(receiver, argument) === '<' + tags[i] + '="&quot;">\ud83d\ude00</' + ends[i] + '>' && log.join() === 'receiver,argument', names[i] + ' conversion order and GC');
    log = [];
    check(caught(function() { method.call(null, argument); }) instanceof TypeError && log.length === 0, names[i] + ' null before argument');
    receiver = {toString: function() { throw marker; }};
    check(caught(function() { method.call(receiver, argument); }) === marker && log.length === 0, names[i] + ' receiver exception');
    check(caught(function() { method.call('x', Symbol()); }) instanceof TypeError, names[i] + ' symbol argument');
    savedVersion = version(170);
    check(method.call('x', '"') === '<' + tags[i] + '=""">x</' + ends[i] + '>', names[i] + ' legacy quoting');
    version(savedVersion);
}
check('"'.bold() === '<b>"</b>', 'content remains unescaped');
check('x'.link('&<>') === '<a href="&<>">x</a>', 'only attribute quotes are escaped');
check(String.prototype.anchor.call(3, 4) === '<a name="4">3</a>', 'primitive receiver and argument');
print('ES6-ANNEX-HTML checks=' + checks + ' failures=0');
