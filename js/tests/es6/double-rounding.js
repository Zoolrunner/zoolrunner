/* ES2015 Number operations use binary64 precision on every supported CPU. */
var x = 9007199254740994.0;
var y = 1.0 - 1 / 65536.0;
var z = x + y;
var difference = z - x;

if (z !== x)
    throw new Error("binary64 addition double-rounded: " + z);

if (difference !== 0)
    throw new Error("binary64 subtraction double-rounded: " + difference);

print("ES6-DOUBLE-ROUNDING checks=2 failures=0");
