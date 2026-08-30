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
MOZCONFIG=/path/to/ZoolRunner/mozconfig make -f client.mk build
```

The build configuration is intentionally kept inside the source tree and uses
`obj-zoolrunner-linux` as the object directory. When the optional in-tree libIDL
bootstrap is enabled, libIDL is also built inside that object directory.

An experimental xlib browser build is also available through `mozconfig.xlib`.
It builds into `obj-zoolrunner-xlib` and is used to exercise the restored
low-dependency X11 widget path. This backend is less complete than the GTK2
build and should currently be treated as development work.

The XPFE suite can be built on the same Linux/GTK2 development system through
`mozconfig.suite`:

```sh
MOZCONFIG=/path/to/ZoolRunner/mozconfig.suite make -f client.mk build
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

## Changes from RetroZilla

Significant work completed after the RetroZilla baseline includes:

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
* Fixed XPFE Preferences pane switching and pane visibility so selecting pages
  in the left-hand tree updates and displays the selected preference pane.
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
* SQLite used by the Mozilla storage layer: `3.3.5`, in
  `db/sqlite3/src/sqlite3.h`.
* SQLite bundled inside NSS: `3.7.15`, in `security/nss/lib/sqlite/sqlite3.h`.
* libpng: `1.2.35`, in `modules/libimg/png/png.h`.
* zlib used by the Mozilla tree: `1.3.2`, in `modules/zlib/src/zlib.h`.
* zlib bundled inside NSS: `1.2.5`, in `security/nss/lib/zlib/zlib.h`.
* bzip2/libbzip2: updated from `1.0.3` to `1.0.8`, in
  `modules/libbz2/src/bzlib_private.h`.
* IJG JPEG library: version `6b`, represented by `JPEG_LIB_VERSION 62` in
  `jpeg/jpeglib.h`.
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

## Legacy Windows Compatibility

Preserving RetroZilla's unusually broad legacy Windows compatibility is a
deliberate ZoolRunner goal. Existing project documentation and source support
cover Windows 95, Windows 98, Windows Me, Windows NT 3.51, Windows NT 4.0,
Windows 2000, and Windows XP-era systems. The tree also contains installer and
compatibility code for later 32-bit-compatible Windows releases, but newer
Windows versions should not be claimed as tested unless they have actually been
tested.

Do not unnecessarily raise the minimum Windows version. Avoid adding dependencies
on newer Windows APIs for convenience when an existing implementation works on
the historical supported systems. Do not require a newer Microsoft compiler
solely for compiler modernization if doing so would break the legacy Windows
build environment.

The historical Windows toolchain may intentionally remain the supported legacy
Windows build environment. Current project documentation inherited from
RetroZilla describes Visual Studio 6.0, VC6 SP5, the VC6 Processor Pack, and
MozillaBuild 1.2 for release-style Win32 builds, with MozillaBuild 1.5 noted for
Windows XP/2003 x64 build hosts.

Using a historical compiler for legacy Windows compatibility does not mean the
entire project must be developed with historical tools. Contemporary GCC and
Clang on Linux are useful for diagnostics, warnings, sanitizers, optimization,
static analysis, architecture bring-up, undefined-behavior discovery, and
memory-safety work.

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

## XULRunner Application Compatibility

XULRunner-style standalone applications are a first-class use case. ZoolRunner
should continue to support application manifests, chrome registration, XPCOM
components, XPConnect integration, profiles, preferences, and privileged
JavaScript patterns used by historical Mozilla applications.

When platform internals are modernized, application-visible behavior should
remain compatible where practical. New behavior should be documented when it
changes assumptions that existing applications may depend on.

## Web Compatibility

Full compatibility with the contemporary Web is not currently a ZoolRunner
project goal. Selective web-platform improvements are acceptable when they are
useful to Mozilla applications, improve robustness, or fit naturally into the
existing engine.

The included browser should remain functional, useful for compatible content, and
reasonably stable. It should avoid crashing on malformed or unsupported content.
It does not need to correctly render every contemporary website for the platform
to be successful.

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
MOZCONFIG=/path/to/ZoolRunner/mozconfig make -f client.mk build
```

The checked-in Linux `mozconfig` currently builds the browser application with
`-j8`, GTK2, bundled JPEG/zlib, crypto, SVG, canvas, and an optional in-tree
libIDL bootstrap.

Legacy Windows build documentation inherited from RetroZilla uses Visual Studio
6.0-era tooling. Keep those paths working when making changes that affect Win32
build logic.

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
