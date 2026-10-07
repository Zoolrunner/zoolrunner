# Detailed reference and validation history

[Quick guide](README.md)

## Speedometer 1.0 baseline (2026-10-06)

The separate `--benchmark-version 1.0` gate reproduced the reported step-three
hang in the existing macOS arm64 Suite at source baseline `dbbbf0fd`, with the
ordinary ES5 content preference. The live, unchanged benchmark completed the
three VanillaJS steps, then Ember Data 1.0.0-beta.6 reported `state has no
properties` at line 4984. `artifacts/speedometer1/before.json` and its log retain
the failed run. This is not a passing benchmark result. The 1.0 gate requires
seven workloads, twenty iterations and 420 successful state checks; the 2.1
requirements remain sixteen workloads, ten iterations and 480 checks.

This reference preserves the original instructions, implementation notes and
validation reports. Read results in revision order: an older failure or pending
check may have been resolved later, and a pass does not validate later changes.
Commands run from the repository root.

<details>
<summary>Browse reference sections</summary>

- [History state bindings (native LoongArch Suite)](#history-state-bindings-native-loongarch-suite)
- [Same-document history traversal](#same-document-history-traversal)
- [Complete GTK2 Suite benchmark result and remaining work](#complete-gtk2-suite-benchmark-result-and-remaining-work)
- [Other application validation after the Suite gate](#other-application-validation-after-the-suite-gate)
- [Synthetic XHR dispatch and callback regressions](#synthetic-xhr-dispatch-and-callback-regressions)
- [Completed native application matrix (2026-09-29 baseline)](#completed-native-application-matrix-2026-09-29-baseline)
- [Dataset liveness diagnostic (known failures)](#dataset-liveness-diagnostic-known-failures)
- [Completed Xlib benchmark and corrected XHR matrix](#completed-xlib-benchmark-and-corrected-xhr-matrix)
- [Dispatch package gates and descriptor baseline](#dispatch-package-gates-and-descriptor-baseline)
- [Live dataset binding, first native pass](#live-dataset-binding-first-native-pass)
- [Dataset interface and realm checks](#dataset-interface-and-realm-checks)
- [HTML attribute names in CSS](#html-attribute-names-in-css)
- [Full benchmark at the native dataset baseline](#full-benchmark-at-the-native-dataset-baseline)

</details>

# Speedometer 2.1 compatibility

Target: all 16 enabled workloads and the default ten iterations of
[Speedometer 2.1](https://browserbench.org/Speedometer2.1/), without rewriting
the benchmark, adding page polyfills, or disabling failing workloads. The
upstream-disabled FlightJS mail client is not one of the 16 enabled workloads.
Language additions are limited to ES2015. Preserve historical JavaScript
modes, embedding interfaces and unchanged Mozilla/XULRunner applications.

Status (2026-09-29): complete native LoongArch Suite runs pass on **GTK2 and
Xlib**, each with 16 enabled workloads, ten iterations, all 480 workload checks,
the original completion callback and process exit zero. The GTK2 pass used the
frozen initial History-binding build; Xlib used the relocated package from
`989aaa00`. Both are preserved baselines, not claims about every later commit.
All eight application/backend package gates passed before the subsequent XHR
changes. The XHR implementation and state-notification correction pass focused
checks across all eight current native builds, with a separate GTK2 full run
using the dispatch implementation still in progress. Complete standards
conformance and rendering remain unfinished. See the result/provenance sections
below; earlier progress entries retain their historical status.

The Suite runner uses a private HOME, temporary profile and profile-local chrome
registration, including a private legacy profile registry. Simultaneous runs do
not edit the caller's application profiles.
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
For long runs, keep the HTTP server owned by the launcher:

```sh
sh build/linux/with-display.sh python3 layout/html/tests/speedometer21/run-local-benchmark.py \
  --runtime obj-speedometer21-suite/dist/bin \
  --serve-root artifacts/speedometer21/upstream \
  --report artifacts/speedometer21/suite-local.json
```

This wrapper serves the files unchanged, records HTTP requests in a sibling
`.http.log`, and defaults to a three-hour deadline. The observer still requires
the complete result; losing the server never counts as a benchmark pass. The
first current-Xlib rerun stopped at 111 checks after its independent server
exited; its incomplete report is retained separately from the owned-server run.
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

After the Suite benchmark gate, the same content runner can be used in Toolkit
applications without changing their sources:

```sh
sh build/linux/with-display.sh python3 layout/html/tests/speedometer21/run-toolkit.py \
  --application browser --runtime obj-browser/dist/bin \
  --url "file://$PWD/layout/html/tests/style/css-set-property.html" \
  --report artifacts/browser-css.json
```

`--application` also accepts `calendar` and `xulrunner`. The launcher copies the
runtime into a disposable directory, registers test chrome there, and uses a
private HOME and profile. Calendar uses the existing test-only command-line
component; XULRunner uses a test application manifest. Neither installed runtime
files nor user profiles are modified. `--restart-url` adds another probe in a
new process using the same disposable profile. Both HTTP fixture drivers accept
`--application`, defaulting to the unchanged Suite launcher. Other application
results are recorded separately from successful Suite tests.
The first native Browser GTK2 run passes all 985 content, 300 History and
159 storage/XHR assertions. Its package/application gate is still running;
Calendar and XULRunner launcher execution has not yet been verified. These
reports are `browser-gtk2-content.json`, `browser-gtk2-history/` and
`browser-gtk2-http/` under `artifacts/speedometer21/`.
For privileged application fixtures, use `--chrome-probe PATH` instead of
`--url`; the XUL fixture must emit the same `SPEEDOMETER-RESULT` JSON marker.
The Browser preferences lifecycle fixture passes 28 checks in its development
runtime and relocated GTK2 package. It reports the old homepage localization
mismatch separately rather than claiming every preferences behavior is correct.

Remaining work includes the complete benchmark and original results UI,
remaining DOM behavior and CSS layout/painting features, followed by the other
application builds and runtime tests. The native ES2015 workload, animation
frame callbacks and basic event constructors have passed focused checks and
workload iterations; those results do not establish exhaustive conformance.
Preserve the full pinned ES5.1 run and the
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

`box-sizing.html` adds 58 tests for the standard property and `style.boxSizing`,
including specified/computed values, initial/unset/inherit, priority replacement,
alias ordering, escapes, invalid input, cloning, width/height and min/max sizing.
The legacy `MozBoxSizing` interface and `padding-box` grammar remain available;
standard `box-sizing` rejects that legacy value. Declaration serialization keeps
the winning spelling. New accessors and strict CSSOM parsing use additive
interfaces, preserving the historical interface vtables.

Native LoongArch GTK2 Suite passes these tests and 748 checks across 27 content
fixtures, 24 Suite lifecycle, 19 window bootstrap and ChatZilla startup/shutdown.
`box-sizing-paint.html` paints content/border boxes, minimum sizing and the
zero-content-size floor. Its 400-by-280 content region matched Chromium 148
pixel-for-pixel when loaded in a normal Suite Navigator window. The minimal
probe chrome window was blank in screenshot capture, so it was not used as
painting evidence. This is bounded validation of
[box sizing](https://www.w3.org/TR/css-sizing-3/#box-sizing), not complete CSS
sizing, application-matrix or Speedometer rendering conformance.

The probe XUL windows now load `chrome://global/skin/`, as ordinary application
windows do. Without it, the minimal harness produced a blank screenshot even
though content layout assertions executed. With the theme loaded, the native
LoongArch GTK2 Suite passes 821 checks across 28 content fixtures, and the full
400-by-300 box-sizing painting fixture matches Chromium pixel-for-pixel. Earlier
benchmark runs using the unthemed harness remain functional workload evidence,
not evidence of on-screen painting or comparable performance.

`event-reinitialization.html` exercises 36 assertions across Event, UIEvent, MouseEvent
and CustomEvent. Reinitialization replaces the bubbling/cancelable flags, clears
cancellation and the target, and does nothing during dispatch (including each
derived initializer's own fields). `defaultPrevented` is exposed through an
additive interface, preserving existing event interface vtables. The focused
fixture and 24 Suite lifecycle checks pass on native LoongArch GTK2. Dispatch
propagation reset and stopImmediatePropagation still require separate work;
this is not a claim of complete Event conformance. MouseEvent initialization
also retains its relatedTarget argument. Chromium 148 passes 34 of the 36
assertions, retaining the old target in two reinitialization cases; those two
expectations follow the explicit target-reset step in the
[DOM event initialization algorithm](https://dom.spec.whatwg.org/#concept-event-initialize).

The previous long benchmark stopped at 408 successful workload checks when its
local HTTP server exited. Its retained log is incomplete and is not a benchmark
pass. A fresh full run uses the themed harness and a frozen runtime; other
application testing remains deferred until Suite completes the benchmark.

`event-propagation-state.html` adds 47 assertions for Event, UIEvent and
CustomEvent propagation. It checks stopping before dispatch, immediate versus
ordinary stopping, reset after dispatch, redispatch, cancelBubble setters, and
retaining a native click event past its original stack-backed dispatch. The
additive propagation interface leaves existing interface vtables intact. Native
LoongArch GTK2 Suite passes all 904 checks across 30 content fixtures after this
change. The native event copy is now made after dispatch flags are cleared;
otherwise a retained event could incorrectly remain marked as dispatching.

`event-type-strings.html` passes 23 checks for event names with embedded NULs,
lone surrogates and non-ASCII characters, including distinct listener lookup and
reinitialization. The event getter now retains the entire DOMString, and replacing
an event type releases its previous allocation. The shared `nsStringKey` hash
uses its stored length instead of truncating and mutating that length at NUL.
Its native 19-check regression reproduced 12 failures before the correction and
passes afterward. Native LoongArch GTK2 Suite passes all 927 content assertions
across 31 fixtures with the correction.

The first successful relocated GTK2 Suite package gate passes 11,540 ES5.1 cases,
28,582 ES2015 modes, 139 focused fixtures, 83 native ES2015 probes, both 2,000-call
XPTCall probes, native embedding and Suite navigation/window/lifecycle/ChatZilla
checks. Reports are in `artifacts/speedometer21/package-suite-gtk2-abi-fixed`.
That package includes the ABI and initial event-state fixes, preceding the later
mouse relatedTarget, propagation and event-name corrections. Its engine hash is
unchanged; later DOM/XPCOM checks are recorded separately above. Xlib package
validation and the full themed benchmark are still running.

The Xlib Suite package gate also passes all 11,540 ES5.1 cases and 28,582 ES2015
modes, 139 focused fixtures, 83 native ES2015 probes, both ABI probes, embedding
and application checks (`package-suite-xlib-propagation`). This package includes
propagation fixes and predates the subsequent string-key correction. The latest
Xlib development runtime passes the same 927 assertions across 31 content
fixtures as GTK2. Its 400-by-300 box-sizing painting region matches the same
Chromium reference with zero differing pixels. Other applications remain untested
in this Speedometer effort, pending the full themed Suite benchmark.

Client rectangle bounds now keep the first fragment separately from the union
of nonzero extents. An initial zero-size point no longer enlarges a later
nonempty box; when every fragment has zero width or height, the first fragment
is retained as required by the
[CSSOM View bounding-box algorithm](https://drafts.csswg.org/cssom-view/#dom-element-getboundingclientrect).
`TestClientRectBounds.cpp` passes 10 direct boundary cases, including fractional
coordinates, all-empty lists and degenerate lines. The existing 33 content
geometry checks and 24 Suite lifecycle checks also pass on native LoongArch
GTK2. SVG/transformed geometry and other layout gaps remain outside this result.

The historical storage event now exposes the shared event state and propagation
interfaces without changing its original domain-based initializer. Both legacy
initializers leave the domain unchanged during dispatch. Serve
`style/legacy-storage-event.html` over HTTP and run it in probe mode: its 36
assertions capture a real session-storage notification, exercise both historical
initializers, cancellation, immediate stopping and redispatch. Native LoongArch
GTK2 passes all 36; the prior binding threw on the inherited state getter.
This fixture requires an HTTP origin (file content has no sessionStorage in this
platform). It does not establish modern StorageEvent constructor or cross-window
local-storage notification support.

`style/xhr-event-lifetime.html` requests its own source over HTTP and retains
both load and progress events beyond their callbacks. All 11 checks pass on
native LoongArch GTK2 after making the native event data owned before script
notification and initializing the progress event's type. Before this correction,
reading the retained load type accessed expired stack data and failed, and the
progress type was uninitialized. Serve this fixture over HTTP in probe mode.
The existing legacy progress position/totalSize API is retained. Broader XHR
event dispatch semantics and modern progress interfaces remain to be completed.
The 24 Suite lifecycle checks and all three isolated AngularJS TodoMVC operations
also pass with this ownership correction. The ongoing full benchmark uses an
earlier frozen runtime and does not validate this later change.

XHR load, progress and error notifications now mark dispatch state, expose the
at-target phase, honor immediate stopping, reject reinitialization during
callbacks, and clear currentTarget/phase/propagation afterward. The progress
wrapper forwards shared event interfaces while preserving its own XPCOM identity
and legacy progress interface. Parser errors get a separate request event rather
than reusing an event still dispatching on its document. These notifications are
non-bubbling and non-cancelable.

Run the HTTP fixtures with a temporary loopback server (including a controlled
connection reset for the network-error case):

```sh
python3 layout/html/tests/speedometer21/run-http-probes.py \
  --runtime obj-speedometer21-suite-merged/dist/bin \
  --reports artifacts/speedometer21/http-probes
```

Native LoongArch GTK2 passes all 155 assertions across its five fixtures: local
storage, legacy storage events, XHR event lifetime, dispatch and network errors.
The dispatch fixture reproduced 12 failures before the correction. General XHR
EventTarget registration/dispatch, callback ordering, and modern ProgressEvent
construction/data fields remain separate gaps; these results are bounded event
notification coverage, not complete XHR conformance.
The 24 Suite lifecycle checks also pass after these dispatch changes.
Xlib passes the same 155 HTTP assertions. Chromium 148 passes 37 of the 39
dispatch checks; it retains the stop-propagation flag after the two XHR
notifications, whereas the DOM dispatch algorithm explicitly clears that flag.
The expectations remain unchanged. The lifetime fixture also deliberately checks
the historical `position` property, which Chromium does not expose.

The sixth HTTP fixture, `xhr-listener-registration.html`, adds four assertions
for handlers registered after open and removing error listeners independently
of load listeners. It reproduced three failures before the correction. Native
LoongArch GTK2 passes all 159 HTTP assertions after correcting the error-listener
array and enabling channel progress notifications when a handler is added after
open, while preserving unrelated channel flags.

The full themed ten-iteration run completed all 480 workload checks successfully,
then failed in the original results callback: `style.setProperty(name, value)`
incorrectly requires a third priority argument in the historical binding.
Therefore this is still **not a full benchmark pass**. The blocked run and a
newer run sharing that binding were stopped; their logs and blocker reports are
retained. Other applications remain deferred until the original results UI also
completes.

Painting inspection exposed opaque rendering of RGBA backgrounds. Rectangular
backgrounds now composite through native image alpha, with corrected transparent
view classification and rounded output channels in GTK2/Xlib. The strict
132,000-pixel fixture passes on both backends, including viewport clipping.
Xlib initially crashed in its alpha scaler; correcting dimensions, source offsets
and clipped buffer strides resolves that crash. Both backends pass all 24 Suite
lifecycle checks afterward. Rounded translucent fills, borders, text and canvas
propagation remain unverified/incomplete; see the layout probe guide.

The CSS setter correction passes 43 focused assertions; both native LoongArch
Suite backends pass 970 content assertions and 24 lifecycle checks. A diagnostic invoking the original
results callback with explicitly synthetic input then reaches another blocker:
`history.pushState` is absent. That diagnostic is only UI coverage, never a
benchmark result. Session-history implementation and a fresh complete run remain
required. A separate constructor-shadowing probe reproduces an older computed-
style bootstrap failure; it remains recorded in the layout guide.

The first history-state change adds an independent `nsISHEntryState` interface
without altering any historical session-entry vtable. Entry clones retain the
immutable serialized payload independently of later replacement or clearing.
Its ten native xpcshell assertions pass in GTK2 and Xlib Suite builds and are
included in the Linux package runner. This storage foundation does not yet
expose pushState/replaceState or establish traversal/serialization support.

An immutable native structured-value serializer now supplies the next history
foundation. Its 63 native checks pass on both LoongArch Suite backends, with
11,540 pinned ES5.1 cases and 28,582 pinned ES2015 modes passing on the same
engine binary. See the ES2015 test guide for supported value types and limits.
It introduces no JavaScript global or post-ES2015 language feature. History
bindings, same-document traversal and popstate integration remain unfinished.

### History state bindings (native LoongArch Suite)

`pushState`, `replaceState` and the `state` accessor now use the native
serializer and additive docshell interface. Existing History and docshell
vtable layouts remain intact. State entries preserve document identity and
clone the child-entry tree; initial blank documents replace their current
entry. URL changes enforce the document-URL rewrite restrictions in the
[HTML History algorithm](https://html.spec.whatwg.org/multipage/nav-history-apis.html#the-history-interface).
The inner window traces the deserialized state without an unconditional GC
root. Reusing a subframe entry for a different document clears its old state.

Run the HTTP regressions with:

```sh
python3 layout/html/tests/speedometer21/run-history-probes.py \
  --runtime obj-suite/dist/bin --reports artifacts/history-probes
```

GTK2 and Xlib each pass **120 assertions** across state values/descriptors/URL
restrictions (76), child realms and detachment (16), initial blank documents
(11), forced GC (10), and replacement-navigation/stale receivers (7). The
original session-entry shell regression still passes all ten checks, and both
backends pass all 24 Suite lifecycle checks. Reports are under
`artifacts/speedometer21/history-http-{gtk2,xlib}/` and
`history-{suite,xlib}-lifecycle.stdout`. File-URL state restrictions also pass.

The original benchmark results callback and summary/details navigation now
pass a four-check diagnostic with explicitly synthetic input. The subsequent
complete run against the frozen earlier binding build also passed, as recorded
below. The state snapshot facility still lacks DOM-specific
serialized types. These are partial History improvements, not full API
conformance. Other application validation has now started.

### Same-document history traversal

Modern `back`, `forward` and `go` calls now queue traversal, restore a fresh
serialized state and dispatch a trusted `PopStateEvent` without reloading a
same-document entry. Child traversal uses the joint session history. Reloading
with `go()` retains serialized state, while explicit legacy JavaScript keeps
its original noninteger-argument handling. Historical XPCOM History interfaces
retain their signatures. Event state uses the owning realm's native weak map,
including retained events after frame teardown, without invoking mutable
JavaScript WeakMap methods.

The expanded history runner covers state bindings (82), realm checks (16),
initial documents (11), GC (10), entry replacement (7), PopStateEvent (44),
top-level traversal (80), child traversal (32), reload (12) and explicit legacy
calls (6). GTK2 passes all **300 HTTP assertions**. Xlib passes the first 282
over HTTP and the 18 reload/legacy assertions over file URLs. Reports are in
`artifacts/speedometer21/history-traversal-http-{gtk2,xlib}/`,
`history-reload-xlib.json` and `history-legacy-xlib.json`.
Both updated Suite backends also pass 970 existing content assertions, 159
networking/storage HTTP assertions, all 24 lifecycle checks and ten native
session-entry checks. Their identical JavaScript library hash remains
`1d29abb799a6522dc58735d5274b00304a10a48b3cbe79a67ffa3b5a3e360782`,
the library from the complete 11,540-case ES5.1 and 28,582-case ES2015 passes.
These DOM changes therefore did not require repeating unchanged engine gates.
Artifacts use `history-{gtk2,xlib}-content-probes.json`,
`history-{gtk2,xlib}-http/` and `history-traversal-*-lifecycle.stdout`.

This does not finish the History API: fragment/hashchange integration,
cross-document/BFcache event behavior, ancestor activity checks and DOM
constructor shadowing still need work. No broader application or complete
specification result follows from these focused tests.

### Complete GTK2 Suite benchmark result and remaining work

`artifacts/speedometer21/benchmark-history-full.json` records a successful full
run, finished at **2026-09-29 07:53:14 UTC**, with all 480 workload postconditions,
16 suites in each of ten result sets, and exit zero. The observer sets its
completion flag only after the original benchmark completion callback returns;
the earlier final-UI exception is therefore no longer present. Upstream workload
sources, iteration counts and enabled-suite selection were unchanged. This is
functional execution evidence, not a visual conformance or performance claim.

The tested runtime is `artifacts/speedometer21/benchmark-history-runtime`, frozen
after `aa2e08c8` plus the initial uncommitted native History bindings. Its
`libmozjs.so` SHA-256 is
`6e24a6fba4c8b3410929eebe1b3c6992f33575e804cfd384f4c26a97b6f25bd6`.
It predates the later history/PopStateEvent fixes and `989aaa00` CSS bootstrap
fix. Those changes have their separate focused/regression results above. The
current Xlib package is running a fresh complete benchmark; do not label it
passed until its completion report exists. Browser GTK2 compilation has started;
Browser, Calendar and XULRunner runtime compatibility is still unverified for
these changes.

An additional diagnostic extracted 284 distinct standard-property declarations
from the 31 CSS files requested by the benchmark. Native GTK2 accepts 263 and
rejects 21. One rejection is the upstream typo `background-repat` and must remain
rejected. The other 20 declarations expose missing appearance, box shadows,
transforms/origins, transitions/durations/properties and word-break support.
Parser acceptance does not establish correct painting: text shadows, rounded
transparency and other existing rendering gaps still require visual checks.
The diagnostic files are `loaded-css-paths.txt`, `loaded-css-declarations.json`
and `loaded-css-parser-gtk2.json` under `artifacts/speedometer21/`; it excludes
vendor-prefixed declarations and is not an exhaustive standards inventory.

The current GTK2 and Xlib builds also pass 12 checks each using the completed
run's real measurements in the original results UI. Summary/details/home and
queued Back/Forward navigation use the page's unchanged handlers without a
document reload (`recorded-results-{gtk2,xlib}.json`). This replay is a UI
regression check, not another benchmark run. The GTK2 summary screenshot
`recorded-summary-suite.png` and Chromium reference
`recorded-summary-chromium.png` confirm that the score is displayed, but the
Suite gauge needle stays upright because CSS rotation is missing. They also
show a border around the linked logo image that the reference omits. Full
painting equivalence is therefore not claimed.

### Other application validation after the Suite gate

Native Browser GTK2 compilation and the complete relocated-package gate now
pass, including both pinned language suites, native/embedding checks and real
Browser navigation/window checks (`browser-package-gtk2/`). Its separate
preferences compatibility probe passes 28 assertions with the documented
pre-existing default-localization limitation. Calendar GTK2 compilation and its
complete package gate now pass, including eight unit-test groups, startup and
all four views. XULRunner GTK2 compilation passes and its full package gate is
running. The three other applications' Xlib validation remains in progress.

Calendar GTK2 passes 985 content and 300 History assertions. The shared CSS
fixtures retain all assertions but now use ordinary linked stylesheet/image
files, because Calendar intentionally excludes the `data:` protocol. Its
unchanged cookie-disabled configuration also lacks the permission service, so
the original allowed-storage/event tests correctly encounter security denial.
Those failed reports remain in `calendar-gtk2-http/`; they are not counted as
storage conformance passes. With explicit `--storage-policy deny`, all nine
legacy denial checks and 69 unchanged XHR assertions pass in
`calendar-gtk2-http-policy/`. The driver defaults to the full allowed-storage
checks and never switches policy implicitly. Application code and build feature
selections were not changed to obtain these results.

XULRunner GTK2 also passes 985 content and 300 History assertions. Its existing
extension list omits the permission-manager extension despite enabling Necko
cookies. A real chrome-global diagnostic confirms that the permission-manager
contract is absent, with storage enabled and cookie preferences at their normal
defaults. Accordingly its initial allowed-storage reports in
`xulrunner-gtk2-http/` record security denial, not storage conformance. Explicit
denial-policy validation passes nine checks plus all 69 XHR assertions in
`xulrunner-gtk2-http-policy/`. No application or build configuration was changed.
The unchanged built ChatZilla extension also passes twelve startup and XBL
input assertions as a standalone XULRunner application
(`xulrunner-gtk2-chatzilla.json`); see the
[standalone test instructions](../../../../extensions/irc/tests/README.md).

XULRunner GTK2's complete relocated-package gate now passes, including both
pinned language suites and native embedding/window checks. Browser Xlib builds
and passes 985 content, 300 History and 159 HTTP assertions, but its first
package run found a native crash in font enumeration while opening preferences.
The retained debugger trace is `browser-xlib-preferences-gdb.log`. The fix uses
an owned screen device instead of an unowned cached font-context pointer.
Both Browser backends now pass twelve direct font checks and thirty preferences
checks, including actual font-menu population; Xlib Suite passes the font
fixture too. Package validation is rerunning after this fix. See the
[font regression notes](../../../../gfx/tests/README-font-enumeration.md).

### Synthetic XHR dispatch and callback regressions

`../style/xhr-synthetic-events.html` exercises the
[DOM dispatch algorithm](https://dom.spec.whatwg.org/#concept-event-dispatch)
on [XMLHttpRequestEventTarget](https://xhr.spec.whatwg.org/#xmlhttprequesteventtarget):
initialization, target/currentTarget, cancellation, listener identity/capture,
handler activation order, removal/readdition, nested dispatch and propagation.
It runs separately from the baseline 985-check batch.
The GTK2 Suite baseline records 22 failures in 27 reached assertions because
synthetic callbacks never run and custom event types cannot be registered.
The native `DispatchEvent` stub also returns success without initializing its
boolean out-parameter. Full expected execution reaches 43 assertions.

The separate Chromium 148.0.7778.180 comparison reaches all 43 but fails three:
capture ordering, clearing propagation flags, and redispatch after an immediate
stop. The DOM algorithm specifies separate capture/bubble invocations and
clearing both propagation flags after dispatch; the fixture retains those
requirements rather than copying that reference browser's behavior. Reports
are `xhr-synthetic-before-gtk2.json` and `xhr-synthetic-chromium.json`. These
results reinforce that benchmark completion and browser comparisons alone are
not evidence of complete conformance. The new native implementation passes all 43 in GTK2 Suite; broader XHR
conformance remains unfinished.

### Completed native application matrix (2026-09-29 baseline)

All eight native LoongArch package gates pass: Suite, Browser, Calendar and
XULRunner, each with GTK2 and Xlib. Reports are under
`artifacts/speedometer21/{history,browser,calendar,xulrunner}-package-{gtk2,xlib}/logs/runtime-result.txt`.
These gates include the full pinned 11,540-case ES5.1 suite, the selected
28,582-case ES2015 suite, native embedding and application-specific runtime
checks. Calendar passes its unit groups, startup and all four views on both
backends. Standalone unchanged ChatZilla passes twelve checks on both XULRunner
backends. Every application/backend also passes the 985-check content batch and
300 History checks; Browser HTTP coverage passes 159 checks and the unchanged
Calendar/XULRunner permission configuration passes the explicit 78-check denial
policy/XHR batch described above.

These are baseline results, not validation of subsequent uncommitted XHR work.
Suite packages are from `989aaa00`; later Browser/Calendar/XULRunner builds
include the Xlib font fix `45f3b7d3`. Older GTK2 package runs precede the new font
fixture; separate current-build font and strengthened preferences runs pass.
XULRunner Xlib's package includes both startup and headless font regressions.
The pending Xlib benchmark uses a frozen Suite package, with its HTTP server
owned by the benchmark driver. A previous separate-server attempt stopped when
that server exited; it is retained as an infrastructure failure, not a pass.
Benchmark completion and this matrix do not establish exhaustive specification
or historical-application compatibility; the feature limitations above remain.

The XHR event registry now handles arbitrary synthetic event types, capture,
callback identity, handler activation order and mutation during dispatch.
Modern requests retain registered listeners across completion/reuse; explicitly
selected historical JavaScript versions keep their legacy callback ordering,
completion cleanup and no-op synthetic dispatch. The latter now initializes its
boolean return value. Modern callback getters return the original JavaScript
function through a security-checked binding, preserving native XPIDL interfaces.

The focused GTK2 Suite run passes 181 assertions across synthetic dispatch,
callback identity, reuse, legacy mode and the four existing network fixtures.
The separate `xhr-gc.xul` fixture passes ten collection/reentrancy assertions.
Reports are `xhr-modern-gtk2-*.json` and `xhr-modern-gtk2-gc.log`.
This does not establish complete XHR compliance: native trust flags, abort and
request event sequencing, callback return-value cancellation, additional XHR
fields and listener options remain separate gaps. In particular the reuse
fixture verifies delivery and ordering, not exact readyState transition counts.
Xlib and other-application validation of these changes is still pending.

### Dataset liveness diagnostic (known failures)

`../style/dataset-live.html` contains 47 checks derived from the
[DOMStringMap algorithms](https://html.spec.whatwg.org/multipage/dom.html#domstringmap).
It covers live attribute changes, writes/deletes, empty and numeric names,
ASCII name conversion, conversion exceptions, prototype collisions and property
descriptors. Current GTK2 Suite fails 28 checks because its dataset object is a
snapshot; Chromium 148.0.7778.180 passes all 47. This is a pending engine fix,
not part of the passing content batch. Reports are
`dataset-baseline-gtk2-dataset-live.json` and `dataset-live-chromium.json` under
`artifacts/speedometer21`. Original TodoMVC editing scripts also write dataset
properties; benchmark completion does not exercise all of those interactions.

XHR focused validation now also passes on Xlib Suite: the same 181 content,
ten collection/reentrancy and 24 application-lifecycle assertions as GTK2.
Current Browser GTK2 passes the expanded 271-check HTTP batch and ten XHR GC
checks. Package checks and the remaining application rebuilds are in progress.

`../style/xhr-ready-state.html` separately checks fifteen synchronous/asynchronous
state-notification assertions. The current Suite baseline fails four: it omits
synchronous OPENED/DONE events and duplicates asynchronous OPENED at send.
Chromium 148 passes all fifteen. This pending sequence fix is kept separate
from the passing callback-dispatch batch; baseline reports are
`xhr-state-baseline-gtk2-xhr-ready-state.json` and
`xhr-ready-state-chromium.json`.

The private-HOME Suite launcher passes simultaneous GTK2/Xlib synthetic-event
and callback-identity sequences (71 checks each), leaving the caller HOME
unchanged. Its 300-check History run, including multi-process persistence,
also passes. The X server is launched outside the caller-HOME isolation test:
Xvfb's own font cache is separate from Suite's profile isolation. Reports are
`private-home-isolation.json`, `private-home-{gtk2,xlib}.json` and
`private-home-history/summary.json`.

The state-notification correction now passes all fifteen assertions in GTK2
Suite. Modern XHR suppresses the internal SENT transition (which still has
public state OPENED), and synchronous requests dispatch OPENED/DONE. Explicit
legacy modes retain their historical notifications. The complete focused XHR
batch now reaches 184 passing assertions: the reuse fixture reaches 21 instead
of 33 because the duplicate OPENED callbacks are gone, and the new sequence
fixture adds fifteen checks. No assertions were removed. Suite lifecycle stays
at 24 passes. The GC fixture now has fourteen passing assertions, including
collection from synchronous ready-state callbacks. The expanded HTTP driver
includes the new sequence fixture. Remaining applications are being rechecked
with the corrected native component; the earlier package results remain a
separate baseline. JavaScript library SHA-256 remains
`1d29abb799a6522dc58735d5274b00304a10a48b3cbe79a67ffa3b5a3e360782`.

### Completed Xlib benchmark and corrected XHR matrix

`benchmark-current-xlib-owned.json` records a complete pass: 16 suites, ten
iterations, 480/480 checked steps and exit zero. It uses the frozen `989aaa00`
Suite Xlib package and JavaScript SHA-256
`1d29abb799a6522dc58735d5274b00304a10a48b3cbe79a67ffa3b5a3e360782`.
The launcher served the unchanged benchmark tree throughout the run. Background
builds/tests were active, so timings are not controlled performance comparisons.
The separate `benchmark-xhr-modern-gtk2` run contains the newer dispatch fix
`79c97eb5`, preceding the state-notification correction `0d546a61`.

The corrected component passes 184 XHR assertions and fourteen GC checks on
both Suite backends. Browser passes the expanded 274-check HTTP batch; Calendar
and XULRunner pass 193 checks with their explicit storage-denial policy, on
both backends. All six toolkit runs also pass fourteen GC assertions. Reports
are `xhr-state-APP-BACKEND-http/summary.json` and the corresponding GC JSON.
Browser preferences (30 checks) and standalone unchanged ChatZilla (12 checks)
pass on both corrected backends. Suite lifecycle remains at 24 passes.

The first incremental XULRunner check loaded stale `libxul.so`: rebuilding
`content/base/src` and `layout/build` updated archives without relinking the
aggregate runtime. Those four old sequencing failures remain recorded in
`xhr-state-xulrunner-gtk2-http-stale-library`. Relinking `toolkit/library` for
`MOZ_ENABLE_LIBXUL` builds and rerunning resolves them; they are not excluded or
counted as passes. The other applications use the shared layout component.

Calendar startup and all four views also pass with the corrected component on
GTK2 and Xlib (`xhr-state-calendar-{gtk2,xlib}-views.log`). The shared window
runner now accepts `--runtime` for native Linux, copying the runtime and using
a private HOME/profile; its existing macOS archive mode is retained. See
[Calendar compatibility checks](../../../../calendar/test/README-compatibility.md).

### Dispatch package gates and descriptor baseline

All eight `xhr-package-APP-BACKEND/logs/runtime-result.txt` reports now pass
with dispatch revision `79c97eb5`. Each includes all 11,540 pinned ES5.1 cases
and 28,582 selected ES2015 cases. These package results precede the subsequent
`0d546a61` state-notification correction; the focused HTTP, GC and application
checks above validate that later component separately.

`../style/dataset-descriptors.html` adds 39 planned descriptor, receiver,
symbol, extensibility and interface assertions. The snapshot implementation
fails 15 of the 33 assertions reached (some test groups terminate on an
exception). Chromium 148 reaches all 39 and fails two: it accepts generic and
accessor descriptors on named properties, although
[Web IDL's named-property definition algorithm](https://webidl.spec.whatwg.org/#legacy-platform-object-defineownproperty)
requires rejecting non-data descriptors. Keep these assertions; the comparison
browser is not the specification. Reports are `dataset-descriptors-chromium.json`
and `dataset-baseline-gtk2-dataset-descriptors.json`. Both dataset fixtures
remain known-failing diagnostics, outside the passing content batch.

The native host-object foundation (`f643a168`) now passes complete Suite GTK2
and Xlib package checks, including 11,540 ES5.1 and 28,582 selected ES2015 cases
on each backend, 35 new native host-object assertions, and existing application
checks. Reports are `host-package-suite-{gtk2,xlib}/logs`; these packages precede
the live DOM dataset binding and do not resolve its conformance gaps.

### Live dataset binding, first native pass

The native binding now passes all 47 `dataset-live.html` assertions, all sixteen
`html-attribute-case.html` assertions and fifteen GC/reentrancy checks in
`dataset-gc.xul` on GTK2 Suite. The combined content regression sequence passes
1,048 checks (the preceding 985 plus the 47 dataset and sixteen attribute-name
checks). Reports are `dataset-native-gtk2-*` and `dataset-suite-gtk2-content.json`.
HTML attribute creation and lookup now fold ASCII letters only, preserving
distinct non-ASCII names. Chromium 148 also passes the sixteen attribute tests.

The binding traces its element wrapper, reads names and values from the current
attribute list, reflects writes/deletes, handles data descriptors through the
named setter, preserves raw symbol values and alternate receivers, and refuses
preventExtensions. Native forwarding uses engine Reflect entry points rather
than replaceable JavaScript methods. Explicit GC during coercion, exceptions,
prototype getters and loss of the last script reference preserves the receiver.

This remains incomplete: `dataset-descriptors.html` reaches 33 assertions with
two failures because DOMStringMap's interface constructor/prototype is not yet
installed. The old own-property cache for `element.dataset` also remains; the
prototype accessor and complete SameObject behavior need further work. The
current binding handles null-namespace attributes. Namespace interpretation,
cross-window behavior and broader application validation remain open. These
results must not be represented as complete Web IDL conformance.

The initial binding build failed on a historical string API mismatch. Its
launcher ran the previous component; those snapshot failures and the build log
are retained in `dataset-initial-build-failure`, not counted as validation of
the new binding. After the compile correction, two remaining liveness failures
exposed Unicode attribute-name folding; those reports are retained separately
in `dataset-before-ascii-attribute-fix`. No assertions were removed.

### Dataset interface and realm checks

The next GTK2 Suite build passes all 47 liveness, 39 descriptor, sixteen
attribute-case and 42 interface/realm assertions, plus fifteen GC assertions.
The interface fixture also passes all 42 with ES5 content enabled; Chromium
148 passes the same 42. Reports are `dataset-native-gtk2-*`,
`dataset-interface-es5-gtk2-*` and `dataset-interface-chromium.json`.

HTMLElement.prototype now has a checked, readonly dataset accessor. Its
per-window weak cache preserves SameObject identity after deleting an absent
own property, removing a shadow property, changing the dataset prototype, or
replacing the public constructor. DOMStringMap has a shared interface prototype
and Symbol.toStringTag. Its native constructor cannot be called or constructed.
Borrowed getters preserve the element's realm and cached identity; TypeErrors
use the native function's realm. Deleting the global constructor no longer
recreates it. Newly introduced Web IDL functions receive ES2015 metadata while
restoring the caller's script edition afterwards. Existing legacy functions
are not changed. Interface prototypes are rooted during native accessor setup.

The earlier live-binding snapshot also passes 300 History, 274 HTTP, fifteen
dataset GC and 24 Suite lifecycle assertions in `dataset-suite-gtk2-*`. These
precede the interface/realm work; its full Suite and other-application checks
are pending. Namespace behavior, non-HTML accessor placement and further
security/lifetime coverage remain open. The passing focused tests are not a
claim of complete DOMStringMap or Web IDL compliance.

Intermediate prototype diagnostics are retained in
`dataset-prototype-first-build-failure`, `dataset-before-realm-fix` and
`dataset-before-function-metadata-fix`. The interface test first exposed two
wrong-realm TypeErrors and constructor resurrection, then metadata inherited
from legacy function creation. The final run retains and passes those checks.

The interface/realm build (46bd462d) passes 1,129 content, 300 History,
274 HTTP, fifteen dataset GC and 24 Suite lifecycle assertions on both native
LoongArch Linux GTK2 and Xlib. Reports are `dataset-interface-suite-*`.
Full package gates and the other applications remain in progress.

`../style/dataset-navigation.html` adds twelve checks for retained objects
across actual iframe document navigation and removal, plus inert-document
identity and writes. GTK2 Suite and Chromium 148 pass all twelve; reports are
`dataset-navigation-gtk2-*` and `dataset-navigation-chromium.json`. The fixture
uses `dataset-navigation-child.html` to force a document replacement.

The navigation fixture also passes twelve checks on Xlib.
`../style/dataset-frame-gc.html` retains only the dataset after removing its
iframe, then requests forced GC through the Suite probe runner. Both native
backends pass ten post-collection read/write, descriptor, enumeration, symbol
and interface checks (`dataset-frame-gc-{gtk2,xlib}-*`). This fixture requires
the runner's GC handshake; opening it alone does not complete the test.

The full native LoongArch Suite package gates now pass on both GTK2 and Xlib
for the 46bd462d dataset interface build. Each passes all 11,540 required-mode
ES5.1 cases and 28,582 historical ES2015 modes, 139 focused shell runs, 86 native
probes (including five internal probes), and the existing embedding, chrome,
ChatZilla and lifecycle checks. The packaged engine SHA-256 is
`1693a711f9b6629c2f7607244ea7ec015336a389d54fbe4251a43b521167f39b`.
See `dataset-package-suite-{gtk2,xlib}/logs/runtime-result.txt` and the adjacent
conformance reports. Other-application matrix checks and the new full benchmark
remain in progress; these gates do not establish full DOM/CSS compliance.

The preceding 79c97eb5 XHR GTK2 snapshot has also completed its full unmodified
benchmark: sixteen suites, ten iterations, 480 successful workload checks and
exit zero in `benchmark-xhr-modern-gtk2.json`. This snapshot predates both the
synchronous ready-state correction and the dataset work.

### HTML attribute names in CSS

The dataset DOM fix exposed inconsistent Unicode case folding in the CSS
parser. Attribute selectors and generated `attr()` content now use ASCII-only
folding for HTML. `html-attribute-selectors.html` improves from nine failures
to nineteen passes on native GTK2 Suite. The larger sequence passes 1,170
content assertions, plus 300 History, 274 HTTP, fifteen dataset GC and 24 Suite
lifecycle assertions (`css-attribute-suite-gtk2-*`). Xlib and the other
application builds for this parser correction remain in progress.

The preceding dataset build passes all 156 focused content assertions, ten
frame-teardown/GC assertions, fifteen native dataset GC assertions and the HTTP
probes in Browser, Calendar and XULRunner on both GTK2 and Xlib. HTTP totals
are 274 per Browser backend and 193 per Calendar/XULRunner backend, preserving
the latter applications' explicit storage-denial policy. Unchanged standalone
ChatZilla passes twelve checks in both XULRunner backends. Reports are
`dataset-content-*`, `dataset-frame-gc-*`, `dataset-matrix-*` and
`dataset-chatzilla-*`. Full package gates remain tracked separately.

The complete dataset package matrix is now green for all eight native LoongArch
Linux application/backend combinations: Suite, Browser, Calendar and XULRunner
on GTK2 and Xlib. Every package passes 11,540 required-mode ES5.1 cases,
28,582 historical ES2015 modes, 139 focused shell runs and 86 native probes
(five internal), with zero recorded language failures, unsupported cases,
timeouts, crashes or harness errors. Both suites pass lifecycle and ChatZilla;
both browsers pass thirty preference checks; both calendars pass their eight
unit tests and startup/four-view checks. Application and window-bootstrap
checks pass throughout. Aggregate counts and engine hashes are in
`artifacts/speedometer21/dataset-complete-native-matrix.json`; original reports
remain under `dataset-package-APP-BACKEND/logs`. These frozen packages contain
the dataset binding from 46bd462d and precede the CSS attribute correction.

The CSS correction (539c1bc3) separately passes 1,170 content, 300 History,
274 HTTP, fifteen dataset GC and 24 Suite lifecycle assertions on **both**
backends (`css-attribute-suite-{gtk2,xlib}-*`). Follow-up application builds
and layout/chrome checks are in progress. No application sources were changed.

The CSS attribute correction now passes the six follow-up application runs as
well: 890 content/layout assertions in each Browser, Calendar and XULRunner
backend, plus thirty Browser preference assertions, Calendar startup/four views,
or twelve unchanged standalone ChatZilla assertions as applicable. Reports are
`css-attribute-APP-BACKEND-{content,application}.json` and
`css-attribute-applications-summary.json`. These runs exercise the changed CSS
parser; the complete unchanged-engine language gates are recorded above.

### Full benchmark at the native dataset baseline

The frozen 46bd462d GTK2 Suite package has now completed all sixteen upstream
Speedometer 2.1 applications for ten iterations, with all 480 workload checks
passing and exit zero. The report is
`artifacts/speedometer21/benchmark-dataset-gtk2.json`; the runtime is
`dataset-package-suite-gtk2/package/zoolrunner-linux-loongarch64-suite-gtk2/runtime`.
This run includes the synchronous XHR ready-state fix and native dataset work.
It predates the subsequent CSS attribute-name, shadow grammar and computed-style
changes. Console permission-denied messages for `XMLHttpRequest.channel` remain
visible in the retained log. Passing the benchmark does not establish full
DOM/CSS specification compliance or validate later revisions.
