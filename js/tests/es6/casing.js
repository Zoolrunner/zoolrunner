/* Modern Unicode mapping and legacy-independent native dispatch.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
(function () {
    var checks = 0;
    function check(value, label) { ++checks; if (!value) throw new Error(label); }
    function throws(action) { var caught = false; try { action(); } catch(e) { caught = e instanceof TypeError; } check(caught, 'expected TypeError'); }
    check('\u00df\ufb03'.toUpperCase() === 'SSFFI', 'expanding mappings');
    check('\u0130'.toLowerCase() === 'i\u0307', 'lower expansion');
    check('\ud801\udc00'.toLowerCase() === '\ud801\udc28', 'supplementary lower');
    check('\ud801\udc28'.toUpperCase() === '\ud801\udc00', 'supplementary upper');
    check('A\u03a3'.toLowerCase() === 'a\u03c2', 'final sigma');
    check('A\u03a3B'.toLowerCase() === 'a\u03c3b', 'nonfinal sigma');
    check('\u03a3'.toLowerCase() === '\u03c3', 'unpreceded sigma');
    check('A\u0345\u03a3\u0345'.toLowerCase() === 'a\u0345\u03c2\u0345', 'ignorable and cased precedence');
    check('\u0345\u03a3'.toLowerCase() === '\u0345\u03c3', 'ignorable does not precede');
    check('A\u03a3\ud834\ude42B'.toLowerCase() === 'a\u03c3\ud834\ude42b', 'supplementary ignorable');
    check('\ud800a\udfff'.toUpperCase() === '\ud800A\udfff', 'lone surrogates');
    check(''.toLowerCase() === '', 'empty');
    check('\u00df'.toLocaleUpperCase() === 'SS', 'default locale upper');
    check('A\u03a3'.toLocaleLowerCase() === 'a\u03c2', 'default locale lower');
    check(String.prototype.toUpperCase.call({toString:function () { gc(); return '\u00df'; }}) === 'SS', 'conversion GC');
    throws(function () { String.prototype.toLowerCase.call(null); });
    throws(function () { String.prototype.toUpperCase.call(void 0); });
    throws(function () { String.prototype.toLowerCase.call(Symbol()); });
    throws(function () { new String.prototype.toLowerCase(); });
    var d = Object.getOwnPropertyDescriptor(String.prototype, 'toLowerCase');
    check(d.writable && d.configurable && !d.enumerable && d.value.length === 0, 'descriptor');
    var long = new Array(4097).join('\u0301');
    check(('A\u03a3' + long).toLowerCase() === 'a\u03c2' + long, 'long ignored tail');
    check(('A\u03a3' + long + 'B').toLowerCase() === 'a\u03c3' + long + 'b', 'long ignored nonfinal tail');
    check(new Array(2049).join('\u00df').toUpperCase().length === 4096, 'buffer growth');
    print('ES6-CASING checks=' + checks + ' failures=0');
}());
