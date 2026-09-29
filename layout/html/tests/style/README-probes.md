These regression probes exercise layout features used by the Basilisk and
Pale Moon websites. They are small, locally authored fixtures; the website's HTML,
stylesheets, images, and fonts are not bundled here.

`dom-selectors.html` adds native DOM selector regressions used during
[Speedometer 2.1 compatibility work](../speedometer21/README.md). It runs as
ordinary content, including detached trees and documents without a presentation.

Open `structural-inline-block.html`, `viewport-media-queries.html`,
`background-size.html`, `linear-gradient.html`, and `faq-toggle.html` in a ZoolRunner browser
or Suite build. Each page reports PASS/FAIL results. The
media-query test resizes an iframe across inclusive width boundaries, checks
stylesheet attributes and imports, mutates media lists through CSSOM, and
opens/closes a responsive menu with `:target`. The FAQ probe checks activation
of hidden checkboxes through labels and adjacent-sibling restyling, matching
the site's CSS-only expandable answers.

For automated execution as a standalone XUL application, run a built XULRunner
with `probe-app/application.ini` and a separate test profile. For example, from
the source root on macOS:

```sh
mkdir -p /tmp/zoolrunner-layout-probes-profile
DYLD_LIBRARY_PATH="$PWD/obj-zoolrunner-macos-arm64-xulrunner/dist/bin" \
MOZ_NO_REMOTE=1 \
obj-zoolrunner-macos-arm64-xulrunner/dist/bin/xulrunner-bin \
  "$PWD/layout/html/tests/style/probe-app/application.ini" \
  -profile /tmp/zoolrunner-layout-probes-profile
```

The harness loads the HTML as file content, prints every result, and exits.
Success requires `LAYOUT-PROBES failures=0` in the output; process exit status
alone is not a test result. The first launch of a new profile may restart to
register components. If it produces no result, repeat the command with the same
profile. On other platforms use the appropriate XULRunner executable and
library search path.

