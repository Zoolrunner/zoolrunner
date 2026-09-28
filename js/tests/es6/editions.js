/* Run with the historical default shell; compile each fixture explicitly. */
var editionChecks = 0;
function editionCheck(ok, message) {
    ++editionChecks;
    if (!ok) throw Error("FAIL editions: " + message);
}
function inEdition(edition, source) {
    var saved = version();
    try {
        version(edition);
        return evaluate(source, "edition-" + edition);
    } finally {
        version(saved);
    }
}
function rejected(edition, source) {
    try { inEdition(edition, source); } catch (error) {
        return error instanceof SyntaxError;
    }
    return false;
}
var repeated = [
    "({x:1,x:2}).x === 2",
    "({get x(){return 1;},get x(){return 2;}}).x === 2",
    "({get x(){return 1;},x:2}).x === 2",
    "({x:1,get x(){return 2;}}).x === 2",
    "({set x(v){},set x(v){this.saved=v;}})"
];
for (var i = 0; i < repeated.length; ++i) {
    editionCheck(!!inEdition(2015, '"use strict"; ' + repeated[i]), "ES2015 repeated " + i);
    editionCheck(rejected(0, '"use strict"; ' + repeated[i]), "ES5 rejects repeated " + i);
    editionCheck(!!inEdition(170, repeated[i]), "legacy repeated " + i);
}
editionCheck(inEdition(2015, "var o={set x(v){this.saved=v;},set x(v){this.saved=v+1;}};o.x=4;o.saved===5"),
             "last setter wins");
editionCheck(inEdition(2015, "var o={get x(){return this.saved;},set x(v){this.saved=v;}};o.x=7;o.x===7"),
             "getter and setter pair");
editionCheck(rejected(2015, "({__proto__:null, '__proto__':null})"), "duplicate prototype setter rejected");
editionCheck(inEdition(2015, "({get __proto__(){return 1;},get __proto__(){return 2;}}).__proto__===2"),
             "ordinary prototype-named getters can repeat");
var legacyAccessor = "var sink; var o={set x(){sink=arguments[0];}};o.x=42;sink===42";
editionCheck(inEdition(170, legacyAccessor), "legacy setter retained");
editionCheck(rejected(0, legacyAccessor), "default setter grammar retained");
editionCheck(rejected(2015, legacyAccessor), "ES2015 setter grammar");
editionCheck(inEdition(170, "(<x/>).toXMLString()==='<x/>'"), "legacy E4X retained");
editionCheck(rejected(2015, "(<x/>)"), "ES2015 has no implicit E4X");
editionCheck(inEdition(170, "/a/('a')[0]==='a'"), "legacy callable regexp retained");
editionCheck(inEdition(170, "typeof /a/ === 'function'"), "legacy regexp typeof retained");
editionCheck(inEdition(2015, "typeof /a/ === 'object'"), "ES2015 regexp typeof");
editionCheck(inEdition(170, "RegExp.input='a'; /a/.test()"), "legacy regexp implicit input retained");
editionCheck(inEdition(2015, "RegExp.input='a'; !/a/.test() && /undefined/.test()"), "ES2015 regexp undefined input");
editionCheck(inEdition(2015, "var caught=false;try{/a/('a');}catch(e){caught=e instanceof TypeError;}caught"),
             "ES2015 regexp not callable");
editionCheck(inEdition(2015, "var x=1;eval('x',{x:2})===1"), "ES2015 ignores extra eval argument");
editionCheck(inEdition(170, "eval('x',{x:2})===2"), "legacy eval scope retained");
// Calls must use the callee's saved edition and restore the caller's edition.
inEdition(170, "function oldEditionCallback(){return version()===170 && /a/('a')[0]==='a';}");
inEdition(2015, "function modernEditionCallback(){'use strict';return version()===2015 && ({x:1,x:2}).x===2 && oldEditionCallback() && version()===2015;}");
editionCheck(inEdition(0, "modernEditionCallback() && version()===0"), "callback edition restoration");
editionCheck(inEdition(2015, "eval('modernEditionCallback()') && version()===2015"), "nested eval edition");
var original = version();
editionCheck(rejected(2015, "var ="), "compilation failure");
editionCheck(version() === original, "edition restored after compilation failure");
print("ES6-EDITIONS checks=" + editionChecks + " failures=0");
