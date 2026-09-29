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
28,582-mode pinned ES2015 suite. The unmodified benchmark reports the first five
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

After the LiveConnect correction both Ember workloads report completed steps.
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

Generic Event dispatch now selects listeners by the event type name without
casting a generic native structure to a keyboard/mouse structure. Event
wrappers retain their first window realm through an additive weak-owner
interface, preserving identity and expandos across same-origin frames. The
native Suite probe passes 50 assertions for cross-frame data, capture,
cancellation, listener removal and nested dispatch; Suite lifecycle passes
all 24 checks.

The benchmark runner now checks TodoMVC item and checked counts after each
measured step. Earlier step totals alone did **not** establish that the
operations succeeded. Validation currently finds failures in deletion and
React/Ember item creation; full benchmark compatibility remains incomplete.
The local mirror also needed Angular's dynamically loaded, unchanged upstream
`todomvc-index.html`, now recorded in its SHA-256 manifest.

Programmatic button/input clicks now use explicit targets, including successive
clicks after a prior target was removed. Native dispatch retains event data for
script-held wrappers; detached/removed nodes can bubble to their retained
parent. Completed event objects can be dispatched again, while simultaneous
redispatch is rejected and cancellation is retained. Existing event handler
properties resolve to null before assignment, and `oninput` uses the native
handler machinery. Suite passes 26 click assertions, 16 handler assertions,
nine redispatch assertions, and the existing Event/CustomEvent/cross-frame
probes (23/29/50). The Suite lifecycle checks pass all 24 assertions, and
unchanged ChatZilla initializes successfully.

Unattended benchmark profiles disable the interactive content slow-script
prompt; the runner's external hard timeout still applies. Per-step item checks
are logged as they become available. The previous run reached 42 steps before
a slow-script modal during Elm startup; it is not a benchmark pass.

The Suite window bootstrap also passes all 19 checks, including unchanged
legacy XUL accessor/regexp syntax and chrome/content global initialization.

The next first-iteration diagnostic reached 45 measured steps: 44 passed the
item-count checks, while Angular 2 deletion left 100 items. All three Elm steps
completed with the unattended profile. Flight initially stopped on missing AMD
resources in the external mirror; after fetching unchanged upstream resources,
it exposed the missing `localStorage` API.

Native local storage now uses origin-separated persistent storage with lossless
UTF-16 key/value transport, including NUL and lone surrogates. Its initial
HTTP-only regression is `style/local-storage.html`; serve it over loopback HTTP
and use the probe command above. Historical `globalStorage` and `sessionStorage`
retain StorageItem results and separate data. Flight's isolated one-iteration
workload passes its add/complete/delete item checks. This is not a full benchmark
pass. Storage event delivery, prototype method dispatch, modern exception
mapping, process-restart persistence and quota boundaries remain unvalidated or
incomplete. Encoded local strings currently consume four quota units per UTF-16
code unit. The native DB encoding is private to the new local-storage namespace.

The runner escapes UTF-16 report strings to ASCII before native console output,
so NUL and lone-surrogate regression results survive the historical output
transport. Invalid result JSON fails validation rather than aborting reporting.

The local-storage HTTP fixture passes all 41 assertions on native LoongArch
GTK2 Suite, including six legacy storage compatibility assertions. Suite window
bootstrap passes all 19 checks. Invalid DOM method receivers also pass six
focused checks after a null-safe wrapped-native query correction.

Angular 2's isolated workload now passes add/complete/delete checks after fixing
native event-method inheritance. EventTarget had been exposed as an interface
name without a shared prototype; per-instance event methods also hid prototype
replacements. EventTarget now supplies shared native operations, and Node,
Window and XMLHttpRequest inherit them while retaining their XPCOM interfaces.
Document fragments use the existing native dispatch machinery and expose their
event-target interfaces. Window setup runs after context initialization and
keeps legacy named-property lookup off the shared EventTarget prototype.

The 53-assertion prototype fixture and all eleven existing event/window fixtures
pass in native LoongArch GTK2 Suite. These cover event construction, cross-frame
identity, clicks, handler properties, redispatch, listener removal, hidden
iframes, Symbol keys, CustomEvent/forced GC, and invalid receivers. Angular's
polyfill still logs a missing HTMLMediaElement interface; this remains a feature
gap even though its isolated TodoMVC operations succeed. Full ten-iteration
benchmark validation remains pending.

