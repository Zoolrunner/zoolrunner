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

# Documentation Maintenance

Update the relevant documentation alongside code changes. Keep `readme.md`,
applicable build and test documentation, and this `AGENTS.md` aligned when
features, support policy, or development workflows change. Document material
limitations and distinguish intended compatibility from verified build and
runtime results.

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

For Cocoa activation/focus changes, test typing immediately after opening the
first window, before switching applications. Verify text insertion as well as
Command shortcuts, a second window, and reactivation. Include the case where
the native window becomes key before its Gecko first responder is installed.
Use disposable test profiles and leave existing user processes/profiles intact.

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

The Basilisk (`basilisk-browser.org`) and Pale Moon (`palemoon.org`) websites
are required HTML/CSS rendering regression targets. Downloadable fonts and
external font libraries are outside this work's scope. Implement the needed
features in the engine rather than adding site-specific rendering exceptions.
Do not claim complete rendering based only on parser/CSSOM tests: compare
painting and layout, exercise menus and resizing, and test the browser and Suite.
Track remaining feature gaps in the README and layout probe documentation.

Acid2 and Acid3 may be used as bounded standards/regression targets.

Passing such tests is useful but does not justify destroying the project's architecture.

Do not introduce major later-Gecko subsystems solely to pass a web compatibility test.

---

# JavaScript / ECMAScript Direction

Improving SpiderMonkey is acceptable.

Full ECMAScript 5 compatibility, using the corrected ECMAScript 5.1
specification, is a required modernization target. The pinned historical
Test262 suite now passes all 11,540 required-mode cases on macOS arm64 and every
Linux aarch64 application/backend matrix entry; this is not an exhaustive
proof of specification correctness. Preserve that result and
track coverage with the pinned suite and focused native
regressions in `js/tests/es5`; report failures honestly. Adding standard-library
method names alone does not establish conformance: strict mode, descriptors,
invocation semantics, parsing, and built-in behavior must also be correct.
The completion target is zero failures in the complete pinned ES5.1 Test262
suite, including inherited earlier-edition coverage. Do not exclude failing
cases, weaken their assertions, or substitute focused test totals for a full
run. Use the pinned upstream runner's mode policy: unmarked tests run in
non-strict mode, and every `onlyStrict` case runs in strict mode. This historical
branch explicitly states that not all unmarked tests are strict-compatible.
Keep optional `--unmarked-default both` runs as additional diagnostics and label
them separately; do not change language semantics to satisfy contradictory
requirements introduced by forcing legacy cases into strict mode. Passing the suite is not a proof that every specification behavior has
been tested. Keep any earlier-edition tests separate when ES5 intentionally
changes their required semantics. The conformance runner must preserve Unicode
source through the shell's Unicode global-script compiler and pass its source
transport preflight; the historical byte-oriented `load` path is not suitable
for the upstream UTF-8 test files. Compile harness setup separately in the same
global object so it cannot mask a test's own directive prologue or satisfy an
expected exception. Follow the upstream negative exception patterns. Record
the timezone: the historical fixed-date cases require America/Los_Angeles;
other timezone runs are separate portability checks. Include callback/reentrancy and garbage-
collection regressions when adding native standard-library state.

Changes to bytecode must update the bytecode cache version and preserve
function/script decompilation. Exercise engine changes in both XULRunner and
the Suite, including browser navigation and the existing layout probes.

Built-in initialization changes must also exercise real chrome and content
window globals; xpcshell alone does not cover DOM security/bootstrap ordering.
Run `js/tests/es5/window-app/application.ini` and the native embedding test,
and smoke-test unchanged historical applications such as ChatZilla in Suite
and XULRunner. Install Object static methods through JS_InitClass's constructor
reference, without reading prototype.constructor during class bootstrap.
Do not work around an engine regression by requiring old applications to change.
Preserve historical accessor syntax in explicitly selected legacy JavaScript
versions used by component/subscript loaders and unversioned XUL scripts;
retain escaped regexp line breaks used by Composer in legacy mode. Default
ES5 and strict code must retain their grammar checks. Distinguish declarative eval/function environments
from embedding object scopes: unqualified XBL/DOM/JSAPI methods need their
historical object receiver. Run `legacy-application.js`, the embedding/window
checks, and `calendar/test/run-compatibility.py` against Calendar. Exercise
Calendar startup and all four views as well as shell tests; compilation and
Test262 alone do not establish application compatibility.
Run `editor/composer/tests/run-lifecycle.py` for editor/lifecycle changes and
`js/tests/es5/debugger-lifecycle.js` for debugger changes. Do not dispatch editor
commands into dying docshells or retain raw script iterators across callbacks;
callbacks may close windows, collect scripts, or turn debugging off.

