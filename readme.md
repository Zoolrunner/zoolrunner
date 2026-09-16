# ZoolRunner

ZoolRunner is a continuation of RetroZilla, itself derived from the Mozilla
1.8.1 codebase. Its primary purpose is to preserve and maintain the classic
Mozilla application platform for standalone XULRunner-style applications and
Mozilla-based desktop applications.

ZoolRunner is not primarily a web browser project. The included browser matters,
and it should remain functional, but the central product is the platform/runtime:
XUL, XBL, XPCOM, XPConnect, XPIDL, chrome packages, privileged JavaScript,
profiles, preferences, networking, layout, widgets, and the surrounding
application infrastructure inherited from Mozilla.

The project modernizes that platform selectively so it can still be built,
debugged, and ported on contemporary development systems while preserving
historical application-facing APIs and operating-system support where practical.

## What ZoolRunner Is

ZoolRunner is:

* a continuation of RetroZilla;
* ultimately derived from Mozilla 1.8.1;
* a general-purpose Mozilla/XUL application platform;
* intended to support XULRunner-style standalone applications;
* intended to support Mozilla-based desktop applications;
* intended to preserve classic Mozilla application technologies;
* selectively updated for contemporary development systems and additional CPU
  architectures; and
* interested in preserving compatibility with historical applications and
  operating systems where that remains practical.

## What ZoolRunner Is Not

ZoolRunner is not an attempt to create another general-purpose contemporary web
browser. The browser should remain usable for compatible content and should fail
gracefully on unsupported content, but catching Gecko 1.8.1 up to current
Firefox, Chromium, or UXP web-platform behavior is not the current project goal.

Unsupported content is allowed to fail. It should not be allowed to crash or
corrupt the process.

## History and Lineage

The source lineage is:

```text
Mozilla 1.8.1 -> RetroZilla -> ZoolRunner
```

Mozilla 1.8.1 provides the original application platform: Gecko, SpiderMonkey,
XUL, XBL, XPCOM, XPConnect, Necko, NSS, NSPR, the XPFE toolkit, the browser,
MailNews, Composer, XULRunner, and many other components.

RetroZilla kept that platform alive with an emphasis on legacy Windows systems
and practical updates to the inherited Mozilla applications. ZoolRunner continues
from RetroZilla, but treats the Mozilla/XUL runtime itself as the main artifact.
The browser remains an important included application, but it is not the sole or
primary focus.

Historical Mozilla and RetroZilla attribution should remain in source files,
credits, changelogs, and license material where it describes authorship or
lineage. Project branding, active application names, package names, and current
documentation should use ZoolRunner.

## The Mozilla Application Platform

ZoolRunner intentionally preserves classic Mozilla application technologies that
later Mozilla projects removed or de-emphasized. These include XUL, XBL, XPCOM,
XPConnect, XPIDL, chrome registration, privileged JavaScript, RDF-era
application infrastructure, installable extensions, themes, profiles, and
preference-driven application configuration.

Standalone third-party XULRunner applications and Mozilla-based desktop
applications are first-class use cases. ZoolRunner should make it possible to use
Mozilla technologies to build applications that have nothing to do with browsing
the Web.

## Why the Browser Is Included

The browser is a valuable platform integration test and sanity check. A working
Mozilla browser exercises an unusually large portion of the underlying platform,
including Gecko, SpiderMonkey, XUL, XBL, XPCOM, XPConnect, Necko, networking and
protocol handlers, preferences, profiles, chrome registration, themes,
localization, DOM implementation, layout, graphics, image decoding, JavaScript
integration, history, bookmarks, downloads, security infrastructure, storage,
platform widgets, and event handling.

Because the browser depends on so many pieces of Mozilla at the same time,
keeping it functional provides strong evidence that platform changes have not
caused broad regressions. Browser functionality is therefore a validation
mechanism and a useful included application, not the primary purpose of
ZoolRunner.

## Included Mozilla Applications

The source tree contains several historical Mozilla applications and application
front ends, including:

* the XPFE suite;
* the browser front end;
* MailNews and the mail front end;
* Composer;
* calendar/Sunbird sources;
* XULRunner; and
* Minimo sources.

These applications are important examples and test cases for ZoolRunner as an
application platform. Their presence in the tree does not mean every application
is currently build-tested on every supported or historical platform.

## Current Status

The currently exercised development configuration in this repository builds the
browser application on contemporary Linux with GTK2 into an in-tree object
directory. On this system the working configuration uses:

