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
The runner drains the engine's pending jobs after test evaluation, before
checking `$DONE`. A job exception cannot satisfy a synchronous negative test.
Async tests that still do not complete are reported as unsupported. Promise and
application checkpoints are covered in the implementation notes below. These
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

At that initial baseline, remaining work included well-known Symbol protocols; lexical scopes
and TDZ; ES2015 functions, destructuring, classes and `super`; iteration consumers, syntax and
generators; collections and typed arrays; standard-library and regexp changes;
Promise jobs; and module compilation, linking and evaluation. Later sections
record subsequent implementations and remaining failures. Preserve XDR and
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


## Prototype mutation

`Object.setPrototypeOf` validates arguments, rejects cycles and changes to
nonextensible objects, permits unchanged prototypes and returns primitive
targets unchanged after validating the requested prototype. It uses the existing
inner/outer-object boundary and embedding access checks, without invoking a user
property named __proto__. A callback's in/out value has a separate root from the
requested prototype. The trusted JS_SetPrototype API keeps its historical policy.

The classic engine shares certain own String/RegExp/Function fields with their
class prototypes. The new operation materializes equivalent native descriptors
before detaching such prototypes, preserving private-data getters/setters and
short ids rather than copying mutable state such as RegExp.lastIndex into a
stale data property. Property caches continue to track prototype changes.

`prototype-mutation.js` passes **100 checks** and `TestPrototypeMutation.c`
passes **15 native embedding checks** on macOS arm64. They cover cycles,
nonextensibility, cached lookups, private fields, GC, access denial, callback
value replacement/reentrancy and the unchanged trusted native API. The pinned
setPrototypeOf subset passes **14/20 execution cases**; the six remaining cases
require Symbol or Proxy. The modern XUL/content fixture and packaged native
embedding checks include this operation.

The complete pinned ES2015 run passes **23,528** cases, with **5,038 failures**,
14 unsupported module cases and two harness errors; there are no timeouts or
crashes. This adds eight passes without losing any previous passes. Runtime
hashes were unchanged throughout the run. All **11,540 required ES5 cases**
pass. Reports are `artifacts/es6/prototype-mutation-full.json` and
`artifacts/es6/prototype-mutation-es5.json`.

All four macOS arm64 applications build, package and pass the packaged shell,
embedding and desktop checks. Additional checks pass for Calendar's eight unit
suites and all four views, Browser's 169 navigation/layout assertions, Suite
Composer/ChatZilla and standalone XULRunner ChatZilla. These results do not
establish validation on other operating systems or architectures.


### Realm intrinsics and literal construction

Modern array/object literals use an intrinsic-constructor bytecode instead of
looking up mutable `Array`/`Object` bindings. Explicit constructor calls still
use the application's binding. Default and explicitly selected legacy scripts
retain their historical literal lookup. Bytecode cache version **36** invalidates
older cached scripts; decompilation and cross-edition XDR tests cover literals.

A private runtime table retains the first built-in constructors for each global,
including classic embedding globals with no reserved JSProto slots. Its global
keys are weak: constructors are traced only through a reachable global, and
entries disappear before dead globals are finalized. `JS_ClearScope` resets the
cache. Reinitializing a deleted global property does not replace a modern realm's
original intrinsic. Public JSClass layouts and existing reserved slots are
unchanged. This is groundwork for ES2015 intrinsic handling, not complete realm
or ES2015 conformance.

Focused shell checks cover shadowed/deleted globals, hostile global getters,
boxing, collection, decompilation and legacy-to-modern calls. Native checks
cover separate globals, rooted and unreachable realm lifetimes and scope reset.
The packaged checks include both probes and mixed-edition XUL/content windows.
All 35 shell and 19 native embedding checks pass; the cross-edition XDR probe
passes its 13 checks with bytecode version 36.

The complete pinned ES2015 run remains **23,528 passes, 5,038 failures, 14
unsupported module cases and two harness errors**, with zero timeouts/crashes.
No previously passing cases regress, and runtime hashes remain unchanged.
All **11,540 required ES5 cases** pass. Reports are
`artifacts/es6/realm-intrinsics-full.json` and `realm-intrinsics-es5.json`.

All four macOS arm64 applications build, package and pass shell, embedding and
desktop checks. Additional checks pass for Calendar's eight unit suites and four
views, Browser's 169 navigation/layout assertions, Suite Composer/ChatZilla and
standalone XULRunner ChatZilla. Other platforms have not been revalidated for
this batch.


### Array.of

`Array.of` constructs with a numeric argument count when its receiver has
[[Construct]], including bound constructors. Otherwise it creates an intrinsic
array in the built-in's defining global, even when called from a legacy script
or another embedding global. Own index descriptors bypass inherited setters;
nonconfigurable properties and nonextensibility reject writes. The final length
assignment uses ordinary setter dispatch with throwing failure semantics.

`array-of.js` passes 50 checks and `TestArrayOf.c` passes 15 cross-global native
embedding checks on macOS arm64. The pinned method subset passes 26/28 cases;
the remaining two need Proxy support.

The complete pinned ES2015 run passes **23,550 cases**, with **5,016 failures**,
14 unsupported module cases and two harness errors. There are no timeouts or
crashes, and runtime hashes remain unchanged. This adds 22 passes with no lost
passes. All **11,540 required ES5 cases** pass. Reports are
`artifacts/es6/array-of-full.json` and `array-of-es5.json`.

All four macOS arm64 applications build, package and pass the shell, native
embedding and desktop checks. Calendar's eight unit suites and four views,
Browser's 169 navigation/layout assertions, Suite Composer/ChatZilla and
standalone XULRunner ChatZilla pass. These are macOS arm64 results; other
platforms have not been revalidated for this batch.


### Symbol primitives

The engine now represents Symbol primitives independently of strings while
preserving existing jsval tags and public JSClass layouts. An unused private
string flag identifies a Symbol payload whose owned UTF-16 display buffer also
contains its description. There are no hidden GC children. A private registry
indexes slices of registered Symbols' owned buffers; it and the eleven ES2015
well-known identities persist for the lifetime of the JSRuntime, including
intervals without contexts. Runtime teardown frees these retained buffers.

`JS_IsSymbolValue` and appended `JSTYPE_SYMBOL` expose the new type. Existing
type enum values, classic global reserved-slot counts and old value encodings
remain unchanged. Older native code viewing the string tag gets bounded display
text from string accessors; dependent-string and concatenation APIs produce
ordinary strings. Language ToString/ToNumber conversions still throw where
required. Symbols cannot be serialized through XDR.

Primitive conversion, boxing, identity comparisons, symbol property keys,
descriptors, own-symbol reflection, assign, enumeration, JSON omission and
@@toPrimitive/@@toStringTag are implemented. Symbol-valued descriptors stay
rooted through callbacks. Additional well-known protocols such as iteration,
matching, species and instanceof still require work; named well-known symbols
alone do not establish their protocols or full ES2015 conformance.

Current macOS arm64 focused validation passes 92 shell checks and 42 native
embedding checks, including cross-global registries, classic 31-slot globals,
GC and context teardown/recreation. A preliminary Symbol Test262 subset passes
48/52 cases; the remaining cases require species getters and classes. The full
pinned ES2015 run passes **23,781 cases**, with **4,785 failures**, 14 unsupported
module cases and two harness errors. There are no timeouts or crashes, no lost
passes, and 231 additional passes relative to Array.of. Runtime hashes remain
unchanged. All **11,540 required ES5 cases** pass. Reports are
`artifacts/es6/symbol-primitives-full.json` and `symbol-primitives-es5.json`.
The native embedding probe also passes with its executable instrumented by
AddressSanitizer; the engine itself was not instrumented. The focused script
passes with macOS malloc scribbling enabled.

All four macOS arm64 applications build, package and pass shell, native
embedding and desktop checks. Calendar passes its eight unit suites and four
views; Browser passes 169 navigation/layout assertions. Suite Composer and
ChatZilla, and standalone XULRunner ChatZilla pass. Windows, Linux and other
architectures have not been revalidated for this batch.


### Symbol.match classification

String `includes`, `startsWith` and `endsWith` now use ES2015 IsRegExp: an
object's observable `Symbol.match` value controls classification unless it is
undefined, when the native RegExp type supplies the result. Truth testing does
not invoke user conversion. Receiver conversion precedes classification, and
classification precedes search-string and position conversion. This does not
implement RegExp's matching, replacement, searching or splitting protocols.

The focused fixture `string-match-classification.js` passes 39 checks on macOS
arm64, including getter exceptions, inherited classification, opt-out, callback
ordering and GC. The pinned String prototype subset passes 1,524/1,572 cases;
all cases for these three methods pass. The full pinned ES2015 run passes
**23,787 cases**, with **4,779 failures**, 14 unsupported module cases and two
harness errors. This gains six passes without losing any; there are no crashes
or timeouts and runtime hashes remain unchanged. All **11,540 required ES5
cases** pass. Reports are `artifacts/es6/match-classification-full.json` and
`match-classification-es5.json`.

All four macOS arm64 applications build, package and pass shell, embedding and
desktop checks. Calendar's eight unit suites and four views, Browser's 169
navigation/layout assertions, Suite Composer/ChatZilla and standalone XULRunner
ChatZilla pass. Other architectures and operating systems have not been
revalidated for this batch.


### Fresh modern RegExp literals

The explicitly selected ES2015 edition emits a dedicated RegExp literal
bytecode. Each evaluation creates a new object with independent identity,
properties and lastIndex while sharing the compiled pattern. Global loops,
function calls and eval follow this rule; explicitly selected legacy editions
retain their historical literal identity. The interpreter and decompiler
handle both ordinary and extended atom operands. Bytecode cache version **37**
invalidates older caches. Clone initialization roots the new object and takes
its pattern reference before later initialization can fail.

`regexp-literals.js` passes 33 focused checks on macOS arm64. The separate
`regexp-literals-wide.js` covers execution and decompilation beyond 65,535
atoms. The edition/XDR probe includes fresh literal state and legacy identity
in its 13 checks; the realm probe passes 22 checks, including RegExp prototypes
across globals and context teardown. The pinned regexp-literal subset passes
116/124 cases; the eight remaining cases require Unicode regexp behavior.
The complete pinned ES2015 run retains **23,787 passes** and **4,779 failures**,
with 14 unsupported module cases and two harness errors. There are no lost
passes, crashes or timeouts; runtime hashes remain unchanged. All **11,540
required ES5 cases** pass. Reports are `artifacts/es6/regexp-literals-full.json`
and `regexp-literals-es5.json`. The focused tests cover the identity gap that
the pinned corpus did not expose.

All four macOS arm64 applications build, package and pass shell, native
embedding and desktop checks. Calendar's eight unit suites and four views,
Browser's 169 navigation/layout assertions, Suite Composer/ChatZilla and
standalone XULRunner ChatZilla pass. Other platforms have not been revalidated
for this batch.


### Symbol.hasInstance

Modern `instanceof` evaluates custom Symbol.hasInstance hooks, propagates getter
and call exceptions, and converts the result to Boolean. Function.prototype's
non-constructible hook implements OrdinaryHasInstance, including primitive
short-circuiting and bound-target delegation. Native state remains rooted
through callbacks. Explicit legacy script modes and the public JS_HasInstance
API keep the historical native class dispatch.

The focused fixture passes 34 checks on macOS arm64. All 75 pinned instanceof
operator cases pass, and 22/24 Function.prototype Symbol.hasInstance cases pass;
the remaining two require Proxy. TestHasInstanceEmbedding passes 13 checks for
native class hooks, version transitions, custom modern hooks and GC.

The full pinned ES2015 run passes **23,811 cases**, with **4,755 failures**,
14 unsupported module cases and two harness errors. This gains 24 passes
without losing any; there are no crashes or timeouts and runtime hashes remain
unchanged. All **11,540 required ES5 cases** pass. Reports are
`artifacts/es6/hasinstance-full.json` and `hasinstance-es5.json`.

All four macOS arm64 applications build, package and pass shell, native
embedding and desktop checks. Calendar's eight unit suites and four views,
Browser's 169 navigation/layout assertions, Suite Composer/ChatZilla and
standalone XULRunner ChatZilla pass. Other platforms have not been revalidated
for this batch.


### Built-in Symbol.toStringTag properties

Math and JSON now define their standard non-writable, non-enumerable,
configurable Symbol.toStringTag properties. Explicit ES2015 Object.prototype
toString uses the specification's default tags when a custom tag is absent or
non-string, including Object for Math, JSON and Symbol wrappers whose tags were
removed. Legacy script modes retain historical native JSClass names. Native
callability is inspected without invoking the object.

`builtin-tags.js` passes 27 focused checks on macOS arm64, including tag
deletion, replacement, Unicode getters and collection. TestBuiltinTags passes
13 native checks, including ordinary/callable native objects and legacy edition
transitions. All 40 pinned Object.prototype.toString cases pass; the separate
Symbol.toStringTag filename subset passes 8/40, with the remaining cases
requiring other unimplemented built-ins.

The full pinned ES2015 run passes **23,815 cases**, with **4,751 failures**,
14 unsupported module cases and two harness errors. This gains four passes
without losing any; there are no crashes or timeouts and runtime hashes remain
unchanged. All **11,540 required ES5 cases** pass. Reports are
`artifacts/es6/builtin-tags-full.json` and `builtin-tags-es5.json`.

All four macOS arm64 applications build, package and pass shell, native
embedding and desktop checks. Calendar's eight unit suites and four views,
Browser's 169 navigation/layout assertions, Suite Composer/ChatZilla and
standalone XULRunner ChatZilla pass. Other platforms have not been revalidated
for this batch.


### Array and String iterators

Array keys/values/entries and Symbol.iterator, String Symbol.iterator, and
modern arguments' own Symbol.iterator are implemented. Array iteration reads
live lengths and visits holes; String iteration combines valid UTF-16 surrogate
pairs. Iterator state is private, exhaustion is permanent, and results and entry
arrays use the executing built-in's realm. Private weak realm caches retain
intrinsic prototypes and the original values function without expanding public
global reserved slots. Classic Iterator/StopIteration and legacy arguments
objects retain their existing behavior.

`modern-iterators.js` passes 62 focused checks and `TestModernIterators.c` passes
23 native embedding checks on macOS arm64, including callbacks, collection,
borrowed methods, context teardown, realm collection and scope clearing.
The String Symbol.iterator subset passes 10/10. The iterator-prototype subset
passes 40/94; its remaining cases require Map, Set or typed arrays.

The full pinned ES2015 run passes **23,909 cases**, with **4,657 failures**,
14 unsupported module cases and two harness errors. This gains 94 passes
without losing any; there are no crashes or timeouts and runtime hashes remain
unchanged. All **11,540 required ES5 cases** pass. Reports are
`artifacts/es6/iterators-full.json` and `iterators-es5.json`. An earlier Array
prototype diagnostic overlapped compilation and is invalid; use the complete
run for results.

All four macOS arm64 applications build, package and pass shell, native
embedding and desktop checks. Calendar's eight unit suites and four views,
Browser's 169 navigation/layout assertions, Suite Composer/ChatZilla and
standalone XULRunner ChatZilla pass. Other platforms have not been revalidated
for this batch. This does not implement for-of, spread, generators or iterable
consumers such as Array.from.


### Array.from

Array.from supports iterable and array-like inputs, generic construction,
mapping with raw receivers, own indexed data properties and throwing length
writes. Its fallback arrays use the built-in's defining realm. IteratorClose
follows the ES2015 edition: return-getter failures replace the original throw;
after successful method lookup the original throw takes precedence over the
return call's outcome. Failures in next, done or value do not close the iterator.
Callback state and index atoms remain rooted through collection.

On macOS arm64, 36 focused checks and 17 native realm checks pass. The pinned
Array.from subset passes 72/78; the remaining cases require ArrayBuffer or
computed method syntax. The full pinned ES2015 run passes **23,975 cases**,
with **4,591 failures**, 14 unsupported module cases and two harness errors.
This gains 66 passes without losing any; there are no crashes or timeouts and
runtime hashes remain unchanged. All **11,540 required ES5 cases** pass.
Reports are `artifacts/es6/array-from-full.json` and `array-from-es5.json`.

All four macOS arm64 applications build, package and pass shell, native
embedding and desktop checks. Calendar's eight unit suites and four views,
Browser's 169 navigation/layout assertions, Suite Composer/ChatZilla and
standalone XULRunner ChatZilla pass. Other platforms have not been revalidated
for this batch.