After the prototype changes, Suite lifecycle passes all 24 checks, chrome/content
window bootstrap passes all 19, and unchanged ChatZilla initializes and shuts
down successfully. Other applications remain deferred until Suite completion.

The created-document title setter now updates the DOM using the HTML/SVG title
rules, and the getter handles an SVG document element. Historical parsed HTML
uses its existing title path. The new title fixture passes 31 assertions, the
original createHTMLDocument fixture still passes 31, and 25 argument-conversion
checks pass after XPConnect stops replacing exceptions thrown by script with
generic conversion errors. Suite lifecycle passes 24 checks and the unchanged
legacy-application JavaScript fixture passes 58. The title algorithm is specified
in [HTML document tree accessors](https://html.spec.whatwg.org/multipage/dom.html#document.title).

Storage methods now live on the shared prototype and select the correct native
behavior for their receiver: localStorage returns strings/null, while historical
globalStorage/sessionStorage retain StorageItem results and range errors. The
HTTP storage fixture passes 54 assertions, including borrowed methods, legacy
key enumeration and conversion exceptions. All 24 Suite lifecycle checks pass;
Flight's isolated three-operation workload also passes on this implementation.
Historical prototype methods remain non-enumerable; complete modern property
metadata/descriptor conformance and storage-event delivery remain unfinished.

The probe runner accepts repeated `--restart-url URL` arguments to launch new
processes with the same disposable profile. This option is restricted to probe
mode and requires every stage to pass. Each restart has a separate log. Serve
`style/storage-persistence.html` on two loopback ports and run:

```sh
sh build/linux/with-display.sh python3 layout/html/tests/speedometer21/run-suite.py \
  --runtime obj-speedometer21-suite-merged/dist/bin --mode probe --timeout 30 \
  --url 'http://127.0.0.1:18762/storage-persistence.html?write' \
  --restart-url 'http://127.0.0.1:18763/storage-persistence.html?isolation' \
  --restart-url 'http://127.0.0.1:18762/storage-persistence.html?read' \
  --report artifacts/speedometer21/storage-persistence.json
```

All 15 assertions across three separate Suite processes pass, including
persistent NUL/lone-surrogate keys and values, empty keys, isolation by port,
and unchanged legacy domain storage shared across ports. This does not yet
validate quota boundaries, opaque origins or shutdown memory accounting.

The HTTP `storage-cache-clear.html` probe exercises three cookie-clearing
notifications in one process, including cached keys and same-origin frames.
All 27 checks pass on the native LoongArch GTK2 Suite (13 failed before the
cache fix). Modern backends stay registered for repeated clearing, and shutdown
drops cached StorageItems without deleting persistent values. The separate
three-process persistence/origin probe still passes all 15 checks, and Suite
lifecycle passes all 24. This does not establish complete storage-event or
quota conformance.

`ui-event-cancel-bubble.html` verifies that reading the propagation flag does
not change event phase, cancellation, bubbling, or the active-dispatch guard.
The native LoongArch GTK2 Suite passes all 12 checks (nine failed before the
fix), alongside 24 Suite lifecycle and 19 window bootstrap checks.

`class-list-methods.html` covers 73 DOMTokenList method, conversion, receiver,
attribute-spelling, selector, cloning and forced-GC assertions. Run it through
the Speedometer probe runner, which services its collection request. Native
LoongArch GTK2 Suite passes all 73, plus 690 checks across 26 existing content
fixtures, 24 Suite lifecycle checks, ChatZilla startup/shutdown and 58 legacy
JavaScript checks. HTML class attributes retain their original UTF-16 spelling
separately from selector atoms; the historical XUL atom-array path remains intact.
The method corrections follow the [DOMTokenList algorithms](https://dom.spec.whatwg.org/#interface-domtokenlist).
In particular, no-token updates preserve an absent attribute as required by
the specification; Chromium 148 creates an empty attribute in this case and
therefore fails the two related assertions in the reference run.
Shared DOMTokenList prototype bindings, indexed properties and iteration remain
unverified/incomplete; these results do not claim full interface conformance.
