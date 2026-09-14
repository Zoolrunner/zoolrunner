/* ES5 library regressions beyond the historical conformance cases. */
var edgeChecks = 0;
function edgeSame(a, b, label) {
    ++edgeChecks;
    if (a !== b) throw Error(label + ': got ' + a);
}
function edgeThrows(fn, kind, label) {
    ++edgeChecks;
    try { fn(); } catch (e) {
        if (e instanceof kind) return;
        throw Error(label + ': wrong exception ' + e);
    }
    throw Error(label + ': missing exception');
}
edgeSame(Date.parse('1970'), 0, 'ISO year');
edgeSame(Date.parse('1970-01'), 0, 'ISO month');
edgeSame(Date.parse('1970-01-01'), 0, 'ISO date');
edgeSame(Date.parse('1970-01-01T00:00'), 0, 'ES5 absent zone is UTC');
edgeSame(Date.parse('1970-01-01T01:30:00.125+01:30'), 125, 'ISO positive offset');
edgeSame(Date.parse('1970-01-01T00:00:00.125-01:30'), 5400125, 'ISO negative offset');
edgeSame(Date.parse('1970-01-01T24:00:00.000Z'), 86400000, 'ISO midnight rollover');
edgeSame(new Date('+010000-01-01T00:00:00.000Z').toISOString(), '+010000-01-01T00:00:00.000Z', 'expanded ISO year');
var invalidISO = ['1970-00', '1970-13', '1970-01-00', '1970-01-32',
                  '1970-01-01T24:01Z', '1970-01-01T00:60Z',
                  '1970-01-01T00:00:60Z', '1970-01-01T00:00+24:00',
                  '1970-01-01T00:00:00.12Z', '-000000-01-01'];
for (var e = 0; e < invalidISO.length; ++e)
    edgeSame(isNaN(Date.parse(invalidISO[e])), true, 'invalid ISO ' + invalidISO[e]);
edgeSame(parseInt('010'), 10, 'parseInt decimal default');
edgeSame(parseInt(' -0x10'), -16, 'parseInt signed hexadecimal');
edgeSame(decodeURIComponent('%EF%BF%BE%EF%BF%BF'), '\ufffe\uffff', 'UTF-8 noncharacters');
edgeSame(new Error(undefined).hasOwnProperty('message'), false, 'undefined error message absent');
edgeSame(new Error('').hasOwnProperty('message'), true, 'empty error message present');
edgeSame((12).toString(undefined), '12', 'undefined radix');
edgeSame(new String('abc')[-1], undefined, 'negative string property');
edgeSame('abc'.slice(1, undefined), 'bc', 'slice undefined end');
edgeSame('abc'.substring(1, undefined), 'bc', 'substring undefined end');
edgeSame('aundefinedb'.split(undefined).join(','), 'aundefinedb', 'undefined separator');
edgeSame('abc'.split(undefined, 0).length, 0, 'zero split limit');
edgeSame('abc'.match()[0], '', 'absent regexp pattern');
edgeSame(/undefined/.exec()[0], 'undefined', 'absent regexp input');
edgeSame(typeof /x/, 'object', 'regexp is not callable');
edgeThrows(function () { /x/(); }, TypeError, 'regexp call rejected');
edgeThrows(function () { new RegExp('', 'gg'); }, SyntaxError, 'duplicate regexp flags');
edgeThrows(function () { new RegExp('[\\Db-G]'); }, SyntaxError, 'range validated after broad class');
edgeSame(/\d/.test('\u0660'), false, 'regexp digits are ASCII');
var edgeArray = {length: 4294967295};
edgeSame(Array.prototype.push.call(edgeArray, 'a', 'b'), 4294967297, 'push does not wrap');
edgeSame(edgeArray[4294967296], 'b', 'push full-width key');
var edgeSplice = {0:'a', 1:'b', 2:'c', length:3};
Array.prototype.splice.call(edgeSplice, 0, 2);
edgeSame(edgeSplice[0], 'c', 'splice shift');
edgeSame(1 in edgeSplice || 2 in edgeSplice, false, 'splice removes generic trailing keys');
try {
    Object.defineProperty(Array.prototype, '0', {value: 100, configurable: true});
    var edgeConcat = Array.prototype.concat.call(101);
    edgeSame(typeof edgeConcat[0], 'object', 'concat boxes receiver');
    edgeSame(+edgeConcat[0], 101, 'concat own element shadows inherited readonly');
} finally { delete Array.prototype[0]; }
var edgeOrder = '';
var edgeLeft = {valueOf: function () { edgeOrder += 'L'; return 2; }};
var edgeRight = {valueOf: function () { edgeOrder += 'R'; return 3; }};
edgeSame(edgeLeft * edgeRight, 6, 'numeric conversion result');
edgeSame(edgeOrder, 'LR', 'numeric conversion order');
edgeThrows(function () { eval('/x/gg'); }, SyntaxError, 'duplicate literal regexp flags');
edgeThrows(function () { eval('({get x(a){}})'); }, SyntaxError, 'getter arity');
edgeThrows(function () { eval('({set x(){}})'); }, SyntaxError, 'setter missing parameter');
edgeThrows(function () { eval('({set x(a,b){}})'); }, SyntaxError, 'setter excess parameters');
edgeSame(Object.prototype.toLocaleString.call({toString:function(){return 17;}}), 17, 'locale string returns method result');
print('ES5-LIBRARY-EDGES checks=' + edgeChecks + ' failures=0');