Full ECMAScript 2015 (ES6) compliance is the next required modernization target.
Track the conformance corpus, failures and implementation status in
`js/tests/es6/README.md`. All required Test262 cases must pass, including syntax,
runtime semantics, modules and asynchronous behavior; unsupported cases remain
unfinished work and must not be excluded to obtain a passing result. Preserve
the complete ES5.1 regression gate and historical application compatibility.
Keep explicit legacy language versions and historical XUL/component script
loading behavior usable while implementing the modern language semantics.
The initial ES2015 boundary uses `JSVERSION_ECMA_2015` (2015), selected by
`xpcshell -v 2015` or a script MIME `version=2015`. Keep default HTML semantics,
unversioned XUL's JS 1.7 selection and existing JSAPI version values stable.
Exercise `js/tests/es6/editions.js`, `TestEditionEmbedding.c` and the mixed-edition
window fixture when changing the boundary. Preserve saved editions through
XDR, eval, decompilation, native callbacks and calls between language versions.
Run the contextual-keyword, lexical-parameter and radix-literal regressions when
changing modern grammar. Preserve the cross-edition destructuring-exception and
strict-parameter-history checks in `js/tests/es5`: diagnostic decompilation must
not suppress exceptions, and shared property-tree flags must not make one
function's duplicate parameters invalidate unrelated strict functions.
Use `xpcshell -E` to select ES2015 before global built-in initialization. Native
embeddings select the edition before `JS_InitStandardClasses`. Keep unversioned
application globals unchanged, and exercise `function-metadata.js` with `-E`
and `TestFunctionMetadata.c` when changing function metadata or initialization.
Keep inferred names separate from declared function names so inference cannot
introduce lexical self-bindings or alter decompiled anonymous-function syntax.
Exercise `inferred-function-names.js`, including legacy accessor construction,
GC, quoted/numeric accessor decompilation and the native XDR/clone checks.
Script selection is not yet a complete policy for global built-in semantics;
do not claim that partial edition support establishes ES2015 compliance.
Run `js/tests/es6/math-integer.js` for Math conversion, signed-zero and rounding
changes; preserve MSVC 2005 compatibility and callback/GC behavior. Selected
bundled fdlibm kernels supply the ES2015 transcendental additions through a
private target-endianness adapter. Keep their historical licenses, existing
ES3/ES5 math policy and subnormal/overflow fixes. Run `math-transcendental.js`
and the optional host `test-math-kernels.py` diagnostic for numerical changes;
a host libm comparison is not target-OS runtime validation.
Run `string-additions.js` for the new String methods. Preserve UTF-16 code units,
observable conversion order, rooting across callbacks/GC, reentrant raw assembly
and checked allocation lengths. Run `string-match-classification.js` for
String search IsRegExp changes, including observable Symbol.match getters.
Classification alone does not implement the RegExp matching protocols.
Keep the private Unicode normalization data checksum-pinned and reproducible;
preserve Unicode licensing in source and packaged license pages. Run
`normalization.js` and the full pinned `test-normalization.py` check when changing
normalization. Preserve lone surrogates, composition exclusions, Hangul, stable
canonical ordering, callback rooting and checked allocation sizes. The separate
XPCOM normalizer and legacy identifier/casing tables remain unchanged.
Run `array-operations.js` for Array find/findIndex/fill/copyWithin changes.
Keep ToLength indices above uint32, snapshot lengths, live property reads,
sparse copying/deletion and callback rooting. Throw-on-rejection writes must
not leak strictness into setter callbacks or change ordinary legacy assignments.
Run `object-additions.js` in an ES2015 global for Object.is/assign and static
method initialization changes. Preserve object identity during ToObject,
ES2015 integer-index ordering through 2^53-1, snapshot keys with live
own/enumerability checks, setter dispatch and callback/GC rooting. Apply
internal constructor flags through the direct constructor reference; the public
JSFunctionSpec flag field remains eight bits for embedding compatibility.
Keep ES2015 reflection boxing and primitive integrity behavior gated by the
selected script edition. Preserve ES5/legacy primitive TypeErrors and own-name
ordering, alongside modern __proto__ ownership and integer-index ordering.
Run `prototype-mutation.js` and `TestPrototypeMutation.c` for the new
Object.setPrototypeOf path. Preserve access checks and inner/outer objects,
keep requested prototypes rooted independently of callback in/out values, and
retain classic own fields and their native private-data hooks when detaching
same-class prototypes. Do not change the trusted JS_SetPrototype API contract.
Test real applications separately: a Test262 pass cannot establish that every
historical application remains compatible.
Run `js/tests/es6/const-writes.js` when changing immutable bindings. Preserve
legacy const behavior, modern operand-coercion order, captured/eval bindings,
exception/finally behavior and decompilation. Run `test-const-large-script.py`
against the matching shell to exercise extended atom operands and object
initializer ordering beyond the 16-bit index boundary. Keep the XDR bytecode
version synchronized with new opcodes and rebuild the XPConnect loader and its
containing library before application tests. Const write checks alone do not
establish lexical scoping or temporal-dead-zone support.

