# ECMAScript 2015 conformance work

Full [ECMAScript 2015 (ES6)](https://262.ecma-international.org/6.0/) compliance
is required. Implementation is in progress; ZoolRunner is not ES6 compliant.
Preserve the classic JSAPI, historical XUL applications and explicitly selected
legacy JavaScript versions. Keep the [ES5.1 regression gate](../es5/README.md)
and application checks alongside this work.

## Reproducible initial baseline

The initial corpus is official TC39 Test262 at
[`5e653f2e6ca14ac1ad8e801955a709cae7ac8a11`](https://github.com/tc39/test262/tree/5e653f2e6ca14ac1ad8e801955a709cae7ac8a11),
the final snapshot from 2015. It includes all 14,968 JavaScript files under
`annexB`, `built-ins` and `language`: 28,582 test/mode cases. ECMA-402's separate
`intl402` suite and the harness's own tests are not ECMA-262 conformance cases.
No failing language features are excluded. A clean pinned checkout is required.

This historical snapshot is a starting baseline, not the complete eventual
coverage target. Later upstream tests also exercise ES2015 semantics, and the
modern Test262 corpus includes later language editions. Even this snapshot
contains 60 test/mode cases tagged `es7id` (including proposed corrections).
The runner records edition references and excludes none of these from the
baseline. A reviewed ES2015 inventory, covering both those edition boundaries
and later ES2015 regressions, is still needed; passing this initial corpus
alone must not be described as satisfying all ES6 tests or proving compliance.

Python 3 and PyYAML are optional test-runner dependencies, not application build
dependencies. Keep the upstream checkout outside the ZoolRunner source tree:

```sh
git init /tmp/test262-es2015
git -C /tmp/test262-es2015 fetch --depth 1 https://github.com/tc39/test262.git \
  5e653f2e6ca14ac1ad8e801955a709cae7ac8a11
git -C /tmp/test262-es2015 checkout --detach FETCH_HEAD
python3 js/tests/es6/test-runner.py
python3 js/tests/es6/test-runner-integration.py \
  --suite /tmp/test262-es2015 \
  --shell obj-zoolrunner-macos-arm64-xulrunner/dist/bin/xpcshell
python3 js/tests/es6/run-test262.py \
  --suite /tmp/test262-es2015 \
  --shell obj-zoolrunner-macos-arm64-xulrunner/dist/bin/xpcshell \
  --report /tmp/zoolrunner-es2015.json
```

Do not replace the shell or its libraries during a run. Unmarked tests run in
both strict and non-strict mode, following this snapshot's upstream runner.
`noStrict`, `onlyStrict`, `raw` and `module` flags are retained. This differs
intentionally from the older ES5 corpus's non-strict default. The test's own
directive prologue remains effective. Unicode test source and the harness are
compiled as separate global scripts, with a transport/strictness preflight.
Expected exceptions use this snapshot's negative-test patterns. A harness
exception, crash, timeout or missing completion marker cannot satisfy a test.
The timezone defaults to `America/Los_Angeles` and is recorded in the report.
The runner now selects the explicit ES2015 edition (`xpcshell -E -v 2015`) and
checks that selection during preflight. `--edition legacy` reproduces the
original default-language baseline. Reports identify the selected edition;
do not compare a legacy run with an ES2015 run as if the semantics were equal.

Modules are currently reported as **unsupported**, not passed or skipped.
Async tests without synchronous `$DONE` completion are also unsupported: a
Promise job queue and host completion pump remain to be implemented. These
statuses count against completion and give the runner a failing exit status.
Every result is recorded. `--filter` is only a diagnostic subset and cannot
establish a full-suite pass. Engine and host support must eventually execute
these tests normally; do not turn unsupported cases into exclusions.

## Implementation and compatibility gates

The first implementation group adds the four non-coercing Number predicates
and `EPSILON`, `MAX_SAFE_INTEGER`, `MIN_SAFE_INTEGER`. Global `isNaN` and
`isFinite` retain their coercing behavior. Run `number.js` through xpcshell
with the matching runtime library path. Its focused checks cover hostile
objects, wrappers, signed zero, fractional and safe-integer boundaries,
constant descriptors and non-constructible native methods.

`window-app/application.ini` is a standalone XULRunner fixture; use a disposable
profile and require `ES6-WINDOW checks=15 failures=0`. It checks the additions
in chrome and content window globals as well as an unchanged legacy XUL setter.
The focused shell test requires `ES6-NUMBER checks=156 failures=0`.

The original Number subset has 338 passes and 26 failures in 364 test/mode
cases on the macOS arm64 Suite. Missing predicates, binary/octal conversion
and Symbol coercion account for those failures. This is not a full ES6 result.
With the Number additions, the native arm64 XULRunner subset passes 358/364;
the six remaining failures concern binary/octal conversion and Symbol coercion.

The complete initial Suite baseline on 2026-09-17, before these additions
(source `f9eccf03`), ran in 847 seconds on macOS arm64:

| Result | Test/mode cases |
| --- | ---: |
| Pass | 22,467 |
| Fail | 6,091 |
| Unsupported | 14 |
| Timeout | 2 |
| Crash | 0 |
| Harness/completion error | 8 |

All 14 unsupported cases are modules. Two harness errors arise because the
TypedArray harness requires the unimplemented `Float64Array`; six nested
destructuring cases exit without a completion marker. The two timeout cases
are strict/non-strict variants of a `const` loop syntax test. These remain
unresolved, counted results; none were silently accepted or excluded.

After the Number additions, the full pinned ES5 corpus passes **11,540/11,540**
on macOS arm64 XULRunner, with zero failures, timeouts, crashes or harness errors.
XULRunner also passes the 156 Number assertions, 15 Number window checks,
18 native embedding checks, 17 existing window-bootstrap checks, the existing
packaged shell/application regressions and all 169 layout assertions.
The subsequent complete run with explicit ES2015 selection is recorded below.
The rebuilt macOS arm64 Suite also passes 358/364 Number cases, all 156 focused
Number assertions, packaging/embedding and the existing desktop regressions,
including Composer lifecycle, window bootstrap and ChatZilla initialization.
Unmodified ChatZilla also initializes with its input widget in a temporary
standalone XULRunner wrapper using ChatZilla's historical application ID.
Calendar passes the 156 Number assertions, eight unchanged compatibility test
files, startup and all four views. These application checks do not establish
compatibility with every historical application.

Major remaining work includes Symbol values and property keys; lexical scopes
and TDZ; ES2015 functions, destructuring, classes and `super`; iterators and
generators; collections and typed arrays; standard-library and regexp changes;
Promise jobs; and module compilation, linking and evaluation. Preserve XDR and
decompilation when bytecode changes. Review compatibility at the script-loading
boundary instead of changing old applications to accommodate engine regressions.

ES2015 also changes some earlier semantics rather than only adding features.
For example, built-in function `length` descriptors become configurable and
strict object literals permit repeated data-property names. ES5 and ES2015
tests for those behaviors cannot both pass under one identical set of semantics.
Before implementing such changes, provide an explicit edition selection for
conformance execution and preserve historical loader/version behavior. Keep
the original ES5 assertions, running them in the ES5 compatibility environment;
run the ES2015 assertions in the ES2015 environment. Record that selection in
reports. The first edition boundary is now implemented through
`JSVERSION_ECMA_2015` / `JS_SetVersion`, with `ECMAv6` as its JSAPI string name.
Existing numeric version values and the historical default remain unchanged.
ES2015 allows repeated ordinary object properties, including strict code;
duplicate literal `__proto__` setters remain an early error. Accessors can
shadow inherited properties while honoring embedding access checks. The new
mode does not inherit legacy E4X operators, callable regexps, implicit regexp
input or eval's second-argument scope extension. Legacy versions retain those
behaviors. The bytecode cache version has been advanced.

HTML and XUL scripts can opt in with `type="application/javascript;version=2015"`.
Unversioned XUL scripts still use the historical JS 1.7 grammar; ordinary
unversioned HTML scripts retain the existing default. This is an implementation
boundary, not a claim of complete ES2015 support. Global built-in initialization
still needs a compatibility policy: script edition selection alone cannot give
one shared function object two incompatible property descriptors.

`editions.js` tests the language boundary. `TestEditionEmbedding.c` tests JSAPI
version round trips, saved editions in XDR, nested eval/Function compilation,
decompilation, cross-edition callbacks, garbage collection, caller restoration
and embedding access controls. Compile it using the same flags as
`../es5/TestObjectEmbedding.c`. The `window-editions.xul` fixture tests explicit
modern scripts alongside unchanged legacy XUL and default HTML scripts; launch
the existing `window-app/application.ini` with
`-chrome chrome://es6window/content/window-editions.xul` and a disposable profile.
Do not approximate edition selection by changing behavior only while a test
is running or by identifying Test262 sources.

## Edition boundary validation (macOS arm64, 2026-09-17)

The complete pinned corpus in explicit ES2015 mode ran all 28,582 cases in
814 seconds, with the runtime binary hashes unchanged throughout:

| Result | Test/mode cases |
| --- | ---: |
| Pass | 22,550 |
| Fail | 6,008 |
| Unsupported | 14 |
| Timeout | 2 |
| Crash | 0 |
| Harness/completion error | 8 |

Compared with the initial historical-default baseline, 123 cases gained a pass
and 40 lost a pass. The latter involve incomplete `let`/`yield` handling in the
new mode, which currently inherits parts of the older block-scope parser.
The subsequent contextual-keyword changes resolve those 40 regressions;
they were never excluded. The object-literal subset
improved from 85/208 to 105/208, with no previous passes lost. This comparison
also includes the earlier Number additions; it is not an isolated measurement
of the edition patch alone. The ES5 default-mode regression gate passes all
11,540 cases again, with no failures, crashes, timeouts or harness errors.

All four native arm64 applications build and pass packaging, 156 Number
checks, 36 edition checks, 18 existing embedding checks and 11 new edition
embedding checks. Their relocated desktop runs pass the existing shell/window
regressions and all five mixed-edition window checks. Browser navigation through
the normal browser window passes all 169 layout assertions. Calendar passes
eight compatibility test files and all four views. ChatZilla initializes in
Suite and in its unchanged standalone XULRunner wrapper with the input widget.

The first Suite lifecycle run exposed a fixture-state problem: some initial
Composer documents were already modified, so insertion caused no new dirty-state
notification. Package diagnostics reproduced that state. The fixture now resets
and verifies its modification count before observing the clean-to-dirty
transition, preserving all editing/undo/shutdown assertions. Three fresh-package
runs pass all 24 checks after that correction. See the
[lifecycle test documentation](../../../editor/composer/tests/README.md).

Modern macOS packaging now includes the focused edition and native embedding
gates, and the desktop runner includes the mixed-edition window fixture. These
are local arm64 results, not new GitHub-hosted, x86_64, Windows or Linux results.

For engine changes, rerun the complete pinned ES5 corpus, the affected ES2015
tests and focused regressions. Native embedding, chrome/content globals,
XULRunner, Suite, ChatZilla and Calendar application checks are separate gates.
Record architectures and runtime results explicitly; macOS arm64 results do
not establish MSVC2005, other architectures or legacy operating-system support.

## Contextual keywords and declaration checks

In the explicitly selected ES2015 edition, `let` is recognized as a lexical
statement only in declaration positions. Ordinary non-strict uses of `let`
and `yield` remain identifiers; explicitly selected JS 1.7 retains its let
expressions and historical generators. Lexical declarations cannot be bare
`if`, loop, `with`, or labelled statement bodies, and a `let` for-in declaration
cannot have an initializer. Escaped identifier spellings do not become a
contextual declaration keyword.

The parser tracks lexical and var declarations by statement-list scope to
reject conflicts without confusing valid shadowing or separate sibling blocks.
`contextual-keywords.js` exercises these rules with 84 assertions, including
strict and non-strict cases, destructuring identifiers, catch parameters and
legacy syntax. The macOS package gate runs this test alongside the earlier
edition and Number tests.

These checks do not complete lexical declarations. Top-level `let` still uses
the historical global storage path; persistent global lexical environments,
temporal dead zones, per-iteration environments, complete parameter/catch
conflict rules, and ES2015 `const` semantics remain unfinished. Modern generator
syntax and iteration semantics also remain unfinished. Do not describe the
contextual-keyword checks as complete `let` or generator support.

A separate diagnostic fix restores TypeError propagation for nested
array destructuring of `null`, `undefined`, or an array hole inside a function.
The upstream assignment/destructuring subset passes 74/267 cases with zero
harness errors or lost previous passes; the remaining 193 are still failures.
All six previously missing completion markers in this subset now pass.
`../es5/destructuring-errors.js` adds 54 cross-edition regression checks,
including complete function decompilation round trips. No bytecode format
changes are involved in this diagnostic fix.


After contextual-keyword and declaration checks, a complete 28,582-case run
on macOS arm64 XULRunner recorded **22,595 passes, 5,963 failures, 14 unsupported,
2 timeouts, 0 crashes, and 8 harness/completion errors** in 735.85 seconds.
Runtime hashes were unchanged. Relative to the earlier explicit-edition run,
45 cases gained a pass and none lost a pass. The corresponding complete ES5
run passed **11,540/11,540** again. These full-run totals precede the subsequent
destructuring-diagnostic and radix-literal fixes.

## Binary and octal numbers

Explicit ES2015 mode accepts `0b`/`0B` and `0o`/`0O` literals and numeric
strings. Conversion uses the existing integer converter, including its
rounding logic for power-of-two radices. Signed radix strings remain invalid;
unary signs on source literals still work. `parseInt` and `parseFloat` retain
their own grammars. Historical modes keep their earlier source and conversion
behavior. `radix-literals.js` supplies 130 checks for grammar, whitespace,
rounding, overflow, property names, coercion callbacks, and decompilation.

The pinned numeric-literal subset passes **170/170** and the Number subset
passes **362/364** on macOS arm64 XULRunner. The two remaining Number failures
require Symbol conversion. A later upstream numeric regression,
`built-ins/Number/string-hex-literal-invalid.js`, also passes in strict and
non-strict modes at current-upstream pin
`35d566604512cba908054eec49f85e64a59f3091`. That is a separately reviewed test,
not a full current-Test262 result. The current corpus inventory has 48,912
JavaScript files under the three core roots and still needs edition review;
its later-language tests must not be confused with ES2015 requirements.

The subsequent complete run including radix support and diagnostic fallback
records **22,609 passes, 5,955 failures, 14 unsupported, 2 timeouts, 0 crashes,
and 2 harness errors** in 543.99 seconds, with unchanged runtime hashes.
It gains 14 passes over the contextual-keyword run and loses none. The two
remaining harness errors require `Float64Array`; the six destructuring
completion failures are resolved. The corresponding full ES5 run passes
**11,540/11,540**. These full totals precede the parameter-history correction.

Function-body `let` declarations now reject conflicts with ordinary or
destructured formal parameters, while nested blocks may shadow parameters.
Catch-body conflicts also report SyntaxError. `lexical-parameters.js` covers
36 cases, including Function construction and preserved JS 1.7 shadowing.
The native embedding fixture now has 13 checks, including modern rejection and
legacy acceptance through `JS_CompileFunction`. Its XDR/decompilation payload
and the real window fixture also exercise lexical bindings, numeric prefixes,
and nested destructuring exceptions. See `../es5/strict-parameter-history.js`
for the separate shared-property duplicate-marker regression.

After the parameter correction, the complete ES5 gate again passes
**11,540/11,540** on macOS arm64 XULRunner. The complete ES2015 language subset
records 6,670 passes, 1,508 failures, 14 unsupported modules, 2 timeouts and no
crashes/harness errors across 8,194 cases. The Function subset records 657
passes and 58 failures across 715 cases. Neither subset loses any previous
passes. They are supplemental checks, not replacements for the full totals
above or claims of complete ES2015 compliance.

The final batch rebuilds the engine for all four macOS arm64 applications.
Suite, Browser, Calendar and XULRunner each pass packaging, all new shell and
13 native edition/embedding checks, and relocated desktop regressions with the
expanded mixed-edition fixture. Calendar passes its eight compatibility files
and four views; Suite passes Composer lifecycle and ChatZilla startup. Browser
navigation passes all 169 existing layout assertions. Unchanged ChatZilla also
initializes its input widget in a disposable standalone XULRunner application.
Other operating systems and architectures have not been revalidated for this
batch; these native macOS results do not establish their compatibility.

## Modern function metadata and globals

`xpcshell -E` selects ES2015 before global built-ins are initialized. The
conformance runner uses this option and checks the native `Number.length`
descriptor during preflight. `-v 2015` still switches subsequent scripts in
an existing global; it does not replace that global's built-ins. Existing
application globals and unversioned shell invocations retain their historical
initialization. Native embeddings can select `JSVERSION_ECMA_2015` before
`JS_InitStandardClasses` to create a modern global.

Modern functions have own, non-enumerable, read-only, configurable `length`
data properties; named functions also have matching `name` properties.
Anonymous expressions without an inferred name have no own `name` property. Function clones preserve their creation edition;
internal templates no longer expose deleted metadata through inheritance.
Metadata is installed after code generation has reserved regexp cache slots.
Direct eval continues resolving locals through the compiler template.
Bound functions compute their metadata without coercing non-number lengths
or non-string names. Modern globals install the shared restricted `caller`
and `arguments` accessors on `Function.prototype`. Legacy globals retain
their earlier descriptors. Anonymous inferred names and complete mixed-global
interoperability still require work; this is not full function conformance.
The XDR version is incremented because cached functions now retain their
creation edition.

`function-metadata.js` has 82 checks including reentrant getters, collection,
closures, bound construction, and restricted accessors. Run it with `-E`.
`TestFunctionMetadata.c` has 17 native checks for modern initialization,
XDR decoding in both directions between modern and legacy modes, and JSAPI cloning of
interpreted and bound functions. Legacy functions created inside a modern
global retain their metadata getters and historical `arguments` behavior. Both
pass on macOS arm64 XULRunner. A complete ES5 rerun passes all 11,540 cases.
The final complete ES2015 run on Suite records **22,846 passes, 5,718 failures,
14 unsupported modules, two timeouts, zero crashes, and two harness errors**
in 462.87 seconds, with unchanged runtime hashes. It gains 237 passes and loses
none relative to the previous full baseline. The final Function subset passes
691/715, gaining 34 passes and losing none; remaining cases exercise Symbols
or Proxies, and accessor-function prototype restrictions also remain incomplete.

All four macOS arm64 applications rebuild and pass packaging with the 82 shell
checks, 17 metadata embedding checks, and existing 13 edition embedding checks.
Relocated desktop tests pass for Suite, Browser, Calendar and XULRunner, including
the expanded mixed-edition window fixture. Suite passes Composer lifecycle and
ChatZilla startup; Browser navigation passes 169 layout assertions. Calendar's
eight compatibility files and four views pass. Unchanged standalone ChatZilla
also initializes its input widget in XULRunner and exits successfully. The
runner's five unit checks and 17 real-shell checks pass. Other operating systems
and architectures have not been revalidated for this batch. Thousands of
conformance failures remain; these results do not establish ES2015 completion.

## Immutable writes and extended operands

Explicit ES2015 mode now rejects missing `const` initializers and constant
statements where a lexical declaration is not permitted. Assignment, compound
assignment, increment/decrement and destructuring writes to initialized
constant bindings throw `TypeError`, including captured bindings and direct
eval. Operand evaluation and numeric coercion happen before the immutable-write
exception; coercion exceptions retain precedence. Legacy script editions keep
their historical ignored-write and optional-initializer behavior. Ordinary
read-only object properties and named-function self-bindings remain separate.

This does **not** complete lexical bindings: block/global lexical environments,
temporal dead zones, per-iteration environments and const for-in/of declarations
remain unfinished. Modern const still uses historical storage internally.

`const-writes.js` has 104 checks covering both language editions, strict and
non-strict writes, closures, eval, GC during coercion, finally blocks and
function decompilation. `test-const-large-script.py --shell PATH` adds 14 checks
across the 16-bit atom-index boundary, including compound/destructuring writes
and legacy/modern object initializer value ordering. These checks execute both
compiled and decompiled functions. The large-script test caught and corrected
an extended-property initializer ordering bug as well as the new opcode's
extended-operand emission. Both regressions run during macOS packaging.
The bytecode cache version is now 34; the native edition/XDR and real-window
fixtures also exercise modern const writes.

The first full macOS arm64 XULRunner run with immutable-write support recorded
**22,884 passes, 5,682 failures, 14 unsupported modules, zero timeouts, zero
crashes and two harness errors** in 507 seconds. It gained 38 passes and lost
none relative to the preceding metadata run. ES5 passed all 11,540 cases.
After the extended-operand fixes, a second complete XULRunner run records the
same counts in 745.97 seconds; its ES5 rerun passes 11,540/11,540 in 540.85
seconds. The final Suite run, including the locked-property lookup adjustment,
also records the same ES2015 counts in 677.97 seconds. Both ES2015 runs
verify unchanged runtime hashes. No previous passing case is lost.

All four macOS arm64 applications pass engine/loader builds, packaging and
relocated desktop checks. XULRunner's aggregate XUL library is relinked after
its XPConnect loader rebuild. Package checks include all 104 immutable-write
checks, 14 extended-operand checks, 82 metadata checks, 17 metadata embedding
checks and 13 edition embedding checks. Suite passes the 24 lifecycle checks
and ChatZilla startup; Browser navigation passes 169 assertions; Calendar
passes eight unit files and all four views. Standalone ChatZilla initializes
its input widget and exits successfully in XULRunner. Runner unit and
integration checks pass. Other operating systems and architectures have not
been revalidated for this batch; full ES2015 conformance remains incomplete.


## Inferred function names

In explicit ES2015 mode, anonymous ordinary function expressions acquire names
from `var`/`let`/`const` initializers, identifier assignments and static object
property definitions. Parentheses around the function expression preserve
eligibility; comma, conditional and logical expressions do not. Parenthesized
assignment targets and property assignments do not infer names in ES2015.
Annex B's `__proto__` prototype setters also do not infer names. Explicit
function names retain precedence. This follows the edition's
[SetFunctionName](https://262.ecma-international.org/6.0/#sec-setfunctionname)
operation and its syntax-specific callers.

Modern object accessors acquire `get ` / `set ` prefixes, have no automatic
`prototype` property and reject construction. Legacy editions retain their
historical anonymous accessor names and constructor behavior. Quoted, numeric
and keyword accessor names decompile using modern accessor syntax in ES2015
scripts. Inferred names are stored separately from the function's lexical name,
traced by GC, retained through cloning and serialized through XDR version 35.
They do not create a self-binding or rewrite anonymous function syntax during
decompilation. The historical JSAPI diagnostic-name accessors remain based on
the declared function name.

`inferred-function-names.js` has 89 checks for inference and its exclusions,
descriptors, Unicode names, closures, GC, regexp slots, cloning, decompilation
and legacy behavior. Run it with `xpcshell -E`. `TestFunctionMetadata.c` now
has 20 checks, retaining the prior named-function and bound-function cases and
adding inferred names through XDR and JSAPI cloning. Both pass in the native
macOS arm64 Browser shell; the existing 82 metadata checks also pass. The
mixed-edition window payload includes inferred and accessor metadata.

The diagnostic `fn-name` subset passes 22/104 test/mode cases. The remaining
cases also require computed properties, concise methods, destructuring defaults,
arrows, generators or classes. Those features remain unfinished.

Parenthesized identifier assignment targets retain a bytecode source note, and
conditional folding preserves anonymous-function branches whose decompilation
would otherwise acquire a name. The earlier extended-operand fixture now has
24 checks. It also covers legacy and modern global reads/writes, increments,
compound assignment, deletion, for-in name/property targets and method calls.
The new coverage corrected existing extended-operand stack/decompilation paths:
`FINDNAME` decompiles as an identifier, compound assignment retains its object
and id, for-in targets retain their literal operands, and source notes are read
at the start of an extended instruction.

The complete Suite ES2015 rerun records **22,900 passes, 5,666 failures,
14 unsupported modules, zero timeouts, zero crashes and two harness errors**.
Compared with the const batch, 16 additional cases pass and no passing case
regresses. The runtime hash remained unchanged throughout the run. The full
required-mode ES5 rerun passes all **11,540 cases**. Reports are
`artifacts/es6/inferred-names-final-full.json` and
`artifacts/es6/inferred-names-final-es5.json`.

All four macOS arm64 applications rebuilt and passed packaging and relocated
runtime checks. Suite passed Composer lifecycle and ChatZilla checks; Browser
passed 169 navigation/layout assertions; Calendar passed eight unit suites and
all four views; standalone ChatZilla initialized with working input in XULRunner.
These results cover this revision on macOS arm64 only. Other operating systems
and architectures have not been revalidated for this batch. Full ES2015
conformance and universal historical-application compatibility are not established.


## Integer Math operations and rounding

`Math.sign`, `Math.trunc`, `Math.clz32` and `Math.imul` are implemented through
portable C89 code and existing engine conversion helpers. They are also
available to legacy scripts, like the earlier Number additions. Conversion
order, exceptions, signed zero and modulo-32-bit multiplication are covered by
`math-integer.js` (169 checks), including callback-triggered GC. The new methods
are nonconstructible and preserve the ordinary Math property descriptors.
`Math.round` now avoids prematurely rounding `x + 0.5`, correcting values just
below one half and large odd integers without changing tie direction.

The diagnostic Math subset passes 376/468 test/mode cases, with all cases for
these four methods and `round` passing. The remaining Math cases require the
other ES2015 numeric methods and Symbol support.

The complete ES2015 run records **22,938 passes, 5,628 failures, 14 unsupported
modules, zero timeouts, zero crashes and two harness errors**. This adds 38 passes
with no previously passing case lost; runtime hashes stayed unchanged. The
complete required-mode ES5 run passes all **11,540 cases**. Reports are
`artifacts/es6/math-integer-full.json` and `artifacts/es6/math-integer-es5.json`.

All four macOS arm64 engines rebuilt and passed packaging and relocated runtime
checks. This includes Suite Composer lifecycle and ChatZilla checks, Browser's
169 navigation/layout assertions, Calendar's eight unit suites and four views,
and standalone ChatZilla startup/input in XULRunner. Other operating systems
and architectures have not been revalidated for this batch; the ES6 goal remains
incomplete.


## Remaining ES2015 numeric Math methods

The standard Math method names are now implemented, including `expm1`, `log1p`,
`cbrt`, the hyperbolic/inverse-hyperbolic functions, `log10`, `log2`, `fround` and
`hypot`. Math's Symbol-based tag remains unavailable until Symbol support lands.
These additions are available in legacy globals as well as modern globals;
existing application-facing methods and embeddings remain in place.

Ten numerical kernels use the already bundled Sun fdlibm sources, with their
original licenses retained. The normal configure/make build and standalone
reference makefile compile only these selected kernels through a private target
endianness adapter. Existing ES3/ES5 transcendental methods retain their current
host/fdlibm policy. No C99-only math imports are required in the application,
including the MSVC 2005 build. The adaptation also corrects stale union state in
subnormal cube roots, signed exponent shifts, an atanh low-word negation and
aliasing-dependent low-word reads in sinh/cosh.

`fround` explicitly rounds a binary32 significand to nearest/even, including
subnormals and overflow, without depending on a host float cast. `log2` preserves
exact powers of two. `hypot` uses scaled compensated summation and performs all
argument conversions in order, including after Infinity or NaN.

`math-transcendental.js` passes **4,206 checks**, covering descriptors,
nonconstructibility, signed zero, special values, coercion/GC, subnormals,
rounding ties, overflow boundaries and many-argument hypot. The upstream Math
subset passes **466/468 test/mode cases**; its two failures require
`Symbol.toStringTag`.

The complete ES2015 run records **23,028 passes, 5,538 failures, 14 unsupported
modules, zero timeouts, zero crashes and two harness errors**. Compared with the
integer Math batch, 90 additional cases pass and no passing case regresses.
Runtime hashes remained unchanged. The complete required-mode ES5 run passes
all **11,540 cases**. Reports are `artifacts/es6/math-numeric-full.json` and
`artifacts/es6/math-numeric-es5.json`.

All four macOS arm64 engines rebuilt and passed packaging and relocated runtime
checks, including Suite Composer/ChatZilla, Browser's 169 navigation assertions,
Calendar's eight unit suites/four views and standalone ChatZilla in XULRunner.
Other operating systems and architectures have not been revalidated for this
batch. The remaining ES2015 failures still prevent a conformance claim.

The optional host diagnostic compares ten kernels with a modern C99 libm using
special/boundary values and deterministic binary64 samples. It checks special
values exactly and finite results within four times binary64 epsilon relative
tolerance (with a minimum-subnormal absolute floor). This is a numerical
cross-check, not proof of correct rounding or target-OS compatibility:

```sh
python3 js/tests/es6/test-math-kernels.py \
  --objdir obj-zoolrunner-macos-arm64-suite --cc clang --sanitize
```

On macOS arm64 this passes **2,000,510 comparisons** with ASan and UBSan. The
application JavaScript fixture remains the portable runtime regression; the
host diagnostic requires a compiler and C99 libm supporting the reference
functions, which MSVC 2005 itself does not provide.


## String code points, repetition, literal search and raw assembly

The engine now implements `String.fromCodePoint`, `String.raw`, and prototype
`codePointAt`, `repeat`, `startsWith`, `endsWith` and `includes`. These additions
are available in legacy globals too; existing `indexOf`, `substr`, `substring`
and other historical methods retain their behavior. Template-literal parsing,
Unicode normalization, Symbol conversion and `Symbol.match` customization
remain separate unfinished work. The literal search methods currently reject
actual RegExp objects; their complete IsRegExp protocol requires Symbols.

New string-producing methods check lengths before allocation and retain the
historical immediate-integer length limit of 1,073,741,823 UTF-16 code units.
`repeat` doubles initialized buffer regions, and `raw` grows its buffer
geometrically. Raw assembly observes length once, retrieves segments in order,
preserves abrupt completions and ignores excess substitutions. Character
pointers are acquired after observable argument conversions; converted strings
and temporary raw values stay rooted across callbacks and GC.

`string-additions.js` passes **175 checks**, including descriptors, constructors,
receiver validation, surrogate pairs/lone surrogates, embedded NULs, conversion
order, GC, reentrant raw calls, exceptions and legacy behavior. The upstream
String subset passes **1,725/1,857 test/mode cases**.

The full ES2015 run records **23,232 passes, 5,334 failures, 14 unsupported
modules, zero timeouts, zero crashes and two harness errors**. This adds 204
passes with no previously passing case lost and unchanged runtime hashes. The
complete required-mode ES5 run passes all **11,540 cases**. Reports are
`artifacts/es6/string-additions-full.json` and
`artifacts/es6/string-additions-es5.json`.

All four macOS arm64 engines rebuilt and passed package/relocated runtime checks,
including Suite Composer/ChatZilla, 169 Browser navigation/layout assertions,
Calendar's eight unit suites/four views and standalone ChatZilla in XULRunner.
Other operating systems and architectures have not been revalidated for this
batch; complete ES2015 compliance remains unfinished.


## Unicode normalization

`String.prototype.normalize` implements NFC, NFD, NFKC and NFKD using private,
checksum-pinned Unicode 18.0.0 tables. It preserves lone UTF-16 surrogates,
canonical ordering/composition exclusions and algorithmic Hangul behavior.
The existing XPCOM normalizer and historical casing/identifier tables are not
changed. See [data provenance and regeneration](../../src/unicode/README.md).
Unicode License V3 is retained in source and both application license pages.

`normalization.js` passes **50 checks**, including receiver/form coercion,
exceptions, GC/reentrancy, canonical ordering, composition and supplementary
compatibility mappings. The Test262 normalization subset passes **22/26
execution cases**; the four remaining failures require Symbol support.

```sh
python3 js/tests/es6/test-normalization.py \
  --data /path/to/ucd-18.0.0/NormalizationTest.txt \
  --shell obj-zoolrunner-macos-arm64-suite/dist/bin/xpcshell \
  --report artifacts/es6/normalization-unicode.json
```

The full Unicode check passes **4,791,252 assertions** on macOS arm64: all
20,171 rows and identity checks for the 1,096,958 code points outside Part 1.
The runner transports strings as ASCII source escapes, preserving UTF-16 code
units through the historical loader. It checks the upstream data checksum and
requires the final completion marker, not just the shell exit status.

The same 4,791,252 assertions pass in `TestNormalizationKernel.c` when compiling
the actual normalization kernel with ASan and UBSan against the native engine.
This instruments the new kernel and diagnostic, not the complete engine:

```sh
xcrun clang -arch arm64 -isysroot "$ZR_MACOS_SDK" -std=gnu89 -O1 -g \
  -DXP_UNIX -DJS_THREADSAFE -DMOZILLA_1_8_BRANCH \
  -Ijs/src -Iobj-zoolrunner-macos-arm64-suite/js/src \
  -Iobj-zoolrunner-macos-arm64-suite/dist/include/nspr \
  -fsanitize=address,undefined -fno-sanitize-recover=all \
  js/tests/es6/TestNormalizationKernel.c js/src/jsnormalization.c \
  -Lobj-zoolrunner-macos-arm64-suite/dist/bin -lmozjs -o /tmp/zool-normalization
DYLD_LIBRARY_PATH="$PWD/obj-zoolrunner-macos-arm64-suite/dist/bin" \
  /tmp/zool-normalization /path/to/ucd-18.0.0/NormalizationTest.txt
```

Use SDK 11.3 and a matching native arm64 object directory for that host command.
The full ES2015 run records **23,250 passes, 5,316 failures, 14 unsupported
modules, zero timeouts, zero crashes and two harness errors**, with unchanged
runtime hashes. It adds 18 passes without losing any previously passing case.
The required-mode ES5 run passes all **11,540 cases**. Reports are
`artifacts/es6/normalization-full.json` and
`artifacts/es6/normalization-es5.json`.

All four macOS arm64 applications rebuilt and passed package/relocated desktop
checks, including Suite Composer/ChatZilla, 169 Browser navigation/layout
assertions, Calendar's eight unit suites/four views and standalone ChatZilla
in XULRunner. Each packaged chrome archive includes the Unicode license notice.
Other operating systems and architectures have not been revalidated for this
batch; complete ES2015 compliance remains unfinished.


## Array-like operations

`Array.prototype.find`, `findIndex`, `fill` and `copyWithin` use ES2015 ToLength
(up to 2^53-1) and generic object receivers. Find visits holes and captures the
length before callbacks; copyWithin checks property presence, preserves holes
and handles overlap in the appropriate direction. Fill/copyWithin reject failed
native property writes and undeletable targets independently of caller strictness.
Setter callbacks retain their own strictness and historical embedding hooks
retain their existing dispatch. Existing Array methods retain their length policy.
Symbol-based unscopables and the other Array additions remain unfinished.

`array-operations.js` passes 89 focused checks on macOS arm64, including large
indices, coercion/access order, inherited properties, sparse arrays, frozen and
nonextensible objects, exceptions, reentrancy, GC and setter strictness isolation.
The same methods are included in the modern-edition XUL/content window probe.
The Array Test262 subset passes 4,609/4,968 execution cases, gaining 124 passes
without losing a previously passing case. For the four new methods, the only
remaining subset failures require Symbol or Proxy. The full ES2015 run records
**23,374 passes, 5,192 failures, 14 unsupported modules, zero timeouts, zero
crashes and two harness errors**, gaining 124 passes with no losses and unchanged
runtime hashes. Reports are `artifacts/es6/array-operations-full.json` and
`artifacts/es6/array-operations-es5.json`. The complete required-mode ES5 run
passes all 11,540 cases.
All four macOS arm64 applications rebuilt and passed package/relocated desktop
checks, including Suite Composer/ChatZilla, 169 Browser navigation/layout
assertions, Calendar's eight unit suites/four views and standalone ChatZilla
in XULRunner. Other operating systems and architectures remain unvalidated for
this batch.


## Object additions and edition-specific reflection

`Object.is` uses SameValue, including NaN equality and distinct signed zeros.
`Object.assign` snapshots each source's own keys when that source is reached,
rechecks ownership/enumerability before each read, and uses throwing writes with
ordinary setter dispatch. It preserves object identity without invoking valueOf.
Native key order follows ES2015 integer indices through 2^53-1, followed by other
string keys in creation order. Symbol keys and Proxy traps remain unfinished.

ES2015 scripts box primitive inputs to getPrototypeOf, getOwnPropertyDescriptor,
getOwnPropertyNames and keys. Integrity mutators return primitive inputs unchanged;
isExtensible returns false and isFrozen/isSealed return true for primitives.
ES5 and explicitly legacy scripts retain their primitive-argument TypeErrors and
own-name order. Modern own-name reflection sorts integer indices and modern
hasOwnProperty no longer treats inherited ordinary Object shared fields as own.

Object's static methods now receive the internal non-constructor flag through
JS_InitClass's constructor reference. The public eight-bit JSFunctionSpec field
could not hold that flag. No public structure was widened, and initialization
does not read prototype.constructor during DOM bootstrap.

`object-additions.js` passes **145 checks** on macOS arm64, covering method
metadata, numeric/object identity, boxed primitives, property order, live
property changes, exceptions, setter dispatch, GC, reentrancy and legacy edition
behavior. The new methods/reflection behavior also appear in the modern XUL and
content-window fixture.

The full ES2015 run records **23,520 passes, 5,046 failures, 14 unsupported
modules, zero timeouts, zero crashes and two harness errors**. This adds 146
passes without losing any previous pass, with unchanged runtime hashes. The
Object subset accounts for 5,890/5,984 passes. Required-mode ES5 passes all
11,540 cases. Reports are `artifacts/es6/object-additions-full.json` and
`artifacts/es6/object-additions-es5.json`.

All four macOS arm64 applications rebuilt and passed package/relocated desktop
checks, including Suite Composer/ChatZilla, 169 Browser navigation/layout
assertions, Calendar's eight unit suites/four views and standalone ChatZilla
in XULRunner. Other operating systems and architectures remain unvalidated for
this batch, and complete ES2015 conformance remains unfinished.
