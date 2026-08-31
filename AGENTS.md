# AGENTS.md — ZoolRunner Development Instructions

## Project Overview

ZoolRunner is a continuation of RetroZilla, ultimately derived from the Mozilla 1.8.1 codebase.

The project exists primarily to preserve, stabilize, maintain, and selectively modernize the classic Mozilla application platform.

ZoolRunner is **not primarily a web browser project**.

The primary goal is a general-purpose platform/runtime for:

* XULRunner applications
* standalone XUL applications
* Mozilla-based desktop applications
* applications using XUL, XBL, XPCOM, XPConnect, XPIDL/XPT, Necko, chrome packages, privileged JavaScript, and related classic Mozilla technologies

The included browser is important, but it is an application built on the platform rather than the purpose of the platform.

The browser is maintained because it exercises an unusually large portion of Mozilla and therefore serves as an excellent integration test and platform sanity check.

The full Mozilla Suite is similarly important because it exercises components not necessarily used by the standalone browser, including MailNews, Composer, Address Book, editor functionality, and additional application infrastructure.

When making architectural decisions, optimize for ZoolRunner as an **application platform**, not merely as a browser.

---

# Project Lineage

The project lineage is:

```
Mozilla 1.8.1
    ↓
RetroZilla
    ↓
ZoolRunner
```

Preserve appropriate historical Mozilla and RetroZilla attribution.

Do not remove historical authorship, copyright, licensing, or attribution merely as part of ZoolRunner branding work.

Do not gratuitously rename Mozilla technologies or compatibility interfaces.

---

# Core Development Philosophy

The general ZoolRunner philosophy is:

**Preserve the platform. Modernize the implementation. Fix the bugs.**

Later Mozilla code, UXP, and other Mozilla descendants may be used as sources of:

* security fixes
* crash fixes
* memory-safety fixes
* compiler fixes
* portability fixes
* dependency updates
* standards implementations
* useful isolated features

However, newer Mozilla architecture is not automatically considered preferable.

Do not adopt a later Mozilla architectural decision merely because Mozilla eventually made that decision.

Evaluate changes according to ZoolRunner's goals.

---

# Preserve the Classic Mozilla Application Platform

The following technologies are intentional parts of ZoolRunner and should not be removed or replaced merely because later Mozilla versions removed them:

* XUL
* XBL
* XPCOM
* XPConnect
* XPIDL
* XPT typelibs
* chrome packages
* privileged JavaScript
* classic Mozilla extensions
* Necko
* RDF where existing applications require it
* classic Mozilla application packaging
* XULRunner application conventions

Preserve historical application-facing APIs wherever reasonably practical.

Do not gratuitously rename or change:

* XPCOM contract IDs
* XPCOM interfaces
* XPIDL interfaces
* XPT interfaces
* Mozilla namespace URIs
* XUL namespace URIs
* preference names
* protocol identifiers
* chrome URLs
* historical XULRunner identifiers
* application IDs when compatibility depends upon them
* externally visible interfaces used by existing Mozilla/XULRunner applications

The project name is ZoolRunner. The underlying Mozilla technology does not need to be renamed to pretend Mozilla never existed.

---

# Application Compatibility

Compatibility with historical XULRunner and Mozilla applications is an important goal.

The preferred strategy is:

```
historical application
        ↓
historical Mozilla-facing API
        ↓
modernized ZoolRunner implementation
```

Whenever practical, modernize the implementation underneath an existing API rather than requiring applications to change.

Do not break historical application compatibility merely to make an internal implementation look newer.

---

# Browser Policy

ZoolRunner includes and maintains a browser.

The browser is **not the primary product objective**.

It is important because it exercises a large amount of:

* Gecko
* SpiderMonkey
* XUL
* XBL
* XPCOM
* XPConnect
* Necko
* protocol handlers
* networking
* security infrastructure
* image decoding
* graphics
* layout
* DOM
* JavaScript bindings
* preferences
* profiles
* chrome registration
* themes
* localization
* history
* bookmarks
* downloads
* platform widgets
* event handling

Therefore:

* The browser must continue to build.
* The browser must continue to run.
* Major browser chrome functionality must work.
* Preferences must work.
* Profiles must work.
* Navigation must work.
* Supported content must render correctly.
* Unsupported content should fail gracefully.

Do not consider a platform-level change validated merely because it compiles.

The browser should be used as a major integration test.

---

# Full Suite Policy

The full Mozilla Suite is an important ZoolRunner regression target.

The Suite currently provides broader platform coverage than the standalone browser because it includes functionality such as:

* Navigator
* Mail & News
* Composer
* Address Book
* editor functionality
* additional preferences/chrome
* additional networking/protocol functionality