Do not automatically attempt complete current ECMAScript compatibility.

Do not replace SpiderMonkey wholesale merely to obtain modern JavaScript.

Do not import enormous portions of later SpiderMonkey without first determining whether the functionality can be implemented cleanly in the existing engine.

Features beyond ECMAScript 2015 may be evaluated separately. Preserve the classic
embedding APIs while implementing the required ES5 and ES2015 behavior.

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
* Expat and Gecko's caller-owned input replay on parser suspension

For Expat changes, preserve the [adapter contract and regression coverage](parser/expat/README.zoolrunner.md), including stylesheet/script pauses and resumed tag-name storage.

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

# Application CI Coverage

Every OS and architecture must have mozconfigs for at least Suite, Browser,
Calendar and XULRunner. Every OS build pipeline must build these four apps.
Linux i686 and x86_64 bring-up and CI must run inside `oraclelinux:8` with
GCC Toolset 14. Preserve the x86 C++ profiles' `-flifetime-dse=1` and
`-fno-strict-aliasing`: frame arena zeroing precedes constructors, and
`nsCOMPtr` typed output parameters alias its stored interface pointer.
Exercise Suite's Venkman lifecycle and DOM interface globals when changing
these compiler settings. Both GTK2 and Xlib must have mozconfigs and CI jobs for all
four applications on both x86 architectures, with packaged runtime testing
for both backends. Linux aarch64 likewise has all four applications on both
backends, using native Oracle Linux 8 / GCC Toolset 14 and the
`ubuntu-24.04-arm` CI runner. Preserve the Linux AAPCS64 register/stack ABI
probe, full pinned ES5.1 checks and Calendar unit/four-view coverage. Keep
Darwin and Linux ARM64 stack argument layouts distinct. All eight ARM jobs
pass local `act` build/package/runtime/upload validation; GitHub-hosted runs
remain unverified. Use matching target multilib dependencies and keep host
tools native to the container. Record
compile, package, runtime and workflow results separately; a new mozconfig
does not establish that an architecture or application has been tested. Keep all four applications in the modern macOS, i386 macOS,
PowerPC macOS and Linux-to-Windows workflow matrices. Windows CI uses the
MSVC 2005/Wine container in `build/win32/msvc8-cross`; keep its mozconfigs,
packaging, runtime checks and build guide aligned. Do not equate Wine test
results with runtime validation on the minimum Windows versions.

# macOS Build Matrix

Keep `mozconfigs/macos/common.mozconfig`, the per-architecture application
profiles, `.github/workflows/macos.yml`, and `mozconfigs/macos/README.md` aligned.
macOS builds currently require SDK **11.3**; do not silently select a newer
host SDK. Maintain both arm64 and x86_64 configurations for complete application
targets. Keep host tools native when cross-compiling x86_64 from Apple Silicon.
CI artifacts must resolve links into the build checkout and retain executable
permissions. Distinguish local build checks from actual GitHub-hosted runs and
from runtime validation on the minimum deployment OS.

On Apple Silicon with Rosetta available, run the x86_64 packaged runtime checks
as well as arm64 checks. Compile embedding and platform probes for the package
architecture, not the host default. Use the relocated-package desktop runners
in `build/macosx/tests` for GUI regressions. Calendar omits `data:` URLs by design;
use ordinary file content when sharing a browser-oriented fixture with Calendar.

