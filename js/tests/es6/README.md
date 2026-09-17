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
The complete ES2015 corpus has not yet been rerun against these additions.
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
reports. No separate ES2015 execution environment has been implemented yet.
Parser/execution edition must survive nested compilation, XDR and callbacks;
global built-in initialization also needs a consistent compatibility policy.
Do not approximate edition selection by changing behavior only while a test
is running or by identifying Test262 sources.

For engine changes, rerun the complete pinned ES5 corpus, the affected ES2015
tests and focused regressions. Native embedding, chrome/content globals,
XULRunner, Suite, ChatZilla and Calendar application checks are separate gates.
Record architectures and runtime results explicitly; macOS arm64 results do
not establish MSVC2005, other architectures or legacy operating-system support.