Platform-level changes should preserve the ability to build the full Suite wherever practical.

When practical, test that the Suite actually runs rather than merely compiling.

Major Suite functionality should be exercised during stabilization.

Do not remove a component merely because the standalone browser does not use it.

---

# Standalone Application Testing

ZoolRunner should be tested using standalone XUL/XULRunner applications in addition to the browser and Suite.

Useful test applications include:

* historical XULRunner applications
* standalone IRC clients
* Gopher clients
* RSS readers
* calendar applications
* multimedia applications
* small reference/test applications

Third-party historical XULRunner applications are particularly valuable because they test the external compatibility surface rather than merely code built from the ZoolRunner source tree.

---

# Stability and Security Are High Priorities

A major ZoolRunner objective is eliminating crashes and memory-safety/security defects from the historical Mozilla codebase.

Actively investigate and fix:

* reproducible crashes
* use-after-free bugs
* buffer overflows
* out-of-bounds reads
* out-of-bounds writes
* pointer truncation
* integer overflow
* incorrect allocation sizes
* null-pointer dereferences
* uninitialized values
* undefined behavior
* invalid shifts
* alignment bugs
* LP64 assumptions
* 32-bit assumptions
* architecture assumptions
* unsafe malformed-input handling
* parser bugs
* resource lifetime problems
* other memory-safety defects

Use modern compiler diagnostics as a bug-finding mechanism.

Use ASan, UBSan, and other useful tooling where practical.

Do not claim ZoolRunner is completely secure, audited, memory-safe, or equivalent in security to a current mainstream browser.

ZoolRunner is a historical browser/application platform undergoing stabilization and modernization.

A central principle is:

**Unsupported functionality may fail. It must fail safely.**

A modern web page using unsupported functionality is allowed to render incorrectly or fail to execute.

It should not be allowed to crash or corrupt the process.

---

# Compiler Warning Policy

Do not make "zero warnings" an objective by itself.

Prioritize warnings that may identify genuine defects, particularly:

* pointer/integer conversions
* narrowing/truncation
* signed/unsigned size problems
* uninitialized variables
* array-bound problems
* allocation-size problems
* incorrect `sizeof`
* format-string mismatches
* function-pointer mismatches
* missing returns
* undefined shifts
* overflow
* strict-aliasing violations
* lifetime problems

Do not create huge mechanical commits merely to silence harmless historical warnings.

Understand a warning before changing historical Mozilla code to eliminate it.

---

# Web Compatibility

Full compatibility with the contemporary Web is **not a primary ZoolRunner goal**.

Do not turn ZoolRunner into an attempt to recreate twenty years of later Gecko development.

Selective standards improvements are welcome when:

* reasonably self-contained
* useful to ZoolRunner applications
* useful to MailNews HTML rendering
* useful to compatible web content
* achievable without major architectural replacement
* helpful for correctness or interoperability

Acid2 and Acid3 may be used as bounded standards/regression targets.

Passing such tests is useful but does not justify destroying the project's architecture.

Do not introduce major later-Gecko subsystems solely to pass a web compatibility test.

---

# JavaScript / ECMAScript Direction

Improving SpiderMonkey is acceptable.

A reasonable future target is strong ECMAScript 5 compatibility.

Selected ECMAScript 2015/ES6 features may also be implemented when practical.

Potentially useful additions include things such as:

* `let`
* `const`
* useful standard-library improvements
* selected syntax improvements
* other reasonably self-contained features

Do not automatically attempt complete current ECMAScript compatibility.

Do not replace SpiderMonkey wholesale merely to obtain modern JavaScript.

Do not import enormous portions of later SpiderMonkey without first determining whether the functionality can be implemented cleanly in the existing engine.

If a JavaScript feature requires disproportionate architectural work, it may be intentionally omitted.

---

# Third-Party Dependency Policy

Prefer maintained/current third-party libraries where practical.

The normal strategy should be:

```
existing ZoolRunner/Mozilla API
          ↓
updated third-party implementation
```

Do not unnecessarily redesign Mozilla code simply because an updated dependency exposes newer APIs.

## Version policy

Use the latest stable upstream release when it can reasonably satisfy ZoolRunner's compatibility requirements.

Small compatibility patches are acceptable.

Examples include:

* old compiler compatibility macros
* missing standard headers
* `inline` compatibility
* small CRT compatibility helpers
* compiler-specific attributes
* small OS compatibility shims
* disabling unsupported optional optimizations
* build-system integration fixes
* small portability changes

Do NOT substantially rewrite a third-party library merely to use its newest release.

If the latest release requires extensive invasive changes, use the newest reasonably maintainable version instead.