Check available disk space before local matrix builds. Run local `act` jobs
one at a time, preserve their archives and diagnostic logs, then remove the
completed job's temporary checkout, object directories, and copied SDKs before
starting the next job. Do not delete existing developer builds or shared SDKs
as part of this cleanup.

---

# macOS Cross-Builds

Keep the validated arm64/x86_64 profiles pinned to SDK 11.3. Experimental i386
profiles use SDK 10.4u for target code and SDK 11.3 for native host tools, with
an explicit 10.4 deployment target. See `mozconfigs/macos/i386/README.md` for
SDK preparation and current limitations. Do not silently raise a deployment
target to satisfy the linker. Host-built configuration generators must use the
target's ABI sizes and alignments; run `build/macosx/check-target-abi.c` with the
target compiler. Cross-build success is not runtime validation: current macOS
cannot execute i386 applications. All four PowerPC 10.0 applications pass the
local `act` build/package/runtime/upload matrix using Linux containers and
original-OS PowerPC emulation. GitHub-hosted and physical hardware runs remain
untested; do not confuse those with the completed local validation.
The legacy deployment goals are Intel 10.4 onward and PowerPC Mac OS X 10.0
through the later PowerPC releases. The 10.3.9 cross-build is an intermediate
milestone, not the final minimum. Preserve 10.0-compatible API paths and audit
the C/C++ runtime, startup objects, loader behavior and graphics APIs against
the actual oldest target; newer SDK compilation is not proof of 10.0 support.
For the 10.0 target, use the separately rebuilt classic linker and original
10.0 Csu startup object described in `mozconfigs/macos/powerpc/10.0-status.md`.
Keep original-library overlay provenance and distinguish host pixel tests,
static import checks, successful links and actual target-OS execution.
Use `build/macosx/legacy_tar.py` when writing early-Mac USTAR packages. Original
10.0 tar corrupts paths whose name field contains 100 bytes without a NUL;
ordinary USTAR output alone is insufficient. Preserve the archive boundary
tests on both the host and original target.

Keep Darwin's rune-table ABI independent of Mozilla's `-fshort-wchar` setting.
The early SDK adaptation preserves 32-bit `rune_t`; target C++ probes check
both ctype-first and stdlib-first include orders alongside 16-bit `wchar_t`.
Both early headers declare `rune_t`, so correcting only `runetype.h` leaves
application startup broken. Original Csu's optional runtime hooks
must also avoid common-symbol collisions when Objective-C components load
after process startup. Preserve the deferred-Cocoa/eager-binding regression.
Check actual GUI screenshots as well as layout assertions. Early ATSUI glyph
outlines must exclude CTM translation from their cached transform, and Cairo
ARGB word extraction must preserve channel order on big-endian machines.
Keep the translated-baseline and image-optimization round-trip probes.
Cairo image frames and GIF composition use top-down rows even on platforms
whose legacy native image backend uses bottom-up rows. Keep that distinction
in `MOZ_PLATFORM_IMAGES_BOTTOM_TO_TOP` and test RGB and alpha row order.
Preserve the requested executable during Cocoa relaunch. Early `NSBundle`
lookup can identify a different program beside the invoked executable;
exercise the production helper with relative, absolute and PATH-based launch
arguments as well as the full application's first-run restart.
Toolkit executables targeting 10.0 require `-bind_at_load`: original dyld's
lazy startup path reproduces a bus error before their first window. Preserve
the executable flag check in packaging and test ordinary launches without
`DYLD_BIND_AT_LAUNCH` or diagnostic instrumentation.

