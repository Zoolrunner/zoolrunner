/* ES2015 Number additions, using syntax shared with historical applications. */
var numberChecks = 0;
function numberCheck(ok, label) {
    ++numberChecks;
    if (!ok) throw Error("FAIL ES6 Number: " + label);
}
var names = ["isNaN", "isFinite", "isInteger", "isSafeInteger"];
var hostile = {};
Object.defineProperty(hostile, "valueOf", {get: function () {
    throw Error("Number predicate coerced an object");
}});
Object.defineProperty(hostile, "toString", {get: function () {
    throw Error("Number predicate coerced an object");
}});
var nonNumbers = [undefined, null, true, false, "0", "NaN", "Infinity",
                  {}, [], function () {}, new Number(0), new Number(NaN), hostile];
for (var i = 0; i < names.length; ++i) {
    var name = names[i], method = Number[name];
    numberCheck(typeof method === "function", name + " exists");
    numberCheck(method.length === 1, name + " arity");
    numberCheck(!method.hasOwnProperty("prototype"), name + " no prototype");
    var desc = Object.getOwnPropertyDescriptor(Number, name);
    numberCheck(desc.writable && desc.configurable && !desc.enumerable,
                name + " descriptor");
    numberCheck(method() === false, name + " missing argument");
    for (var j = 0; j < nonNumbers.length; ++j)
        numberCheck(method(nonNumbers[j]) === false, name + " non-number " + j);
    var threw = false;
    try { new method(1); } catch (error) { threw = error instanceof TypeError; }
    numberCheck(threw, name + " is not a constructor");
    numberCheck(method.call(hostile, 1) === method(1), name + " ignores receiver");
}
// value, isNaN, isFinite, isInteger, isSafeInteger
var cases = [
    [NaN, true, false, false, false],
    [Infinity, false, false, false, false],
    [-Infinity, false, false, false, false],
    [0, false, true, true, true], [-0, false, true, true, true],
    [1, false, true, true, true], [-1, false, true, true, true],
    [1.5, false, true, false, false], [-1.5, false, true, false, false],
    [Number.MIN_VALUE, false, true, false, false],
    [Number.MAX_VALUE, false, true, true, false],
    [4503599627370495.5, false, true, false, false],
    [4503599627370496, false, true, true, true],
    [9007199254740991, false, true, true, true],
    [-9007199254740991, false, true, true, true],
    [9007199254740992, false, true, true, false],
    [-9007199254740992, false, true, true, false]
];
for (i = 0; i < cases.length; ++i)
    for (j = 0; j < names.length; ++j)
        numberCheck(Number[names[j]](cases[i][0]) === cases[i][j + 1],
                    names[j] + " numeric case " + i);
var constants = [["EPSILON", Math.pow(2, -52)],
                 ["MAX_SAFE_INTEGER", Math.pow(2, 53) - 1],
                 ["MIN_SAFE_INTEGER", -(Math.pow(2, 53) - 1)]];
for (i = 0; i < constants.length; ++i) {
    name = constants[i][0];
    numberCheck(Number[name] === constants[i][1], name + " value");
    desc = Object.getOwnPropertyDescriptor(Number, name);
    numberCheck(!desc.writable && !desc.enumerable && !desc.configurable,
                name + " descriptor");
}
var conversions = 0;
var coercible = {valueOf: function () { ++conversions; return "7"; }};
numberCheck(isFinite(coercible) && !isNaN(coercible) && conversions === 2,
            "legacy global predicates still coerce");
numberCheck(isFinite("0") && isNaN(undefined), "legacy primitive coercions");
print("ES6-NUMBER checks=" + numberChecks + " failures=0");