A deliberate dependency freeze is acceptable when required for compatibility.

Document why a dependency is frozen.

## Platform-specific versions

One dependency version across all platforms is preferable.

However, platform-specific versions are acceptable when necessary.

For example:

```
modern systems
    → current library

legacy Windows
    → newest practical compatible release
```

Do not sacrifice important historical platform compatibility merely to increment a dependency version.

## Dependency changes

Update dependencies individually.

Build and test after each significant dependency update.

Keep dependency commits focused.

Do not update several unrelated libraries in one commit unless there is a genuine technical dependency between those changes.

## Modified bundled copies

Do not blindly consolidate duplicate libraries.

Some Mozilla/NSS copies may contain substantial local modifications.

Before replacing or consolidating a bundled library:

1. compare it with pristine upstream
2. identify Mozilla modifications
3. identify RetroZilla modifications
4. identify ZoolRunner modifications
5. determine whether those modifications remain necessary

Examples requiring special care include:

* NSS private zlib
* NSS/JAR zlib-derived code
* NSS private SQLite
* DBM implementations
* fdlibm

---

# Known Third-Party Dependency Direction

Maintain accurate versions in the README/documentation.

Current modernization work includes or may include:

* zlib
* bzip2
* libjpeg/libjpeg-turbo
* SQLite
* Expat
* libpng
* NSS
* NSPR
* Cairo
* pixman
* Boehm GC

Do not infer that all of these should automatically be upgraded to current upstream.

Evaluate each individually.

Some components require additional caution:

## libIDL

libIDL 0.8.14 is the final upstream release.

It has been vendored because contemporary systems may no longer package it.

Do not replace XPIDL merely to eliminate libIDL.

A future self-contained XPIDL implementation may be considered separately.

## fdlibm

Treat SpiderMonkey's fdlibm code conservatively.

Floating-point behavior may depend on it.

Do not casually replace it with the host system math library.

## NSPR

Treat NSPR upgrades cautiously because NSPR is deeply involved in:

* threading
* synchronization
* sockets
* filesystem behavior
* platform abstraction
* legacy Windows support

Do not upgrade NSPR without carefully validating legacy platforms.

## NSS

The ZoolRunner/RetroZilla tree contains a substantially newer/customized NSS than the original Mozilla 1.8.1 code.

Understand the existing integration before performing major NSS changes.

## Cairo/pixman

The bundled Cairo/pixman versions are extremely old.

A major version jump may constitute a graphics-port project rather than a routine dependency update.

Do not perform such a migration casually.

---

# Build-System Policy

Use ZoolRunner's existing configure/make-based build system.

Do not introduce a new meta-build system merely because a dependency uses one upstream.

In particular, do not introduce:

* CMake
* Meson
* Ninja
* mach

Third-party libraries bundled with ZoolRunner should normally build through ZoolRunner's existing build infrastructure.

Do not require developers to separately configure/build bundled dependencies using their upstream build systems.

Avoid unnecessary build-time dependencies.

A checkout should contain enough of its obscure or obsolete build dependencies to remain reasonably buildable on contemporary systems.

---

# Shell Portability

Shell scripts should use portable POSIX shell syntax wherever practical.

Do not introduce Bash-specific syntax without a genuine platform-specific requirement.

Avoid assuming GNU userland when portable alternatives exist.

ZoolRunner is intended to remain viable on traditional Unix and unusual Unix-like systems.

---

# Static/Self-Contained Builds

Mostly static/self-contained ZoolRunner builds are desirable where practical.

Bundled third-party libraries may be linked directly into ZoolRunner.

A standalone ZoolRunner application should ideally not depend on a large collection of exact third-party library versions installed by the host OS.

Do not make 100% static linking an ideological requirement.

Dynamic dependencies on fundamental host platform interfaces may remain appropriate.

For example, an Xlib build may reasonably depend on system X11 interfaces.

The goal is deployment portability, not static linking for its own sake.

---

# Legacy Windows Compatibility

Preserving RetroZilla's legacy Windows compatibility is an important ZoolRunner requirement.

Historical supported targets include:

* Windows NT 3.51
* Windows 95
* Windows NT 4.0
* Windows 98
* Windows 98 Second Edition
* Windows Me
* Windows 2000
* Windows XP
* later compatible Windows releases

Do not unnecessarily raise the minimum Windows version.

Do not introduce newer Win32 API dependencies merely for convenience.

## Compiler

The legacy Windows build uses Microsoft Visual C++ 6 / VC6.

Preserve VC6 source compatibility where reasonably practical.

Do not require a newer Microsoft compiler merely because it is newer.