The early C++ ABI runtime probe passes on original 10.0 under emulation, but
does not establish full libstdc++ or application compatibility. Retain the
GCC unwind-registration bridge and thread-safe initialization when extending
this port; see `mozconfigs/macos/powerpc/early-cxx-runtime.md` for its scope.
The 10.0 profiles use `prepare-10.0-sdk.py` to combine checksum-pinned 10.1.5
headers with original 4K78 libraries, and `build-early-stdlib.sh` for their
private C++ runtime. Do not call this an original 10.0 SDK. The early runtime
omits wide-character C++ streams, not application Unicode support. Keep the
original-OS NSPR/SQLite probes and host Quartz pixel comparisons as regression
checks. Preserve the completed four-application PowerPC workflow validation:
fresh compilation, package audits, original-OS execution and artifact uploads.
Revalidate the affected applications when changing this deployment path.
The 4K78 installation CD omits installed-system AppleCSP, QuickTime and AGL.
Use `build/macosx/tests/prepare-10.0-system-files.py` for disposable guest
images. It extracts the original files from Essentials and removes that
installer archive only from the disposable copy to make space. Preserve
runtime-file provenance. The guest reports `/tmp` mounts as `/private/tmp`;
derive scratch devices from the mounted transfer volume. A guest PASS requires
the payload to complete, since an early zsh EXIT trap alone can misreport an
explicit exit's status.
The initial i386 10.8 build is an intermediate result, not the desired final
minimum. Linux-hosted cross-toolchain workflows are acceptable. Validate
startup objects, target SDK APIs, linked dependencies and target runtime
behavior before lowering a claimed supported OS version.
Preserve the historical 32-bit Cocoa/Carbon APIs and resources when fixing
modern-host build problems.
Cocoa's `NS_NATIVE_DISPLAY` returns an `NSView`, not a Carbon `WindowRef`.
Do not pass it to Carbon window-event APIs. Menu command callbacks must use
live menu-bar state when windows change or close. Exercise both native File
and application-name menus on the oldest target, including actual commands;
a rendered menu bar alone does not validate native menu interaction. In the
original-10.0 UTM guest, open bundles through Launch Services for interactive
tests; a direct shell launch can display a window without correct native
application-menu registration. XULRunner wrapper bundles must preserve the
stub launcher as `argv[0]` and set `XRE_BINARY_PATH` to the runtime, following
`nsXULStubOSX.cpp`; using the runtime as `argv[0]` can lose keyboard input.
See `mozconfigs/macos/powerpc/utm.md`.
On 32-bit Cocoa, route Command keys at the application event loop: original
AppKit does not forward unmatched shortcuts to view/window key equivalents.
Match native Carbon commands using `IsMenuKeyEvent` and its returned MenuRef;
system application menus cannot reliably be found by a numeric menu ID.
Keep Suite's early Cocoa initialization and Toolkit's application shell using
the same dispatch policy. Check editing, navigation and Quit in the original
OS, while leaving the existing 64-bit AppKit event path intact.
Linux cross-builds may use the small generated classic resources in
`config/macos/resources`, but must verify the source and resource hashes.
Regenerate them with Apple's tools when their authoritative sources change;
do not silently use stale resources or drop cursors and AppleScript metadata.

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

The minimum Windows targets are **Windows 95** and **Windows NT 4.0**.
Treat the Windows 9x and Windows NT families as independent compatibility
requirements. Preserve support for Windows 98, Windows 98 Second Edition,
Windows Me, Windows 2000, Windows XP, and later compatible releases where
practical. Record tested service packs and optional updates explicitly.

Do not unnecessarily raise either minimum Windows version.
Do not introduce newer Win32 API dependencies merely for convenience.

## Compiler and build host

Windows builds are performed from a **Linux or macOS host**, using genuine
**Microsoft Visual C++ 2005 (MSVC 8.0 / VC8)** tools through **Wine**.
CrossOver is a supported Wine provider on macOS. Configure, make, and host
utilities run natively on the build host; target compiler, linker, and resource
tools run through Wine. Native Windows/VC6 builds are historical workflows,
not the current Windows build procedure.

Preserve MSVC 2005 source compatibility in code used by Windows targets.
Static CRT DLL boundaries must not exchange CRT-owned `FILE*` streams. Read
files in the owning module and pass bytes through the existing APIs. Aggregate
component builds must initialize their module table in command-line utilities
as well as GUI applications; exercise `xpcshell -e`, `-f`, stdin and `load()`
when changing that initialization.
Validate the packaged Suite, not only `dist/bin`: aggregate native components
still require the complete application-facing XPT typelibs. Keep the static
Windows package manifest synchronized with enabled platform components, and
exercise Composer and scripted application shutdown from the package.
Do not require a newer Microsoft compiler merely because it is newer.
Modern compiler diagnostics, sanitizers, and architecture work may use current
GCC/Clang on Linux or macOS.

Use the existing wrappers and mozconfigs documented in
[build/win32/msvc8-cross/README.md](build/win32/msvc8-cross/README.md).
Keep the static CRT policy and the legacy Suite's component aggregation and
shared process-heap integration. Do not reintroduce a VC80 runtime DLL dependency.