### Symbol.unscopables

Modern syntactic with environments consult Symbol.unscopables after finding a
property. Array.prototype provides the seven ES2015 exclusions in a null-prototype
table. Legacy scripts and ordinary embedding/global scopes retain their prior
lookup policy. Callback state and identifier atoms remain rooted; native
property handles are released before getters run. A private binding marker
preserves the lookup decision when a getter deletes the property. Reads then
use object operations, and implicit method calls receive the binding object.

On macOS arm64, 37 focused checks, 14 native embedding checks, all 14 targeted
cases and all 155 upstream with-statement cases pass. The full pinned ES2015
run passes **23,982 cases**, with **4,584 failures**, 14 unsupported module
cases and two harness errors. This gains seven passes without losing any;
there are no crashes or timeouts and runtime hashes remain unchanged. All
**11,540 required ES5 cases** pass. Reports are
`artifacts/es6/unscopables-full.json` and `unscopables-es5.json`.

All four macOS arm64 applications build, package and pass shell, native
embedding and desktop checks. Calendar's eight unit suites and four views,
Browser's 169 navigation/layout assertions, Suite Composer/ChatZilla and
packaged standalone XULRunner ChatZilla pass. Other platforms have not been
revalidated for this batch.


### Modern object-literal properties

Explicit ES2015 code supports computed data properties, ordinary concise
methods, computed getters/setters and shorthand identifier properties. Computed
keys are converted before evaluating values. Anonymous function names belong
to each closure, including Symbol-derived names and the distinction between
absent and empty Symbol descriptions. Methods/accessors are non-constructible;
simple method parameter names are checked for duplicates. The historical accessor embedding access
check still runs, with identifier atoms rooted through callback collection.
Computed and shorthand __proto__ fields create own data properties. Legacy
script modes keep their existing grammar.

The new bytecodes use cache version **38**, with matching script decompilation
and XDR coverage. On macOS arm64, 58 focused checks, two extended-atom checks
and 14 edition/embedding checks pass. The object-expression subset passes
142/208; the broader computed-name subset passes 20/90, retaining class,
generator and other missing-feature cases. Array.from improves to 76/78; its
remaining two cases require ArrayBuffer. These subsets are diagnostics.

The complete pinned ES2015 run passes **24,063 cases**, with **4,503 failures**,
14 unsupported module cases and two Float64Array harness errors. There are no
crashes or timeouts, the runtime hash stays unchanged, and 81 cases newly pass
without losing any previously passing case. All **11,540 ES5 cases** pass.
The scanner recognizes the ES2015 arrow punctuator separately from assignment;
arrow-function parsing remains unimplemented. This preserves SyntaxError for
invalid arrow bindings now that shorthand properties parse successfully.

All four macOS arm64 applications compile and pass package shell/native checks
and relocated desktop checks, including real window globals and classic scripts.
Calendar passes eight unit suites and all four views; Browser passes 169
navigation/layout checks; Suite passes Composer lifecycle and ChatZilla checks,
and packaged standalone XULRunner passes ChatZilla initialization/input checks.
Other platforms have not been revalidated. Computed destructuring, generator
methods, super and remaining parameter grammar still require work.

### Map and Set

Native Map and Set now use an ordered SameValueZero hash table with tombstones
for live iteration. Methods, iterable constructors, size accessors, iterator
aliases, tags, species and iterator cleanup are implemented. Clear/delete/reinsert
and additions during forEach or next retain iteration order. Shared native
storage ownership protects either finalizer order; iterators also trace their
collection objects. Native operations release object locks before callbacks,
error reporting or GC allocation. The collection classes append prototype keys
without increasing the classic global reserved-slot requirement.

The storage regression `TestCollectionTable.c` currently passes 229,123 checks,
including 50,000 deterministic randomized operations compared with an independent
ordered-array model. A separate ASan/UBSan build of the storage and test passes;
this is not whole-engine instrumentation. That build disables shift-base checks
for the historical signed `INT_TO_JSVAL` tagging macro. The focused JavaScript
fixture passes 53 checks, and the native cross-global/lifetime probe passes 26.
The pinned Map subset passes 257/277; its remaining cases require WeakMap.
The Set subset passes 352/368, retaining missing WeakSet and arrow-function
coverage. These subsets do not replace the full suite.

The complete pinned ES2015 run passes **24,708 cases**, with **3,858 failures**,
14 unsupported module cases and two Float64Array harness errors. All 645 newly
passing cases are gains; no previously passing case regresses. The runtime hash
stays unchanged, with no crashes or timeouts. All **11,540 ES5 cases** pass.
All four macOS arm64 applications compile, package and pass relocated shell,
native embedding and desktop checks, including Map/Set in chrome and content
window globals. Calendar passes eight unit suites and four views; Browser passes
169 navigation/layout checks; Suite passes Composer and ChatZilla checks; packaged
standalone XULRunner passes ChatZilla initialization/input. Other platforms have
not been revalidated for this batch.

### Weak collections

WeakMap and WeakSet have native object-only keys, iterable constructors and
standard methods/tags, without exposing enumeration. Their tables do not trace
keys or values during ordinary marking. The collector computes ephemeron
reachability to a fixed point: a reachable owner and key may retain a value,
and that value may expose another key or owner. Dead keys are removed before
sweeping. Unreachable value-to-key cycles cannot keep themselves alive.

Weak closure runs before legacy generator-close discovery, after its marking,
and around the embedding mark callback. A host's mark call during MARK_END
returns with weak reachability complete, preserving the classic immediate
finalization guarantee. The native regression observes actual finalizer counts
for key/value cycles, dead owners, transitive weak chains, callback-only roots,
classic generators and destroyed contexts.

On macOS arm64, 60 focused checks, 57 native GC/embedding checks, all 174 pinned
WeakMap cases and all 148 WeakSet cases pass. The focused/native checks also
pass with malloc scribbling enabled; this is not full-engine sanitizer coverage.
The complete pinned ES2015 run passes **25,064 cases**, with **3,502 failures**,
14 unsupported module cases and two Float64Array harness errors. All 356 new
passes are gains, with no lost passes, crashes or timeouts; the runtime hash
stays unchanged. All **11,540 ES5 cases** pass.

All four macOS arm64 applications compile and pass packaged shell/native and
relocated desktop checks, including weak collections in chrome/content globals.
Calendar passes eight unit suites and all four views; Browser passes 169
navigation/layout checks; Suite passes Composer lifecycle and ChatZilla checks;
packaged standalone XULRunner passes ChatZilla initialization/input. Other
platforms have not been revalidated for this batch.


### Reflect

The native ES2015 Reflect namespace implements all 14 operations, including
`enumerate` from the 2015 edition. Property operations preserve raw accessor
receivers and return false for ordinary descriptor, extensibility and deletion
rejections; user exceptions still propagate. Construction supports alternate
`newTarget` prototypes, bound constructors and intrinsic fallback from the
new target's realm without replacing the historical embedding APIs.

The focused script currently passes 53 checks. `TestReflect.c` passes 43 native
checks covering foreign globals after their context is destroyed, intrinsic
fallback, legacy callers, native setter hooks with collection during callbacks,
enumerator reentry, security callback rejection, namespace deletion and cache
reset. Before the Proxy implementation, the pinned Reflect subset passed
266 of 288 cases; all 22 remaining cases depended on Proxy.
The complete pinned ES2015 run passes **25,330 cases**, with **3,236 failures**,
14 unsupported module cases and two harness errors. This adds 266 passes and
loses none relative to the weak-collection baseline. The runtime hashes remained
unchanged during the run, with no crashes or timeouts. All **11,540 ES5.1 cases**
pass. All four macOS arm64 applications compile and pass packaged shell/native
and relocated desktop checks, including Reflect in chrome/content globals.
Calendar passes eight unit suites and all four views; Browser passes 169
navigation/layout checks; Suite passes all 24 lifecycle checks and ChatZilla;
packaged standalone XULRunner passes ChatZilla initialization and input checks.
Other platforms have not been revalidated for this batch.

Nonstandard embedding object operations retain their classic
hooks; a distinct receiver cannot be forwarded through a historical get hook
that has no receiver parameter. The Reflect batch alone did not establish
Proxy support or complete host-object receiver semantics.


### Proxy

The native Proxy implementation adds forwarding and traps for property reads,
writes, own descriptors, definition/deletion, keys, prototype/extensibility,
call/construction and the ES2015 `enumerate` operation. Trap results are checked
against target invariants, including non-configurable properties and
non-extensible targets. Property receivers and alternate construction targets
are retained. Callable/constructible capabilities survive revocation, while
operations on revoked proxies throw. Captured target/handler roots survive
revocation and collection inside trap getters; revocation releases their strong
references. Revoker closures also retain their state when cloned through JSAPI.

The public JSClass/JSObjectOps layouts are unchanged. Private dispatch prevents
native scope/property-cache paths from interpreting Proxy property handles as
native properties. Historical objects retain their existing embedding hooks.
Proxy array recognition is shared by Array.isArray, JSON, concat, Object tags
and JS_IsArrayObject. Object integrity/assignment and own-property queries
use the corresponding traps, and inherited enumeration delegates to the
prototype Proxy without calling its `has` trap.

The focused script passes 65 checks; TestProxy.c passes 55 checks, including
foreign realms after context destruction, cloned revokers, GC during callbacks,
finalization of target/handler pairs, 1,000 opaque JSAPI lookups without retained
roots, and intrinsic-cache reset. The final pinned run passes all 398 Proxy
cases and all 288 Reflect cases. It records **25,767 passes**, **2,799 failures**,
14 unsupported module cases and two harness errors: **437 gained, zero lost**
relative to Reflect. Runtime hashes remained unchanged, with no crashes or
timeouts. All **11,540 required-mode ES5.1 cases** pass. Conformance ran on the
validated XULRunner shell with America/Los_Angeles as the test timezone.

All four macOS arm64 applications pass root compilation, package shell/native
checks and relocated desktop checks with SDK 11.3. Calendar passes eight unit
suites and all four views; Browser passes 169 navigation/layout assertions;
Suite passes its 24 lifecycle assertions and ChatZilla; standalone packaged
XULRunner initializes ChatZilla and its input. No other platform or architecture
has been revalidated for this batch.


Native C getter/setter hooks installed directly on a Proxy through JSAPI are
not represented by ES property descriptors and currently reject that operation;
existing native objects are unchanged. The historical native `__proto__`
accessor needed receiver adaptation when reached through a Proxy in this batch;
the Annex B follow-up below supplies modern accessors while retaining legacy
hooks. Reflect.getPrototypeOf and Object.getPrototypeOf use the Proxy trap directly.
These results do not establish full ES6
conformance or all host-object wrapping semantics.


### Annex B built-ins

Globals explicitly initialized in ES2015 mode now expose `__proto__` as a
configurable, non-enumerable accessor. Its getter/setter preserve raw receivers,
Proxy traps, rejection exceptions, foreign-realm primitive prototypes and
classic embedding access checks. Legacy-initialized globals retain the old
short-id property hooks. Script edition selection alone does not recreate a
window's pre-existing built-in prototypes.

In ES2015 code, the four attribute-bearing String HTML helpers convert their
receiver before the attribute and replace attribute quotes with `&quot;`.
Legacy editions retain their previous order and quoting. The implementation
checks the expanded allocation length and roots converted strings through
callbacks and GC. Modern globals also retain deletion of escape/unescape and
related String helpers, including when accessed from legacy code; a private
per-global initialization marker preserves the legacy lazy-resolution behavior
in legacy globals. JS_ClearScope starts a fresh cache lifetime.

Focused checks currently pass: 32 prototype, 27 HTML, 25 global-binding and 28
native embedding checks. The Annex B B.2 subset passes all 52 cases. Native
coverage includes host access denial, callbacks replacing in/out values and
collecting, JSAPI-cloned accessors, foreign contexts destroyed before use,
standard-class enumeration after deletion, and reset to a legacy global.
The complete pinned ES2015 run passes **25,783 cases**, with **2,783 failures**,
14 unsupported module cases and two harness errors: **16 gained, zero lost**
relative to Proxy. The runtime remained unchanged and had no crashes/timeouts.
All **11,540 required-mode ES5.1 cases** pass (America/Los_Angeles).

All four macOS arm64 / SDK 11.3 root builds, package checks and relocated desktop
checks pass. Calendar passes eight unit suites and all four views; Browser
passes 169 navigation/layout assertions; Suite passes all 24 lifecycle assertions
and ChatZilla; standalone packaged XULRunner passes ChatZilla initialization and
input checks. The unchanged Object additions regression also passes all 145
checks, including legacy virtual __proto__ ownership in a modern global.
Windows, Linux and other architectures have not been revalidated for this batch.
Full ES6 compliance remains incomplete.

### RegExp prototype fields

ES2015-initialized globals use an ordinary RegExp prototype and configurable,
non-enumerable accessors for source, global, ignoreCase and multiline. Instances
retain their internal matcher and own writable, non-enumerable, non-configurable
lastIndex. The source getter escapes slashes and line terminators for literal
round trips and reports the empty pattern as `(?:)`. Generic flags reads all five
ES2015 flag properties in order; generic toString converts source before flags.
The ordinary prototype has no matcher slots: ES2015 accessors reject it with
TypeError. The special prototype values found in later editions do not apply.
This does not implement Unicode/sticky matching or the remaining RegExp symbol
protocols and constructor semantics. Script edition selection alone does not
reinitialize a legacy window's built-in prototypes.

Legacy-initialized globals retain callable/matcher-bearing RegExp prototypes
and virtual own fields. The prototype-mutation regression now explicitly checks
ES2015 inherited accessors; its original own-field expectation is retained in
the native legacy-global test. Native construction supplies the modern
prototype's parent when differing private-slot layouts prevent map sharing.
XDR decodes RegExp literals using the instance class, independently of the
modern prototype's ordinary-object class. Decoding failure leaves installed
matcher state owned by its object; a debugger allocation hook forces the
lastIndex failure path and GC checks cleanup under MallocScribble. Inherited
read-only lastIndex properties cannot block modern own-field creation. Serialized bytes and bytecodes are
unchanged. Function/script decompilation continues to read internal patterns
rather than mutable public source/flags properties.

Focused coverage has 66 script checks and 45 native embedding checks, including
GC/reentrancy, cloned accessors, foreign contexts destroyed before use, native
UTF-16 construction, cached-script decoding under a legacy context edition, and
reset to a legacy global, and lazy class initialization by a modern script in
a legacy global, plus RegExp as the first lazily resolved class. RegExp initialization and stringification follow the global
policy even when the triggering caller has another edition. These checks are included in macOS packaging. The
RegExp slice of the complete pinned run passes 1,120 of 1,546 cases.

The complete pinned ES2015 run passes **25,839 cases**, with **2,727 failures**,
14 unsupported module cases and two harness errors: **56 gained, zero lost**
relative to Annex B. The frozen runtime remained unchanged; no crashes or
timeouts occurred. All **11,540 required-mode ES5.1 cases** pass in
America/Los_Angeles. Reports are `artifacts/es6/regexp-fields-full.json` and
`regexp-fields-es5.json`; the final snapshot is
`/tmp/zr-regexp-fields-conformance-final3-20260918`.

All four macOS arm64 / SDK 11.3 applications pass root builds, final package
checks and relocated desktop checks. Calendar passes eight unit suites and all
four views. Browser passes 169 navigation/layout checks. Suite passes 24
lifecycle assertions and ChatZilla; standalone packaged XULRunner passes
ChatZilla initialization and input. Final desktop reports are under
`artifacts/es6/regexp-fields-runtime-final3`. Earlier canceled conformance runs
and superseded package/desktop diagnostics are not completion evidence.
Windows, Linux and other architectures have not been revalidated for this batch.
Full ES6 compliance remains incomplete.