Modern compiler diagnostics, sanitizers, and architecture work may be performed on Linux using current GCC/Clang.

The Windows build may intentionally remain based on a historical compiler in order to preserve old Windows compatibility.

If a VC6-generated 32-bit executable works correctly on both historical and current Windows versions, a separate modern-MSVC build is not inherently required.

## Dependency updates

Dependency updates must take legacy Windows into account.

Small VC6 compatibility patches are acceptable.

Do not perform major third-party rewrites merely to support VC6.

If necessary, freeze the legacy Windows build at the newest practical compatible dependency version.

Do not claim NT 3.51 or Windows 95 compatibility for a changed dependency until actually tested.

## Older Windows experiments

Windows NT 3.1, Windows NT 3.5, and Windows 3.1/Win32s are interesting possible experimental targets.

They are not currently hard compatibility requirements.

Do not compromise required NT 3.51/Windows 95 support merely to support these experiments.

---

# CPU Architecture Portability

Do not assume x86.

Do not assume x86_64.

Do not assume 32-bit pointers.

Do not assume little-endian unless an algorithm explicitly requires it.

Do not assume unaligned memory access is safe.

Do not use pointer-to-32-bit-integer conversions.

Generic code should remain suitable for unusual architectures.

Important/current or potential targets include:

* x86
* x86_64
* LoongArch
* AArch64
* Alpha
* other Unix architectures where practical

## LoongArch

LoongArch is an intentional ZoolRunner architecture target and portability test.

Do not treat LoongArch support as disposable or incidental.

LoongArch is useful for exposing assumptions hidden by x86-centric development.

## AArch64

AArch64 Linux is a desired future target.

Prefer bringing AArch64 up on Linux before native arm64 macOS.

This separates architecture problems from macOS platform problems.

## Alpha

Alpha is an interesting future portability target.

Do not claim current Alpha support unless tested.

If Alpha work is undertaken, a Unix-like Alpha environment should generally be used to establish architecture support before attempting OpenVMS.

---

# Unix Toolkit Strategy

ZoolRunner should not become dependent on one continually changing Unix GUI toolkit.

Multiple historical backends are valuable.

## GTK2

GTK2 remains an important existing Mozilla backend.

Preserve it.

Do not remove GTK2 merely because newer GTK generations exist.

## gtk2-ng

A maintained GTK2 API/ABI-compatible implementation such as gtk2-ng is a desirable option for contemporary Linux systems.

The intended relationship is:

```
ZoolRunner GTK2 backend
        ↓
   GTK2 API/ABI
    /        \
GTK2       gtk2-ng
```

ZoolRunner should ideally not require gtk2-ng-specific application code.

If gtk2-ng provides full GTK2 API/ABI compatibility, ZoolRunner should continue targeting the GTK2 interface rather than becoming tied specifically to gtk2-ng.

A bundled gtk2-ng may eventually be appropriate for official Linux builds while retaining support for system GTK2.

## Xlib

The historical Mozilla Xlib backend should be restored and maintained where practical.

Xlib is valuable as a minimal/common-denominator Unix GUI backend.

Potential uses include:

* minimal Linux systems
* BSD
* traditional Unix
* unusual architectures
* systems without GTK
* systems where GTK is undesirable
* XQuartz on macOS
* portability testing

Avoid adding Linux-specific assumptions to the Xlib backend.

Keep it as close as practical to ordinary X11/Xlib interfaces.

The Xlib backend may contain significant historical bitrot. Fix root causes rather than replacing it with a new toolkit merely because restoration requires work.

## Qt 3

The tree contains a historical Qt 3 backend.

Preserve genuine Qt 3 compatibility where practical.

## Motif

A Motif backend may be interesting but is not currently required if the existing Xlib backend provides the desired minimal X11 fallback.

Do not implement another toolkit backend without considering whether an existing historical backend already solves the problem.

## GTK3/GTK4

ZoolRunner should not migrate to GTK3 or GTK4.

GTK3 and GTK4 are not desired toolkit targets for this project.

Do not start GTK3 or GTK4 porting work.

Do not add GTK3 or GTK4 as new supported frontends.

Do not introduce GTK3 or GTK4 dependencies into generic Unix code, bundled
third-party libraries, application code, or build-system defaults.

The preferred Unix toolkit strategy is to preserve and maintain the existing
GTK2 backend, keep it compatible with genuine GTK2 where practical, and allow
maintained GTK2-compatible implementations such as gtk2-ng to serve as modern
providers of that API/ABI.

For environments where GTK2 is unavailable or inappropriate, prefer restoring
or maintaining historical alternatives such as Xlib and Qt 3 rather than
porting ZoolRunner to GTK3 or GTK4.