```sh
cp mozconfigs/linux/loongarch64/gtk2_browser_gcc.mozconfig mozconfig
make -f client.mk build
```

The build configuration is intentionally kept inside the source tree and uses
`obj-zoolrunner-linux` as the object directory. When the optional in-tree libIDL
bootstrap is enabled, libIDL is also built inside that object directory.

An experimental xlib browser build is also available through
`mozconfigs/linux/loongarch64/xlib_browser_gcc.mozconfig`. It builds into
`obj-zoolrunner-xlib` and is used to exercise the restored low-dependency X11
widget path. This backend is less complete than the GTK2 build and should
currently be treated as development work.

```sh
cp mozconfigs/linux/loongarch64/xlib_browser_gcc.mozconfig mozconfig
make -f client.mk build
```

The XPFE suite can also be built against the Xlib backend with
`mozconfigs/linux/loongarch64/xlib_suite_gcc.mozconfig`.

The XPFE suite can be built on the same Linux/GTK2 development system through
`mozconfigs/linux/loongarch64/gtk2_suite_gcc.mozconfig`:

```sh
cp mozconfigs/linux/loongarch64/gtk2_suite_gcc.mozconfig mozconfig
make -f client.mk build
```

That configuration builds into `obj-zoolrunner-suite` and enables the suite
application, MailNews, Composer, LDAP, S/MIME, Chatzilla, SVG, canvas, bundled
JPEG/zlib, crypto, and the optional in-tree libIDL bootstrap.

Run the browser test application from the built dist directory:

```sh
cd obj-zoolrunner-linux/dist/bin
sh ./run-mozilla.sh ./zrbrowser
```

Run the built suite browser from its dist directory:

```sh
cd obj-zoolrunner-suite/dist/bin
LD_LIBRARY_PATH=. MOZ_NO_REMOTE=1 ./zoolrunner -browser
```

MailNews and Composer can be started from the same suite build with `-mail` and
`-edit`.

Native macOS arm64 Cocoa Suite and standalone XULRunner builds have also been
built and run. On 2026-09-13, both passed the 169 assertions in the
[HTML/CSS regression probes](layout/html/tests/style/README-probes.md); the Suite
also passed a structural check of the live Basilisk homepage. These are
development validation results, not a claim of complete website or platform
conformance. The rendering changes have not yet been validated on Windows.

