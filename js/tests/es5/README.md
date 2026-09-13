# ECMAScript 5.1 conformance work

The target is the complete [ECMAScript 5.1 specification](https://262.ecma-international.org/5.1/),
the corrected edition of ES5, in the existing SpiderMonkey implementation.
This is unfinished engine work, not a claim of current conformance.

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
chapter test in a fresh xpcshell process in strict and non-strict modes, subject
to the upstream mode annotations. Expected exceptions use upstream negative-test
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

There were no crashes, timeouts, or harness errors in these 22,029 cases. Strict
mode is not implemented: passing a test prefixed with `"use strict"` does not
establish that strict semantics worked. Test262 is evidence, not an exhaustive
proof of specification compliance. Browser, Suite, and embedding regressions
must also be checked after engine changes. Windows 95/NT 4.0 runtime validation
remains separate from the macOS results.

Current focused changes add native `Array.isArray` and
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

The next implementation adds native `Object.getPrototypeOf`, `Object.keys`, and
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
and Suite builds. Descriptor-writing methods, extensibility controls, and
strict semantics remain unfinished; these read-only queries do not substitute
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
