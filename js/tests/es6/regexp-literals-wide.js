/* Extended regexp atom operands; kept separate from fast package checks. */
var checks = 0;
function check(ok, label) { ++checks; if (!ok) throw Error('FAIL regexp literal: ' + label); }
/* Exercise the extended atom operand in execution and decompilation. */
var padding = [];
for (var i = 0; i < 65540; ++i) padding.push('"literal' + i + '"');
var wide = evaluate('(function(){var unused=[' + padding.join(',') +
                    '];return /wide/g;})', 'regexp-wide-atom');
check(wide() !== wide() && wide().source === 'wide', 'extended atom execution');
var wideAgain = eval('(' + wide.toString() + ')');
check(wideAgain() !== wideAgain() && wideAgain().global, 'extended atom decompilation');
print('ES6-REGEXP-LITERALS-WIDE checks=' + checks + ' failures=0');