Cocoa startup now activates the application through AppKit after ordering its
first ordinary window, and delivers Gecko activation when the first keyboard
responder arrives after the native window became key. See the
[macOS startup keyboard checks](mozconfigs/macos/arm64/README.md#startup-keyboard-regression)
when changing activation or focus handling; Command shortcuts alone do not
validate text input.
The user confirmed on 2026-09-14 that the rebuilt macOS app accepts typing
immediately after startup without switching applications.

## Changes from RetroZilla

Significant work completed after the RetroZilla baseline includes:

* Added structural HTML block elements, standard `inline-block` handling,
  circular `border-radius` aliases, and viewport width/height/orientation media
  queries, including resize-driven restyling.
* Added single-layer `background-size` and `linear-gradient()` rendering through
  the existing graphics backends, with CSSOM and painting regression fixtures.

* Added initial LoongArch64 Linux support, including NSPR platform metadata and
  an xptcall backend.
* Added a Linux browser build configuration for contemporary distributions,
  using GTK2 and an in-tree object directory.
* Added a Linux suite build configuration for contemporary distributions,
  using GTK2 and an in-tree object directory.
* Added an opt-in in-tree libIDL bootstrap for systems without a suitable system
  libIDL package. Existing configurations can continue to use system libIDL by
  leaving `BOOTSTRAP_IN_TREE_LIBIDL` unset.
* Fixed build failures exposed by contemporary GCC on Linux.
* Improved the build system around compiler wrappers, dependency-file
  generation, parallel builds, and local dependency paths.
* Updated GTK2/Pango/Cairo/Xft build integration for contemporary Linux headers
  and pkg-config layouts.
* Added or repaired runtime packaging for browser components and NSS private
  freebl libraries needed by the built application.
* Removed the obsolete in-product changelog page and changed release/startup
  links that referenced it to use the active ZoolRunner information page.
* Fixed several startup and browsing crashes caused by undefined initialization,
  ownership, and teardown assumptions in old Gecko code.
* Fixed categories of memory-safety and portability defects exposed while
  running modern stress content, including null dereferences, stale pointer
  traversal, invalid ownership assumptions, and LP64/64-bit assumptions.
* Hardened selected XML/navigation teardown paths and malformed-content handling
  so unsupported content is less likely to crash the process.
* Fixed XPFE Preferences pane switching issues in the classic suite preference
  window and restored pane visibility in the browser preference window.
* Restored the xlib browser build far enough to create and paint browser
  windows on the current Linux development system, including fixes for popup
  menu positioning in the xlib widget backend.

This list is intentionally not a commit log. Individual bug fixes remain in Git
history and should be tracked there or in a separate changelog.

## Third-Party Dependency Updates

Bundled dependency versions verified directly from the source tree include:

* NSS: `3.42` with the tree's customized beta marker, in
  `security/nss/lib/nss/nss.h`.
* NSPR: `4.7.7`, in `nsprpub/pr/include/prinit.h`.
* SQLite used by the Mozilla storage layer: updated from `3.3.5` to `3.53.4`,
  in `db/sqlite3/src/sqlite3.h`.  The old copy's README incorrectly claimed
  `3.3.4`.
* SQLite bundled inside NSS: `3.7.15`, in `security/nss/lib/sqlite/sqlite3.h`.
* libpng: `1.6.58`, in `modules/libimg/png/png.h`.
* zlib used by the Mozilla tree: `1.3.2`, in `modules/zlib/src/zlib.h`.
* zlib bundled inside NSS: `1.2.5`, in `security/nss/lib/zlib/zlib.h`.
* bzip2/libbzip2: updated from `1.0.3` to `1.0.8`, in
  `modules/libbz2/src/bzlib_private.h`.
* JPEG library: updated from IJG libjpeg `6b` to libjpeg-turbo `3.2.0`,
  configured for the traditional libjpeg 6b-compatible API with
  `JPEG_LIB_VERSION 62`.
* cairo: `1.0.2`, in `gfx/cairo/cairo/src/cairo-features.h.in` and documented
  in `gfx/cairo/README`.
* libIDL: `0.8.14`, vendored under `build/unix/libIDL`.

The libIDL copy is included because suitable libIDL packages are no longer
reliably available on contemporary Linux distributions. It remains optional:
system libIDL remains the default behavior unless the build configuration opts
into `BOOTSTRAP_IN_TREE_LIBIDL`.

Where an older replaced version is not readily determinable from the current
repository history, this document lists the verified bundled version without
claiming a specific replacement version.

## Supported Platforms

ZoolRunner inherits broad platform code from Mozilla 1.8.1 and RetroZilla. The
source tree contains support code for many historical Mozilla targets, but
current build status varies by target.

Currently verified in this development branch:

* Linux browser build on LoongArch64 with GCC and GTK2.
* Linux XPFE suite build on LoongArch64 with GCC and GTK2.
* Experimental Linux browser build on LoongArch64 with GCC and xlib.

Source-present or inherited targets include:

* Win32 targets, including legacy Windows paths inherited from RetroZilla and
  Mozilla;
* Linux on other supported CPU architectures where the inherited Mozilla code and
  required xptcall/NSPR support are present;
* OS/2 support code;
* classic Unix targets inherited from Mozilla;
* macOS/Camino-era source paths; and
* XULRunner, suite, mail, Composer, calendar/Sunbird, and Minimo application
  sources.

Planned or desired platform work should be documented separately from verified
support. Architecture portability is part of keeping ZoolRunner useful as an
application runtime, not merely a browser-porting exercise.

Linux i686 and x86_64 bring-up covers GTK2 and Xlib for all four applications,
using Oracle Linux 8 containers and GCC Toolset 14. Each backend has profiles
for Suite, Browser, Calendar and XULRunner.
The [Linux build guide](build/linux/README.md) records the procedure and
validation status; compilation and runtime verification are in progress.

All OS build pipelines cover at least **Suite, Browser, Calendar and
XULRunner**. The [Windows workflow](.github/workflows/windows.yml) cross-builds
Windows x86 on Linux using MSVC 2005 and Wine, packages each application and
runs import audits and Wine regressions. Local pipeline validation is in
progress; see the [build guide](build/win32/msvc8-cross/README.md).

## Legacy Windows Compatibility

The minimum Windows targets are **Windows 95** and **Windows NT 4.0**.
The Windows 9x and NT families are independent compatibility targets. Preserve
Windows 98, Me, 2000, XP, and later compatible releases where practical, and
record tested service packs and optional updates explicitly. Older NT 3.x
support is experimental rather than a current minimum requirement.

Windows builds are produced on **Linux or macOS hosts**, using genuine
**Microsoft Visual C++ 2005 (MSVC 8.0 / VC8)** through **Wine**. CrossOver can
provide Wine on macOS. Configure, make, and host utilities run natively; the
Windows compiler, linker, and resource tools run under Wine. Historical VC6 and
native-Windows build instructions do not describe the current workflow.

Use the static CRT and the checked-in cross-build wrappers. The legacy Suite
configuration aggregates ordinary XPCOM components to stay within the old
systems' TLS limits and uses the shared process-heap integration for cross-DLL
allocations. See the [Windows build guide](build/win32/msvc8-cross/README.md).

Minimum targets are not a claim that every current binary has been verified on
those systems. The [compatibility status](build/win32/msvc8-cross/COMPATIBILITY.md)
records unresolved CRT/import blockers and the required Windows 95 and NT 4
runtime checks. A successful build or Wine launch alone does not establish
compatibility. The [Suite VM regression payload](build/win32/msvc8-cross/tests/README.md)
runs existing JavaScript and GUI checks with an isolated profile and saved logs.
The 2026-09-16 Suite package passed all 17 regression groups on NT4 reporting
SP6, Windows Me and Windows 2000 SP4; see the compatibility status for scope
and remaining Windows 95 validation.
Do not raise the OS or compiler requirements merely for convenience.

## Compatibility Philosophy

The central compatibility principle is:

**Preserve application-facing APIs where practical while modernizing
implementations underneath them.**

Updating an underlying library should ideally not require existing XULRunner or
Mozilla-based applications to change how they use the corresponding Mozilla APIs.
Do not remove classic Mozilla functionality merely because later Mozilla releases
removed it. XUL, XBL, XPCOM, XPConnect, XPIDL, chrome packages, privileged
JavaScript, and the classic Mozilla extension/application architecture are
intentional features of ZoolRunner.

Compatibility-sensitive identifiers should not be renamed casually. This includes
XPCOM contract IDs, XPCOM interface names, XPIDL interfaces, XPT typelibs,
Mozilla and XUL namespace URIs, preference names, protocol identifiers,
application-facing APIs, historical XULRunner conventions, and user-agent
compatibility tokens where changing them could break existing applications.

## macOS Application Build Matrix

macOS mozconfigs cover Suite, Browser, Calendar/Sunbird, and XULRunner for both
Apple Silicon and x86_64. The XULRunner profiles also build the Simple example
and standalone Layout Debugger. These profiles require **macOS SDK 11.3**.
See [macOS build instructions](mozconfigs/macos/README.md) for configuration
paths, dependencies, packaging, and the eight-job GitHub Actions workflow.
All eight configurations pass local act build, package and artifact-upload
jobs with the upstream act artifact-server fix. Package compatibility checks
and Calendar's existing unit tests pass on arm64 natively and x86_64 through
Rosetta. GitHub-hosted execution awaits the first workflow run.

Experimental [i386 profiles and PowerPC research](mozconfigs/macos/i386/README.md)
use SDK 10.4u for i386 target code and SDK 11.3 for native host utilities. The
explicit i386 deployment target is 10.4; actual runtime compatibility remains
unverified. All four i386 applications pass local `act` build and packaging
checks in [macos-i386.yml](.github/workflows/macos-i386.yml); artifact upload
hits the same local server limitation. A Linux-hosted PowerPC cross-toolchain
has built and packaged Suite against SDK 10.3.9, with target ABI assertions and
static dependency-location checks passing for 110 packaged binaries. This is an
intermediate milestone toward the required PowerPC minimum of Mac OS X 10.0.
Foundation/C++, shared-library template coalescing and C++ ABI runtime probes
now execute successfully on original 10.0 under PowerPC emulation. The runtime
probe covers allocation, RTTI, exception unwinding and threaded initialization.
The shared C++ library, NSPR threading/loading/semaphore probe, and SQLite
concurrency tests also pass on original 10.0. A Linux workflow now exists in
[macos-powerpc.yml](.github/workflows/macos-powerpc.yml). All four applications
compile and package on Linux, with ABI and deployment audits passing. Suite,
Browser, Calendar and XULRunner pass the full local `act` workflow, including
original-OS GUI checks and both artifact uploads. Uploaded packages and logs
pass integrity checks. GitHub-hosted runs and physical PowerPC hardware remain
untested; the completed runtime checks use original Mac OS X 10.0 under emulation.
The early Quartz software path passes pixel comparisons on both the host and
original 10.0, and original-OS ATSUI text placement now passes as well. The Linux guest
runner also completes automated C++ runtime checks. NSS initialization,
cryptographic known answers, authenticated-decryption rejection, and SQL database
creation now pass on original 10.0, as does a Cocoa window/event-loop probe.
All four packaged application GUI tests pass on original 10.0. Toolkit
launchers bind imports at load to avoid the original-dyld startup bus error;
Browser navigation, Calendar's event component and standalone XULRunner
JavaScript/C++ components pass. Packaging also accounts for an
original-tar bug that corrupts exactly full 100-byte archive filename fields;
both host and target tests cover that boundary.
All four complete packages now pass the original-OS platform probes and 167
targeted JavaScript checks. The early SDK preserves Darwin's 32-bit locale-table
ABI alongside Mozilla's 16-bit `wchar_t`, and corrected startup hooks allow
Cocoa components to load after process startup under eager binding.
Cairo image-frame row order and ARGB readback now have passing component tests
on native arm64 and original-10.0 PowerPC; Suite screenshot checks also verify
toolbar orientation and translated text placement.
See the [10.0 progress and constraints](mozconfigs/macos/powerpc/10.0-status.md).
The same test environment can be copied into UTM for interactive testing; see
the [UTM configuration and startup guide](mozconfigs/macos/powerpc/utm.md).
The 32-bit Cocoa application loop now routes Command shortcuts through native
menus and Gecko. Original-10.0 Browser and Suite checks cover text selection,
clipboard commands and Quit. Calendar selection/Quit and standalone XULRunner
text entry also pass; see the UTM guide for the input results.
The existing 64-bit AppKit event path remains in use on modern macOS.

XML parsing uses bundled Expat 2.8.4 with the classic Gecko pause/replay
interface. Regression coverage includes stylesheet/script pauses and rendered
network-error pages; see the [Expat integration notes](parser/expat/README.zoolrunner.md).

## XULRunner Application Compatibility

XULRunner-style standalone applications are a first-class use case. ZoolRunner
should continue to support application manifests, chrome registration, XPCOM
components, XPConnect integration, profiles, preferences, and privileged
JavaScript patterns used by historical Mozilla applications.

When platform internals are modernized, application-visible behavior should
remain compatible where practical. New behavior should be documented when it
changes assumptions that existing applications may depend on.

## ECMAScript Compatibility

The pinned official ES5 Test262 suite passes **11,540 / 11,540 cases** in all
four macOS application packages on arm64 and x86_64 (through Rosetta), with
zero failures, crashes, timeouts, or harness errors. This includes
the suite's earlier-edition coverage and every annotated strict-mode case.
Unmarked cases use the upstream non-strict default. The existing SpiderMonkey
engine implements strict execution, eval environments, arguments snapshots,
property descriptors and integrity controls, JSON, function binding, and the
associated parsing and built-in corrections. Test source and assertions are
unchanged; Unicode transport, directive prologues, expected exceptions, and
historical timezone requirements are preserved by the runner.

Full specification correctness remains the project target; a passing finite
suite is not an exhaustive proof. See [ES5 testing](js/tests/es5/README.md) for
the pinned revision, reproducible commands, historical measurements, and the
application/embedding validation record. Historical Mozilla embedding APIs and
application compatibility remain requirements. Both the Suite and XULRunner
builds pass the full suite, 394 focused JavaScript assertions, 18 embedding
checks, and 169 layout assertions; Suite live HTTPS navigation also passes.
These results are from macOS 15.7.1 arm64 and do not establish legacy Windows
runtime compatibility.

ChatZilla's startup `Object.hasOwnProperty` error was an engine bootstrap
regression. Object's ES5 static methods now use the normal class initialization
path, avoiding a premature constructor lookup in window globals. Unchanged
ChatZilla initializes in both Suite and a temporary standalone XULRunner
wrapper. Chrome/content window regression checks also cover the constructor
chains and legacy methods. This verifies startup, not IRC network operation
or compatibility with every historical application.

Calendar's legacy accessor syntax and XBL method receivers are also preserved
in the engine, without changing Calendar scripts. Explicit historical language
versions accept the older accessor forms; default ES5 and strict grammar still
reject invalid forms. Declarative eval/function scopes remain distinct from
DOM/XBL object scopes. Compatibility checks cover 58 legacy-language assertions,
17 chrome/content and event-handler assertions, and Calendar's eight unchanged
unit tests, including memory/SQLite providers. Fresh-profile Calendar GUI tests
exercise startup and navigation in day, week, multiweek, and month views.
Composer's historical regexp syntax is restored for legacy scripts;
unversioned XUL scripts use the classic JS 1.7 grammar while HTML scripts keep
the ES5 default. Composer edit/undo/close cycles now pass with no late command
observer errors. The native updater stops at document teardown, and JSD script
enumeration remains safe when callbacks collect scripts or stop debugging.
See [application lifecycle checks](editor/composer/tests/README.md).

## Web Compatibility

Full compatibility with the contemporary Web is not currently a ZoolRunner
project goal. Selective web-platform improvements are acceptable when they are
useful to Mozilla applications, improve robustness, or fit naturally into the
existing engine.

The included browser should remain functional, useful for compatible content, and
reasonably stable. It should avoid crashing on malformed or unsupported content.
It does not need to correctly render every contemporary website for the platform
to be successful.

The Basilisk website is a bounded HTML/CSS integration target. Structural
layout, responsive menus, FAQ checkbox toggles, circular radii, single-layer
background sizing, and linear gradients are covered by the
[layout probes](layout/html/tests/style/README-probes.md). Shadows, transitions,
and keyframe animation remain unfinished; downloadable fonts are excluded from
this rendering task. Multiple background layers and other newer CSS syntax
are not implied by the implemented subset.

The Pale Moon website is another required HTML/CSS rendering target, excluding
external font libraries and downloads. Its homepage and shared subpage styles
require flexbox alignment, layered backgrounds, shadows, and opacity groups.
Complete rendering is not yet verified. Native opacity groups now use an
optional rendering-context interface; historical backends retain the existing
blender path. See the layout probes for painting coverage.

## Stability and Security

A major current objective is to eliminate reproducible crashes and fix
memory-safety/security defects in the historical codebase. Relevant defect
classes include use-after-free bugs, buffer overflows, out-of-bounds memory
access, pointer truncation, integer overflow, null-pointer dereferences,
undefined behavior, LP64/64-bit assumptions, unsafe parsing of untrusted input,
and malformed-input handling.

ZoolRunner should not be described as secure, memory-safe, audited, or suitable
as a hardened replacement for a current mainstream web browser. It is an old
Mozilla codebase undergoing substantial stabilization and modernization.

## Building

The currently used Linux development build is:

```sh
cp mozconfigs/linux/loongarch64/gtk2_browser_gcc.mozconfig mozconfig
make -f client.mk build
```

The checked-in Linux mozconfig examples currently cover the GTK2 browser, GTK2
suite, GTK2 XULRunner, Xlib browser, and Xlib suite builds. The GTK2 browser
configuration uses `-j8`, bundled JPEG/zlib, crypto, SVG, canvas, and the
optional in-tree libIDL bootstrap.

For Windows x86 builds, run the MSVC 2005 tools through Wine from Linux or
macOS. For the legacy Suite configuration:

```sh
MSVC8_ROOT=/path/to/msvc8.0 \
WINE=wine \
MOZCONFIG="$PWD/mozconfigs/cross/win32-msvc8-suite-legacy.mozconfig" \
make -f client.mk build
```

The [Windows build guide](build/win32/msvc8-cross/README.md) describes toolchain
layout, macOS/CrossOver selection, the XULRunner configuration, packaging, and
PE auditing. [Mozconfig examples](mozconfigs/README.md) also cover native
macOS arm64 Cocoa builds.

For incremental builds, run `make` from the corresponding directory in the object
directory after a full build has completed. For example, after changing a file
under `xpfe/browser/resources/content`, build the matching object-directory
subdirectory instead of rebuilding the entire tree.

## Development Philosophy

Prefer narrow, compatibility-preserving changes. Modernize implementations and
build support where that improves maintainability, portability, diagnostics, or
runtime stability, but avoid gratuitous API churn.

Do not remove historical Mozilla application technologies simply because they are
unusual by contemporary standards. Those technologies are the reason ZoolRunner
exists.

## Contributing

Contributions should preserve existing operating-system and CPU architecture
support unless a compatibility break is explicitly discussed and accepted. Pay
particular attention to legacy Windows, XULRunner application compatibility, and
Mozilla-facing API stability.

Bug fixes should include enough context to explain whether they affect platform
APIs, application behavior, or only an implementation detail. Build-system
changes should keep generated files and dependency build products inside the
source tree or the configured object directory.

## License and Credits

ZoolRunner inherits Mozilla and RetroZilla source, licensing, and attribution.
See the license files and in-tree credits for the applicable terms and historical
contributors.
