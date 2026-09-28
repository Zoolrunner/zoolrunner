# Speedometer 2.1 compatibility

Target: all 16 enabled workloads and the default ten iterations of
[Speedometer 2.1](https://browserbench.org/Speedometer2.1/), without rewriting
the benchmark, adding page polyfills, or disabling failing workloads. The
upstream-disabled FlightJS mail client is not one of the 16 enabled workloads.
Language additions are limited to ES2015. Preserve historical JavaScript
modes, embedding interfaces and unchanged Mozilla/XULRunner applications.

Status: implementation and validation in progress. No full benchmark pass or
complete standards conformance is claimed. Existing developer binaries may
predate the source; use a fresh build and record the compiler and architecture.
Test native LoongArch Linux Suite first. Browser, Calendar and XULRunner testing
follows only after the complete benchmark runs in Suite.

The Suite runner uses a temporary profile and profile-local chrome registration.
It observes the original benchmark's completion callback, retains console errors
and results, and requires 16 workloads, ten iterations and all expected steps.
Missing result markers, crashes and timeouts fail. It does not change measured
workload functions or their iteration counts. Use a private X server on Linux:

```sh
sh build/linux/with-display.sh python3 layout/html/tests/speedometer21/run-suite.py \
  --runtime obj-speedometer21-suite/dist/bin \
  --url http://127.0.0.1:8000/Speedometer2.1/ \
  --report artifacts/speedometer21/suite.json
```

Serve an unchanged external benchmark copy with its complete resource tree.
Keep upstream assets outside tracked source files and record their hashes.
The runner emits JSON plus a sibling `.log` file. A benchmark completion is
separate from visual rendering validation and focused specification tests.

The same runner can load ordinary content regression pages that expose
`testDone`, `testFailures`, and `testResults`:

```sh
sh build/linux/with-display.sh python3 layout/html/tests/speedometer21/run-suite.py \
  --runtime obj-speedometer21-suite/dist/bin --mode probe --timeout 60 \
  --url "file://$PWD/layout/html/tests/style/dom-selectors.html" \
  --report artifacts/speedometer21/selectors.json
```

The selector regression covers document, element and fragment queries,
detached trees, XML case sensitivity, static NodeLists, group de-duplication,
ancestor matching outside the queried subtree, malformed selectors and
visited-link privacy. The implementation reuses the engine's CSS selector
grammar; this test does not establish support for every Selectors feature.
Relevant specifications are [Selectors API](https://www.w3.org/TR/selectors-api/),
[Selectors Level 3](https://www.w3.org/TR/selectors-3/), and
[ECMAScript 2015](https://262.ecma-international.org/6.0/).

Remaining work includes the native ES2015 workload, animation frame callbacks,
DOM token lists and event constructors, further framework dependencies, and
CSS layout/painting features. Re-evaluate this list against actual benchmark
failures as support progresses. Preserve the full pinned ES5.1 run and the
existing embedding, chrome/content, application lifecycle and layout probes.

Pre-merge native LoongArch Suite results (GCC 15.3.0, 2026-09-28): the first
selector fixture passed 41 checks and the first window-binding fixture passed
10 checks. Additional conversion regressions are pending revalidation. The
unmodified benchmark still fails at startup: `benchmark-report.js` uses a
default parameter, and the UI requires `console`. The initial full ES5.1 run
passed 11,538 cases with two URI-test timeouts at the ten-second limit; rerun
the full suite with a longer limit after the build is stable.

The initial Suite lifecycle run passed Composer, Address Book and Inspector
checks, then crashed during Venkman layout. The core shows an invalid style
context in `nsFrame::Init`, consistent with the frame-arena constructor issue
already documented for other GCC targets. LoongArch mozconfigs now carry
`-flifetime-dse=1 -fno-strict-aliasing`; a fresh rebuild and lifecycle rerun are
required before reporting this fixed. Other applications have not been tested.

The subsequent merge of `origin/master` at `790b4ab7` brings in the upstream
ES2015 implementation and native DOM selectors, classList, console, dataset,
optional computed-style and event-capture bindings. The upstream implementations
replace the overlapping initial local implementations. The local content
regressions and Suite benchmark harness remain in place; all merged behavior
requires fresh native LoongArch validation. The pre-merge results above do not
establish the status of the merged tree.