Current coverage includes structural block elements, inline-block dimensions
and float containment, circular border-radius aliases, and viewport width,
height, and orientation queries. The media grammar follows
[Media Queries Level 3](https://www.w3.org/TR/mediaqueries-3/), with other media
features treated as unknown (`not all`). Query evaluation uses the initial
font for relative units, not the element's author-specified font.

Background coverage includes single-layer `background-size` (lengths,
percentages, auto, contain, cover) and `linear-gradient()` (angles, side/corner
directions, explicit and omitted stops, and premultiplied sRGB interpolation).
The CSSOM probes exercise inheritance, declaration ordering, alpha-preserving
serialization, malformed input, and quirks-mode parsing. New background-size
values are exposed through `getPropertyValue`/`setProperty`; the historical
CSS2 XPCOM interface has not been extended with a `backgroundSize` accessor.

`background-size-paint.html` displays raster backgrounds beside reference
layouts built from solid-color elements. `linear-gradient-paint.html` provides
small painting fixtures for comparison with another engine: directions, hard
stops, stop-position fixup, transparency, repetition, and percentage sizing and
positioning. Painting probes require visual or screenshot inspection; the
CSSOM tests alone do not validate pixel output. Account for display color
management when comparing screenshots.

Sizing follows [CSS Backgrounds Level 3](https://www.w3.org/TR/css-backgrounds-3/#background-size);
gradient geometry and interpolation follow
[CSS Images Level 3](https://www.w3.org/TR/css-images-3/#linear-gradients).
Generated tiles use the existing portable image-frame API and cache one raster
per computed gradient. Rasterization is bounded to 8192 pixels per dimension
and 32 megapixels; oversized or unavailable images fall back to the background
color. Unchanged raster backgrounds retain the existing tiling path.

These probes do not establish complete site rendering or full CSS conformance.
Multiple background layers, background shorthand `/` sizing, radial/repeating
gradient functions, and newer gradient syntax are not implemented here.
Shadows, transitions, and keyframe animations remain separate work.
Downloadable fonts are outside this work's scope.

`opacity-paint.html` compares opacity groups with opaque reference colors. It
checks overlapping children (opacity must apply to the group), nested opacity,
clipping, and translucent text. The native Thebes backend uses optional alpha
groups rather than its unimplemented legacy blender. Other backends retain
the existing blender implementation. Include scrolling/resizing and menu hover
repaints in visual checks; CSSOM assertions cannot detect missing paint.

The macOS bitmap test in [gfx/tests](../../../../gfx/tests/README-quartz-opacity.md)
checks native compositing without monitor color profiles. On 2026-09-13 the
XULRunner and Suite each passed the 169 HTML/CSS assertions after the opacity
changes; the Suite also passed the live Basilisk structural check. Pale Moon's
menu and download link now paint in the native comparison, but flexbox sizing
and alignment, shadows, and subpage background layers remain unfinished.

The Suite-first Speedometer probes also cover `Event`, animation-frame
callbacks, client geometry, live class-name collections, inert HTML documents
and contextual HTML insertion. `custom-event.html` additionally requests forced
GC from the [Suite probe runner](../speedometer21/run-suite.py); run that fixture
through the runner rather than opening it directly. Its rooted detail data
survives GC, but collection of detail cycles remains unvalidated. These tests
exercise native APIs and do not establish exhaustive DOM or CSS conformance.

`cross-frame-event.html` checks generic event dispatch for built-in event names,
same-origin cross-frame identity/expandos, capture, cancellation, listener
removal and nested dispatch. It passes 50 assertions in native LoongArch Suite.

`programmatic-click.html` covers button/input targets, cross-frame delegation,
hidden and disabled controls, cancellation, recursive clicks, removal during
dispatch and batch deletion from a static NodeList. `input-handler.html` covers
handler detection, null defaults, assignment, inline handlers and removal.
`event-redispatch.html` covers reuse, propagation reset, persistent cancellation,
concurrent-dispatch rejection and bubbling in detached subtrees.

`dom-invalid-receivers.html` checks that the new native DOM methods reject
ordinary objects as receivers without dereferencing a missing XPConnect
wrapper. Six assertions pass in native LoongArch Suite; the unchanged Suite
lifecycle fixture also passes all 24 checks after the shared null-query fix.

`event-prototype.html` checks the shared EventTarget prototype across Node,
Window and XMLHttpRequest; method replacement on existing/new nodes; native
listener identity; dispatch; document-fragment capture/bubbling/currentTarget;
and frame realm separation. It also preserves the historical standards/quirks
boundary for unqualified named window properties, without exposing those names
through the prototype shared by ordinary nodes. All 53 checks pass in native
LoongArch GTK2 Suite. This is focused coverage, not complete DOM conformance.

`inert-document-title.html` checks DOM-backed title reads/writes on created HTML
documents, including SVG document elements, text replacement, whitespace,
conversion, element identity and absent/foreign roots. Its 31 assertions pass.
Historical parsed documents keep their existing title path. The additional
25-assertion `dom-conversion-exceptions.html` verifies that XPConnect preserves
script-thrown argument-conversion exceptions, including reentrant conversions,
without running the outer native mutation or suppressing ordinary type errors.

The HTTP-only `local-storage.html` and `storage-persistence.html` fixtures cover
modern and legacy storage separately. The former passes 54 assertions; the
latter passes 15 across three processes and two origins. See the
[restart-probe instructions](../speedometer21/README.md) for serving and running
them. They are not in the file-URL layout list.

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

`event-reinitialization.html` passes 36 native Suite assertions covering flag
replacement, cancellation state, target reset, inert initializers during dispatch,
and MouseEvent relatedTarget retention. `defaultPrevented` uses an additive
interface, preserving legacy event interface layouts. Suite's 24 lifecycle
checks pass after these changes. Chromium 148 differs on two target-reset
assertions; the fixture retains the DOM specification's required null target.

`event-propagation-state.html` covers stopImmediatePropagation and cancelBubble
on Event, UIEvent and CustomEvent, propagation reset and retained native-event
redispatch (47 assertions). Native LoongArch GTK2 Suite passes these tests and
all 904 assertions across the current 30-fixture content collection.

`event-type-strings.html` adds 23 assertions for complete UTF-16 event names and
distinct listener lookup, including embedded NULs. Native LoongArch GTK2 Suite
passes 927 assertions across 31 content fixtures after the string-key correction.
The native `xpcom/tests/TestStringKeyLength.cpp` additionally checks hashing,
cloning, lookup and removal (19 checks).

The latest native LoongArch Xlib Suite also passes the 927 content assertions.
The 400-by-300 box-sizing painting region matches the Chromium reference with
zero differing pixels on both GTK2 and Xlib. This remains bounded painting
coverage; shadows, transforms and other required workload styles are incomplete.

Client rectangle bounds now keep the first fragment separately from the union
of nonzero extents. An initial zero-size point no longer enlarges a later
nonempty box; when every fragment has zero width or height, the first fragment
is retained as required by the
[CSSOM View bounding-box algorithm](https://drafts.csswg.org/cssom-view/#dom-element-getboundingclientrect).
`TestClientRectBounds.cpp` passes 10 direct boundary cases, including fractional
coordinates, all-empty lists and degenerate lines. The existing 33 content
geometry checks and 24 Suite lifecycle checks also pass on native LoongArch
GTK2. SVG/transformed geometry and other layout gaps remain outside this result.

`rgba-background-paint.html` checks rectangular background compositing, including
alpha zero/one, very faint colors, overlapping translucent colors and overflow
clipping. Backgrounds use the existing native image compositor when the primitive
renderer cannot retain alpha. Translucent views are no longer classified as
opaque or safe uniform scrolling backgrounds. GTK2/Xlib image channel composition
rounds at the output; the public historical truncating blending macro is unchanged.

After taking a screenshot at 1x scale, run:

```sh
python3 layout/html/tests/style/check-rgba-background.py screenshot.png \
  --report artifacts/rgba-background.json
```

The optional checker requires Pillow and aligns the 440-by-300 content region
using the magenta marker. Its independently composited reference matches Chromium
148 exactly. Native LoongArch GTK2 also matches all 132,000 pixels exactly; the
original rendering differed at 27,600 pixels. This tests the
[CSS Color alpha-compositing rule](https://www.w3.org/TR/css-color-3/#alpha),
not full color conformance. Translucent rounded fills, borders, text and canvas
background propagation need additional implementation and painting coverage.

The final fixture also places translucent backgrounds partly outside the top
and left viewport edges. Xlib now matches all 132,000 reference pixels as well.
This exposed and corrected its image scaler's alpha-plane argument mismatch,
cropped buffer dimensions and source/destination offset confusion. The earlier
Xlib capture crashed in `Stretch8`; the corrected scaled image path passes the
strict pixel comparison. Fully transparent rounded backgrounds are also skipped.

`css-set-property.html` exercises the optional CSSOM priority argument through
shared native prototype methods, including computed declarations and style rules.
Its 43 checks cover WebIDL conversion order/null handling, invalid receivers and
Symbols, priority replacement, shorthand priorities, atomic rejection of extra
declarations, and read-only computed styles. The frozen three-string XPCOM
interface remains unchanged; CSSOM parsing uses the existing additive parser
interface instead of concatenating a declaration string.

`css-constructor-shadowing.html` now passes 15 assertions in native LoongArch
Suite on GTK2 and Xlib and is included in the layout probe list. Computed-style
inheritance uses the realm's cached native CSSStyleDeclaration prototype rather
than reading the replaceable global constructor. A throwing constructor getter,
a throwing public prototype getter, and a null constructor leave computed values,
interface identity and the optional-priority binding intact. Before this fix,
the expanded fixture reported five failures. The complete content regression
set now passes 985 assertions on each backend, and both unchanged Suite lifecycle
runners pass all 24 checks. Artifacts are under
`artifacts/speedometer21/css-shadowing-{gtk2,xlib}-content-probes.json` and
`css-shadowing-{gtk2,xlib}-lifecycle.stdout`. This bounded fix does not
establish that all DOM constructors are independent of public property changes.

Shared media-query and gradient fixtures use ordinary stylesheet/image files
instead of `data:` URLs so Calendar can exercise the same CSS assertions while
retaining its intentionally restricted protocol build. The linked/imported
stylesheet checks still total 69 assertions, and the gradient replacement and
round-trip checks still total 39. Both Suite backends and Browser GTK2 pass
these resource changes; Calendar GTK2 passes the complete 985-assertion content
set with them.

Calendar also intentionally omits cookies and their permission service. Storage
continues to fail closed in that configuration. `storage-policy-denied.html`
checks nine legacy denial/child-window behaviors; it is not a substitute for
the 90 allowed-storage/event assertions in Browser and Suite. Run the HTTP
driver with explicit `--storage-policy deny` for this configuration. Its four
XHR groups still run unchanged and pass all 69 assertions in Calendar GTK2.

`html-attribute-selectors.html` checks ASCII-only folding of HTML attribute
names in selectors and generated `attr()` content, including layout changes
after attribute mutation and case-sensitive XML selectors. The initial native
LoongArch GTK2 Suite run (46bd462d) fails nine of nineteen checks; Chromium
148 passes nineteen. Reports are `html-attribute-selectors-gtk2-*` and
`html-attribute-selectors-chromium.json` under `artifacts/speedometer21`.
The parser still applies Unicode case folding here, despite the corrected
HTML DOM attribute APIs. This fixture records the remaining defect and is not
yet included in the default passing probe sequence. The expected distinction
follows [Selectors case sensitivity](https://www.w3.org/TR/selectors-3/#casesens)
and HTML's ASCII case rules; generated-content assertions measure layout,
not only parser acceptance.

The native parser now folds only ASCII characters for HTML attribute selectors
and `attr()` references, preserving distinct non-ASCII names. XML remains
case-sensitive. GTK2 Suite passes all nineteen checks after this correction,
including generated-content widths and mutation-driven layout updates, as part
of 1,170 content checks. The same build passes 300 History, 274 HTTP, fifteen
dataset GC and 24 Suite lifecycle checks. Reports are
`artifacts/speedometer21/css-attribute-suite-gtk2-*`; Xlib and other-application
validation is still running. Namespace-prefix parsing is unchanged.

Both Suite backends now pass the corrected nineteen-case selector fixture and
the full 1,170-case content sequence, with their History, HTTP, GC and lifecycle
checks also green. `html-attribute-case.html` and
`html-attribute-selectors.html` are now included in the default
`layout-probes.xul` sequence, so the DOM and CSS attribute-name distinction
remains part of routine layout regression coverage.

The six follow-up Browser/Calendar/XULRunner builds also pass 890 content/layout
assertions each after the attribute-case correction, together with Browser
preferences, Calendar's four views and unchanged standalone ChatZilla checks.
See `artifacts/speedometer21/css-attribute-APP-BACKEND-*`.

`text-shadow-parser.html` records 72 grammar, atomic rejection, CSS2 property
and serialization checks for the next rendering feature. Chromium 148 passes
all 72; the preceding native Suite build fails seventeen. Its initial 65-case
version failed twelve; the added keyword spelling/clone assertions are retained.
`text-shadow-quirks.html` separately checks thirteen compound-value and legacy
length-quirk cases. Chromium passes thirteen. Parser acceptance alone will not
establish text-shadow support: computed style and painting are still absent.

The first text-shadow parser correction reached 71/72: the remaining failure
exposed CSS2 IDL property assignment retaining an old `!important` declaration.
That diagnostic is retained in `text-shadow-before-css2-assignment-fix`. The
parser now accepts standard initial/unset spelling, currentColor and transparent,
rejects negative blur atomically, and preserves commas with omitted blur values.
Its compound grammar does not admit quirks-mode unitless lengths or hashless
colors; ordinary legacy width/height quirks remain intact. The original native
quirks baseline failed eight of thirteen assertions.

Standard/default and ES2015 CSS2 assignments now use the existing CSSOM
setProperty replacement/validation path. Explicit JavaScript 1.x and native
callers retain their historical path. `css-property-assignment.html` passes
22 checks in GTK2 Suite. Chromium 148 disagrees on one: it accepts
`inherit !important` as an assignment value. The test retains rejection because
[CSSOM setProperty parsing](https://drafts.csswg.org/cssom/#dom-cssstyledeclaration-setproperty)
excludes priorities in value strings; a reference browser is not the specification.
The chrome fixture `../speedometer21/css-assignment-legacy.xul` passes ten
legacy/modern checks, including the historical `-moz-initial` spelling.

The GTK2 Suite integration passes 1,277 content, 300 History, 274 HTTP, fifteen
dataset GC and 24 lifecycle assertions. Its ES5-content run passes the same
107 new content assertions (72 shadow grammar, thirteen quirks and 22 property
assignment checks). Reports are `text-shadow-parser-suite-gtk2-*` and
`css-assignment-{legacy,es5}-gtk2.*` under `artifacts/speedometer21`. Xlib and
other-application checks remain pending for these changes. Computed text-shadow
values and shadow painting remain unfinished; these parser passes do not
establish rendering support or full CSS conformance.

The Xlib Suite build now passes the same 1,277 content, 300 History, 274 HTTP,
fifteen dataset GC and 24 lifecycle assertions after 36b2fad8. Both backends
also pass 107 assertions in ES5 content mode, ten explicit legacy/modern chrome
assertions, and unchanged Suite ChatZilla startup. Reports are
`text-shadow-parser-suite-{gtk2,xlib}-*`, `css-assignment-*` and
`text-shadow-parser-chatzilla-*`. Browser, Calendar and XULRunner now pass the follow-up checks on both backends:
997 content assertions and ten legacy/modern assignment assertions per build,
plus browser preferences, Calendar startup/four views and standalone ChatZilla
as applicable. The six-result aggregate is
`text-shadow-parser-applications-summary.json`. This remains parser/CSSOM work,
not text-shadow painting.

`text-shadow-computed.html` now passes thirty computed-value checks in native
LoongArch64 GTK2 Suite and Chromium 148. Coverage includes inherited absolute
lengths, deferred currentColor, explicit/transparent colors, list order, font
and color mutations, declaration removal, cloning, unrendered elements,
pseudo-elements, shared rules and root inheritance. The expanded GTK2 Suite
run passes 1,307 content, 300 History, 274 HTTP, fifteen dataset GC and 24
lifecycle assertions. The final null-color guard also passes the thirty new
checks in ES5 content mode. Reports are `text-shadow-computed-*` under
`artifacts/speedometer21`; Xlib and other-application follow-up is pending.

Enabling rule mapping initially exposed incorrect ownership of the borrowed
specified shadow list. `GetTextData` now clears that borrowed pointer before
its temporary rule-data destructor runs, matching the existing content/quotes
ownership convention. The failed run and crash trace are retained as
`text-shadow-computed-borrowed-list-crash.*`; the subsequent mutation and
lifecycle checks pass. Computed lists are independently owned and shared
immutably through inheritance. **Shadow painting remains unimplemented**;
computed-style success is not rendering conformance.

The separate chrome fixture `../speedometer21/text-shadow-paint.xul` compares
captured pixels against independently positioned text. At 6873bdbf, its no-shadow
control and scrollable-area check pass, while positive/negative offsets, list
painting order, translucency and default shadow color fail (five of seven).
`text-shadow-paint-baseline.log` retains the pixel counts. This deliberately
failing rendering target is not part of the passing default probe sequence;
blur, selection and decoration coverage must be added when painting is
implemented. Do not use computed-style passes as a substitute for these pixels.
