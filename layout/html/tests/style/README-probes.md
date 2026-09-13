These regression probes exercise the layout features used by the Basilisk
homepage. They are small, locally authored fixtures; the website's HTML,
stylesheets, images, and fonts are not bundled here.

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