The implementation follows [the original ES2015 RegExp specification](https://262.ecma-international.org/6.0/#sec-properties-of-the-regexp-prototype-object),
including its rejection of the ordinary prototype by source/flag accessors.

### RegExp match/search protocols

Modern globals now provide generic RegExp test, Symbol.match and Symbol.search,
and String match/search dispatch through symbol methods. These operations keep
raw receivers and observable getter/conversion order, invoke overridden exec,
reject primitive exec results, and preserve user exceptions. String fallback
uses the defining realm's cached RegExp constructor. Legacy globals retain their
original String/RegExp methods and ignore exec overrides as before.

The original ES2015 RegExpBuiltinExec reads lastIndex, global and sticky through
ordinary properties. It clamps lastIndex with ToLength, snapshots matcher state
after callbacks, and checks index writes, including failed nonglobal matches.
Sticky property overrides anchor the matcher, but the y flag/parser and Unicode
matching remain unimplemented. Global match advances empty matches by code point
when the observable unicode property is true, while preserving code-unit offsets.
Search performs both index writes unconditionally in this edition and does not
restore after an exec exception. Later-edition algorithms differ.

Native result arrays use the executing method's realm; a callable exec may
return an array from its own realm. The native probe covers foreign contexts
destroyed before use, cloned methods, primitive symbol getter/method receivers,
GC, interrupting a native match loop and collection after references are removed.
There are 56 focused script checks and 28 native embedding checks. The diagnostic
RegExp subset passes 1,236/1,546 cases with no timeouts; all 146 String match/search
cases pass. The complete pinned ES2015 run passes **25,969 cases**, with
**2,597 failures**, 14 unsupported module cases and two harness errors:
**130 gained, zero lost** relative to RegExp fields. No crashes or timeouts
occurred and the frozen runtime remained unchanged. All **11,540 required-mode
ES5.1 cases** pass in America/Los_Angeles. Reports are
`artifacts/es6/regexp-protocols-full.json` and `regexp-protocols-es5.json`;
the frozen runtime is `/tmp/zr-regexp-protocols-conformance-20260918`.

All four macOS arm64 / SDK 11.3 root builds, package checks and relocated desktop
checks pass. Calendar passes eight unit suites and all four views. Browser
passes 169 navigation/layout assertions. Suite passes 24 lifecycle assertions
and ChatZilla; standalone packaged XULRunner passes ChatZilla initialization and
input. Final desktop reports are under `artifacts/es6/regexp-protocols-runtime`.
Windows, Linux and other architectures have not been revalidated for this batch.
Constructor details, y/u flag support, replacement/splitting protocols and other
ES6 work remain incomplete.

The prototype-mutation and Reflect focused tests now supply an explicit global
property when replacing a RegExp's prototype, preserving their original
lastIndex assertions while following ES2015's property lookup semantics. The
legacy prototype/field behavior remains covered separately. The upstream
Test262 files and assertions are unchanged. No bytecode or cache-format changes
are needed for these runtime operations.

### RegExp constructors and sticky flags

Modern RegExp construction observes Symbol.match and regexp-like properties
before newTarget.prototype, preserves constructor identity short-circuits,
copies actual matcher state without reading public source/flags overrides,
and accepts explicit flag overrides. A dedicated native constructor delays
allocation until the required observable steps complete. Bound and Proxy
newTarget fallback uses the existing constructor-realm logic. RegExp species
is a configurable accessor returning its raw receiver.

The y flag is accepted by ES2015 literals and modern constructors; legacy
source grammar still rejects it. Native JSAPI callers can request JSREG_STICKY.
Both the initial simple-matcher search and the outer search loop honor
anchoring. Literal decompilation and XDR preserve the flag. Cache version 39
invalidates component caches made before the new RegExp flag semantics. Unicode
matching and the u flag remain unimplemented.

String match/search fallback now calls private RegExpCreate directly. That
operation converts the pattern instead of performing the constructor's
IsRegExp, identity or matcher-copy steps. The distinction prevents a second
symbol lookup and preserves fallback behavior when a RegExp's symbol method
is null or undefined. Constructor and String fallback share initialization
of fresh matcher objects without changing public JSAPI layouts.

Focused checks: 35 script and 36 native embedding checks under MallocScribble.
Native coverage includes JS_NewRegExpObject, the classic construct-with-arguments
API, foreign contexts destroyed before use, bound/Proxy constructor realms,
JSAPI-cloned constructors, legacy callers, sticky XDR decode/execution under
a different context edition, and return to legacy globals. Classic JSAPI
function clones create their own ordinary prototypes; tests use intrinsic
accessors to inspect their matcher state rather than changing that contract.
The diagnostic RegExp subset passes 1,328/1,546 cases. The complete pinned
ES2015 run passes **26,061 cases**, with **2,505 failures**, 14 unsupported
module cases and two harness errors: **92 gained, zero lost** relative to
match/search. No crashes or timeouts occurred; the frozen runtime remained
unchanged. All **11,540 required-mode ES5.1 cases** pass in America/Los_Angeles.
Reports: `artifacts/es6/regexp-constructor-full.json` and
`regexp-constructor-es5.json`. Frozen runtime:
`/tmp/zr-regexp-constructor-conformance-20260918`.

All four macOS arm64 / SDK 11.3 root builds, packages and relocated desktop
checks pass. Calendar passes eight unit suites and all four views. Browser
passes 169 navigation/layout assertions. Suite passes 24 lifecycle assertions
and ChatZilla; standalone packaged XULRunner passes ChatZilla initialization
and input. Desktop reports: `artifacts/es6/regexp-constructor-runtime`.
Windows, Linux and other architectures have not been revalidated for this batch.
Unicode matching, replacement/splitting protocols and further ES6 work remain
incomplete. Modern constructor selection follows the initialized global's
policy; running an ES2015 script alone does not replace legacy window built-ins.

### RegExp and String split protocols

Modern globals dispatch String split through Symbol.split before converting the
receiver. RegExp split resolves the species constructor, appends the sticky flag,
constructs a separate matcher, preserves capture values without string coercion,
and creates arrays in the executing method's realm. String fallback converts
ordinary separators and preserves the legacy limit conversion and empty-piece
behavior. Existing legacy globals retain their original split implementation.

The pinned ES2015 corpus incorporates the [July 2015 limit correction](https://tc39.es/archives/bugzilla/4432/):
split limits use ToUint32, preserving negative-limit compatibility. Match indices
and capture counts still use ToLength. Native loops remain interruptible,
including custom exec results with excessive capture counts. Out-of-range custom
match indices cannot create out-of-bounds dependent strings.

The diagnostic subsets pass all 210 String split cases and 80/84 RegExp split
cases; the four remaining failures require Unicode u matching. There are 33
focused script checks and 27 native embedding checks under MallocScribble,
covering callback GC, constructor order, primitive receivers, foreign contexts
destroyed before use, cloned methods, result realms and legacy behavior.
The full pinned ES2015 run passes **26,143 cases**, with **2,423 failures**,
14 unsupported module cases and two harness errors: **82 gained, zero lost**
relative to constructor/sticky. No crashes/timeouts occurred; the frozen runtime
remained unchanged. All **11,540 required-mode ES5.1 cases** pass in
America/Los_Angeles. Reports: `artifacts/es6/regexp-split-full.json` and
`regexp-split-es5.json`; runtime: `/tmp/zr-regexp-split-conformance-20260918`.

All four macOS arm64 / SDK 11.3 builds, packages and relocated desktop checks
pass on the completed desktop run. Calendar passes eight unit suites and four
views; Browser passes 169 navigation/layout assertions; Suite and XULRunner pass
ChatZilla initialization/input checks. The first Suite run recorded an
intermittent Inspector `mDocPanel` lifecycle error under concurrent conformance
load despite all 24 explicit assertions passing. The unchanged package passed
its rerun; retain `regexp-split-runtime/suite-first-failure` as diagnostic evidence.
Two additional lifecycle runs after conformance each pass all 24 assertions
without console errors. These repeats do not establish that the intermittent
Inspector issue is fixed. No application code or
upstream assertions were changed to bypass the failure. Other operating systems
and architectures have not been revalidated. Unicode matching, replacement
protocols and the remaining ES6 work are still incomplete.

### RegExp and String replacement protocols

Modern RegExp Symbol.replace collects exec results before invoking replacement
callbacks, then observes result length, matched text, index and captures in
order. Capture conversion preserves undefined, callback receivers remain raw,
and overlapping results still invoke callbacks. A checked native UTF-16 output
buffer handles substitutions without quadratic concatenation. Native capture
loops remain interruptible. String replacement dispatches Symbol.replace before
receiver conversion and uses literal string matching for fallback; legacy
globals retain their historical method and `$+` behavior.

Diagnostic subsets pass 106/108 RegExp replacement cases (two Unicode u failures)
and all 86 String replacement cases. There are 34 focused script checks and
27 native embedding checks under MallocScribble, covering callback GC, ordering,
foreign realms, cloned methods, primitive receivers and interrupt recovery.
The full pinned ES2015 run passes **26,249 cases**, with **2,317 failures**,
14 unsupported module cases and two harness errors: **106 gained, zero lost**
relative to split. No crashes/timeouts occurred; the frozen runtime remained
unchanged. All **11,540 required-mode ES5.1 cases** pass in America/Los_Angeles.
Reports: `artifacts/es6/regexp-replace-full.json` and `regexp-replace-es5.json`;
runtime: `/tmp/zr-regexp-replace-conformance-20260918`.

All four macOS arm64 / SDK 11.3 builds, packages and relocated desktop checks
pass. Calendar passes eight unit suites and all four views; Browser passes 169
navigation/layout assertions; Suite passes 24 lifecycle assertions and ChatZilla;
standalone packaged XULRunner passes ChatZilla initialization and input. Desktop
reports: `artifacts/es6/regexp-replace-runtime`. Other operating systems and
architectures have not been revalidated. Unicode RegExp matching and the other
remaining ES6 features are still incomplete.

### Date conversion and prototype

Modern Date globals provide the configurable, non-writable Symbol.toPrimitive
method. Its generic ordinary conversion preserves getter exceptions, calls
methods with zero arguments, accepts primitive results including Symbols, and
rejects invalid hints without coercing them. Removing the hook from a modern
Date selects ordinary conversion rather than the historical hint-argument path.
Modern valueOf ignores extra arguments. Existing legacy globals retain their
hint-sensitive valueOf and NaN-valued Date prototype, even when Date is first
resolved by a modern caller.

Modern Date.prototype retains the private native class layout but has no date
value; instance methods reject it, and its default ES2015 Object tag is Object.
The Date constructor copies real instances' stored values without observable
conversion, and uses the default primitive hint for other objects before
parsing strings or converting numbers. Classic native Date creation/getter APIs
and foreign-context lifetimes are covered.

The complete Date diagnostic subset passes 898/898 cases. There are 29 focused
script and 32 native embedding checks under MallocScribble; C89 checks pass.
The complete pinned ES2015 run passes **26,281 cases**, with **2,285 failures**,
14 unsupported module cases and two harness errors: **32 gained, zero lost**
relative to replacement. No crashes/timeouts occurred; the frozen runtime
remained unchanged. All **11,540 required-mode ES5.1 cases** pass in
America/Los_Angeles. Reports: `artifacts/es6/date-primitive-full.json` and
`date-primitive-es5.json`; runtime: `/tmp/zr-date-primitive-conformance-20260918`.
Its library hash matches the completed Suite build.

All four macOS arm64 / SDK 11.3 builds, packages and relocated desktop checks
pass. Calendar passes eight unit suites and four views; Browser passes 169
navigation/layout assertions; Suite passes 24 lifecycle assertions and ChatZilla;
standalone XULRunner passes packaged ChatZilla initialization/input. Reports:
`artifacts/es6/date-primitive-runtime`. The first Date
subset was stopped because it started before compilation completed; only the
completed third subset is the final diagnostic result. A native fixture used
`undefined` before initializing that global; replacing it with `void 0` preserves
the assertion without requiring extra global bootstrap. Upstream tests are
unchanged. Other operating systems and architectures have not been revalidated.

### Array concat and species

Modern concat observes Symbol.isConcatSpreadable, preserves sparse properties,
uses ToLength and checked safe-integer output indices, and creates results through
ArraySpeciesCreate. Foreign intrinsic Array constructors select the executing
method's default Array rather than reading the foreign species accessor; custom
constructors still participate. Primitive receiver wrappers use the method's
realm. Modern Array has a configurable species getter returning its raw receiver.
Legacy globals retain the previous concat implementation.

Diagnostic concat coverage: 89/95 cases; the remaining six require typed arrays
or classes. Focused checks: 29 script and 36 native embedding checks under
MallocScribble, including GC, foreign contexts, cloned methods, primitive wrapper
realms, custom species, Proxy trap order and native-loop interruption. The native
GC fixture clears its non-deletable boxed-object variable before collecting the
foreign global. The full pinned ES2015 run passes **26,330 cases**, with **2,236
failures**, 14 unsupported module cases and two harness errors: **49 gained,
zero lost** relative to Date. No crashes/timeouts occurred; the frozen runtime
remained unchanged. All **11,540 required-mode ES5.1 cases** pass in
America/Los_Angeles. Reports: `artifacts/es6/array-concat-full-final2.json` and
`array-concat-es5-final2.json`; snapshot:
`/tmp/zr-array-concat-conformance-final2-20260918`.

All four macOS arm64 / SDK 11.3 builds, packages and relocated desktop checks
pass. Calendar passes eight unit suites and four views; Browser passes 169
navigation/layout assertions; Suite passes 24 lifecycle assertions and ChatZilla;
standalone XULRunner passes packaged ChatZilla initialization/input. Reports:
`artifacts/es6/array-concat-runtime`. Other platforms remain unvalidated.

Modern concat has a distinct native entry point, so a JSAPI clone into a legacy
global preserves the modern method's semantics while using that destination's
intrinsics. The superseded first full runs were stopped before completion and
must not be reported as validation; final runs use a second frozen snapshot.

### Array callback methods

Modern forEach, map, filter, some, every, reduce and reduceRight use ToLength,
live HasProperty/Get traversal with a snapshot length, and raw callback receivers.
Map/filter use ArraySpeciesCreate and CreateDataProperty without an extra length
Set on custom results. Distinct native entry points preserve modern semantics
when JSAPI-cloned into legacy globals. Legacy globals retain their original
methods and ToUint32 lengths. Long native loops remain interruptible.

All 3,048 pinned cases in the seven method subsets pass: map 383, filter 438,
forEach 366, some 424, every 421, reduce 509 and reduceRight 507. Focused coverage
passes 37 script checks and 35 native embedding checks under MallocScribble.
The first diagnostic invocation resolved xpcshell's symlink out of dist/bin and
failed source preflight due to its missing runtime library; no cases executed.
Corrected runs preserve the executable's dist/bin path and report unchanged
runtime hashes. The native GC fixture releases the foreign cloned method before
checking finalization, keeping a local method rooted for the later legacy-clone
check. Upstream tests and assertions are unchanged.

The full pinned ES2015 run passes **26,394 cases**, with **2,172 failures**,
14 unsupported module cases and two harness errors: **64 gained, zero lost**
relative to concat. No crashes/timeouts occurred; the frozen runtime remained
unchanged. All **11,540 required-mode ES5.1 cases** pass in America/Los_Angeles.
Reports: `artifacts/es6/array-iteration-full.json` and `array-iteration-es5.json`;
snapshot: `/tmp/zr-array-iteration-conformance-20260918`. Its libmozjs SHA-256
`11abfd5ba54784ea0622a37f18b3bcea8a2323cbdf1b3339b7746babd225d543` matches the
completed Suite root build.

All four macOS arm64 / SDK 11.3 builds, packages and relocated desktops pass.
Calendar passes eight unit suites and four views; Browser passes 169 navigation/
layout assertions; Suite passes 24 lifecycle assertions and ChatZilla; standalone
XULRunner passes packaged ChatZilla initialization/input. Desktop reports:
`artifacts/es6/array-iteration-runtime`. Other platforms remain unvalidated;
full ES6 remains incomplete.

### Array indexed methods

Modern push, pop, shift, unshift, reverse, slice, splice, indexOf and lastIndexOf
use ToLength and safe-integer indices, throwing writes/deletes and observable
property ordering. Slice/splice create species results, preserve holes and set
the result length before returning or mutating the source. Native loops remain
interruptible; boxed receivers/results use the executing method's realm. Legacy
globals retain their previous methods. Length conversion and search results
normalize negative zero to positive zero where required.

The nine diagnostic subsets pass 1,144/1,144 cases; focused checks pass 59 script
and 48 native embedding assertions under MallocScribble. The first subsets found
eight signed-zero failures in pop/shift/indexOf/lastIndexOf; corrected subsets
pass without upstream changes. C89 checks pass.

The full pinned ES2015 run passes **26,452 cases**, with **2,114 failures**,
14 unsupported module cases and two harness errors: **58 gained, zero lost**
relative to callback methods. No crashes/timeouts occurred; the frozen runtime
remained unchanged. All **11,540 required-mode ES5.1 cases** pass in
America/Los_Angeles. Reports: `artifacts/es6/array-indexed-full.json` and
`array-indexed-es5.json`; snapshot: `/tmp/zr-array-indexed-conformance-20260918`.
Its libmozjs SHA-256 `a02956086d6cd0320988edb64c8e3abf9d806ac8908f939d896caf3f9357c95a`
matches the completed Suite build.

All four macOS arm64 / SDK 11.3 builds, packages and relocated desktops pass.
Calendar passes eight unit suites and four views; Browser passes 169 navigation/
layout assertions; Suite passes 24 lifecycle assertions and ChatZilla; standalone
XULRunner passes packaged ChatZilla initialization/input. Desktop reports:
`artifacts/es6/array-indexed-runtime`. Other platforms and full ES6 remain
incomplete.

### Array string conversion and sorting

Modern join/toLocaleString use ToLength, checked UTF-16 accumulation and direct
Get operations without the historical sharp-map enumeration. A per-context,
stack-owned cycle guard is unwound on failure and interruption; its object stays
rooted independently. Modern Array.toString invokes an observed callable join or
the intrinsic Object tag operation. Modern Object.toLocaleString preserves raw
primitive receivers in both property lookup and invocation. Boxing uses the
executing built-in's realm through a shared private helper. Legacy methods remain
separate, including legacy toString and boxed locale forwarding.

Modern sort collects sparse properties with a dynamically grown rooted vector,
orders values/undefined/holes, performs throwing writes/deletes, and remains
interruptible during traversal and comparison. Even identical objects undergo
observable default string conversions. The private join state is appended to
JSContext and all embedding builds must be refreshed.

Focused checks pass 36 script and 44 native embedding assertions under
MallocScribble, including comparison-phase interruption, foreign realms,
cloned methods, primitive receivers, cycle recovery and GC. C89 checks pass.
Initial subsets exposed inherited locale boxing and identical-value comparison
shortcuts; the modern paths were corrected with added regressions. Corrected
subsets pass **141/141** cases: join 42, toLocaleString 20, toString 24 and sort 55.

The full pinned ES2015 run passes **26,462 cases**, with **2,104 failures**,
14 unsupported module cases and two harness errors: **10 gained, zero lost**
relative to indexed methods. No crashes/timeouts occurred; the frozen runtime
remained unchanged. All **11,540 required-mode ES5.1 cases** pass in
America/Los_Angeles. Reports: `artifacts/es6/array-text-full.json` and
`array-text-es5.json`; snapshot: `/tmp/zr-array-text-conformance-20260918`.
Its libmozjs SHA-256 `f7b7a8c29a8c50134818f9f0f7bbd06b393b34347762b483bdbae6f382f3a37b`
matches the completed Suite build.

All four macOS arm64 / SDK 11.3 builds, packages and relocated desktops pass.
Calendar passes eight unit suites and four views; Browser passes 169 navigation/
layout assertions; Suite passes 24 lifecycle assertions and ChatZilla; standalone
XULRunner passes packaged ChatZilla initialization/input. Desktop reports:
`artifacts/es6/array-text-runtime`. Other platforms and full ES6 remain incomplete.

### Error construction and prototypes

Modern Error instances use a nonconstructible native class retaining the classic
report/stack representation. Prototypes are ordinary objects, NativeError
constructors inherit from Error, and each prototype owns its empty message.
Modern message properties are configurable own data properties that stay deleted;
lazy native resolution does not recreate them. The classic report APIs recognize
both native classes. Legacy globals keep their original constructor/prototype
and lazy-resolution behavior.

Modern constructors allow nested Error creation from message conversion and
ignore the legacy optional filename/line arguments. Generic Error.toString uses
ordered name/message conversion with standard defaults and rejects primitive
receivers. NewTarget and JSAPI-cloned constructors retain the appropriate realm
and native Error identity. Engine-generated exceptions retain native reports,
file/line extensions and garbage-collected stacks.

Diagnostic subsets pass 148/148 cases: Error 74 and Object.getPrototypeOf 74.
Focused checks pass 72 script and 32 native assertions under MallocScribble; C89
checks pass. The native fixture enables JSOPTION_DONT_REPORT_UNCAUGHT before
inspecting a pending exception; otherwise outermost evaluation reports and clears
it. Upstream tests are unchanged.

The full pinned ES2015 run passes **26,482 cases**, with **2,084 failures**,
14 unsupported module cases and two harness errors: **20 gained, zero lost**
relative to string/sort methods. No crashes/timeouts occurred; the frozen runtime
remained unchanged. All **11,540 required-mode ES5.1 cases** pass in
America/Los_Angeles. Reports: `artifacts/es6/error-modern-full.json` and
`error-modern-es5.json`; snapshot: `/tmp/zr-error-modern-conformance-20260918`.
Its libmozjs SHA-256 `005485a3545a71c1452d84b3583c0fb8265d8693f154a063cb0e347b97c4e60e`
matches the completed Suite build.

All four macOS arm64 / SDK 11.3 builds, packages and relocated desktops pass.
Calendar passes eight unit suites and four views; Browser passes 169 navigation/
layout assertions; Suite passes 24 lifecycle assertions and ChatZilla; standalone
XULRunner passes packaged ChatZilla initialization/input. Desktop reports:
`artifacts/es6/error-modern-runtime`. Other platforms and full ES6 remain incomplete.

### ArrayBuffer and DataView

Native ArrayBuffer storage is zero initialized and limited to 2^31-1 bytes per
buffer on all targets; allocation failure raises RangeError. DataView implements
all eight integer/floating-point accessor pairs with unaligned, endian-neutral
byte operations. Float32 writes share Math.fround's explicit ties-to-even rounding
rather than relying on overflowing host casts. Views retain their buffer object;
coercion, species/newTarget callbacks and chunked-copy interrupts recheck detached
storage. An internal native detachment hook supports embedding lifetime tests.

Constructors follow the pinned ES2015 edition, including its stricter ArrayBuffer
length/index conversion rules. Optional DataView offsets follow the
[TC39 #4516 correction](https://tc39.es/archives/bugzilla/4516/); setters convert
values before final bounds checks, following the
[TC39 #4536 correction](https://tc39.es/archives/bugzilla/4536/) in the pinned suite.
NewTarget prototype lookup occurs after argument conversion. ArrayBuffer slicing
honors species, copies independent storage, and rejects detached/undersized/same
result buffers. Deleted global bindings stay deleted without losing intrinsics.

Initial diagnostic subsets pass all 146 cases (ArrayBuffer 90, DataView 56).
Focused checks pass 68 script and 41 native assertions under MallocScribble,
including float boundaries, GC, foreign realms, JSAPI cloning, detachment of both
copy buffers, interruption/recovery and constructor observation order. C89 checks
pass. The initial compiler pass exposed a missing Boolean helper declaration;
its header was added before full validation.

The full pinned ES2015 run passes **26,630 cases**, with **1,936 failures**,
14 unsupported module cases and two harness errors: **148 gained, zero lost**
relative to Error changes. No crashes/timeouts occurred; the frozen runtime
remained unchanged. All **11,540 required-mode ES5.1 cases** pass in
America/Los_Angeles. Reports: `artifacts/es6/binary-data-full.json` and
`binary-data-es5.json`; snapshot: `/tmp/zr-binary-data-conformance-20260918`.
Its libmozjs SHA-256 is
`11e151f7465868bfd03d94464205a31a8ae6a580d8e499d06ceb0fb1d2284c52`.
The snapshot was copied from the completed Suite build before casing changes.

All four macOS arm64 / SDK 11.3 builds, packages and relocated desktops pass.
Calendar passes eight unit suites and four views; Browser passes 169 navigation/
layout assertions; Suite passes 24 lifecycle assertions and ChatZilla; standalone
XULRunner passes packaged ChatZilla initialization/input. Desktop reports:
`artifacts/es6/binary-data-runtime`. Typed arrays remain unimplemented;
ArrayBuffer.isView currently recognizes DataView only. Other platforms and full
ES6 remain incomplete.

### Modern Unicode casing

ES2015 globals use separate native upper/lowercase methods backed by checksum-
pinned Unicode 18.0.0 data. Full mappings cover expansions and supplementary
characters; Final_Sigma uses original-text Cased and Case_Ignorable context,
including characters with both properties. Native loops are interruptible and
UTF-16 growth is checked. Legacy globals retain historical methods; cloned modern
methods retain modern mappings even when installed into a legacy global. Explicit embedding locale
callbacks remain authoritative; default modern locale casing uses the full tables.
Platform-wide Unicode tables and regexp case folding remain unchanged.

Focused checks pass 23 script and 26 native assertions under MallocScribble;
C89 checks pass. The String `prototype/to*` diagnostic subset passes 204/204.
The pinned UCD runner checks every code point through four methods: all
4,456,448 comparisons pass, including identity mappings and lone surrogates.
Native tests exercise GC, long ignored runs, interruption/recovery, foreign
contexts, JSAPI cloning and legacy isolation. The initial compile required adding
the private function-flags header.

The full pinned ES2015 run passes **26,648 cases**, with **1,918 failures**,
14 unsupported module cases and two harness errors: **18 gained, zero lost**
relative to binary data. No crashes/timeouts occurred; the frozen runtime
remained unchanged. All **11,540 required-mode ES5.1 cases** pass in
America/Los_Angeles. Reports: `artifacts/es6/casing-full.json` and
`casing-es5.json`; snapshot: `/tmp/zr-casing-conformance-20260918`.
It combines the preceding frozen runtime with the completed casing engine;
libmozjs SHA-256 `4c8b2ab27b15cde35d732b2f11f33a0f85bfc6c64958d16bac921b001ce2f9f2`
matches the completed Suite build.

All four macOS arm64 / SDK 11.3 builds, packages and relocated desktops pass.
Calendar passes eight unit suites and four views; Browser passes 169 navigation/
layout assertions; Suite passes 24 lifecycle assertions and ChatZilla; standalone
XULRunner passes packaged ChatZilla initialization/input. Desktop reports:
`artifacts/es6/casing-runtime`. Other platforms and full ES6 remain incomplete.

### Modern Function invocation

Modern call/apply reject noncallable receivers without invoking their conversion
hooks. Apply uses ToLength instead of wrapping argument-list lengths to 32 bits,
preserves raw target receivers and ordered element reads, and permits native-loop
interruption. Exceeding the existing argument-storage limit raises RangeError
before allocation; legacy apply retains its original ToUint32 behavior. Modern
methods retain their behavior when cloned into a legacy global.

Native modern errors now use the executing function's realm. The historical
caller scope remains intact for embedding/eval behavior; callback-thrown errors
retain the callback's realm. An initial native test exposed caller-realm errors;
the exception prototype lookup was corrected with foreign TypeError/RangeError
and callback regressions.

Focused checks pass 24 script and 29 native assertions under MallocScribble; C89
checks pass. The diagnostic Function subset passes 715/715 cases.

The full pinned ES2015 run preserves **26,648 passes**, **1,918 failures**,
14 unsupported module cases and two harness errors: **zero gained, zero lost**
relative to casing. These fixes cover additional semantics beyond the pinned
suite's failing cases. No crashes/timeouts occurred; the frozen runtime remained
unchanged. All **11,540 required-mode ES5.1 cases** pass in America/Los_Angeles.
Reports: `artifacts/es6/function-invoke-full.json` and `function-invoke-es5.json`;
snapshot: `/tmp/zr-function-invoke-conformance-20260918`. It combines the preceding
frozen runtime with the completed invocation engine. Its libmozjs SHA-256
`3f81e339d2679060864da2053abe4ecb3377aeb2d9a1c6132d5b14af4b6e6c05`
matches the completed Suite build.

All four macOS arm64 / SDK 11.3 builds, packages and relocated desktops pass.
Calendar passes eight unit suites and four views; Browser passes 169 navigation/
layout assertions; Suite passes 24 lifecycle assertions and ChatZilla; standalone
XULRunner passes packaged ChatZilla initialization/input. Desktop reports:
`artifacts/es6/function-invoke-runtime`. Other platforms and full ES6 remain incomplete.

### Reflection key ordering and array realms

Modern Object.keys sorts ordinary native keys by the original ES2015 integer-
index rule, then string creation order. Object.keys, getOwnPropertyNames and
getOwnPropertySymbols allocate result arrays in the executing built-in's realm.
Proxy-supplied ordering remains unchanged. Selected ES5/legacy script ordering
and primitive TypeErrors remain unchanged, including calls into modern globals.

JSON's internal calls into the shared key collector do not have a native argv
callee slot. The array helper derives its realm from the active operation frame,
so JSON stringify/reviver enumeration follows modern ordering safely. Copied key
identifiers stay rooted across garbage collection and native-loop interrupts.

Focused checks pass 16 script and 28 native assertions under MallocScribble; C89
checks pass. Diagnostic subsets pass Object 5,984/5,984 and JSON 208/208. Native
checks cover foreign ordinary/proxy results, cloned methods, interruption and
recovery, internal JSON calls, and explicitly selected legacy script behavior.
The full pinned ES2015 run passes **26,654 cases**, with **1,912 failures**,
14 unsupported module cases and two harness errors: **six gained, zero lost**
relative to invocation changes. No crashes/timeouts occurred; the frozen runtime
remained unchanged. All **11,540 required-mode ES5.1 cases** pass in
America/Los_Angeles. Reports: `artifacts/es6/reflection-keys-full.json` and
`reflection-keys-es5.json`; snapshot: `/tmp/zr-reflection-keys-conformance-20260918`.
It combines the preceding frozen runtime with the completed reflection engine;
libmozjs SHA-256 `56cd289961ae4c8fbdf9d7f4e84aeded103f024f3cba844650942982300b710c`
matches the completed Suite build.

All four macOS arm64 / SDK 11.3 builds, packages and relocated desktops pass.
Calendar passes eight unit suites and four views; Browser passes 169 navigation/
layout assertions; Suite passes 24 lifecycle assertions and ChatZilla; standalone
XULRunner passes packaged ChatZilla initialization/input. Desktop reports:
`artifacts/es6/reflection-keys-runtime`. Other platforms and full ES6 remain incomplete.

## ECMAScript job queue foundation

`JS_EnqueueJob`, `JS_HasPendingJobs` and `JS_RunJobs` provide explicit embedding
checkpoints. Contexts on the same runtime/thread share a FIFO queue; separate
threads never execute one another's jobs. Queued callbacks remain rooted across
GC and temporary-context destruction. Removing the last context from a thread
cancels its remaining jobs. A nested drain is a no-op. A pending exception blocks
a drain, and an abrupt callback stops it with later jobs still queued.

Both shells provide `enqueueJob` and `drainJobQueue` testing hooks and drain after
a command-line script turn. Nested `load` and `evaluate` calls do not themselves
checkpoint. Uncaught job failures give the shell a failing exit status. Test262
captures the drain hook before running test code, then checks async completion
only after the queue drains; queued failures cannot satisfy synchronous negative
exception patterns. These hooks are not exposed as web globals.

Focused validation uses `TestJobQueue.c` (51 checks), `test-job-shell.py` (ten
xpcshell checks and nine standalone-shell checks), and the 23-check runner
integration fixture. This foundation did not itself implement Promise or DOM
checkpoints; the following batch adds those features.

The final macOS arm64/SDK 11.3 queue run preserves all **26,654 ES2015 passes**,
with 1,912 failures, 14 unsupported module cases, two harness errors and no
crashes/timeouts. Zero cases gained or lost against reflection. All **11,540
ES5.1 cases** pass in America/Los_Angeles. Reports are
`artifacts/es6/promise-jobs-fixed-full.json` and `promise-jobs-fixed-es5.json`;
the frozen runtime `/tmp/zr-job-queue-fixed-conformance-20260918` remained unchanged.
All four root builds, packages and relocated application checks passed, along
with Calendar's eight unit suites/four views, Browser's 169 navigation/layout
checks, Suite's 24 lifecycle checks/ChatZilla, and standalone XULRunner ChatZilla.
Other platforms and full ES6 remain unvalidated/incomplete.

Application testing caught a thread-teardown crash that shell conformance did
not: direct context migration could leave the old thread's queue root registered
with an empty context list. Migration now detaches the previous owner normally;
thread teardown clears surviving contexts' owner pointers for later TLS cleanup.
The native migration fixture crashes on the pre-fix build and passes on the fix.
Pre-fix crash reports and logs are retained. One fixed-package lifecycle attempt
also timed out without a crash report; the unchanged package subsequently passed.
That isolated timeout is not claimed fixed. The lifecycle runner now preserves
partial timeout output, and the desktop runner reports nonzero GUI exits early.
Final runtime reports are under `artifacts/es6/promise-jobs-fixed-runtime`.


## Promise and application checkpoints

The engine implements Promise construction, then/catch, resolve/reject, all/race,
thenable assimilation, reaction jobs and species constructors. Resolving functions
share an already-resolved record, and queued work retains traced records rather
than pointers into C stack storage. The implementation preserves custom constructor
callbacks, raw handler receivers, callback exceptions and foreign intrinsic realms.
The pinned corpus incorporates the correction removing static all/race species
lookup; instance then still uses species. Promise.all limits its result array to
2^32−1 elements and rejects larger input, subject to allocation limits.

Promise metadata uses ES2015 attributes even in legacy globals. Existing legacy
built-ins and syntax remain version-selected. The added JSProto entry changes
internal runtime layout, requiring complete application/embedding rebuilds;
bytecode is unchanged. Embedders outside ZoolRunner must arrange their own
JS_RunJobs checkpoints.

DOM script completion drains jobs only after the outer script returns. The full
context stack is inspected, including frames below null barriers; nested event
loops cannot drain a suspended outer script's queue. Context ownership survives
jobs that close their window. XPConnect also provides an outer event-boundary
checkpoint, plus a checkpoint after each completed wrapped-JS component callback
so batched native events preserve job ordering. It reports abrupt job errors
and prevents safe-context replacement during a checkpoint. A marked native job entry frame supplies
the callback realm for security principal lookup before a handler enters its
script; the generic XULRunner safe context must neither block legitimate
sandbox handlers nor supply chrome privileges to content handlers. Native and
window fixtures cover job scopes, unprivileged component-access denial and
existing cross-principal constructor access restrictions. Bound/proxy callbacks
use their target realm; revoked callbacks still reject asynchronously.
Promise rejection is represented as Promise state; this batch does not add an unhandled-rejection notification API.

Focused fixtures cover 40 shell assertions and 55 native assertions, including
GC, interruptions, foreign contexts, legacy globals and cloned resolver captures.
The chrome/content fixture covers 16 checks including component timers, nested
event processing, result realms and queued work after closing a child window. The initial synthetic
command event did not execute its listener; the corrected fixture uses a custom
synthetic event, explicitly accepts untrusted events and verifies listener entry.

The final macOS arm64/SDK 11.3 full pinned run passes **27,036 ES2015 cases**,
with **1,530 failures**, 14 unsupported module cases, two harness errors and
zero crashes/timeouts. Compared with the queue baseline, 382 cases gained and
zero previously passing cases were lost. The Promise group passes 380/396;
all 16 remaining failures require unsupported class syntax, with no exclusions.
All **11,540 ES5.1 cases** pass in America/Los_Angeles. Reports are
`artifacts/es6/promise-realm-full.json`, `promise-realm-es5.json` and
`promise-realm-pass-comparison.json`. The frozen XULRunner runtime at
`/tmp/zr-promise-realm-conformance-20260918` remained unchanged during both runs;
its engine hash matches all four completed applications.

All four root builds, packages and relocated GUI checks pass, including the
16-check Promise fixture. Calendar passes eight unit suites and all four views;
Browser passes 169 navigation/layout checks; Suite passes 24 lifecycle checks
and ChatZilla; standalone XULRunner ChatZilla initializes with its input widget.
The native fixture passes under MallocScribble; C89 checks and all 23 runner
integration checks pass. Final desktop reports are under
`artifacts/es6/promise-realm-runtime`. These results cover macOS arm64 only;
other platforms and complete ES6 remain unvalidated/incomplete.

Application validation exposed a security-bootstrap defect absent from shell
results: XULRunner's generic safe context could drain a job yet reject a sloppy
sandbox handler's implicit parent lookup before entering its script. Suite's
hidden-window context masked this. Explicit job scopes and callback-realm
selection fix that path while preserving content/component and cross-principal
access restrictions. Foreign-window result arrays and both privilege levels are
covered by the final fixture. Earlier failures and diagnostic runs are retained
in `artifacts/es6/promise-*`; focused diagnostics are not full application passes.

### Function-environment new.target

ES2015 ordinary functions now parse and evaluate `new.target`. Ordinary calls
return undefined; constructor calls retain the actual newTarget, including
Reflect.construct, bound functions and proxies. Direct eval inherits this
binding through strict and nested evaluations. Global and indirect eval reject
the syntax, and a nested ordinary function has its own binding. Selected legacy
language versions retain their old grammar. Arrow/class environments remain
unfinished; this is a prerequisite, not completion of those features.

The new opcode decompiles as `new.target` and advances the bytecode cache to
version 40. `new-target.js` passes 38 checks covering call/construction, eval
boundaries, syntax, GC and decompilation. `TestEditionEmbedding.c` includes the operation in its
serialized, decoded and decompiled program. `edition-modern.js` exercises it
in actual chrome and content windows.

The final macOS arm64/SDK 11.3 run preserves all **27,036 ES2015 passes**, with
**1,530 failures**, 14 unsupported modules, two harness errors and no crashes or
timeouts. Exact pass-set comparison shows zero gains and zero losses: the
remaining syntax features are still needed to pass their complete cases.
All **11,540 ES5.1 cases** pass. Reports are
`artifacts/es6/new-target-final-es6.json`, `new-target-final-es5.json` and
`new-target-final-pass-comparison.json`. The frozen runtime at
`/tmp/zr-new-target-final-conformance-20260918` remained unchanged through both
runs. Its libmozjs SHA-256 is
`f76069069ec87817aed9bdec65d31366b539fbfc5db834775047fba7fc33862f`.

All four applications pass root builds, packages and relocated desktop checks;
their engine hashes match the frozen runtime. Calendar passes its unit suites
and four views; Browser passes 169 navigation/layout checks; Suite passes
lifecycle and ChatZilla checks; standalone XULRunner ChatZilla initializes with
its input widget. C89 checks and 23 runner integration checks pass. Desktop
reports are under `artifacts/es6/new-target-final-runtime`. Other platforms have
not been revalidated for this batch.

The first full run caught two regressions: lookahead after `new` incorrectly
scanned a regexp as division. Operand scanning and a dedicated regression now
preserve the required runtime TypeError for `new /pattern/()`. Invalid
new.target assignment and destructuring targets report SyntaxError. Earlier
`new-target-*` reports precede these corrections and are diagnostic only.

### Property assignment references

ES2015 property assignments now check null/undefined bases and convert computed
keys before the right-hand side. Compound assignments reuse that converted key
for reading and writing. The key expression still runs before the base check;
primitive receivers stay unboxed until the actual property operation. Earlier
selected-language modes retain their existing reference-evaluation behavior.
This batch does not fix unresolved identifier references, lexical TDZ or
remaining destructuring semantics.

`assignment-reference.js` covers 44 checks, including callback exceptions,
Symbol keys, single conversion, raw primitive setters, GC and decompilation.
The initial fixture had 18 failures. `assignment-reference-wide.js` covers 12
large-script execution/decompilation checks in ES2015 and JS 1.7, including
accessor initializers and historical getter/setter assignment syntax. The
large-script checks found existing extended-operand defects: property writes
could reverse the key and value, and catch source notes were read at the wrong
position, causing decompilation to fail. Extended stores now keep their operand
order, and block/catch decompilation retains the prefix source position.

Bytecode cache version 41 adds the reference-check opcodes and extended property
store dispatch. Native XDR and real chrome/content fixtures include reference
ordering. The complete pinned macOS arm64 run passes **27,064 ES2015 cases**,
with **1,502 failures**, 14 unsupported module cases, two harness errors and no
crashes/timeouts: **28 gained, zero lost** versus the new.target baseline.
All **11,540 ES5.1 cases** and 23 runner integration checks pass. Both suites
used the frozen runtime `/tmp/zr-assignment-reference-conformance-20260918`,
whose hashes remained unchanged through both runs. Reports are
`artifacts/es6/assignment-reference-{es6,es5}.json` and
`assignment-reference-pass-comparison.json`.

All four macOS arm64 applications build and package successfully, with identical
engine binaries. Relocated desktop checks pass for XULRunner, Suite, Browser
and Calendar, including real chrome/content globals, Suite lifecycle/ChatZilla,
Calendar's four views, browser navigation/layout and actual XULRunner ChatZilla
initialization/input. Logs and reports use the `assignment-reference-` prefix
under `artifacts/es6`. C89 compatibility checks pass. Other platforms and full
ES2015 compliance remain unverified.

### Identifier assignment references

ES2015 unoptimized identifier assignments now retain both their resolved scope
and whether resolution succeeded before the RHS. Strict unresolved writes still
throw if the RHS introduces that global name. Resolved object bindings remain
the target even if a callback deletes the property or changes `unscopables`.
Compound assignments perform GetBindingValue before the RHS, including its
existence check, without repeating scope resolution at the write. Older selected
language modes keep their existing assignment path. This implements the
[ES2015 environment binding algorithms](https://262.ecma-international.org/6.0/#sec-object-environment-records-setmutablebinding-n-v-s).

The reference pair is traced on the operand stack, avoiding hidden heap objects
and preserving embedding scope receivers. Three new bytecodes and cache version
42 preserve the pair through callbacks. Wide identifier reads and increments
retain their name opcodes, rather than becoming element operations that could
return undefined/NaN for unresolved names. Extended operands retain source notes
and decompilation behavior.

`identifier-reference.js` passes 33 focused checks and
`identifier-reference-wide.js` passes 14 execution/decompilation checks above
the 16-bit atom-index boundary. Native XDR and chrome/content checks include
resolved-deleted and initially unresolved writes. The complete pinned macOS
arm64 run passes **27,100 ES2015 cases**, with **1,466 failures**, 14 unsupported
module cases, two harness errors and no crashes/timeouts: **36 gained, zero
lost** versus the property-reference baseline. All **11,540 ES5.1 cases** and
23 runner integration checks pass. The complete compound-assignment group now
passes **703/703** cases. Both full suites used the frozen runtime
`/tmp/zr-identifier-reference-conformance-20260918`; its hashes stayed unchanged.
Reports use the `identifier-reference-` prefix under `artifacts/es6`.

That first run also passed all four application build/package/desktop checks,
Calendar's four views, browser navigation and ChatZilla. An additional native
probe then found that extended opcodes exposed a PC inside their atom operand
to embedding hooks: `JS_IsAssigning` returned false for a large-script write.
Frames and operand provenance now retain the actual prefix address. Assignment
and resolve hints decode extended/debugger opcodes, including their full length;
BINDREF preserves the classic assignment hint to native resolve hooks.

`TestReferenceEmbedding.c` adds 38 checks across JS 1.7 and ES2015, small scripts,
66,000/131,000 atom tables, setter/resolve callbacks, nested evaluation, GC and
traps on the extended writes. The final corrected run preserves all **27,100
ES2015 passes** and **11,540 ES5.1 passes**, with the same 1,466 failures,
14 unsupported modules, two harness errors and no crashes/timeouts. There are
**36 gained, zero lost** versus the property-reference baseline. All four
macOS arm64 applications pass root build, package and relocated desktop checks;
Calendar's four views, browser navigation/layout and Suite/XULRunner ChatZilla
also pass. The 23 runner checks, C89 checks, debugger lifecycle and six additional
large-script error/decompilation diagnostics pass.

Final reports use `artifacts/es6/identifier-reference-final-*`; the frozen
runtime is `/tmp/zr-identifier-reference-final-conformance-20260918`. Hashes
remain unchanged through both suites and match all four application engines
(SHA-256 `1bb529de9a08dda7bb994c6f434797f9e82e46d8e966aff5a268b5c546d8342e`).
Earlier `identifier-reference-*` reports precede the native prefix correction.
Destructuring, lexical TDZ, arrow/class semantics, other platforms and full
ES2015 compliance remain unfinished.

### Template literals

ES2015 ordinary and tagged template parsing/evaluation are implemented in the
existing engine. Ordinary substitutions perform ToString with the string hint
before the next expression. Tags receive unconverted values, preserve their
reference receiver, and treat tagged `eval` as an indirect call. Selected legacy
language versions continue rejecting template syntax.

The registry follows [ES2015 GetTemplateObject](https://262.ecma-international.org/6.0/#sec-gettemplateobject):
equal ordered raw strings share an immutable template array within one realm,
including different source sites and substitution expressions. This intentionally
differs from later editions' site identity rule. ES2015 also rejects malformed
escapes in tagged templates. The private registry uses the existing weak-global
intrinsic cache, immutable raw/cooked arrays, traced operands and native roots;
it does not call mutable global Array/Object helpers.

Cache version 43 adds ToString, template-object and tag-call bytecodes. Encoded
UTF-16 raw/cooked atoms survive XDR. The decompiler preserves raw Unicode,
including lone surrogates, NUL and line separators, through its byte-oriented
internal buffers without changing the public byte-string JSAPI. Long scanner
lines preserve CRLF pairs across buffer boundaries and only normalize a line
terminator once it actually belongs to the copied segment.

`template-literals.js` passes 45 focused checks; `template-boundaries.js` passes
5,416 checks covering long lines, raw strings, nested decompilation and wide atom
indices. `TestTemplateEmbedding.c` passes 33 checks for native Unicode compilation,
XDR, GC, distinct realms, cross-realm calls, selected legacy editions and native
allocation hooks that reenter while the registry or an entry is being created.
The modern chrome/content fixture also exercises templates. C89 syntax checks
pass. The upstream template diagnostic passes 132/134 cases; the two remaining
cases additionally require arrow functions. These are retained as failures.

The final frozen macOS arm64 runtime passes **27,217 ES2015 cases**, with
**1,349 failures**, 14 unsupported module cases, two harness errors and no
crashes/timeouts: **117 gained, zero lost** against the identifier-reference
baseline. All **11,540 ES5.1 cases** pass on the same frozen runtime, whose hashes
remain unchanged through both complete suites. String.raw passes all 58 upstream
cases and runner integration passes all 23 checks. Reports use `template-final-*`
under `artifacts/es6`; the frozen runtime is
`/tmp/zr-template-final-conformance-20260918`.

All four macOS arm64 applications pass root build, package and relocated
desktop checks, including real chrome/content globals, Calendar's four views,
browser navigation/layout (169 checks) and Suite/XULRunner ChatZilla. Their
engine binaries match the frozen conformance runtime (SHA-256
`43b65c08df3e0f981329df11a0dc0a733cf79c367f6fc659f1ba45d58683e5f2`).
The 33 native checks also cover 540 file-input CRLF boundaries; the engine opens
its own file stream to preserve Windows static-CRT ownership. Six supplemental
checks cover primitive/getter tag receivers, evaluation order, non-callable
errors, conditional-callee decompilation and intrinsic eval tagging.

Full ES2015 compliance and other-platform validation remain unfinished.
Earlier `template-untagged-full-*` reports are intermediate diagnostics and
precede tagged-template implementation.

### Arrow functions

ES2015 arrows with simple named parameters now parse and execute in the existing
engine. Expression/block bodies, duplicate-parameter rejection, lexical `this`,
lexical `arguments`, lexical `new.target`, metadata and constructor rejection are
implemented. Parenthesis provenance prevents arbitrary expressions or nested
parameter parentheses from being accepted as arrow formals. Default and
destructured arrow parameters, and class/super integration remain unfinished;
named rest parameters are covered in the subsequent section.

A private function-kind field occupies existing structure padding; public
function flags and the native frame ABI remain intact. Traced per-activation
cells retain the raw receiver and new target, including strict primitives and
undefined. Arrows retain outer argument environments without introducing an
implicit arguments binding of their own. Native cloning retains the original
lexical cell, and interpreted closure creation allocates independent instances.
Cache version 44 serializes function kind. Function/script decompilation retains
arrow syntax and invocation precedence.

Native allocation hooks exercise GC and debugger evaluation that recursively
creates arrows during lexical-cell allocation. This exposed two additional
lifetime defects: already materialized mapped argument indices needed their
last live parameter values saved before frame destruction, and debugger/eval
`this` conversion must follow the enclosing function rather than the evaluated
script's directive prologue. The fixes preserve deleted/detached argument
indices and do not invoke user index accessors during frame cleanup.

Focused checks pass: `arrow.js` (37), `TestArrowEmbedding.c` (20), and
`../es5/arguments-lifetime.js` (12). The native checks also cover cross-realm
calls/cloning, XDR, source reconstruction, legacy rejection and explicit GC.
Package validation registers these checks; the modern chrome/content fixture
checks arrows with object and strict primitive receivers. All changed C files
pass C89 declaration and implicit-function checks.

The final frozen macOS arm64 runtime passes **27,296 ES2015 cases**, with
**1,270 failures**, 14 unsupported modules, two harness errors and no crashes or
timeouts: **79 gained, zero lost** against the template baseline. All
**11,540 ES5.1 cases** pass; runtime hashes remain unchanged through both full
suites. The arrow diagnostic passes 120/146 cases; remaining cases need parameter,
destructuring or class/super support. Runner integration passes all 23 checks.
Reports use `arrow-final-*` under `artifacts/es6`; the frozen runtime is
`/tmp/zr-arrow-final-conformance-20260918`.

All four macOS arm64 applications pass root build and package checks. Their
engine binaries match the frozen conformance runtime (SHA-256
`c5910c8616ee44c6ed0c0c9b8043a828c8a6eaec27ea8c47311598694bcb91d7`).
All four relocated desktop checks pass, including real chrome/content globals,
Calendar's four views, browser navigation/layout (169 checks), and unchanged
Suite/XULRunner ChatZilla. The first desktop run caught an incorrect expectation
in the new strict-mode fixture (arguments must retain the original value);
the corrected fixture passes in all four applications. Failed-run diagnostics
remain in `arrow-final-*-strict-fixture-error*` artifacts.
The earlier `arrow-basic-full-*` diagnostic precedes the two callback-discovered
corrections. Four argument/eval regressions fail on the committed template
runtime and pass with these corrections. Full ES2015 compliance and current
other-platform validation remain unfinished.

### Rest parameters

Named rest formals are implemented for ordinary functions, arrow functions,
object methods and the dynamic `Function` constructor. A hidden local holds the
rest array; `nargs` continues to represent the fixed parameter count and hence
`Function.length`. Cache version 45 adds a rest-initialization opcode and extends
private function-kind flags. The initializer runs in the prolog before body
function declarations, creates own array elements without invoking inherited
setters, and roots the incomplete array independently of debugger-visible
bindings. It does not read mutable global Array helpers.

Ordinary rest functions use an unmapped arguments snapshot even when non-strict;
arrow rest functions still resolve outer arguments. Fast indexed argument reads
also honor the snapshot. Parsing rejects duplicate bindings, invalid rest
placement, getter/setter rest lists and explicit strict directives with
non-simple formals. Legacy edition grammar remains separate. Rest source
reconstruction retains its parameter name; inherited strictness remains in the
enclosing source rather than introducing an illegal directive inside the rest
function. Default parameters and rest destructuring patterns remain unfinished.

Focused `rest-parameters.js` passes 56 cases; `TestRestEmbedding.c` passes
24 checks, including XDR, decompilation, cross-realm calls, native cloning, GC
and debugger reentry while rest arrays are allocated. Modern chrome/content
fixtures exercise rest arrows and dynamic Function. ES2015 unmapped arguments
create the caller/callee poison properties in the specified order, while older
editions retain their historical ordering. The
final frozen macOS arm64 runtime passes **27,316 ES2015 cases**, with
**1,250 failures**, 14 unsupported modules, two harness errors and no crashes or
timeouts: **20 gained, zero lost** against the arrow baseline. All
**11,540 ES5.1 cases** pass, and runtime hashes remain unchanged through both
complete suites. All four macOS arm64 applications pass root build, package and
relocated desktop checks, including modern chrome/content globals, Calendar's
four views, browser navigation/layout (169 checks) and Suite/XULRunner ChatZilla.
Their engine binaries match the frozen conformance runtime (SHA-256
`bdb8da644b063b05604a47007995cecc9bcc1e83680f9341c8ba4aaa7c163a0c`).
Reports use `rest-corrected-*`; the frozen runtime is
`/tmp/zr-rest-corrected-conformance-20260918`. The upstream rest-parameter folder
passes 16/22 cases; the remaining six modes require patterns or class/new-target
integration. Runner integration passes all 23 checks on an unchanged retry; its
first launch exceeded the two-second per-case limit, and both logs are retained.
C89 checks pass. Other platforms have not been revalidated for these changes.

`rest-final-*` precedes the metadata correction and was superseded after its
active build/runtime children completed.
Earlier `rest-basic-full-*` reports precede inherited-setter and dynamic-Function
corrections and are intermediate diagnostics only.


### Block lexical initialization

Modern block and function-body `let` bindings now retain an uninitialized state
until their declaration executes. Reads, writes, `typeof`, increments, direct
eval and captured closures check that state, including after the frame exits.
Discarded lexical reads still execute because they can throw. Initialization
uses separate bytecodes from assignment; cache version 46 also records template
initialization metadata in XDR. Block entry preserves producer PCs used by value
decompilation. Selected legacy-version let semantics remain unchanged.

`lexical-initialization.js` passes 44 focused checks; `TestLexicalEmbedding.c`
passes 14 checks, including GC, debugger reentry during function cloning, XDR
and decompilation. Modern chrome/content fixtures exercise initialized and
uninitialized bindings. Runner integration passes 23 checks and C89 checks pass.

The final frozen macOS arm64 runtime passes **27,338 ES2015 cases**, with
**1,228 failures**, 14 unsupported modules, two harness errors, and no crashes or
timeouts: **22 gained, zero lost** against the rest-parameter baseline. All
**11,540 ES5.1 cases** pass. Reports use `block-tdz-final-*`; the frozen runtime is
`/tmp/zr-block-tdz-final-conformance-20260918`, with engine SHA-256
`068b6779b57b2dbbec84a7e1791e60e703679c3dc988c8d646891bef8ecb558f`.
All four macOS arm64 applications pass root build, package and relocated desktop
checks, including modern chrome/content globals, Calendar's four views, browser
navigation/layout (169 checks), and Suite/XULRunner ChatZilla. All four engine
binaries match the frozen runtime; hashes remain unchanged through both complete
conformance runs.

This does not complete lexical environments: persistent global declarative
bindings, block const scoping and fresh per-iteration environments still require
work. The earlier `block-tdz-diagnostic-*` run precedes the bare for-in assignment
and producer-PC follow-ups; it has the same totals but is not the final runtime.
Other operating systems and architectures have not been revalidated for these
changes.


### Block const and per-iteration bindings

Modern block/function-body const declarations now use lexical storage, retain
TDZ checks and immutable writes, and can shadow outer bindings. Const identity
survives captured-frame detachment, source reconstruction and XDR. Assignment
checks initialization after evaluating the RHS; compound assignments read first.
Selected legacy const behavior remains separate.

Named C-style let loop bindings are fresh before the first condition and before
each update, including continue paths with no update expression. Named let/const
for-in bindings receive a fresh environment for each iteration; captured RHS
bindings remain uninitialized. The old environment stays live while a replacement
is allocated so native callbacks can capture it safely. Exception unwinding
retains the replacement on the scope chain even when detachment fails. Ordinary
var loops, including catch-variable loops, preserve their shared bindings.

Cache version 47 preserves block const metadata and adds immutable local writes
and environment transitions. Decompilation retains const in destructuring and
for-in declarations and hides internal iteration transitions, including loops
without an explicit updater.

Focused const and loop scripts pass 40 and 26 checks, alongside the prior 44
initialization checks. The native lexical probe passes 17 assertions, covering
both const and mutable bindings, XDR/source round-trips, GC and debugger reentry
while loop environments are allocated. Modern window checks exercise const and
loop closures. C89 checks and 23 runner integration checks pass.

The final frozen macOS arm64 runtime passes **27,377 ES2015 cases**, with
**1,189 failures**, 14 unsupported modules, two harness errors and no crashes or
timeouts: **39 gained, zero lost** against the block-TDZ baseline. All
**11,540 ES5.1 cases** pass, with engine hashes unchanged through both suites.
Reports use `lexical-scope-final-*`; the frozen runtime is
`/tmp/zr-lexical-scope-final-conformance-20260918`, with engine SHA-256
`3954a8bcf7d44bc998d364df7395977ecca6ebb1b5d1a11e88181a288106dd67`.
All four macOS arm64 applications pass root builds, packaging and packaged
desktop checks with engines matching the frozen runtime. Calendar passes all
four views; Browser navigation/layout passes 169 checks; Suite and XULRunner
ChatZilla checks pass. These results do not establish validation on other
architectures or operating systems.

The intermediate `block-const-basic-*` snapshot passes 27,365 ES6 modes
(1,201 failing, 14 unsupported, two harness errors), 27 gained/zero lost from
block TDZ, plus all 11,540 ES5 modes. It precedes iteration, source-reconstruction
and callback/unwind follow-ups and is diagnostic only.

Global lexical environments, module environments, default/destructured parameters,
and the broader iterator/destructuring semantics remain unfinished. This batch
does not implement for-of. Non-macOS-arm64 validation remains outstanding.

### Strict function declaration positions

Modern strict code now rejects function declarations used directly as a single
statement body, including `if`, loops and labels. Declarations in blocks,
switch clauses and function bodies remain accepted. The check applies to strict
eval, Function construction and inherited strictness while leaving selected
historical grammar and existing sloppy extensions intact. This is a parser
change with no bytecode-format change.

The focused `function-statement-grammar.js` regression passes 32 checks and is
registered in packaged validation. C89 syntax checks pass. The targeted upstream
function-declaration group passes 9/9 modes. The complete ES2015 run passes
**27,383 modes**, with **1,183 failures**, 14 unsupported modules, two harness
errors and no crashes or timeouts: **six gained, zero lost** from the preceding
lexical-scope baseline. The six gains include strict labelled declarations.
All **11,540 ES5.1 cases** pass. Both full suites used the frozen runtime at
`/tmp/zr-function-grammar-final-conformance-20260918`, with engine SHA-256
`fc537655b5d3567f21a7d5a897b4d6b186ab254a80a529e790c63ba64976912e`,
unchanged through both runs. All four application engines match it. Reports use
`function-grammar-final-*`. All four macOS arm64 applications pass root builds,
packaging and desktop checks, including Calendar's four views, 169 Browser
navigation/layout checks and Suite/XULRunner ChatZilla. Eight additional lexical
unwind checks pass, covering labelled continue, finally, with/eval closures and
GC. Other platforms have not been revalidated for this parser change.

Grammar reference: https://262.ecma-international.org/6.0/#sec-block
and the strict restrictions in sections 13.13 and B.3.4. This does not complete
block function binding/redeclaration semantics.


### For-of iteration

Modern `for…of` now uses the ES2015 Symbol.iterator protocol separately from
historical for-in/for-each enumeration. It supports ordinary assignment targets,
var declarations and named let/const iteration bindings. Iterator values are
acquired before evaluating assignment targets; lexical RHS captures retain their
uninitialized head environment, and body captures receive distinct bindings.
Existing destructuring patterns work in loop heads, but complete ES2015
pattern/default/rest/iterator semantics remain unfinished.

A private traced state object retains the iterator and current value across
callbacks and GC. Failures in next/done/value do not close the iterator. Abrupt
binding/body completion closes it, with distinct throw and normal-return error
precedence. Same-loop continue and normal exhaustion do not close. Nested
labels and finally clauses retain cleanup order. Exception-handler ranges
exclude later outer cleanup after an inner loop has already been closed and
popped; an outer close failure must not resurrect an invalid inner stack slot.
The legacy generator-return sentinel is treated as a return completion by the
cleanup helper, without implementing ES2015 generator syntax.

Grammar keeps `of` contextual and unescaped, rejects initializers/multiple
bindings and invalid statement bodies, and preserves the head's `let` lookahead
restriction. Printed source protects comma RHS expressions and normalized `let`
assignment targets with parentheses. Source notes, large jumps, extended atom
indices and XDR preserve the loop. Cache version **48** adds the iterator
instructions; stack limits reject an unrepresentable handler depth before it
can wrap. The statement-name table is aligned with its internal enum.

Focused validation passes **66 iteration checks**, **seven boundary checks**
and the expanded **19-check native lexical embedding probe**. The native probe
collects during allocations, captures head bindings through debugger reentry,
and executes XDR-decoded and decompiled scripts. Modern chrome/content probes
exercise iteration and break cleanup. Explicit legacy checks retain for-in
key/value destructuring and for-each behavior; for-of remains edition selected.
C89 syntax checks pass. Final review corrected the XML wildcard emitter case
placement; a selected-legacy E4X regression now covers it.

The corrected complete ES2015 run passes **27,485 modes**, with **1,081 failures**,
14 unsupported modules, two harness errors and no crashes or timeouts:
**102 gained, zero lost** against the preceding strict-declaration baseline.
It uses the frozen runtime at `/tmp/zr-for-of-reviewed-conformance-20260918`,
with engine SHA-256
`4fe74a0cf894edfc7383b43816adfaa1800642e4cab65df716409b381e1e8a1f`.
All **11,540 ES5.1 cases** pass. Engine hashes remained unchanged through both
full runs, and all four application engines match the frozen runtime. All four
macOS arm64 applications pass root builds, packaging and desktop checks,
including Calendar startup/four views, 169 Browser navigation/layout checks
and Suite/XULRunner ChatZilla. Reports use `for-of-reviewed-*`.
The preceding `for-of-validated-*` run had the same ES2015 totals and passed
all ES5 cases and four application checks, but predates the XML case correction.

The initial pinned for-of diagnostic passes **96/206 modes**; the remaining
110 require missing generator syntax (74) or typed arrays (36). This subset
is not a full-suite result. Earlier `for-of-final-*` and
`for-of-corrected-final-*` matrix/conformance attempts were stopped for grammar
and source-round-trip follow-ups; retain their partial logs as diagnostics.
The first large-array source-printing probe exceeded its time limit; a follow-up
completed, and the unchanged baseline also exhibited slow large-array printing.
The registered atom-boundary probe uses separate assignments to exercise the
same extended atom indices without that unrelated quadratic array formatting.

Global lexical environments, modules, generators, typed arrays and broader
parameter/destructuring semantics still require work. The implementation follows
[the ES2015 iteration algorithms](https://262.ecma-international.org/6.0/#sec-for-in-and-for-of-statements)
and [IteratorClose](https://262.ecma-international.org/6.0/#sec-iteratorclose).
Other-platform and minimum-OS validation remains outstanding for this batch.


### Unicode identifier code points

Modern identifier tokens accept brace Unicode escapes and supplementary raw
characters using private Unicode 18.0.0 ID_Start/ID_Continue tables. Legacy
editions retain their historical identifier rules. Source reconstruction emits
valid Unicode identifier escapes instead of string-only hex escapes or separate
surrogate escapes. String and XML text escaping remains separate.

`identifier-codepoints.js` passes 55 checks, including malformed/overflowing
escapes, contextual and reserved words, surrogate boundaries, long tokens,
source round-trips and legacy XML. The pinned identifier subset passes all
267 modes; the independent Unicode property-boundary probe passes 15,736 checks.
The complete run passes **27,499 ES2015 modes**, with **1,067 failures**,
14 unsupported modules, two harness errors and no crashes/timeouts:
**14 gained, zero lost** against the for-of baseline. All **11,540 ES5.1 cases**
pass. All four macOS arm64 applications pass root builds, packaging and desktop
checks, including Calendar's four views, 169 Browser navigation/layout checks
and Suite/XULRunner ChatZilla. Reports use `identifier-codepoint-final-*`.
The frozen runtime `/tmp/zr-identifier-codepoint-final-conformance-20260918`
remained unchanged through both suites; its engine SHA-256 is
`47078f7aa13cdd3a766fc22299c27464c7eb0fbf1df87104ea92eb8a764171c4`,
matching all four application engines. C89 syntax checks pass. No bytecode-format
change is needed. Other platforms remain unvalidated for this batch.


### Modern generators

Modern function-star declarations/expressions and generator methods use a
separate intrinsic prototype graph and resume protocol. Classic generators
retain next/send/throw/close and StopIteration. Modern next/throw/return handle
newborn, suspended, running and closed states; return values survive yielding
finally blocks. Strict/rest arguments stay unmapped when frames are moved.
Abandoned modern generators do not run legacy GC close hooks; escaped block,
call and mapped-argument environments retain their suspended generator.

Yield-star delegates next/throw/return, preserves done-false result identity,
and closes on a missing throw method. Abrupt cleanup interoperates with for-of.
Function kind survives cloning/XDR, generator functions reject construction,
and the decompiler retains function-star and yield-star syntax. Cache version
49 records the new function kind and delegated-yield opcode.

Focused validation passes 78 checks and a 14-check native embedding probe with
GC on allocation, debugger reentry, suspended return values, XDR and source
round-trips. Targeted upstream groups pass 114/114 Generator built-in cases,
122/122 generator syntax cases and 29/29 yield cases. The for-of group now
passes 170/206; the remaining 36 depend on typed arrays. The first diagnostic
full run (`generators-first-*`) predates completion-value, contextual-yield and
strict-argument corrections. The final full run passes **27,878 modes**, with
**688 failures**, 14 unsupported modules, two harness errors and no crashes or
timeouts: **379 gained, zero lost** against the identifier baseline. All
**11,540 ES5.1 cases** pass. All four macOS arm64 applications pass root builds,
packaging and desktop checks, including Calendar's four views, 169 Browser
navigation/layout checks and Suite/XULRunner ChatZilla. C89 checks pass.
Reports use `generators-validated-final-*`; the frozen runtime remained
unchanged through both complete suites. Its engine SHA-256 is
`07ea6ede90438b0282945f319ccd9c25fa35479e5c5c4d442178a5d756b61b9d`,
matching all four application engines. Other platforms remain unvalidated for
this batch. Typed arrays, classes, modules, Unicode regular expressions and
broader parameter/destructuring/global lexical semantics still require work.


### Typed arrays

All nine ES2015 typed-array types now use integer-indexed object operations
over shared ArrayBuffer storage. Numeric properties remain separate from
ordinary string/symbol properties. Array iteration reads internal lengths;
reflection, inherited receivers and buffer detachment follow the original
ES2015 rules. Native methods implement conversion, callbacks, species, shared
views and byte-preserving copies, without changing classic embedding APIs.

`typed-arrays.js` passes 252 checks and `TestTypedArrays.c` passes 29 native
checks, including reflection and collection during allocation, property-store
initialization before embedding hooks, and detachment inside callbacks.
The frozen foundation subset reports (`typedarray-foundation-*`) pass the
typed-array, typed-array concat, ArrayIteratorPrototype, for-of and Symbol
number-coercion groups. Those subsets are diagnostics, not full conformance.
The complete pinned run passes **27,940 modes**, with **628 failures**, 14
unsupported modules, no harness errors, crashes or timeouts: **62 gained, zero
lost** against the generator baseline. All **11,540 ES5.1 cases** pass. The
initial parallel ES5 startup failed during XPCOM component registration; the
unchanged frozen runtime completed the full suite on rerun. Its initial startup
log is retained separately. Future relocated snapshots initialize their
component registry before concurrent conformance processes.

All four macOS arm64 applications pass root builds, packaging and desktop
checks, including Calendar's four views, 169 Browser navigation/layout checks
and Suite/XULRunner ChatZilla. C89 checks pass. Reports use
`typedarray-allocation-final-*`; the engine SHA-256 is
`0fa3c91c88d04260de966b17e3c4f91cd48340ce62362bf57d4315151fc354d1`,
matching all four applications. An initial Browser package check exposed a
stale object after staging files with preserved timestamps; rebuilding it with
current timestamps restored the matching engine. The failed log is retained.
Review also fixed throwing writes to invalid
indices and a native allocation-hook crash: each view's ordinary property
store now exists before the hook can inspect or restrict it. No bytecode change
is needed for this batch. Other platforms remain unvalidated for these changes.
Classes, modules, Unicode regular expressions and broader parameter,
destructuring and lexical-environment semantics remain unfinished.


### Basic object patterns and update targets

Modern object patterns now accept shorthand bindings, validate null/undefined
sources even for empty or nested patterns, and reject invalid assignment targets
at compilation. Their decompiled source retains the distinction between empty
object and array patterns. Cache version 50 records this emission/source-note
change. Legacy destructuring remains edition selected. Modern update expressions
parse their complete unary operand and report early ReferenceError for invalid
non-simple targets, following the original ES2015 rules.

Focused object-pattern and update-target fixtures pass 27 and 19 checks. The
native embedding probe passes 13 checks, including GC during property access,
XDR into a legacy-version context and decompiled script evaluation. The final
complete pinned run passes **27,997 modes**, with **571 failures**, 14 unsupported
modules and no harness errors, crashes or timeouts: **57 gained, zero lost**
against the typed-array baseline. All **11,540 ES5.1 cases** pass. All four macOS
arm64 applications pass root builds, packaging and desktop checks, including
Calendar's four views, 169 Browser navigation/layout checks and Suite/XULRunner
ChatZilla. C89 checks pass. The frozen runtime remained unchanged through both
suites and matches all four application engines; its SHA-256 is
`e28cdb4f23395f9709f2c6e3572a68bba0c5ce8cbf2acdc2e794783a6ad45b61`.
Other platforms remain unvalidated for this batch. Arbitrary
computed pattern keys, defaults, rest and complete array iterator semantics
remain unfinished. Reports use `object-patterns-stage-*`; the main-build checks
use `object-patterns-final-*`.

The older focused tests now verify successful constant-key patterns and the
original ES2015 ReferenceError for invalid new.target updates. Their previous
unsupported-feature/error-type expectations no longer matched the implemented
behavior. The pinned upstream tests and runner policy remain unchanged.


### Computed object pattern keys

Object patterns now evaluate computed keys, including Symbol conversion and
yield expressions, and preserve their source through decompilation. Cache 51
records the computed-key source-note regions. The focused fixture passes 25
checks, including GC, primitive receivers, for-of bindings and long branches;
the 13-check native pattern probe now includes computed keys through XDR and
script decompilation. The complete pinned ES2015 run passes **28,002 modes**,
with **566 failures**, 14 unsupported modules and no harness errors, crashes or
timeouts: **five gained, zero lost**. All **11,540 required ES5 cases** pass on
the same frozen macOS arm64 runtime, with unchanged binary hashes before and
after both runs. Reports use `artifacts/es6/computed-patterns-final-*`.
All four applications pass root build, package and relocated desktop checks,
including Calendar's four views, 169 Browser navigation/layout checks and
ChatZilla in Suite and XULRunner. The engine SHA-256 is identical in all four
builds and the conformance runtime:
`306d5709832f0c684fdbec3f8dace9ce5a441053e97c9c83ff2aa7dbd62e6af1`.
C89 diagnostics pass. These results cover macOS arm64 only. Defaults, rest,
assignment-reference ordering and complete array iterator behavior remain
unfinished; this is not full ES2015 compliance.


### Destructuring defaults

Modern object and array patterns accept initializers, evaluating them only for
undefined values. Object shorthand defaults remain cover grammar and are
rejected when used as ordinary object literals, including dead branches.
Source notes preserve initializers and cache version 52 records their emission.
Object-pattern formal decompilation also skips the coercibility opcode.
The focused fixture passes 28 checks, including GC, inferred names, lexical
initialization, generator suspension, parameter/source round trips and wide
branches. The native 13-check pattern probe includes defaults through XDR and
script decompilation; real-window coverage includes lexical and formal defaults.
The complete pinned ES2015 run passes **28,085 modes**, with **483 failures**,
14 unsupported modules and no harness errors, crashes or timeouts: **83 gained,
zero lost**. All **11,540 ES5 cases** pass on the same frozen macOS arm64 runtime.
All four applications pass root build, package and relocated desktop checks,
including Calendar's four views, 169 Browser navigation/layout checks and
ChatZilla in Suite and XULRunner. Reports use
`artifacts/es6/pattern-defaults-final-*`; all four application engine hashes
match the conformance runtime: `885a599b9536dee8b5290abb0e885f5d45901bdc3d1b68f2f704137ee90f887b`.
C89 diagnostics and the existing focused lexical, generator, for-of, arrow,
reference and legacy-application fixtures pass. Other platforms have not been
revalidated for this batch. Parameter initializers on the whole parameter,
complete parameter-environment semantics, rest elements, assignment-reference
ordering and iterator-based array patterns remain unfinished. This is not full
ES2015 compliance.


### Iterator-based patterns and captured assignment targets

ES2015 array patterns use iterators, retain exhaustion, skip elision values,
collect rest elements into arrays and close unfinished iterators. Destination
references are captured before source access or defaults for both object and
array assignments. Legacy selected-edition patterns retain indexed behavior.
Cache version 54 records the iterator opcodes and source notes; source and XDR
round trips include nested/rest patterns, reference capture and wide branches.
The focused reference, array, rest and wide fixtures pass 60 checks, and the
native pattern probe passes 13 checks. The upstream destructuring subset passes
291 of 299 cases; its remaining cases require classes or whole-parameter
defaults. This diagnostic result is not a complete conformance result.
The complete pinned ES2015 run passes **28,147 modes**, with **421 failures**,
14 unsupported modules and no harness errors, crashes or timeouts: **62 gained,
zero lost**. All **11,540 ES5 cases** pass on the same frozen macOS arm64 runtime.
All four applications pass root build, package and relocated desktop checks,
including Calendar's four views, 169 Browser navigation/layout checks and
ChatZilla in Suite and XULRunner. Reports use `artifacts/es6/array-patterns-final-*`;
all four engine hashes match the conformance runtime:
`8a0abdef0880abc202bb7e139fba773d59d11d66764096c265a2815c3bb2dbb4`.
C89 diagnostics and focused legacy-application checks pass. Array-literal
spread, function-call spread, whole-parameter defaults, complete parameter
environments, classes and modules remain unfinished. This is not full ES2015
compliance; other platforms have not been revalidated for this batch.

The wide immutable-write fixture now supplies `Array.prototype[Symbol.iterator]`
on its array-like ES2015 input, so execution reaches the intended constant-write
error. Its 24 assertions remain unchanged. The ES5 diagnostic likewise retains
54 exception assertions with an iterable modern input and original legacy
inputs. Initial package-check logs are retained under
`array-patterns-final-*.initial*.log`; upstream Test262 files are unchanged.

The initial orchestration watchers stopped while a failed package log was being
rotated. Their diagnostics are retained; complete restarted conformance and
desktop runs above passed their required checks against the unchanged engine.

## Unicode regular expressions

ES2015 regular expressions accept `u`, expose the `unicode` accessor, and match
code points while preserving UTF-16 offsets for captures and `lastIndex`.
Patterns support supplementary literals, paired and braced Unicode escapes,
code-point classes/ranges, Unicode-aware backreferences and strict Unicode
escape grammar. Unicode ignore-case matching uses pinned Unicode 18.0.0
simple/common case folds, including word-class/boundary canonicalization;
non-Unicode matching and explicitly selected legacy editions retain their paths.
Existing match/replace/split protocols advance empty Unicode matches by code point.

Unicode class bitmaps are completed before publishing the regexp. Compilation
callbacks can cancel, collect or compile another regexp. Decoded regexp objects
and script atoms remain rooted across those callbacks. Cache version 55 records
the new flag semantics; script/function source and XDR round-trips are exercised.

`regexp-unicode.js` supplies 48 focused checks and `TestRegExpUnicode.c` supplies
22 native embedding checks. The real-window edition fixture exercises Unicode
matching and empty global matches. `test-regexp-casefold.py` checks every pinned
simple/common mapping (15,330 assertions); see the [Unicode data guide](../../src/unicode/README.md).

The final full macOS arm64 run passes **28,193 modes**, with **375 failures**,
**14 unsupported modules**, and no harness errors, crashes or timeouts: **46 gained,
zero lost** against iterator patterns. All **11,540 ES5 cases** pass against the
same frozen runtime. All four applications pass root builds, packaging and
relocated desktop checks, including Calendar's four views, 169 Browser
navigation/layout checks and ChatZilla in Suite and XULRunner. Native, mapping,
C89 and 9,450 optional Node differential checks pass. Final reports use
`artifacts/es6/regexp-unicode-verified-final-*`. All four engine hashes match
the frozen conformance runtime:
`136d604f959eb1b7468ccd071c735e9a0349e465269fc79142d6075229f54af6`.
Full ES2015 compliance remains unfinished; other platforms are not revalidated
for this batch.

The first desktop run exposed mixed-edition integration missing from the modern
shell: historical globals did not expose the new `unicode` field, and their
String match/replace/split loops advanced empty Unicode matches by a code unit.
The legacy regexp property hook now reports the stored flag, and those loops
advance paired surrogates together only for Unicode regexps. Existing callable
regexp behavior and non-Unicode matching remain covered. The unchanged failing
window assertions now pass, with additional replace/split assertions, after
this engine correction. A second diagnostic run also caught an omitted legacy
property-table entry; the strengthened native probe creates an independent
legacy global and reproduces that failure before the correction. The first failure log is retained under
`artifacts/es6/regexp-unicode-final-runtime/xulrunner/editions.log`.

## Global declarations and statement completion values

Modern native-global scripts and eval validate all function/variable declarations
before creating any of them. Function checks run in reverse declaration order,
ignore earlier duplicate functions, and inspect own descriptors without invoking
getters. Configurable properties can become functions; allowed nonconfigurable
data properties retain their attributes. Inherited properties do not prevent an
own variable binding. Local/strict eval and explicitly selected legacy editions
retain their separate paths. Persistent global lexical environments remain
unfinished; these checks do not implement global `let`/`const` semantics.

Result-producing modern statements implement ES2015's undefined completion for
empty branches/loops/handlers. Finally preserves a normal try/handler result;
abrupt finally completion still overrides it. The immutable script edition
controls finally stack layout even if an embedding callback changes the context's
selected edition. Explicit legacy scripts retain their old completion behavior.
Cache version 56 records this layout change. Constant folding, source
reconstruction, wide branches, generators and XDR round trips are covered.

The focused declaration fixture has 22 checks; its native embedding probe has
13. Completion fixtures cover 27 basic, four wide-branch and ten error-source
checks, plus 17 native embedding assertions. The real-window edition fixture
also checks statement results and caught errors. An initial isolated full run
exposed incorrect stack accounting in error-source reconstruction: a hidden
PUSH/POPV reset must be skipped as a pair, and finally needs its additional saved
completion slot. All 96 affected cases pass after correction; original diagnostic
reports are retained under `artifacts/es6/completion-stage-*` and
`completion-reviewed-*`. The complete final macOS arm64 run passes **28,199
ES2015 modes**, with **369 failures**, **14 unsupported modules** and no harness
errors, crashes or timeouts: **six gained, zero lost**. All **11,540 ES5 cases**
pass on the same frozen runtime. All four applications pass root builds,
packaging and relocated desktop checks, including Calendar's four views,
169 Browser navigation/layout checks and ChatZilla in Suite and XULRunner.
Final reports use `artifacts/es6/completion-final-*`. All four engine hashes
match the frozen conformance runtime:
`387d3d062c068c8387587beeb29bec7e9c767d89943a7ae8f56a95fbbd0eec74`.
C89 diagnostics pass. Full ES2015 compliance remains unfinished; other platforms
have not been revalidated for this batch.

## Array, call and constructor spread

The validated implementation adds iterator-based array literals and function/
constructor argument spread. Array accumulation preserves holes around spreads,
UTF-16 string iteration, own element definitions and evaluation order. It uses
neither mutable array methods nor indexed reads in place of iteration. Call
accumulation retains the callee and receiver before evaluating arguments, keeps
direct `eval` attached to its original caller, and uses the existing constructor,
new-target, bound-function and Proxy paths. Temporary constructor argument
vectors are marked as internal invocations, including native constructors and
bound/Reflect/Proxy forwarding.

Fixed-arity opcodes 253/254 describe array accumulation and spread invocation;
cache version 58 records them. Source reconstruction preserves comma expressions
and parentheses around constructor expressions. Selected legacy editions still
reject spread syntax. Focused array/call fixtures supply 21/20 checks, with
23 assertions in each native embedding probe covering collection, callback
reentry, cancellation, XDR and source round trips. Optional Node differential
checks cover 970 array and 624 call/constructor cases without differences.
Real-window checks include array holes, method receivers, construction and direct
eval. Seven additional checks exercise 66,000 arguments through ordinary,
Proxy and bound construction, direct/indirect eval and source round trips.
These exposed an allocator assumption about a caller's operand stack: nested
internal invocations can temporarily point the stack pointer into another rooted
argument segment. Operand-tail initialization now checks its frame bounds and
runs before redirecting that pointer. The native probe also collects during
large Proxy, bound and Reflect construction. The original failure and LLDB
trace remain under `artifacts/es6/call-spread-wide*`.

The final corrected runtime retains **28,199 passes**, **369 failures** and
**14 unsupported modules**, with zero lost passes, harness errors, crashes or
timeouts. All **11,540 ES5 cases** pass on the same frozen runtime. All four
applications pass root builds, packaging and relocated desktop checks, including
Calendar's four views, 169 Browser navigation/layout checks and ChatZilla in
Suite and XULRunner. Final reports use `artifacts/es6/spread-wide-final-*`.
All four application engine hashes match the frozen conformance runtime:
`cbe636e5eb1ed8d075c1a21337c98299cf15e4396e8fe8fd7387e379a2b6e30e`.
The earlier corpus runs alone missed the wide-argument regression; the added
wide and native collection probes cover it. Full ES2015 compliance remains
unfinished; validation of this batch is macOS arm64 only.

## Persistent global and eval lexical bindings

The validated implementation stores modern global `let`/`const` bindings in a
private environment retained by the owning global. They are absent from the
global object's properties, and functions from earlier scripts capture the same
environment. Reads, writes and `typeof` observe uninitialized bindings until the
declaration executes. Fresh eval records isolate eval-local declarations while
retaining captured closures. Non-strict eval checks intervening lexical records;
global declared-var history is independent of ordinary property deletion.
Explicitly selected legacy declarations and embedding object receivers retain
their existing paths. The historical `__parent__` accessor hides these private
records, as it already hides function/block environments.

Script metadata records the binding template in the atom map. Bytecode cache
version 59 includes that metadata and an atom-directed extended instruction for
binding initialization, including destructuring. Source reconstruction and XDR
cover wide atom indices. The split prolog/body JSAPI path gives the prolog its own
STOP instruction: truncating the script length alone did not stop the threaded
interpreter from executing the body. The body does not instantiate bindings a
second time.

Focused script checks pass 17 assertions. Native probes pass 55 compiler/scope,
49 record-lifetime and 13 wide-atom checks, including GC, multiple globals,
nonextensible globals, failed initializers, mixed caller editions, source/XDR
round trips and separate prolog/body execution. The contextual-keyword fixture
now assigns distinct global names to its independent shadowing cases; all 84
assertions remain. Real-window fixtures exercise lexical declarations across
separate scripts in both chrome and content globals.

The initial complete macOS arm64 diagnostic run passes **28,213 ES2015 modes**,
with **355 failures**, **14 unsupported modules** and no harness errors, crashes
or timeouts: **14 gained, zero lost**. All **11,540 ES5 cases** pass. These reports
use `artifacts/es6/global-lexical-compiler-*`. The final corrected runtime retains
those totals and the 14 gained/zero lost result. All four applications pass root
builds, packaging and relocated desktop checks, including Calendar's four views,
169 Browser navigation/layout checks and ChatZilla in Suite and XULRunner.
Final reports use `artifacts/es6/global-lexical-final-*`. All four engine hashes
match the frozen conformance runtime:
`28ca09fd4b1a86436944ece57598ac3cdb316c25c7b7bc00ad2bb0fa90b6ac21`.
C89 checks pass. Full ES2015 compliance remains unfinished; this batch's
validation is macOS arm64 only.


## Method home objects and super properties

Object methods and accessors retain their own traced home object; cloning,
computed methods, generators and nested arrows preserve it. Modern `super.x`
and `super[key]` support reads, calls, tagged/spread calls, assignment, compound
assignment, updates, destructuring targets and for-in/for-of targets. Direct
eval inherits method context; ordinary nested functions and indirect eval do
not. A private reference captures the base, receiver, key and strictness before
RHS callbacks. Base lookup follows the home object's current prototype, while
getter/setter calls retain the original receiver. Deleting a super property
throws ReferenceError. Class declarations and `super()` remain unfinished.

Cache version 60 stores the function kind needed to reconstruct home objects;
integer selectors extend the existing atom-directed instruction. The new tests
are `super-properties.js` (33 assertions), `TestMethodHome.c` (75),
`TestSuperReference.c` (36) and `TestSuperWide.c` (16). The wide probe exercises
66,000 distinct atoms, XDR, source reconstruction and collection in accessors.
It exposed a decompiler buffer relocation bug: copying an expression already
inside the growing buffer must relocate its source pointer as well. Thirty
repeated wide runs pass with allocation scribbling enabled. Real chrome/content
fixtures also execute super accessors, an escaped arrow and direct eval.

The initial complete isolated macOS arm64 run gains 16 cases with zero lost:
**28,229 pass, 339 fail, 14 unsupported**, with zero harness errors, crashes or
timeouts. All **11,540 ES5 cases** pass. The 1,020-case comparison against Node
agrees after fixing a recursive Proxy trap and comparing global identity rather
than host-specific global tags in the diagnostic fixture. Reports use
`artifacts/es6/super-property-first-*`. Final integrated reports under
`artifacts/es6/super-property-final-*` retain these totals and the 16 gained/zero
lost result. All four applications pass root builds, packaging and relocated
desktop checks, including Calendar's four views, 169 Browser navigation/layout
checks and ChatZilla in Suite and XULRunner. All four engine hashes match the
unchanged frozen conformance runtime:
`284ae2f98e5d99b54a3407afa01e520861ced00348641700dc23d96d50445149`.
C89 checks pass. Full ES2015 compliance remains unfinished; this batch's
validation is macOS arm64 only.


## Scripted setter assignment results

Standard-edition assignment retains the assigned value when a scripted setter
returns another value or returns implicitly. Explicit historical editions keep
their old result behavior unless the caller opts into strict mode. Native setter
hooks retain their embedding contract. The assigned value is rooted across
callbacks; the setter's exception still propagates.

`setter-result.js` passes 16 checks in default ES5 and ES2015 modes, covering
computed/inherited/primitive receivers, compound assignment, updates, descriptor
replacement and reentrancy. `TestSetterResult.c` passes 66 checks with collection
and mixed setter/caller editions. The 20-case object-literal diagnostic subset
passes completely. Final integrated reports under
`artifacts/es6/setter-result-final-*` record **28,231 ES2015 passes**, **337
failures**, **14 unsupported modules** and zero harness errors, crashes or
timeouts: **two gained, zero lost**. All **11,540 ES5 cases** pass. All four
applications pass build, package and relocated desktop checks, including
Calendar's four views, 169 Browser navigation/layout checks and Suite/XULRunner
ChatZilla. The frozen conformance runtime remains unchanged, and all four
engine hashes match: `fca775a9ee16720547e1373307306ccddd6dd85899649a6a6e73668d25577515`.
C89 checks pass. This batch is validated on macOS arm64 only; full ES2015
compliance remains unfinished.


## Classes and derived constructors

ES2015 class declarations and expressions now support heritage, default and
explicit constructors, instance/static methods, accessors, generators, computed
keys, inner class-name bindings and `super()` through arrows and direct eval.
Derived receivers remain uninitialized until a successful super call; object
returns, primitive-return errors and repeated initialization follow separate
paths. Constructor and method descriptors retain their ES2015 ordering.

Cache version 61 records the new function kinds and complete class source.
The public function decompiler retains class expressions, including nested and
Unicode source; enclosing script decompilation reconstructs class instructions.
Source recording covers memory and file token streams in ES2015 mode. Explicit
legacy editions keep their existing parser paths.

The first isolated full run records **28,509 passes, 59 failures, 14 unsupported
modules**, no harness errors/crashes/timeouts, **278 gained and zero lost**.
ES5 passes all **11,540 cases**. Subsequent class-name fixes pass **322/324**
class-related modes; the other two require parameter defaults. Reports are
`artifacts/es6/class-first-*` and `class-names-fixed-subset.json`. These are
intermediate results. Final integrated reports under `artifacts/es6/classes-reviewed-*`
record **28,519 passes, 49 failures, 14 unsupported modules**, zero harness errors,
crashes or timeouts: **288 gained, zero lost**. All **11,540 ES5 cases** pass.
Focused probes cover 53 class assertions, eight source cases, 47 native
constructor checks and 16 wide-operand/cache/collection checks. Ten repeated
wide runs pass with allocation scribbling; 157 additional class behavior cases
agree with Node. C89 checks pass.

Real content-window validation exposed an initialization path that looked up
internal constructor-state classes by name. Their objects now use the cached
built-in prototype, matching the other private engine records. All four macOS
arm64 applications pass root build, packaging and relocated desktop checks,
including Calendar's four views, 169 Browser navigation/layout assertions and
ChatZilla in Suite and XULRunner. All four engine hashes match the unchanged
frozen conformance runtime:
`b0a096716f708d35d39719f0426b603c9f657f75ba311be3da6c30af6e0bdfa5`.
Other platforms are not revalidated for this batch. Full parameter environments,
module execution and full ES2015 conformance remain unfinished.


## Catch variables and block function declarations (validation in progress)

A catch-local slot used by an initializer no longer suppresses its separate
outer `var` declaration in ES2015 scripts/eval. ES2015 also rejects duplicate
block function declarations according to its original
[13.2.1 early errors](https://262.ecma-international.org/6.0/#sec-block-static-semantics-early-errors);
explicit legacy and default editions retain their existing behavior. The
[catch-variable extension](https://262.ecma-international.org/6.0/#sec-variablestatements-in-catch-blocks)
applies to strict as well as non-strict cases. These three pinned failures were
implementation issues, not contradictory test metadata.

The final integrated macOS arm64 run passes **28,522 cases**, with **46 failures**,
**14 unsupported modules**, zero harness errors/crashes/timeouts, **three gained
and zero lost**. `catch-declarations.js` passes 18 checks, and the native wide
probe passes 16. The latter also exposed missing decompiler handling for wide
prolog declarations; they now retain the same source-note behavior as narrow
instructions. All **11,540 ES5 cases** pass. All four applications pass root
builds, packaging and relocated desktop checks, including Calendar's four
views, Browser navigation/layout (169 checks), and Suite/XULRunner ChatZilla.
The four application libraries and frozen conformance runtime share SHA-256
`4faae023331deddd2dcee1e37f1bdd3b56962b7d57211935dbd1eb03865b1330`.
C89 declaration/implicit-function checks pass. Cache version remains 61.
Reports use `artifacts/es6/catch-declarations-final-*`. Other operating systems
and architectures have not been revalidated for this batch.


### Parameter environments (integrated macOS arm64 validation)

ES2015 defaults and binding patterns now share a formal parser across ordinary
functions, methods, generators, arrows and dynamic Function constructors.
Parameters initialize left to right with TDZ checks, unmapped arguments,
initializer eval scopes, and separate body bindings when necessary. Defaults
run before a generator first suspends. The body owns a separately traced and
serialized initializer script so destructuring exception tables and IteratorClose
retain their normal offsets. The dynamic constructor parses formals and body
as separate streams. Legacy editions retain their existing parser path.

The isolated full run in `artifacts/es6/parameter-reviewed-*` records **28,568
passes, zero failures, 14 unsupported module cases**, and zero harness errors,
crashes or timeouts. All **11,540 ES5 cases** pass. This is not completion:
module compilation/linking/evaluation remains unsupported. Additional native
checks subsequently fixed destructured argument holes and duplicate binding
metadata in XDR, Unicode binding serialization, and raw parameter source output.
The wide initializer test covers 66,000 atoms, GC during script callbacks,
balanced script notifications, and cache/source round trips. Cache version is 62.
`default-parameters.js` passes 60 focused checks and `TestParameterWide.c`
passes 17 checks with 12 balanced script lifecycles. All four applications pass
root builds, packaging and relocated desktop checks, including Calendar's four
views, Browser navigation/layout (169 checks), and Suite/XULRunner ChatZilla.
The final packaged-runtime full run confirms **28,568 passes, zero failures,
14 unsupported modules**, zero other errors, **46 gained and zero lost**;
ES5 remains **11,540/11,540**. Reports use `artifacts/es6/parameters-final-*`.
All four libraries and the frozen runtime share SHA-256
`85ea999d5c43a23af1ffda28c7369bd6ba2db8d1ccbbfed569c61a8e9ec9a85d`.
C89 checks pass. Other platforms are not revalidated for this batch.

The pinned corpus also includes later rest binding-pattern cases. Those formals
are accepted without widening ordinary variable-declaration rest grammar; the
unchanged `array-rest.js` negative cases and all 56 rest-parameter checks pass.


### Modules (integrated macOS arm64 validation)

The native engine now compiles Unicode module source, records imports/exports,
instantiates private bindings, resolves live imports and re-exports through
cycles, and evaluates each dependency graph once. Missing or ambiguous named
exports fail linking. Star ambiguities are omitted from namespaces. Namespace
objects expose sorted live bindings and preserve the original ES2015 edition's
key iterator and definition restrictions (9.4.6 and 26.3); these differ from
later editions. Module `this` is undefined, including arrows/direct eval, and
module declarations never become properties of the application global.

Embedding APIs are additive: `JS_CompileUCModule`, `JS_GetModuleRequests`,
`JS_SetModuleDependency`, `JS_InstantiateModule`, `JS_EvaluateModule` and
`JS_GetModuleNamespace`. Root returned record/namespace objects through the
normal JSAPI. Compilation temporarily selects ES2015 and restores the caller's
edition; existing XUL/component loaders retain their classic script behavior.
The host supplies dependency records for each requested specifier before linking.
No filesystem/network resolver or HTML module-script loader is implied. Records
own their compiled scripts and are not ordinary XDR script-cache objects.

The shell exposes `compileModule(source, filename)`, `moduleRequests(record)`,
`linkModule(record, specifier, dependency)`, `instantiateModule(record)`,
`evaluateModule(record)` and `namespaceModule(record)`. Test262 harness setup
remains a separately compiled global script; module tests then use the actual
module compiler/evaluator. All 14 pinned module cases are negative cases, so
positive coverage is supplied separately by grammar, linking, namespace, lifetime
and embedding regressions. Do not equate 14 negative-test passes with a complete
module implementation. The full integrated results are recorded below.

A separate wide-declaration probe also covers more than 65,535 atoms before
exported var/function declarations. The native host probe checks dependency
request deduplication, live values, repeat evaluation, global isolation, context
edition restoration, namespace-only reachability and balanced script hooks
with collection during compilation. C89 declaration/implicit-function checks
pass. These checks do not replace the full conformance and application runs.

The later-coverage inventory is reproducible with `inventory-test262.py`, using
the original baseline and the clean later checkout at
`35d566604512cba908054eec49f85e64a59f3091`. It inventories all 48,912 files in
the same three ECMA-262 roots: 36,100 added paths, 10,684 changed bodies, and
2,128 identical bodies at retained paths. These are file comparisons, not test
passes or edition decisions. Every entry remains in the review queue. An
`es6id` is insufficient to determine the expected edition: for example, later
module namespace tests retain ES6 references while asserting changed namespace
symbol descriptors. The pinned historical run alone does not complete this
later-coverage review.

The frozen integrated run passes **28,582/28,582 pinned ES6 modes**, with **zero
failures, unsupported cases, harness errors, crashes or timeouts**: 14 gained
and zero lost. All **11,540 ES5 modes** pass. All four applications pass root
builds, packaging and relocated desktop checks, including Calendar's four
views, Browser navigation/layout (169 checks), and Suite/XULRunner ChatZilla.
All four libraries and the frozen runtime share SHA-256
`acc1c5edf759d2803a256403802a83902edffe433776c067a70d7f3c68815cfe`.
Reports use `artifacts/es6/modules-validated-*`. C89 checks and 27 runner
integration checks pass. Cache version remains 62. Other operating systems and
architectures were not revalidated for this batch.

This is the first complete pinned-corpus pass, not completion of the user's
full ES2015 coverage objective. The later `es6id` diagnostic ran 5,852 modes:
5,734 diagnostic passes, 109 failures and nine harness errors. Its edition
review and modern host support are incomplete, so these are not conformance
counts. Follow-up probes also found escaped module-contextual keywords,
export-list semicolon insertion, and top-level-arrow `new.target` gaps; fixes
are being validated separately. No upstream assertions or failing cases were
removed to obtain the complete historical pass.


The module-context follow-up corrects literal `as`/`from` parsing, export-list
semicolon insertion, and lexical `new.target` availability through arrows and
direct eval. Public namespace retrieval now propagates failed module records.
The integrated macOS arm64 run preserves **28,582/28,582 ES6** and
**11,540/11,540 ES5**, with zero failures or unsupported cases. All four
applications pass builds, packages and relocated desktop checks, including
Calendar views, Browser navigation/layout and Suite/XULRunner ChatZilla.
Reports: `artifacts/es6/module-context-final-*`. Focused coverage includes
9 contextual-keyword, 8 module-production and 12 `new.target` checks, plus
24 native module checks across five scripts. C89 checks pass. Other platforms
remain unvalidated for this batch; the later-test review remains open.