A successful cross-build or Wine launch does not prove Windows 95 or NT 4.0
runtime compatibility. Preserve the distinction between minimum targets and
verified binaries. Track import/CRT blockers and actual operating-system tests
in [COMPATIBILITY.md](build/win32/msvc8-cross/COMPATIBILITY.md).

## Dependency updates

Dependency updates must take Windows 95, Windows NT 4.0, and MSVC 2005 into account.
Small MSVC 2005 compatibility patches are acceptable. Do not perform major
third-party rewrites merely to support that compiler. If necessary, freeze the
legacy Windows build at the newest practical compatible dependency version.

Do not claim Windows 95 or Windows NT 4.0 compatibility for a changed dependency
until actually tested on the target operating system.

## Older Windows experiments

Windows NT 3.1, Windows NT 3.5, Windows NT 3.51, and Windows 3.1/Win32s are
possible experimental targets. Preserve useful historical support code, but
these systems are not current minimum requirements.

Do not compromise required Windows 95/Windows NT 4.0 support to support these
experiments.

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

Xt input callbacks must dispatch their subscribed event queue, including nested
modal queues. Keep input IDs pointer-sized through registration and removal.
Exercise Suite's Address Book/Account Wizard and lifecycle checks when changing
Xlib event dispatch.

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


For intrinsic changes, preserve classic globals without reserved JSProto slots.
Run `js/tests/es6/realm-intrinsics.js` and `TestRealmIntrinsics.c`; verify live
intrinsics survive GC, dead globals are collected, and JS_ClearScope resets the
cache. Modern literal construction bypasses mutable Array/Object bindings;
legacy scripts retain their historical lookup. Keep bytecode versioning,
decompilation, cross-edition XDR and real XUL/content globals covered.


Symbol changes must preserve old jsval encodings and global reserved-slot
capacity. Run `symbol-primitives.js` and `TestSymbolEmbedding.c`, including
registry lifetime across context recreation, native string views, property-key
GC roots and real window globals. Keep well-known protocol gaps explicit;
exposing named symbols alone is not conformance. Do not serialize Symbol
identity as a string in XDR.


Modern RegExp literal changes must preserve explicitly selected legacy literal
identity. Run `js/tests/es6/regexp-literals.js`, the separate extended-atom
`regexp-literals-wide.js`, and the edition/XDR and realm embedding probes.
Keep normal and extended opcode dispatch/decompilation paths aligned and bump
the bytecode cache version when introducing new instructions.

For Symbol.hasInstance changes, run `js/tests/es6/has-instance.js` and
`TestHasInstanceEmbedding.c`. Preserve native JSClass instance hooks for legacy
scripts and the public JS_HasInstance API while validating modern custom hooks,
bound targets, raw builtin receivers and collection during callbacks.

For Object.prototype.toString and built-in tag changes, run
`js/tests/es6/builtin-tags.js` and `TestBuiltinTags.c`. Preserve historical
JSClass names in legacy modes; modern fallback tags must follow the specified
internal types and must not invoke native callable objects to classify them.


For modern Array/String iterator changes, run `js/tests/es6/modern-iterators.js`
and `TestModernIterators.c`. Preserve classic Iterator/StopIteration behavior
and legacy arguments objects. Check live lengths, callback reentrancy, GC,
surrogate pairs, permanent exhaustion, defining-realm result prototypes, weak
realm-cache lifetime and JS_ClearScope. Modern arguments must use the original
Array values function even if its public property was replaced. Iterator
interfaces do not establish support for iteration syntax or consumers.


For Array.from changes, run `js/tests/es6/array-from.js` and `TestArrayFrom.c`.
Preserve primitive receivers, defining-realm fallback arrays, generic constructor
argument counts, own property creation and ES2015 IteratorClose exception
precedence. Next/done/value failures and final length failures do not close
the iterator; mapper and indexed-definition failures do. Root index atoms and
pending exceptions through callback collection.


For Symbol.unscopables changes, run `js/tests/es6/unscopables.js` and
`TestUnscopables.c`. Filter only modern syntactic with environments; preserve
legacy scripts and ordinary embedding/global scopes. HasProperty precedes the
exclusion getter, which can delete the resolved property or collect. Do not
retain native properties/locks through callbacks or repeat lookup to decide
whether the binding originally existed. Preserve implicit method receivers.
