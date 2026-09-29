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

Remaining work includes validating the native ES2015 workload, animation frame
callbacks and event constructors, further framework dependencies, and
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

The disposable runner profile enables `javascript.options.content.es2015` by
default (use `--content-edition es5` for the historical default). This native
preference selects ES2015 before initializing non-privileged HTML window
built-ins and for unversioned HTML scripts. Explicit script editions still
win; privileged HTML and XUL keep their existing edition selection. The
preference defaults to false in ordinary application profiles. Set it before
navigation, rather than changing it during a document's lifetime. The benchmark
files are unchanged. `style/content-edition.html` exercises this opt-in boundary;
it is separate from the default-edition layout probes.

Merged native LoongArch GTK2 Suite validation (2026-09-28, GCC 15.3.0):
48 selector assertions, 12 window-binding assertions, 23 Event-constructor
assertions and seven opt-in edition assertions pass. The full pinned ES5.1
run passes all 11,540 cases with zero failures/timeouts/crashes/harness errors.
The unchanged Suite lifecycle runner passes all 24 checks, including Venkman
startup and close, with the corrected compiler flags. The unchanged benchmark
now reaches its first workload and stops at the missing `requestAnimationFrame`.
The complete pinned ES2015 run passes all 28,582 modes with zero failures,
unsupported modes, timeouts, crashes or harness errors. These results cover Suite
only and do not establish complete CSS, DOM or language conformance.

Native animation-frame scheduling now batches callbacks before layout and
painting, preserves cancellation through callback reentry, defers newly queued
callbacks to a later frame and clears callbacks during window teardown. The
LoongArch runtime uses CLOCK_MONOTONIC timestamps; Windows/macOS timing paths
are compiled conditionally but have not been validated in this work. The
animation probe passes 17 assertions in Suite. The next benchmark blocker is
`Element.getBoundingClientRect`; complete workload execution remains pending.

Further native LoongArch GTK2 Suite validation: client geometry passes 33
assertions, live class-name collections pass 29, and inert HTML-document
creation passes 31. The implementations use native layout boxes, live content
lists, and an additive DOMImplementation interface. Created HTML documents
have HTML namespaces, a live head getter, and no active window; classic parsed
HTML and XHTML retain their existing document modes. Transform and SVG geometry
remain unvalidated. These focused fixtures are also in the layout probe list.

The native ES2015 driver now accepts `loongarch64`. With the Suite runtime it
passes 139 focused JavaScript fixtures, 83 native probes and the complete
28,582-mode pinned ES2015 suite. The unmodified benchmark passes the first five
workloads (15 steps), then Ember requires `createHTMLDocument`; the subsequent
benchmark run with that implementation is pending. No other application has
been tested during this Suite-first stage.

With inert HTML documents available, Ember next requires `insertAdjacentHTML`.
The native fragment-parser binding passes 21 Suite assertions covering all four
positions, conversion order, existing node identity, inert scripts, table and
XML contexts, malformed XML and standard exceptions. Historical XPCOM exception
names remain unchanged; the new API supplies its standard exception name.
Full benchmark completion remains pending.

Native CustomEvent construction and `document.createEvent("CustomEvent")` now
use an additive interface, leaving the historical `nsIDOMCustomEvent` unchanged.
The Suite fixture passes 29 assertions, including object detail identity,
reinitialization during/after dispatch and survival through forced garbage
collection. Its GC step uses the Suite debugger service through the probe
runner. Native roots preserve event data, but collection of cycles involving
`detail` has not been validated and remains a lifecycle limitation to resolve.

Ember startup also exposed an independent LiveConnect failure: inspecting
`Packages` with no usable Java VM returned false from a native resolver without
setting an exception. The script then stopped without running catch/finally.
`liveconnect-unavailable.html` reproduces the timeout before the fix and passes
four checks afterward on this host. The resolver now supplies a catchable error
if the Java bridge did not already set one. LiveConnect and its historical APIs
remain available; no benchmark or application source changes are required.
This fixture requires a host without a usable Java VM and is not in the generic
layout probe list. Timeout reports now include frame DOM and Ember boot/queue
state, and the runner captures plain console messages as well as script errors.

After the LiveConnect correction both Ember workloads complete their steps.
The debug workload exposed Symbol-key handling in DOM helpers: Symbol keys
must not be converted to numeric indices or document/storage names, and Window
must forward them to its inner global. The opt-in `dom-symbol-keys.html` probe
passes 28 assertions across collections, Document and Window after reproducing
Document/Window failures before the correction. It is separate from the
ES5-default layout list. The benchmark next fails in Backbone's hidden iframe
fallback because the newly inserted iframe has no `contentWindow`.

The optional capture flag is now handled consistently by native
`removeEventListener` and `addEventListener`, preserving the historical fourth
add-listener argument. Twelve removal/identity/conversion assertions pass in
Suite. A small hidden-iframe fixture passes eight assertions both from file
and HTTP, including synchronous insertion during parsing; Backbone's nested
case still needs diagnosis. `--debug-errors FILE_SUBSTRING` enables debugger
throw-stack diagnostics; do not use its timings as benchmark measurements.

The Backbone iframe failure was recursive navigation, not missing frame
creation: after `document.open/close`, assigning the hidden frame's hash
reloaded the original application and repeated until the frame-depth limit.
Anchor recognition now compares the public URL of generated documents and
runs before the optional presentation-shell scrolling step. The extended
hidden-iframe probe reproduces two failures before this correction and passes
all ten checks afterward. The Suite lifecycle runner also passes all 24 checks
again, including Composer, Address Book, Inspector and Venkman. The frame-depth
limit and LiveConnect support remain unchanged.
