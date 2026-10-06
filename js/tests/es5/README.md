# ECMAScript 5.1 testing

The pinned historical Test262 suite has recorded passes of **11,540 / 11,540
required-mode cases**. Preserve this gate alongside historical application
compatibility. A finite suite does not prove every specification behavior.

[Documentation index](../../../docs/README.md) · [ES2015 tests](../es6/README.md) ·
[Detailed instructions and validation history](DETAILS.md)

## Run the complete suite

You need Python 3, a built ZoolRunner `xpcshell`, and the pinned upstream suite
outside this checkout. Run from the repository root, replacing the shell path
with your matching build:

```sh
git clone --branch es5-tests https://github.com/tc39/test262.git /tmp/test262-es5
git -C /tmp/test262-es5 checkout 7da91bceb9ce7613f87db47ddd1292a2dda58b42
python3 js/tests/es5/run-test262.py \
  --suite /tmp/test262-es5 \
  --shell obj-zoolrunner-macos-arm64-xulrunner/dist/bin/xpcshell \
  --report /tmp/zoolrunner-es5.json
```

Keep the shell and its libraries unchanged throughout the run. Use the matching
platform's runtime library environment as described in its build guide.

## Interpret the result

- Unmarked cases run non-strict; `onlyStrict` cases run strict. Optional
  `--unmarked-default both` runs are separate diagnostics, not the required gate.
- The default timezone is `America/Los_Angeles`, required by historical fixed-date
  cases. Use `--timezone` for separately reported portability checks.
- The runner preserves Unicode source and compiles harness setup separately.
  Crashes, timeouts, harness errors and missing completion markers cannot pass.
- `--filter` is useful for diagnosis; a filtered pass cannot replace a full run.

## Check applications too

Engine changes also need focused regressions, native embedding tests and real
chrome/content globals. Use `window-app/application.ini`, unchanged historical
applications and the platform package runners. Test262 alone cannot establish
application compatibility.

The [detailed reference](DETAILS.md) retains native compile commands, fixture
expectations and the implementation history. Useful entry points:

- [Complete pinned-suite result](DETAILS.md#complete-pinned-suite-pass)
- [Window bootstrap and ChatZilla](DETAILS.md#window-bootstrap--chatzilla-regression-2026-09-14)
- [Historical application syntax and scopes](DETAILS.md#historical-application-syntax-and-object-scopes-2026-09-14)
- [Linux AArch64 matrix](DETAILS.md#linux-aarch64-matrix)

Recorded results apply to their named revisions and platforms. Keep old failures
and intermediate counts in the detailed record, with their original context.
