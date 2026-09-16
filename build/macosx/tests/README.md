# macOS runtime regressions

These tests exercise the classic application platform and its deployment paths.
Compile them for the target and run against the matching runtime. Compilation
and static import audits do not replace execution on the target OS.

## Modern macOS package regressions

Run both architecture matrices with SDK 11.3. On Apple Silicon, Intel package
checks run through Rosetta when it is available; this is distinct from testing
on physical Intel hardware. Packaging compiles its embedding probe for the
package's architecture, including when the host uses a different architecture.

From a macOS desktop session, test a relocated package with:

```sh
python3 build/macosx/tests/run-modern-package.py arm64 suite \
  --archive artifacts/zoolrunner-macos-arm64-suite-sdk11.3.tar.gz \
  --report-dir /tmp/zool-suite-runtime
python3 build/macosx/tests/run-modern-platform.py arm64 suite \
  --archive artifacts/zoolrunner-macos-arm64-suite-sdk11.3.tar.gz \
  --report-dir /tmp/zool-suite-platform
```

Repeat for `x86_64` and for Browser, Calendar and XULRunner. The matching default
object directory supplies the generated JSAPI/XPCOM headers; `--objdir` accepts
another matching directory. SDK 11.3 comes from `ZR_MACOS_SDK` or the existing
`~/dev/macos-sdk/MacOSX11.3.sdk` installation; its version is checked.
Run these GUI tests sequentially. They use disposable package copies/profiles
and restore the Suite's selected profile after their checks.

The package runner checks the focused JavaScript suites, regexp cancellation,
application chrome and components, and the 17 window-global assertions. Suite
also runs the 24 lifecycle checks and initializes ChatZilla without connecting.
Calendar deliberately omits the `data:` protocol, so the disposable window
fixture uses an ordinary local HTML file for its content global. The platform
runner checks image rows/channels and absolute, relative and PATH-based relaunch.
The full pinned Test262 suite remains a separate run through
`js/tests/es5/run-test262.py`.
When using a freshly extracted Intel package under Rosetta, launch its
`xpcshell -e 'print("Runtime ready");'` once before starting Test262. Initial
translation/registration can exceed Test262's ten-second per-process timeout.
Keep the normal timeout for the actual conformance cases.

The Quartz runner accepts an optional third architecture argument, for example
`check-quartz-strokes.sh OBJDIR SDK x86_64`, to exercise Intel rendering through
Rosetta using the matching static libraries. It recompiles the tested Quartz
implementation in a temporary directory and leaves developer objects unchanged.
Also run the [nine Quartz opacity cases](../../../gfx/tests/README-quartz-opacity.md)
with `-arch arm64` or `-arch x86_64`, the matching object directory, and
`-isysroot` pointing to SDK 11.3.

## Image buffers and frames

`early-image.cpp` exercises the real XPCOM image and image-frame components:

* Writing separate RGB and alpha rows must preserve Cairo's top-down order.
* Optimization and subsequent readback must preserve RGB and alpha channels,
  including a partially transparent pixel and a fully transparent pixel.

The old row convention failed on both native macOS arm64 and original Mac OS X
10.0 PowerPC. The old ARGB word extraction also failed on PowerPC, reading a
red channel of 0 instead of 255. Both tests pass with the corrections on those
targets. Native platform image backends retain their original row convention;
the top-down exception applies to Cairo.

Build the test with the target's generated `mozilla-config.h`,
`MOZILLA_INTERNAL_API`, and its `dist/include` directories for `xpcom`, `string`,
`gfx`, and `nspr`. Link the matching XPCOM implementation and NSPR libraries.
For a libXUL build, use its XUL library. Run the executable beside the runtime's
libraries and `components` directory so XPCOM discovers the tested components.
Success prints both:

```
Image frame RGB/alpha top-down rows passed
Image RGB/alpha optimization round trip passed
```

After changing the row-order macro, rebuild the shared graphics code, Thebes
graphics component, and GIF decoder/container, then relink their enclosing
components or XUL library. Inspect an application screenshot too: sprite
cropping makes an inverted image appear as unrelated or damaged toolbar icons.

The original-10.0 payload builder includes this test automatically. See
[PowerPC deployment status](../../../mozconfigs/macos/powerpc/10.0-status.md)
for the complete application and CI validation status.

## Early ATSUI text placement

`early-font.c` checks original ATSUI glyph mapping, outline callbacks and actual
Cairo glyph ink. Its first drawing context has a translation, so it detects
translation being applied again inside a cached glyph outline. Original 10.0
reproduced an incorrect bottom edge at 47 for baseline 40; the corrected path
ends at 40. CTM translation must not enter the outline cache, which deliberately
ignores it when matching fonts. Font-matrix translation remains part of the font.

These pixel checks complement the application test's DOM/layout assertions.
Inspect saved GUI screenshots before claiming the application paints correctly.

## Cocoa application relaunch

`early-relaunch.mm` calls the production `LaunchChildMac` helper and requires
the requested executable to run its child mode. Run it beside the packaged
application: early Cocoa's `NSBundle` may return that application's executable
instead of the invoked test program. The original helper reproduced this wrong
selection on Mac OS X 10.0. The payload tests relative, absolute and PATH-based
invocation. The helper also preserves an application stub's deliberate
substitution of `argv[0]`; bundle lookup remains the fallback for an unavailable
argument. The corrected relative-path regression passes on original Mac OS X
10.0; native arm64 passes all three invocation forms.

The complete Toolkit GUI test also catches an original-dyld startup bus error.
The clean Browser fails with lazy binding and passes with `MH_BINDATLOAD` set
on its executable, without tracing or environment overrides. Target-10.0
Toolkit programs therefore link with `-bind_at_load`; packaging checks the
main executable's flag. Keep the profile/first-run restart and GUI tests after
the smaller relaunch probe, since successful child creation alone misses this
later initialization failure.

Calendar's historical command-line handler opens its own window and ignores
`-chrome`. Its test payload adds `early-calendar-commandline.js` as a temporary
XPCOM component, registered ahead of that handler. `-zoolrunner-test` opens the
test controller, which then opens and checks the real Calendar window. This
component is included only in the disposable test payload, not application
archives. It also exercises classic JavaScript component/category registration.

Calendar and standalone XUL tests focus the application window and leave their
successful state visible for 20 seconds before quitting. The guest captures a
frame every 10 seconds; this pause preserves screenshots for visual review
after the component assertions have passed.

## Quartz image drawing and toolbar sprites

`check-quartz-images.sh OBJDIR SDK ARCH` runs 24 pixel comparisons against
Cairo's software renderer, plus the nine opacity regressions. It covers decoded
image surfaces, both Quartz source orientations, flipped and unflipped native
contexts, scaling, sprite cropping, and repeated hover-state style repaints.
The modern macOS workflow runs it for every application and architecture.

The decoded-image drawing path previously chose its vertical flip before
converting the image to a Quartz surface. That left converted images upside
down and selected the wrong rows from toolbar sprites. Determine orientation
from the actual Quartz source, including converted images. Buffer row tests
alone do not exercise this presentation step.
