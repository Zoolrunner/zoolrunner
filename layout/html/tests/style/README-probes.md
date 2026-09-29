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
