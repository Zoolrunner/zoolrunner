/* Historical SpiderMonkey syntax must remain usable by XUL/XPCOM apps. */
var legacyChecks = 0;
function legacyCheck(ok, label) {
    ++legacyChecks;
    if (!ok) throw new Error("FAIL legacy application: " + label);
}
function legacySyntaxError(source) {
    try { eval(source); } catch (e) { return e instanceof SyntaxError; }
    return false;
}
var legacyOldVersion = version();
try {
    var legacyVersions = [150, 160, 170, 180];
    for (var legacyIndex = 0; legacyIndex < legacyVersions.length; ++legacyIndex) {
        version(legacyVersions[legacyIndex]);
        var continuedRegexp = "/a" + String.fromCharCode(92, 10) + "b/";
        legacyCheck(eval(continuedRegexp).test("a\nb"), "escaped regexp line break " + version());
        legacyCheck(legacySyntaxError("'use strict'; " + continuedRegexp),
                    "strict regexp line break " + version());
        legacyCheck(legacySyntaxError("/a" + String.fromCharCode(92)),
                    "unterminated regexp escape " + version());
        legacyCheck(eval("var sink; var o={set value(){sink=arguments[0];}};o.value=42;sink===42"),
                    "zero-argument setter " + version());
        legacyCheck(eval("var o={get value(unused){return arguments.length;}};o.value===0"),
                    "getter parameter " + version());
        legacyCheck(eval("var sink; var o={set value(a,b){sink=[a,b,arguments.length];}};o.value=42;sink[0]===42 && sink[1]===undefined && sink[2]===1"),
                    "extra setter parameter " + version());
        legacyCheck(eval("var o={get value namedGetter(){return 7;}};o.value===7"),
                    "named accessor " + version());
        legacyCheck(eval("({value:0,get value(){return 7;}}).value===7"),
                    "data overridden by accessor " + version());
        legacyCheck(eval("({get value(){return 7;},value:2}).value===2"),
                    "accessor overridden by data " + version());
        legacyCheck(eval("({get value(){return 1;},get value(){return 2;}}).value===2"),
                    "repeated getter " + version());
        legacyCheck(legacySyntaxError("'use strict'; ({value:0,get value(){return 7;}})"),
                    "strict property collision " + version());
        legacyCheck(legacySyntaxError("({set value(){'use strict';}})"),
                    "strict accessor still rejects invalid arity " + version());
        legacyCheck(legacySyntaxError("'use strict'; ({get value(a){}})"),
                    "inherited strict mode still rejects invalid arity " + version());
    }
    version(0);
    legacyCheck(legacySyntaxError(continuedRegexp), "ES5 regexp line break");
    legacyCheck(legacySyntaxError("({value:0,get value(){return 7;}})"), "ES5 data/accessor collision");
    legacyCheck(legacySyntaxError("({get value(){return 1;},get value(){return 2;}})"), "ES5 repeated getter");
    legacyCheck(legacySyntaxError("({set value(){}})"), "ES5 setter arity");
    legacyCheck(legacySyntaxError("({get value(a){}})"), "ES5 getter arity");
    legacyCheck(legacySyntaxError("({set value(a,b){}})"), "ES5 setter excess arguments");
} finally {
    version(legacyOldVersion);
}
print("LEGACY-APPLICATION checks=" + legacyChecks + " failures=0");
