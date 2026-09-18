/* Loaded with an explicit ES2015 MIME version in XUL and HTML. */
"use strict";
function modernWindowEdition() {
    let radixValue = 0b101 + 0o17;
    var caught = false;
    try { var nested; [[nested]] = [null]; }
    catch (error) { caught = error instanceof TypeError; }
    var conflict = false;
    try { eval("(function(parameter){let parameter;})"); }
    catch (error) { conflict = error instanceof SyntaxError; }
    var length = Object.getOwnPropertyDescriptor(modernWindowEdition, "length");
    var name = Object.getOwnPropertyDescriptor(modernWindowEdition, "name");
    var metadata = length.value === 0 && length.configurable && !length.writable &&
                   name.value === "modernWindowEdition" && name.configurable;
    var windowSymbol = Symbol('window'), symbolTarget = {};
    symbolTarget[windowSymbol] = 42;
    var modernObject = {radixValue, [windowSymbol]() { return this; },
                        get ["computedAccessor"]() { return 27; }};
    var symbolChecks = typeof windowSymbol === 'symbol' &&
                       Object.getOwnPropertySymbols(symbolTarget)[0] === windowSymbol &&
                       Object.keys(symbolTarget).length === 0 &&
                       Object.assign({}, symbolTarget)[windowSymbol] === 42 &&
                       Symbol.for('window-registry') === Symbol.for('window-registry');
    var inferred = function() { return 42; };
    var accessor = Object.getOwnPropertyDescriptor({get value() {return 42;}}, "value").get;
    var inferredMetadata = inferred.name === "inferred" && accessor.name === "get value" &&
                           !accessor.hasOwnProperty("prototype");
    const fixed = 1;
    var immutable = false;
    try { fixed++; } catch (error) { immutable = error instanceof TypeError; }
    return radixValue === 20 && Number("0b101") === 5 && caught && conflict && ({value:1,value:2}).value === 2 &&
           metadata && inferredMetadata && immutable && symbolChecks &&
           modernObject.radixValue === 20 && modernObject[windowSymbol]() === modernObject &&
           modernObject[windowSymbol].name === "[window]" && modernObject.computedAccessor === 27 &&
           (function(Array, Object) { return [].length === 0 && ({answer:42}).answer === 42; })(null, null) &&
           Math.imul(4294967295, 5) === -5 && Math.clz32(1) === 31 &&
           1 / Math.trunc(-0.25) === -Infinity && Math.sign(-7) === -1 &&
           Math.round(0.49999999999999994) === 0 && Math.hypot(3, 4) === 5 &&
           Math.log2(Number.MIN_VALUE) === -1074 && Math.cbrt(-8) === -2 &&
           1 / Math.fround(-Number.MIN_VALUE) === -Infinity &&
           Math.expm1(Number.MIN_VALUE) === Number.MIN_VALUE &&
           String.fromCodePoint(0x1f600).codePointAt(0) === 0x1f600 &&
           "ab".repeat(2) === "abab" && "abc".startsWith("b", 1) &&
           "abc".endsWith("b", 2) && "abc".includes("bc") &&
           (function() { var rx = /x/; rx[Symbol.match] = false;
                         return "/x/".includes(rx); })() &&
           String.raw({raw:["a", "b"]}, "!") === "a!b" &&
           Array.of(1, 2).join() === "1,2" &&
           Array.from("a\ud83d\ude00").length === 2 &&
           Array.from({0:3,length:1}, function(v) { return v + 1; })[0] === 4 &&
           [1, 2].find(function(v) { return v > 1; }) === 2 &&
           [1, 2].findIndex(function(v) { return v > 1; }) === 1 &&
           [1, 2, 3].fill(9, 1, 2).join() === "1,9,3" &&
           [1, 2, 3].copyWithin(1, 0, 2).join() === "1,1,2" &&
           Object.is(NaN, NaN) && !Object.is(0, -0) &&
           Object.assign({}, {a: 1}, {a: 2}).a === 2 &&
           Object.getPrototypeOf(3) === Number.prototype &&
           Object.keys("ab").join() === "0,1" && Object.isFrozen(1) &&
           Object.freeze(1) === 1 && !({}).hasOwnProperty("__proto__") &&
           (function() { var box = new String("ab"); Object.setPrototypeOf(box, null);
                         return box.length === 2 && box[1] === "b"; })() &&
           "e\u0301".normalize() === "\u00e9" && "\uac01".normalize("NFD").length === 3 &&
           modernWindowEdition.bind(null).name === "bound modernWindowEdition" &&
           eval("({value:1,value:2}).value") === 2 &&
           (function() { var values = []; for (var i = 0; i < 2; ++i) values.push(/fresh/g);
                         values[0].lastIndex = 9;
                         return values[0] !== values[1] && values[1].lastIndex === 0; })() &&
           (function() { var target = {}; target[Symbol.hasInstance] = function(v) { return v === 7; };
                         return 7 instanceof target && !(8 instanceof target); })() &&
           Math[Symbol.toStringTag] === "Math" && JSON[Symbol.toStringTag] === "JSON" &&
           [3].values().next().value === 3 && [3].entries().next().value.join() === "0,3" &&
           "\ud83d\ude00"[Symbol.iterator]().next().value.length === 2 &&
           (function(v) { return arguments[Symbol.iterator]().next().value === v; })(17) &&
           Function("var values = 19; with ([]) { return values === 19; }")() &&
           new Map([[windowSymbol, 31]]).get(windowSymbol) === 31 &&
           Array.from(new Set([1, 1, 2])).join() === "1,2" &&
           new WeakMap([[symbolTarget, windowSymbol]]).get(symbolTarget) === windowSymbol &&
           new WeakSet([symbolTarget]).has(symbolTarget) &&
           Reflect.get(symbolTarget, windowSymbol) === symbolTarget[windowSymbol] &&
           Reflect.ownKeys({reflectWindow: 1})[0] === "reflectWindow" &&
           Reflect.enumerate({reflectWindow: 1}).next().value === "reflectWindow" &&
           new Proxy({proxyWindow: 23}, {}).proxyWindow === 23 &&
           JSON.stringify(new Proxy([1, 2], {})) === "[1,2]" &&
           (function() { var p = Proxy.revocable({}, {}); p.revoke();
                         try { Reflect.ownKeys(p.proxy); return false; }
                         catch (e) { return e instanceof TypeError; } })() &&
           "x".link('"') === '<a href="&quot;">x</a>' &&
           (function() { var order = []; String.prototype.anchor.call(
                         {toString:function(){order.push("receiver");return "x";}},
                         {toString:function(){order.push("attribute");return "y";}});
                         return order.join() === "receiver,attribute"; })() &&
           typeof /a/ === "object";
}
var modernWindowLoaded = modernWindowEdition();
