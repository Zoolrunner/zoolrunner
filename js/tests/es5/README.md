# ECMAScript 5.1 conformance work

The target is the complete [ECMAScript 5.1 specification](https://262.ecma-international.org/5.1/),
the corrected edition of ES5, in the existing SpiderMonkey implementation.
The pinned suite now passes all 11,540 required-mode cases on macOS arm64.
This is a measured test result, not a proof of every specification behavior.

Use the official TC39 historical Test262 ES5 branch, pinned to
`7da91bceb9ce7613f87db47ddd1292a2dda58b42`. Keep the external suite outside the
source tree; no test or runtime third-party library is bundled by this work.
Python 3 is needed only for this optional runner.

```sh
git clone --branch es5-tests https://github.com/tc39/test262.git /tmp/test262-es5
git -C /tmp/test262-es5 checkout 7da91bceb9ce7613f87db47ddd1292a2dda58b42
python3 js/tests/es5/run-test262.py \
  --suite /tmp/test262-es5 \
  --shell obj-zoolrunner-macos-arm64-xulrunner/dist/bin/xpcshell \
  --report /tmp/zoolrunner-es5.json
```

The runner uses the suite's own metadata parser and harness files. It runs each
chapter test in a fresh xpcshell process using the upstream mode annotations
and the upstream non-strict default for unmarked tests. Expected exceptions use upstream negative-test
patterns. Crashes, timeouts, harness initialization failures, and missing result
markers cannot pass. The unrelated internationalization suite is excluded.
Reports retain each test/mode result; `--filter` selects a path substring.
Do not rebuild or replace the shell's libraries during a run.

Initial baseline on macOS arm64, 2026-09-13, commit `f130af13`:

| Mode | Pass | Fail |
| --- | ---: | ---: |
| Non-strict | 6,852 | 4,042 |
| Strict | 6,696 | 4,439 |
| Total | 13,548 | 8,481 |

There were no crashes, timeouts, or harness errors in these 22,029 cases. At that baseline, strict
mode was not implemented: passing a test prefixed with `"use strict"` does not
establish that strict semantics worked. Test262 is evidence, not an exhaustive
proof of specification compliance. Browser, Suite, and embedding regressions
must also be checked after engine changes. Windows 95/NT 4.0 runtime validation
remains separate from the macOS results.

Initial focused changes add native `Array.isArray` and
`Date.prototype.toISOString` (including invalid dates and expanded years), and
fix sparse initial accumulators and full-width callback indices in
`reduce`/`reduceRight`. The focused native regression can be run with:

```sh
DYLD_LIBRARY_PATH="$PWD/obj-zoolrunner-macos-arm64-xulrunner/dist/bin" \
obj-zoolrunner-macos-arm64-xulrunner/dist/bin/xpcshell \
  -f js/tests/es5/array-date.js
```

Require `ES5-ARRAY-DATE checks=50 failures=0`. Use the corresponding executable
and library path on Linux/Windows. Focused Test262 results after these changes:
`toISOString` 30/30; `Array.isArray` 46/48 (two cases require the missing JSON
object). These do not replace the full-suite baseline or resolve the remaining
strict-mode, descriptor, JSON, binding, parsing, and other built-in failures.

The full rerun after the array/date implementation passed 13,666 cases and
failed 8,363: 118 new passes, no regressions, crashes, timeouts, or harness
errors against the baseline. A subsequent built-in function correction adds
an internal non-constructor flag, suppresses implicit `prototype` creation for
the new methods and `Function.prototype`, and preserves the historical
`JSFunctionSpec` layout. After that correction, all 50 focused assertions pass
in XULRunner and Suite; the 720 Function chapter cases retain the same 440
passes and 280 failures. The full-suite counts above precede that final flag
correction; the focused Array and Date Test262 results remain unchanged.

The object-query milestone adds native `Object.getPrototypeOf`, `Object.keys`, and
`Object.getOwnPropertyDescriptor`. Descriptor queries preserve accessor identity
without invoking getters. Prototype queries hide the engine's internal shared
closure objects and retain embedding access checks. Argument indices are now
enumerable, function `prototype` is non-enumerable, and the global `undefined`,
`NaN`, and `Infinity` properties are read-only as required by ES5. Ordinary
global `var` redeclarations leave those properties unchanged; legacy `const`
bindings keep their redeclaration checks. Read-only globals are excluded from
the interpreter optimization that writes directly to global slots. String property
names `"-0"` and `"0"` remain distinct.

Array literal elisions now leave holes instead of writing `undefined`, and
literal elements define own properties without invoking inherited setters.
Explicit `undefined` elements still exist. A dedicated internal hole bytecode
preserves decompilation; the bytecode cache version changes with it.

Run `object-reflection.js` with the same xpcshell command above, substituting
its filename. Require `ES5-OBJECT-REFLECTION checks=101 failures=0`. These 101
assertions and the 50 array/date assertions pass in the macOS arm64 XULRunner
and Suite builds. At that milestone, descriptor-writing methods, extensibility controls, and
strict semantics remained unfinished; these read-only queries do not substitute
for those implementations.

`TestObjectEmbedding.c` exercises the classic JSAPI access-control callback and
an XDR encode/decode/execute round trip containing sparse array literals. It
also checks a global class without the optional global flags, and preserves
legacy destructuring-`const` redeclaration errors. On
macOS, after the normal build:

```sh
clang -DXP_UNIX -DJS_THREADSAFE -DMOZILLA_1_8_BRANCH \
  -Iobj-zoolrunner-macos-arm64-xulrunner/dist/include/js \
  -Iobj-zoolrunner-macos-arm64-xulrunner/dist/include/nspr \
  js/tests/es5/TestObjectEmbedding.c \
  -Lobj-zoolrunner-macos-arm64-xulrunner/dist/bin -lmozjs \
  -o /tmp/zoolrunner-es5-embedding
DYLD_LIBRARY_PATH="$PWD/obj-zoolrunner-macos-arm64-xulrunner/dist/bin" \
  /tmp/zoolrunner-es5-embedding
```

Require `ES5-EMBEDDING checks=7 failures=0`; all seven checks pass against both
XULRunner and Suite libraries. Both builds pass all 169 existing layout assertions after these engine
changes; the Suite also passes live HTTPS navigation.

The object/array-literal full rerun passes **14,397 of 22,029** cases, with
**7,632 failures**, no crashes, timeouts, or harness errors. Relative to the
array/date full rerun, 732 cases newly pass and one previous pass disappears.
That case, `15.3.5.4_2-96gs`, requires a TypeError for strict-function caller
access: it previously passed accidentally because the missing descriptor method
threw a TypeError. It now exposes the unfinished strict-mode behavior. Six real
global-redeclaration regressions found in the intermediate run were corrected
and pass again in this rerun.

This full rerun precedes the final two legacy compatibility adjustments for
unflagged embedding globals and destructuring `const`; both are covered by the
seven native embedding checks. It does not establish complete ES5 conformance.

Descriptor-writing work adds native `Object.defineProperty`, `defineProperties`,
`create`, `getOwnPropertyNames`, `preventExtensions`, `seal`, `freeze`, and their
integrity queries. Non-extensibility has a separate scope flag from the legacy
`JS_SealObject` behavior. Descriptor conversion roots values across user getters
and completes before `defineProperties` starts writing. Replacing an accessor
uses a complete descriptor rather than the legacy half-accessor merge. Native
access checks remain in the property-definition path.

The first full descriptor run passes **19,924 of 22,029**, with **2,105 failures**
and no crashes, timeouts, or harness errors. It precedes subsequent corrections
for array indices, mapped arguments, inherited accessor ownership, and descriptor
enumeration. Four prior passes disappear because strict-caller negative tests
previously accepted the TypeError from a missing `defineProperty`; those tests
still require strict-mode implementation. This is not complete conformance.

`object-descriptors.js` exercises descriptor defaults, SameValue, accessor
replacement, conversion order with garbage collection, inherited properties,
two-phase definition, integrity controls, sparse array shrinking and failed
shrinks, mapped arguments, and RegExp `lastIndex` storage and read-only writes.
Require `ES5-OBJECT-DESCRIPTORS checks=68 failures=0`.
The embedding check also verifies that descriptor writes respect the access
callback and preserves the legacy `JS_SealObject` boundary; its expected count
is now nine. All 68 descriptor assertions, 151 existing JavaScript assertions,
and nine embedding checks pass in both macOS
arm64 XULRunner and Suite builds. Both applications pass the 169 layout
assertions; the Suite also passes live HTTPS navigation. Windows runtime
validation is separate and has not been performed for these changes.

The final XULRunner full run passes **20,010 of 22,029** cases and fails **2,019**,
without crashes, timeouts, or harness errors. Compared with the previous object-query
baseline, 5,617 cases newly pass and four accidental strict-caller passes disappear
(as explained above). This full run precedes property-name allocation growth,
temporary-root, and scope-lock review changes. The complete Suite rerun after
those changes produces the same **20,010 passes and 2,019 failures**, with no
crashes, timeouts, or harness errors. Subsequent guards for RegExp conversion
roots and already-sealed legacy scopes pass the focused JavaScript and embedding
checks. The full conformance target remains unfinished.

The focused final Object chapter passes 5,535 of 5,711 cases; Array passes
4,397 of 4,555; RegExp passes 1,040 of 1,088 with no new failures against the
preceding full run. The allocation review also checks 50,000 own property names
and shrinking 20,000 sparse non-enumerable array indices. Clang's static analyzer
reports no diagnostics in `jsobjes5.c`; this is not a security audit or proof of
conformance. No failing Test262 cases or strict-mode runs have been excluded.


## Native JSON, binding, and receiver semantics (2026-09-14)

`jsjson.c` implements the ES5 JSON grammar without evaluating source, reviver
walking, serialization with replacers and indentation, and cycle detection.
JSON participates in eager and lazy standard-class initialization without changing
public runtime layouts or the serialized JSProto enumeration. `Date.toJSON`,
`Function.bind`, `String.trim`, general object argument lists for `apply`, and
null/undefined receiver validation are implemented in the existing engine.
Bound functions preserve the XPConnect reserved slots, trace their bound state,
and forward construction and `instanceof` to their target. Native built-in
methods do not acquire constructor behavior. Numeric and quoted accessor names
are accepted by the existing parser.

The receiver/String stage passed 20,400 of 22,029 Test262 cases, with 1,629
failures and no crashes, timeouts, or harness errors; it introduced no failures
against the preceding 20,010-pass run. The initial native JSON chapter run passes
all 206 cases. These are intermediate measurements, not conformance completion.
The combined byte-loader run passes 21,021 cases and fails 1,008, with no crashes,
timeouts, or harness errors. It gains 627 passes and loses six accidental passes
where a missing `bind` previously threw the TypeError intended for a strict
caller-access check. The Unicode measurements below supersede this byte-loader result.

Run `json-bind-string.js` alongside the previous three focused scripts. Its
71 assertions cover grammar rejection, Unicode whitespace, callback ordering,
reviver deletion, JSON prototype handling, cycles, generic Date serialization,
bound calls/construction, poisoned accessors, and collection while values are
reachable only through engine state. The earlier 219 assertions still pass in
the XULRunner build. All 290 assertions and 13 embedding checks pass in both macOS arm64 builds.
The embedding check covers lazy/eager JSON and cloning a bound function. Suite
live HTTPS navigation and all 169 layout assertions pass after waiting for the
browser document shell to exist before starting the integration probe.


Unicode source transport was corrected in the runner: the historical shell
`load` path maps each file byte to a code unit and cannot load the upstream UTF-8
source faithfully. The new shell `evaluate` entry point uses the Unicode global
script compiler. The driver passes the complete unmodified source through an
ASCII-escaped transport string, and the preflight checks a supplementary Unicode
character. Reports label this transport explicitly. Earlier byte-loader totals
remain historical measurements and must not be presented as Unicode-conformance
results. No test assertions or failure annotations are changed.


## Upstream mode policy and strict-mode implementation

The pinned upstream `tools/packaging/test262.py` sets `--unmarked_default` to
`non_strict` with an explicit comment that not all tests are strict-compatible.
The local runner now follows that policy and still runs every `onlyStrict` case
in strict mode. `--unmarked-default both` preserves the earlier 22,029-case
experiment as an additional diagnostic. For example, `S15.3.4.4_A3_T6.js`
requires a null call receiver to become the global object and has no mode tag;
forcing it into strict mode contradicts ES5 10.4.3. No test source, assertion,
or upstream annotation is altered. Earlier totals used both modes for unmarked
cases and are not the required-mode conformance total.

Before strict-mode changes, Unicode diagnostic runs completed at 21,047 passes /
982 failures in the Suite and 21,061 / 968 in XULRunner. The latter includes the
shared ES5 whitespace predicate and the last bound-state guards. Neither run
had crashes, timeouts, or harness errors. Strict-mode parsing and execution are
implemented in the following stage; those diagnostic totals are intermediate
measurements.


## Strict execution and remaining conformance work (2026-09-14)

Strict function/script metadata now survives bytecode serialization. Directive
prologues, strict bindings and early errors, raw call receivers, isolated direct
eval environments, indirect global eval, strict arguments snapshots, and shared
poison accessors are implemented in the existing interpreter. Property writes
and deletes enforce strict failures, including the property-cache path.
Library corrections include callback receivers, operand conversion order,
object literal property definitions, regexp validation, array result properties,
and ISO date parsing. Historical explicit `__proto__` initializers remain
supported; callable RegExp objects and implicit RegExp input remain available
only with an explicitly selected historical language version.

Required-mode Unicode runs progressed through 11,278/11,540, 11,389/11,540,
and 11,470/11,540 passes (262, 151, and 70 failures respectively), with no
crashes, timeouts, or harness errors. These intermediate runs precede the latest
array/parser corrections and the harness separation below. They are not a
completion claim. `strict-mode.js` adds 46 regression assertions; all 336 focused
JavaScript assertions passed in the Suite engine at that intermediate point.
The final application and embedding results are recorded below.

The harness now compiles separately from each test, in the same global object.
Concatenating harness declarations before a test masks a test's own directive
prologue, including `10.1.1-2gs`, `-5gs`, and `-8gs`. The test source is unchanged.
Negative tests follow their actual metadata exception pattern; an intentional
`$FAIL` is the expected outcome of the historical negative `S12.5_A2` test.
Harness initialization is outside the test's exception handler and cannot
satisfy a negative test. The default timezone is `America/Los_Angeles`, matching
fixed epoch expectations in the historical Date constructor tests; use
`--timezone` for separate portability runs. Reports record timezone and harness
layout alongside source transport, suite revision, and mode policy.


## Complete pinned-suite pass

Both final macOS 15.7.1 arm64 builds pass **11,540 / 11,540**, with zero
failures, crashes, timeouts, or harness errors. The external suite checkout is
clean at the pinned revision. Each full run contains 10,894 non-strict cases
and all 646 annotated strict cases. The final builds also pass all 394 focused
assertions, 15 embedding checks, and 169 layout assertions. Suite live HTTPS
navigation passes, and the temporary Suite profile is unregistered with the
previous profile selection restored. Windows runtime validation remains
separate.

[Conformance results](conformance-results.json) records the suite revision,
runner hash, source patch hash, full-report hashes, environment, mode counts,
and integration totals. No test source, assertion, or metadata was changed.
These are complete passes of this pinned suite, not an exhaustive proof of the
entire specification.

`strict-mode.js` now has 55 assertions, including primitive getters, setters,
method calls, parenthesized non-directives, and decompilation/recompilation of
strict functions. `library-edge-cases.js` adds 49 assertions for ISO parsing,
regexp flags and ranges, accessor arity, URI noncharacters, missing optional
arguments, generic array mutation, and conversion ordering. Together with the
four earlier files, this gives 394 focused JavaScript assertions. Run all six
files with separate `-f` arguments and require each `failures=0` summary;
xpcshell can print a script error without returning a nonzero process status.

The embedding program now has 15 checks, covering strict JSAPI function clones
and a bytecode round trip containing a strict function and sparse array.
Eager arguments creation exposed an uninitialized instruction pointer in an
inline frame before a resolve callback. Initializing the frame before exposure
fixes the reproduced crash; 30 consecutive Suite embedding runs pass. The
XULRunner embedding check and 169 layout assertions also pass. Static analysis
still reports historical diagnostics; this work is not a security audit.


Run the focused checks against either build:

```sh
DYLD_LIBRARY_PATH="$PWD/obj-zoolrunner-macos-arm64-suite/dist/bin" \
obj-zoolrunner-macos-arm64-suite/dist/bin/xpcshell \
  -f js/tests/es5/strict-mode.js \
  -f js/tests/es5/library-edge-cases.js \
  -f js/tests/es5/json-bind-string.js \
  -f js/tests/es5/object-descriptors.js \
  -f js/tests/es5/object-reflection.js \
  -f js/tests/es5/array-date.js
```

Require six `failures=0` summaries with counts 55, 49, 71, 68, 101, and 50.
Build and run `TestObjectEmbedding.c` as described above; require 18 checks.

### Window bootstrap / ChatZilla regression (2026-09-14)

Installing Object static methods via `JS_GetConstructor(Object.prototype)`
during bootstrap triggered embedding access checks before DOM globals were
ready. Object initialization aborted, leaving inherited methods and ES5 static
methods missing. Registering the same methods through JS_InitClass's static
function table avoids that premature script-visible lookup. ChatZilla source
and historical APIs are unchanged.

`TestObjectEmbedding.c` initially gained a sixteenth check here. That lazy-bootstrap check
runs with an embedding callback denying constructor access and reproduces the
failure with the previous engine. Existing protected-object checks still verify
that reflection honors embedding access restrictions.

The standalone window test includes 12 bootstrap assertions across chrome and
content globals: inherited hasOwnProperty, ES5 static methods, constructor identities
and prototype chains, and legacy watch/__defineGetter__ methods. Run with a
fresh temporary profile (the test exits the application):

```sh
profile=$(mktemp -d /tmp/zool-window-test.XXXXXX)
DYLD_LIBRARY_PATH="$PWD/obj-zoolrunner-macos-arm64-xulrunner/dist/bin" \
MOZ_NO_REMOTE=1 obj-zoolrunner-macos-arm64-xulrunner/dist/bin/xulrunner-bin \
  "$PWD/js/tests/es5/window-app/application.ini" -profile "$profile"
```

Require `WINDOW-BOOTSTRAP checks=17 failures=0`. The profile may contain runtime
files; remove that temporary directory after inspecting results.

Unmodified ChatZilla 0.9.86.1 initializes with its input widget in Suite and in a
temporary standalone XULRunner wrapper, using fresh profiles and no configured
IRC startup URLs. Standalone packaging remains separate work. These smoke tests
do not establish IRC connectivity or universal historical-app compatibility.
At this stage both macOS builds passed 12 window assertions, 16 embedding
checks, 394 focused JavaScript assertions, and 169 layout assertions. The latest
expanded counts are recorded below. Suite live HTTPS
homepage structural navigation also passes.

After the bootstrap fix, both macOS engines again pass all 11,540 pinned
Test262 cases, with zero failures, crashes, timeouts, or harness errors. Updated
report hashes and validation counts are in `conformance-results.json`.

### Historical application syntax and object scopes (2026-09-14)

Calendar exposed ES5 regressions in unchanged component and XBL scripts.
`legacy-application.js` checks 58 assertions across JS 1.5/1.6/1.7/1.8 and the
default version: legacy regexp continuations, accessor argument lists, and data/accessor overrides
remain accepted in explicitly selected historical language versions, while
ES5-default and strict syntax validation remains active. Component/subscript
loaders already select the historical version. Unversioned XUL scripts now
select JS 1.7 as well, restoring Composer's escaped regexp line breaks without
weakening ordinary HTML/ES5 parsing. EOF after a regexp escape remains an error
in every mode. Application scripts need no edits.
Run it with `xpcshell -f js/tests/es5/legacy-application.js` and require
`LEGACY-APPLICATION checks=58 failures=0`.

The embedding test now requires 18 checks. It includes a strict method called
by an unqualified name from a plain embedding object scope, reproducing the
lost receiver in the earlier interpreter. Internal eval and named-function
scopes are explicitly declarative; arbitrary embedding objects retain their
implicit receiver. The window fixture now requires 17 checks, including a real
XUL command handler's element method and a strict global function. An external,
unversioned XUL script checks historical setters, property overrides, and regexp
continuations, reproducing the unchanged Composer script-loading failure. Existing
strict-mode checks cover the distinct strict-eval receiver behavior.

Run Calendar's eight existing unit tests without modifying their sources:

```sh
python3 calendar/test/run-compatibility.py \
  --shell obj-zoolrunner-macos-arm64-calendar/dist/bin/xpcshell \
  --library-path obj-zoolrunner-macos-arm64-calendar/dist/bin \
  --report-dir /tmp/zool-calendar-tests
```

The runner gives each test an isolated profile/storage directory and supplies
the two assertion helpers missing from the older bundled harness. Omit
`--library-path` when testing a packaged runtime. Require
`CALENDAR-COMPATIBILITY tests=8 failures=0`. Native macOS packaging runs these
tests for Calendar, and legacy syntax/embedding checks for all four runtimes.
Fresh-profile Calendar GUI validation additionally checks startup, all four
views, forward/backward navigation, and absence of JavaScript console errors.
These checks complement Test262; they do not prove every old application works.

To repeat the macOS Calendar GUI check from a desktop session:

```sh
python3 calendar/test/run-window-compatibility.py \
  --archive artifacts/zoolrunner-macos-arm64-calendar-sdk11.3.tar.gz \
  --report /tmp/zool-calendar-window.log
```

It extracts a private runtime, registers only a test overlay, and uses a fresh
profile. Require `CALENDAR-WINDOW views=4 failures=0`. The installed application
and user profiles are not used. This desktop check is separate from hosted CI.

The remaining Composer scripted-close observer errors are fixed in the native
commands updater. It stops notifying and scheduling work when its document or
docshell is being destroyed, including when an editor reference survives window
closure. Notification groups recheck state between observer calls. Application
scripts remain unchanged.

A Venkman crash exposed a raw script-list iterator surviving reentrant debugger
callbacks. JSD now snapshots reference-counted script wrappers before callbacks
and skips wrappers invalidated by GC or debugger shutdown. Run
`debugger-lifecycle.js` through xpcshell and require
`DEBUGGER-LIFECYCLE checks=5 failures=0` (metadata, surviving scripts, callback
shutdown/restart, and invalid-argument checks). This runs in native packaging.
See [application lifecycle tests](../../../editor/composer/tests/README.md) for
the 24-check GUI regression covering repeated Composer edit/undo/close cycles
and Address Book, Inspector, and Venkman startup/close with no console errors.

Final engine validation after these compatibility fixes: both Suite and
XULRunner pass all 11,540 pinned Test262 cases, 394 focused JavaScript assertions,
58 legacy-language assertions, 18 embedding checks, 17 window assertions, and
169 layout assertions. All eight SDK 11.3 macOS builds/packages were refreshed;
native packages pass reflection, legacy-language, and embedding checks, with
Calendar additionally passing its eight existing unit tests. Updated report
hashes and application lifecycle results are recorded in
`conformance-results.json`.
