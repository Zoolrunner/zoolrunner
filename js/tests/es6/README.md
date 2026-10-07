# ECMAScript 2015 testing

The complete pinned historical corpus has recorded passes of **28,582 / 28,582
modes**, including modules. Preserve the [ES5.1 gate](../es5/README.md), classic
JSAPI and historical application compatibility alongside this work.

The historical suite, later-edition diagnostics and application tests measure
different things. The pinned pass does not prove exhaustive specification
coverage or validate every platform and subsequent revision.

[Documentation index](../../../docs/README.md) ·
[Implementation and validation reference](DETAILS.md)

## Run the complete suite

You need Python 3, PyYAML and a built ZoolRunner `xpcshell`. Keep Test262 outside
the source tree. From the repository root, substitute your matching shell path:

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

Keep the shell and libraries unchanged during a run. The runner selects ES2015
with `-E -v 2015`, verifies Unicode transport and defaults to the recorded
`America/Los_Angeles` timezone. Unmarked tests run in both strict and non-strict
mode, unlike the ES5 corpus. Module and async cases remain part of the gate;
failures, unsupported cases, crashes, timeouts and harness errors cannot pass.
`--filter` runs only a diagnostic subset.

## Select the language edition

Use `xpcshell -E` to initialize modern built-ins and `-v 2015` for modern scripts.
Native embeddings select ES2015 before `JS_InitStandardClasses`. Changing a
script's edition alone does not replace its global's built-ins.

`javascript.options.content.es2015` enables modern non-privileged HTML content.
It defaults to true in Browser and Suite, and remains false in Calendar and
XULRunner. Existing explicit user values take precedence over these defaults.
The Speedometer runner selects the edition in its disposable profile.
Classic XUL/component loaders and explicit script editions are unchanged. Native
module APIs do not imply an HTML module-script loader or ordinary module XDR.

## Results and remaining work

Use these sections of the detailed record to find reports and their scope:

- [Completed macOS arm64 Suite gate](DETAILS.md#completed-es2015-gate-on-the-macos-arm64-suite)
- [Cross-platform validation](DETAILS.md#current-cross-platform-validation)
- [macOS arm64 application matrix](DETAILS.md#current-macos-arm64-application-matrix)
- [Later Test262 diagnostics and host controls](DETAILS.md#later-coverage-diagnostics)
- [Corpus, mode policy and runner details](DETAILS.md#reproducible-initial-baseline)
- [Feature implementation and compatibility gates](DETAILS.md#implementation-and-compatibility-gates)

The completed Suite gate records original-edition conformance work separately
from a nonzero later-corpus diagnostic. Feature tags do not determine edition
requirements; retain every diagnostic result and exact-source review. Broader
later-test inventory and cross-platform coverage have their own limits in the
linked records. Earlier entries describe intermediate implementation gaps.

After engine changes, run the complete pinned suites, relevant focused/native
regressions and affected packaged applications. Shell results cannot replace
window bootstrap, navigation, Calendar views or historical application checks.
