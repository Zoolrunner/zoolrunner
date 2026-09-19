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
Native setter and descriptor changes must preserve modern assignment results
without changing the native JSAPI's normalized output or explicit legacy
behavior. Run `property-coercion.js` and `TestNativeSetterResult.c`, including
collection and array-length callbacks that change descriptor attributes.
Catch parameter defaults must run outside the catch body lexical environment;
pattern bindings must retain their temporal dead zone. Run `catch-environments.js`,
`string-codepoint-escapes.js`, `generator-let-newline.js` and
`typedarray-zero-indices.js` for parser/index changes. Keep numeric zero indices
distinct from the canonical property string `"-0"`. Record later-edition
Test262 reviews with exact source hashes; never treat a review ledger as a
passing conformance run or a complete edition inventory.
Switch discriminants must execute outside their case-body lexical environment.
Run `switch-environments.js` and `TestLexicalEmbedding.c` for block-entry changes,
including allocation-callback collection, default-only bodies and multi-binding
cache/source round trips. Keep explicit legacy scope ordering. Syntactic modern
with environments perform GetBindingValue's existence check after unscopables;
do not repeat the unscopables lookup or add later-edition write checks. Run
`with-binding-value.js`, `unscopables.js` and `TestUnscopables.c`, preserving
embedding object receivers and explicit legacy lookup behavior.
Property replacement must preserve ES2015 creation order and keep compiler
metadata private. Run `property-order.js` and `function-key-order.js`, including
watchpoints, duplicate parameters, collection and deletion/re-addition. Keep
explicit legacy ordering and native bootstrap conventions intact.
Tail-call changes must preserve captured arguments/locals, direct eval,
legacy editions and balanced debugger hooks. Run the `tail-call-*.js` fixtures
and `TestTailCalls.c`, including branch cancellation, collection and script
serialization. Use original ES2015 tail-call realm rules; later normative
changes must be reviewed explicitly rather than imported from current tests.
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
Keep the later Test262 diagnostic separate from the pinned historical gate.
Metadata and feature tags select review candidates, not edition exemptions.
Verify modern negative-test phases with isolated host realms and keep harness
errors visible. Preserve `test262-realms.js`, `test-later-runner.py` and wide
block-function XDR/source round trips when changing those paths.
For primitive-conversion changes, preserve `conversion-builtin-edges.js` and
`TestConversionRealms.c`: modern calls must handle foreign legacy values while
explicit legacy callers retain their historical conversion convention and
native embedding class conversion hooks remain usable.
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


For modern object-literal syntax, run `js/tests/es6/computed-properties.js`,
the separate extended-atom `computed-properties-wide.js`, and the edition/XDR
embedding probe. Convert computed keys before values; infer names per closure
without changing shared function templates. Keep Symbol() distinct from
Symbol('') for inferred names. Preserve method/accessor non-constructibility,
embedding accessor access checks, identifier references in shorthand fields,
and legacy parsing. Bytecode changes require cache-version and decompiler
updates. Computed destructuring, generator methods and super remain separate
features; do not confuse object-literal support with their implementation.

For Map/Set changes, run `js/tests/es6/collections.js`, `TestCollections.c`, and
`TestCollectionTable.c`. Preserve SameValueZero keys, insertion order, live
iteration across clear/delete/reinsert, sticky exhaustion and defining-realm
iterator results. Never retain hash/vector pointers across callbacks or GC.
Keep shared native storage ownership safe in either collection/iterator
finalizer order, while tracing the collection from a live iterator. Allocate
storage without GC under object locks, then unlock before reporting errors or
calling JavaScript. Deleted configurable Map/Set global bindings must not be
resurrected by lazy resolution or standard-class enumeration; keep their
intrinsics available through the private cache without enlarging classic globals.

For weak collection or GC changes, run `js/tests/es6/weak-collections.js` and
`TestWeakCollectionGC.c`. Weak keys must remain weak, including value-to-key
cycles and unreachable owners. Compute ephemeron closure before legacy generator
close discovery and after marking introduces more reachable keys/owners; clear
dead keys before sweeping. Preserve the classic MARK_END callback guarantee:
a host mark call returns with transitive marking, including weak values,
complete. Validate native finalizer counts, transitive weak chains, callback-only
roots, legacy generator cleanup and destroyed contexts, not just API names.


For Reflect and Proxy changes, run `js/tests/es6/reflect.js`, `proxy.js`,
`TestReflect.c` and `TestProxy.c`, together with the full ES5/ES2015 gates and
real application globals. Preserve raw receivers, alternate newTarget realms,
boolean rejection versus user exceptions, and ES2015 trap ordering/invariants.
The 2015 enumerate operation, revoked-Proxy creation rules and anonymous
revoker metadata differ from later editions: use the pinned 2015 specification
and tests. Do not substitute current-engine behavior as the only oracle.
Capture and root Proxy target/handler state before trap getters; revocation
inside a callback must release stored references without invalidating the
operation already in progress. Keep revoker state separate from function
metadata and preserve it through JSAPI cloning. Exercise native finalizers and
opaque lookup-handle release. Do not treat Proxy object maps/properties as
native scopes or pass them to native object-lock fast paths. Preserve public
JSClass/JSObjectOps layouts and the existing non-Proxy embedding hooks.


For Annex B built-in changes, run `annex-prototype.js`, `annex-html.js`,
`annex-globals.js` and `TestAnnexBuiltins.c`. Keep legacy-initialized __proto__
hooks and legacy HTML helper behavior intact. Modern accessors need raw
receivers, defining-realm primitive prototypes and native access checks;
CreateHTML needs receiver-before-attribute conversion, rooted strings and
checked quote expansion. Deleted modern global helpers must stay deleted
across caller-edition changes; the private global cache records initialization
policy and JS_ClearScope resets it. Test mixed-edition chrome/content windows
separately from globals explicitly initialized with -E.


For RegExp prototype changes, run `js/tests/es6/regexp-fields.js` and
`TestRegExpFields.c` alongside full conformance and application checks. Preserve
legacy matcher-bearing prototypes and virtual fields in legacy globals. Modern
prototype accessors must reject all receivers without matcher state, including
their own realm's original prototype (the later-edition exception does not apply
to ES2015). Keep native construction parented, own
lastIndex live, source escaping GC-safe, and generic getter/conversion order
observable. XDR must select the RegExp instance class even when its prototype
is an ordinary object; exercise decoding under a different context edition.
Decompilation must use internal patterns rather than mutable public properties.


For RegExp execution and symbol protocols, run `regexp-protocols.js` and
`TestRegExpProtocols.c`. Use the original ES2015 algorithms: exec observes the
lastIndex/global/sticky properties, search always writes/restores lastIndex
on its normal path, and user exec exceptions bypass restoration. Later editions
differ. Root matcher source across reentrancy, select matcher state after
observable conversions, and allocate results in the executing method's realm.
Native match loops must remain interruptible. Preserve the legacy native/String
paths for legacy globals, and distinguish script-edition selection from modern
global initialization.


For RegExp construction/sticky changes, run `regexp-constructor.js` and
`TestRegExpConstructor.c`. Preserve IsRegExp/source/flags/newTarget lookup order
and distinguish RegExpCreate from the public constructor. Delayed allocation
must keep all inputs rooted across callbacks. Preserve native constructor
cloning conventions and validate fallback realms through bound and Proxy
newTargets, including revocation during prototype lookup. Native sticky flags,
legacy grammar rejection, decompilation and XDR need separate checks. Cache
version 39 invalidates components serialized before sticky flag semantics.

For RegExp split changes, run `regexp-split.js` and `TestRegExpSplit.c`.
Preserve species-constructor ordering, raw capture values, foreign result-array
realms, primitive String symbol receivers and interruptible native capture loops.
The pinned ES2015 suite incorporates the July 2015 correction restoring ToUint32
for split limits; do not regress negative-limit compatibility to the original
publication's ToLength text. Custom exec indices must never create out-of-bounds
string slices. Legacy globals retain the historical split path.

For replacement protocol changes, run `regexp-replace.js` and
`TestRegExpReplace.c`. Preserve match collection before replacement callbacks,
observable result/capture conversion order, raw callback receivers, overlap
side effects and checked UTF-16 buffer growth. Keep native capture loops
interruptible and exercise foreign contexts, cloned methods and GC callbacks.
Legacy globals retain the original replace method and its historical `$+` token.

For Date conversion changes, run `date-primitive.js` and `TestDatePrimitive.c`.
Preserve the initialized global's policy across lazy Date resolution and foreign
callers. Modern Date methods reject prototypes without a date value, while
legacy globals retain their historical NaN-valued prototype and hint-sensitive
valueOf. Exercise native Date creation, cloned conversion methods, foreign
contexts, getter exceptions, removal of Symbol.toPrimitive and GC callbacks.
Do not route ordinary primitive conversion back through its symbol hook.

For concat/species changes, run `array-concat.js` and `TestArrayConcat.c`.
Preserve HasProperty/Get ordering, sparse entries, safe-integer output limits,
custom result property definitions and final throwing length assignment. Check
foreign intrinsic Array handling, primitive wrapper realms, callback GC,
cloned methods, native loop interruption and legacy-global isolation.

For Array callback-method changes, run `array-iteration.js` and
`TestArrayIteration.c`. Preserve legacy ToUint32 behavior in legacy globals,
modern ToLength and species ordering, sparse/live property traversal, raw callback
receivers, native-loop interruption and foreign/JSAPI-cloned method realms.

For indexed Array method changes, run `array-indexed.js` and `TestArrayIndexed.c`.
Preserve positive-zero length/index results, safe-integer bounds, observable
Has/Get/Set/Delete ordering, species results and interruptible native loops.
Keep legacy ToUint32 methods and JSAPI cloning behavior covered.

For Array string/sort changes, run `array-text.js` and `TestArrayText.c`.
Modern join cycle detection must not enumerate properties. Unwind its per-context
state on errors/interrupts, root sorted vectors across comparison callbacks, and
preserve primitive receivers through Object.toLocaleString forwarding. Keep
legacy string/sort behavior isolated and rebuild embeddings after context changes.

For Error changes, run `error-modern.js` and `TestErrorModern.c`. Preserve native
error reports and stack GC, legacy constructor/prototype behavior, and error
construction during conversion callbacks. Modern deleted message properties must
not be recreated by lazy resolution. Test engine-generated errors and foreign/
cloned constructors as well as explicit JavaScript construction.

For ArrayBuffer/DataView changes, run `binary-data.js` and `TestBinaryData.c`.
Exercise buffer detachment during coercion, species/newTarget getters and native
copy-loop callbacks. Views retain the buffer object, never a raw storage pointer.
Preserve corrected ES2015 optional DataView offsets and setter conversion order;
keep the pinned edition's stricter ArrayBuffer length and view access indices.
Use explicit float32 rounding, endian-neutral byte access and checked bounds.

For modern Unicode casing changes, run `casing.js`, `TestCasing.c` and the pinned
UCD exhaustive `test-casing.py` runner. Preserve legacy-global tables and explicit
embedding locale callbacks. Keep full mappings, supplementary code points,
original-text Final_Sigma context, interruptible loops and checked UTF-16 growth.
Do not silently change identifier, regexp or platform-wide Unicode tables.

For Function call/apply changes, run `function-invoke.js` and
`TestFunctionInvoke.c`. Modern apply uses ToLength without 32-bit wrapping, rejects
noncallable receivers before conversions, and keeps argument-list loops
interruptible. Preserve raw target receivers, partial-list rooting, legacy methods
and cloned modern methods. Native modern exceptions use the executing function's
realm; exceptions thrown by user callbacks retain their own realm. Keep native
caller scopes intact for classic embedding/eval behavior.

For reflection key ordering/array changes, run `reflection-keys.js` and
`TestReflectionKeys.c`. Preserve selected legacy script ordering and primitive
TypeErrors, proxy-supplied key order, and JSON's internal key-enumeration callers.
Those callers do not supply argv[-2]; obtain result-array realms from the active
operation frame. Keep copied identifiers rooted across GC/interrupt callbacks.

For ECMAScript job queue changes, run `TestJobQueue.c`, `test-job-shell.py` and
`test-runner-integration.py`. Preserve FIFO order, roots across GC and context
destruction, per-runtime/thread ownership, nested checkpoint suppression and
exception/interruption recovery. Hosts choose checkpoints; nested load/evaluate
calls must not drain the outer script's jobs. Last-context thread detach cancels
pending jobs, so embeddings that need completion must drain before teardown.
Shell queue support alone does not establish Promise or DOM event-loop support.

For Promise changes, run `promise.js`, `TestPromise.c` and the packaged
`window-promise.xul` chrome/content fixture. Keep resolving functions and reaction
records traced across GC, context destruction and cross-global callbacks. Preserve
once-only resolution, iterator closing, intrinsic realms and interruption without
turning fatal engine failures into successful rejections. DOM checkpoints must
inspect the entire context stack, including contexts below null barriers, and
must not drain jobs during a suspended outer script. Keep contexts alive through
callbacks that close windows; test queued work after window closure. Component
event checkpoints must preserve their safe context and report abrupt job errors.
Native job entry frames carry their owning global for principal lookup before a
sloppy handler enters its script. Preserve access checks; test both system and
unprivileged sandbox callbacks so safe-context differences cannot grant chrome
privileges or prevent legitimate handlers from running.
Promise metadata follows ES2015 even in legacy globals; other legacy built-ins
and application syntax must retain their selected-version behavior.

For `new.target` and lexical function-environment changes, run `new-target.js`,
`TestEditionEmbedding.c` and the mixed-edition chrome/content window checks.
Preserve direct-eval constructor identity, including strict and nested eval;
indirect eval and global scripts must not acquire a caller's function binding.
Lookahead after `new` must retain operand scanning: `new /pattern/()` parses
and then throws TypeError in modern mode, rather than becoming a syntax error.
Retain ordinary, bound, proxy and alternate constructor identities across GC,
decompilation and XDR. Bytecode cache version 40 adds the new.target opcode;
keep cache-version assertions synchronized when adding later opcodes. Selected
legacy script versions retain their grammar. Ordinary-function support alone
is not arrow-function or class support.

For property-reference evaluation changes, run `assignment-reference.js`,
`assignment-reference-wide.js`, `TestEditionEmbedding.c` and mixed-edition
chrome/content checks. Modern computed assignments must check the base and
convert the key before the RHS, then reuse that key for GetValue and PutValue;
keep primitive setter receivers raw and converted keys rooted across callbacks.
Exercise extended atom indices, compound writes, legacy accessors, catch blocks
and function decompilation. Extended property stores retain their original
operand order; catch/block source notes belong to the extended opcode prefix.
Cache version 41 adds reference checks and extended property-store dispatch.

Identifier-reference changes must retain the resolved environment and resolution
success across RHS callbacks. Check resolved bindings deleted during evaluation,
initially unresolved strict writes whose RHS adds a global, proxy traps,
`unscopables`, callback GC and source/XDR round trips. Run
`identifier-reference.js`, `identifier-reference-wide.js`, embedding and window
checks. Wide reads/increments must preserve name ReferenceErrors and `typeof`
semantics. Cache version 42 adds traced identifier reference pairs; keep selected
legacy edition behavior separate from the ES2015 binding algorithms.

Keep frame PCs and operand provenance at real instruction boundaries when
executing extended atom opcodes. Native assignment/resolve hints must decode
both extended operands and debugger traps. Run `TestReferenceEmbedding.c` with
small and large atom tables, callback GC and nested evaluation, plus debugger
lifecycle checks when changing this path.

For template changes, run `template-literals.js`, `template-boundaries.js`,
`TestTemplateEmbedding.c` and the modern chrome/content checks. Follow the
requested ES2015 edition: equal raw segment lists share one template object per
realm, and malformed escapes remain syntax errors even in tagged templates.
Keep registry state private, preserve reentrant native allocation/GC behavior,
and avoid mutable global constructors. Decompilation must preserve exact raw
UTF-16, including lone surrogates, NUL and line separators; escaping raw Unicode
as source escape sequences changes observable tagged-template values. Exercise
nested functions, wide atoms, XDR and scanner line-buffer boundaries. Cache
version 43 adds template evaluation opcodes. Arrow-dependent upstream template
cases remain part of the complete suite even before arrow support is finished.

For arrow/captured-environment changes, run `js/tests/es6/arrow.js`,
`TestArrowEmbedding.c`, `js/tests/es5/arguments-lifetime.js`, the debugger
lifecycle regression and modern chrome/content checks. Preserve raw strict
receivers, lexical arguments/new.target and independent closure instances.
Native allocation callbacks may collect and evaluate in the allocating frame;
recheck shared-cell publication after reentry. Keep the native frame ABI and
public function flags intact, preserve arrow syntax through XDR/decompilation,
and invalidate old serialized code (cache version 44 adds function kind).
Snapshot live mapped argument values before detaching a returning frame without
reviving deleted mappings or invoking replacement accessors. Eval/debugger
receiver conversion follows the enclosing function's binding, not the eval
script's directive. Simple and named-rest arrow parameters are implemented; default/destructured
parameters and class/super behavior still require implementation
and complete conformance/application validation.

For rest-parameter changes, run `js/tests/es6/rest-parameters.js`,
`TestRestEmbedding.c` and modern chrome/content checks. Keep ordinary non-simple
arguments unmapped, arrow arguments lexical, and rest initialization before body
function declarations. Create own rest-array elements without inherited setters
and keep partial arrays rooted across native callbacks. Exercise dynamic
Function, strict inheritance, forbidden explicit strict directives, decompilation,
XDR and native cloning. Cache version 45 adds rest initialization. Named rest
bindings are implemented; defaults and rest patterns remain unfinished, and
focused checks do not replace complete conformance/application runs.


For block lexical initialization changes, run
`js/tests/es6/lexical-initialization.js`, `TestLexicalEmbedding.c` and modern
chrome/content checks. Keep initialization distinct from assignment, preserve
uninitialized state in detached closures and XDR, and retain producer PCs used
by value decompilation. Discarded lexical reads can throw and must not be
optimized away. Exercise direct eval, native GC/debugger reentry and selected
legacy let semantics. Cache version 46 adds lexical initialization metadata and
instructions. Global lexical environments remain unfinished; focused checks do
not replace full conformance and application validation.


For block const and iteration changes, also run `js/tests/es6/lexical-const.js`
and `lexical-loops.js`. Preserve immutable flags in detached environments and
XDR, and check TDZ before rejecting an immutable write, after its RHS executes.
Freshen C-style let bindings before the first condition and before each update,
including continue without an update expression. Named lexical for-in bindings
need distinct iteration environments; ordinary var/catch bindings stay shared.
Keep the old environment live while allocating its replacement and the new one
on the unwind chain if detachment fails. Exercise native GC/debugger callbacks
that capture bindings during this transition. Preserve const in reconstructed
source and omit internal transitions from printed loop updaters. Cache version
47 adds const metadata and iteration instructions. Global lexical environments
and broader iterator/destructuring semantics remain unfinished.


For declaration-position grammar changes, run
`js/tests/es6/function-statement-grammar.js`. Modern strict declarations must
remain distinct from single statement bodies; preserve valid function/block/
switch declarations, inherited strictness and selected historical syntax.
Check both eval and Function construction. Parser-only fixes do not establish
that block-function binding and redeclaration semantics are complete.


For modern for-of changes, run `js/tests/es6/for-of.js`,
`for-of-boundaries.js`, `TestLexicalEmbedding.c` and modern chrome/content probes.
Keep the ES2015 Symbol.iterator path separate from classic for-in/for-each.
Preserve the legacy XML wildcard emitter case and exercise selected-edition E4X.
Acquire the value before evaluating the target, trace iterator/value state across
callbacks, and distinguish stepping failures from abrupt binding/body completion.
Preserve original ES2015 IteratorClose exception precedence and nested finally/
label cleanup order. Exclude outer cleanup and its final jump from an already
exited loop's exception ranges; never restore a popped iterator slot after an
outer close fails. Treat the legacy generator-return sentinel as return, not
ordinary throw. Source reconstruction must retain AssignmentExpression RHS
parentheses and protect normalized let targets. Exercise wide source notes,
extended atoms, XDR and debugger/GC reentry, and reject overflowing handler
depths. Cache version 48 adds private iterator state instructions. Missing ES6
generators, typed arrays and broader destructuring remain separate conformance
gaps, not reasons to exclude failing upstream cases.


For Unicode identifier changes, preserve selected legacy BMP rules and use
separate modern property tables. Check raw and escaped supplementary names,
malformed escapes, token-buffer boundaries and decompiled source. Identifier
escaping must remain distinct from string/XML text escaping. Run
`js/tests/es6/identifier-codepoints.js` and `test-identifiers.py` against the
checksum-pinned UCD inputs; generation is maintenance work, not a build dependency.


For modern generator changes, run `js/tests/es6/generators.js` and
`TestGeneratorEmbedding.c` alongside complete pinned conformance and application
checks. Preserve classic next/send/throw/close/StopIteration semantics. Keep
modern return completion separately rooted across yielding finally blocks, trace
delegated iterator state, and retain suspended generators through escaped lexical
objects. Moving a frame must not remap strict/rest arguments. Preserve original
ES2015 delegated-yield result identity and missing-throw cleanup precedence.
Check generator intrinsic descriptors, non-constructibility, contextual yield,
source reconstruction and XDR. Cache version 49 records the generator function
kind and delegated-yield instruction. The validated macOS arm64 baseline passes
27,878 ES2015 modes and all 11,540 ES5 modes; 688 failures, 14 unsupported modules
and two harness errors remain. All four macOS arm64 applications pass packaged
runtime checks; other platforms remain unvalidated for this batch.


For typed-array changes, run `js/tests/es6/typed-arrays.js` and
`TestTypedArrays.c` alongside complete conformance and all four application
checks. Preserve canonical numeric index handling, ordinary property receivers,
buffer detachment, overlapping copies and original ES2015 species semantics.
Root callback state and reacquire byte storage after JavaScript calls. Initialize
the ordinary property store before native allocation hooks can reflect on the
view, and preserve restrictions those hooks install. Test throwing writes as
well as Reflect's boolean results. Original ES2015 fill converts per write;
constructor handling of explicit undefined differs from later editions.
The validated macOS arm64 baseline passes 27,940 ES2015 modes and all 11,540
ES5 modes; 628 failures and 14 unsupported modules remain, with no harness
errors, crashes or timeouts in the completed runs. All four applications pass
packaged runtime checks; other platforms remain unvalidated for this batch.
Warm relocated runtimes once before starting concurrent conformance processes,
so XPCOM component-cache regeneration is serialized. Retain failed startup
logs and report complete reruns honestly.


For object-pattern and update-expression changes, run
`js/tests/es6/object-patterns.js`, `update-targets.js`, and
`TestObjectPatterns.c`, plus the existing computed-properties and new-target
checks. Retain empty-object coercibility checks through decompilation and XDR;
cache version 50 records the new source notes/emission. Preserve legacy
selected-edition destructuring and update operands. Modern invalid non-simple
update targets use original ES2015 early ReferenceError; test no side effects
before the error. The macOS arm64 baseline passes 27,997 ES2015 modes and all
11,540 ES5 modes, with 571 failures and 14 unsupported modules remaining.
All four applications pass build/package/runtime checks. Computed pattern keys,
defaults, rest and complete iterator-based array patterns still require work;
other platforms remain unvalidated for this batch.


Computed object-pattern keys use cache version 51. Run `computed-patterns.js`
and retain Symbol conversion, null-before-key ordering, generator suspension,
wide-branch decompilation, native XDR and real-window coverage. The validated
macOS arm64 baseline passes 28,002 ES2015 modes and all 11,540 ES5 modes, with
566 failures and 14 unsupported modules remaining and no harness errors,
crashes or timeouts. All four applications pass build/package/runtime checks,
including Calendar views and Suite/XULRunner ChatZilla. Defaults, rest,
assignment-reference ordering and complete array iterator semantics still
require work; other platforms remain unvalidated for this batch.


Destructuring defaults use cache version 52. Run `pattern-defaults.js` and the
native pattern/XDR test. Preserve rejected object-literal cover initializers,
undefined-only evaluation, inferred names, lexical initialization, generator
suspension, wide branches and formal-pattern source round trips. The validated
macOS arm64 baseline passes 28,085 ES2015 modes and all 11,540 ES5 modes, with
483 failures and 14 unsupported modules remaining and no harness errors,
crashes or timeouts. All four applications pass build/package/runtime checks,
including Calendar views and Suite/XULRunner ChatZilla. Whole-parameter
defaults, parameter environments, rest patterns, assignment-reference ordering
and iterator-based array patterns remain unfinished. Other platforms have not
been revalidated for this batch.


Modern iterator patterns and captured assignment references use cache version
54. Run `array-patterns.js`, `array-rest.js`, `array-patterns-wide.js`,
`pattern-references.js`, `pattern-references-wide.js` and the native pattern/XDR
probe. Preserve source-key/target/getter/default ordering, one-time reference
resolution, elision behavior, exhaustion, iterator closing, generator return,
wide branches, rest arrays and selected-legacy indexed patterns. Local modern
error fixtures must use iterables when testing a later getter/constant-write
error; retain their legacy inputs and exception assertions. The validated
macOS arm64 baseline passes 28,147 ES2015 modes and all 11,540 ES5 modes, with
421 failures and 14 unsupported modules remaining and no harness errors,
crashes or timeouts. All four applications pass build/package/runtime checks,
including Calendar views and Suite/XULRunner ChatZilla. Other platforms remain
unvalidated for this batch. Spread expressions, whole-parameter defaults,
parameter environments, classes and modules remain unfinished.

Preserve persistent ES2015 global lexical records separately from global object
properties. Functions compiled before a later lexical declaration must capture
the same record. Eval-local declarations need fresh records; keep declared-var
history distinct from property deletion and retain explicit legacy scope rules.
Run `global-lexical.js`, `TestGlobalLexicalCompiler.c`,
`TestGlobalLexicalStore.c` and `TestGlobalLexicalWide.c` for scope changes,
including cross-script window fixtures, GC, source/XDR round trips and split
prolog/body execution. Internal lexical records must remain hidden from the
historical `__parent__` accessor. Report the complete conformance and four-app
results separately from focused probe totals.


Preserve method home objects on each function instance, including clones,
computed/accessor/generator methods and nested arrows. Direct eval inherits
super-property context; indirect eval and ordinary nested functions do not.
Captured super references must retain base, receiver and key across callbacks.
Run `super-properties.js`, `TestMethodHome.c`, `TestSuperReference.c` and
`TestSuperWide.c`, including XDR/source reconstruction, wide operands and GC.
When the decompiler copies text from its own buffer, preserve source offsets
across arena growth. Real chrome/content fixtures must also exercise super.


Scripted setters must preserve assignment results in standard editions while
retaining explicit legacy result behavior and native property-hook contracts.
Run `setter-result.js` in default ES5 and ES2015 modes and `TestSetterResult.c`
for mixed caller/setter editions, strict callers, callbacks and GC.

Preserve ES2015 class home objects, derived-constructor this initialization,
lexical super calls through arrows/direct eval, and the distinction between
mutable declaration bindings and immutable inner class names. Retain complete
class source across collection and XDR, including file-backed compilation.
Run `classes.js`, `class-source.js`, `TestClassRuntime.c` and `TestClassWide.c`,
plus the chrome/content edition fixture. Keep explicit legacy parsing unchanged.

For ES2015 declaration rules, preserve catch-local initializer targets while
instantiating `var` in the enclosing variable environment. Follow the original
ES2015 duplicate block-function early error; explicit legacy/default editions
retain their existing grammar. Run `catch-declarations.js` and
`TestCatchDeclarations.c`, including wide prolog operands and XDR/source checks.


For non-simple ES2015 formals, preserve the separate parameter and body records,
per-initializer eval scope, unmapped arguments, and generator call-time defaults.
Keep dynamic Function formals separate from its body token stream. Parameter
initializer scripts are owned by their body scripts: trace, serialize, destroy
and notify debugger hooks consistently. Run `default-parameters.js` and
`TestParameterWide.c`, including wide operands, callback-triggered GC and
cache/source round trips. Preserve legacy formal parsing and embedding APIs.


Module compilation is an explicit Unicode host API, separate from classic
load/evaluate. Preserve private module environments, immutable live imports,
cycle-aware instantiation/evaluation, and per-module exception retention. Hosts
resolve requested specifiers explicitly; do not add an implicit loader to old
XUL scripts. Run `modules-grammar.js`, `modules-link.js`, `modules-namespace.js`,
`modules-extra.js`, `modules-edges.js` and `TestModules.c`, including GC during
script hooks, namespace-only lifetime, Unicode sources and legacy context
version restoration. Keep module grammar separate from direct eval and classic
script grammar. Module records own their scripts and cannot use ordinary script
XDR caches. Namespace behavior follows the original ES2015 edition, including
its key iterator and rejection of even no-op property/prototype definitions.
The complete pinned corpus and all four application gates remain required.

For JSON reviver writes, use complete own data descriptors and preserve atomic
rejection: a non-configurable property must not be partially overwritten.
Ignore false definition results while propagating callback exceptions. Run
`json-reviver.js` in both default ES5 and ES2015 modes, including frozen and
nonextensible containers, accessors, Proxy traps, reentrant parsing and GC.

Modern JSON methods retain their initialized edition when called from legacy
scripts or cloned through JSAPI. Use ToLength for Proxy-array iteration and
allocate results/root callback holders in the native method's realm. Preserve
legacy ToUint32 behavior and interruptible native loops. Run
`json-realms-length.js` and `TestJSONRealms.c`; give independent native test
globals a null parent so primitive lookup cannot reach another global.

Inspector browser pageshow can precede asynchronous viewer-registry readiness.
Queue that load until the document panel exists, and discard pending work and
listeners during teardown. Keep the controller load-order test alongside the
real Suite lifecycle check; do not hide script errors by delaying the test.

Modern localeCompare must compare canonical Unicode equivalents as equal even
without a host collator, with consistent ordering against other strings. Keep
legacy callback input unchanged. Run `locale-compare.js` and
`TestLocaleCompare.c`, including callback GC, exceptions and JSAPI clones.

For non-Unicode RegExp hexadecimal escapes, consume digits only for a complete
escape before applying the original ES2015 Annex B identity fallback. Preserve
explicit legacy parsing and strict Unicode patterns. Lazy class bitmaps retain
the compilation edition, and XDR stores it independently of the decoding
context. Cache version 65 invalidates the older regexp record format. Run
`regexp-incomplete-hex.js` and `TestRegExpHex.c`, including standalone-object
and script cache roundtrips in both edition directions, source and GC checks.

Modern Date arithmetic must convert all supplied fields before rejecting a
nonfinite value, truncate before the two-digit year offset, preserve explicit
zero/negative days and add positive zero after TimeClip truncation. Keep the
historical method policy in legacy globals and retain modern native entry
points through JSAPI clones. Date setters snapshot the old value before
callbacks. Original ES2015 stores the final NaN even when argument conversion
mutates an invalid Date; do not adopt the later early-return rule silently.
Run `date-numeric.js` and `TestDateNumeric.c`, including callback collection,
exceptions, large fields and multiple timezones. Modern offsetless ISO
date-times use local time. Date-only forms retain UTC for compatibility with
the inherited pinned cases and later corrections; document this distinction
from the published original ES2015 wording. Legacy parsing remains unchanged.

Modern class/object modifiers require literal contextual-keyword spellings;
escaped IdentifierNames remain valid property and ordinary method names.
Keep historical object-accessor grammar in legacy editions. Class method and modern object-accessor keys
must be followed immediately by their parameter list; the shared FunctionExpr
parser must not consume a second function name or generator marker. Run
`contextual-escapes.js` and `TestContextualEscapes.c`, including exact parse
SyntaxErrors, legacy accessors and collected source/cache roundtrips.

Preserve the observable exception from modern `this` reads before derived
constructor initialization, even when an expression result is discarded.
Exercise bare/unary/comma/delete reads, arrows before and after `super()`,
callbacks/GC and cross-edition source/XDR round trips with
`derived-this-effects.js` and `TestDerivedThisEffects.c`. Emission changes
invalidate embedding bytecode caches (version 66 for this correction);
legacy effect analysis remains unchanged.

Linux ES2015 validation uses `build/linux/test-es6.py`: keep complete pinned
reports separate from focused/native results. Private engine unit probes link
the production Makefile's objects in standalone executables; do not export
private interfaces merely to link tests against a package. Preserve the
object hashes, packaged-library identity check and generated target headers.

Discarded operations must retain required coercions, protocol calls and property
reads in default ES5, strict and ES2015 code. Keep explicit non-strict legacy
optimization and saved-edition behavior. Standard arguments detachment must not
invoke overridden length/callee accessors or consult prototype traps on return;
preserve mapped values and deleted/detached properties. Run
`discarded-operations.js` and `arguments-exit.js` in default and modern modes,
`arguments-exit-prototype.js`, and `TestDiscardedEffects.c`, including source/XDR
round trips. This emitter correction advances the embedding cache to 67.

Modern mapped arguments must establish initial length/callee string-key order
before user reads or mutations. Preserve lazy numeric parameter mappings and
earlier-edition behavior. Run `arguments-property-order.js` and
`TestArgumentsPropertyOrder.c`, including descriptors, GC, delete/re-addition,
freezing and source/XDR execution across editions. Resolver callbacks receive
jsval property values, not internal jsid encodings.

URI decoding must reject non-shortest UTF-8 and encoded surrogate values in
default ES5/ES2015 calls while preserving explicit legacy decoding. Do not alter
the shared byte decoder to implement URI-only validation. Run `uri-decoding.js`
in default and modern modes and `TestURIDecoding.c`, including conversion, GC,
raw UTF-16 input and saved-edition source/XDR execution.
Eager Math initialization must select the target realm Object prototype without
recursively resolving Math as its own class prototype. Run
`math-realm-prototype.js` and `TestMathRealmPrototype.c`, preserving explicit
legacy initialization and testing independent globals and collection.

Number formatting changes must preserve explicit legacy radix/precision behavior
while default ES5/ES2015 use ToInteger radix validation and the specified
nonfinite/precision conversion order. Run `number-formatting.js` in both standard
modes and `TestNumberFormatting.c`; keep shared dtoa behavior and permitted
legacy fixed-precision extensions intact.
For HTML close comments, preserve line terminators inside block comments, module
rejection and saved editions. Run `html-close-comments.js` and
`TestHTMLCloseComments.c`.
Immutable native data properties must retain their frozen values in standard
modes. Run `frozen-native-properties.js` in both standard modes and
`TestFrozenNativeProperties.c`; preserve explicit legacy behavior, live mutable
fields, accessor properties and array/arguments/embedding native hooks.

Modern non-strict block functions require entry-time lexical initialization and
the original ES2015 B.3.3 function-body variable bridge. Check completed outer
lexical scopes and all formal bound names before adding that bridge; preserve
implicit arguments, simple versus destructured catch bindings, if-arm scope and
labelled declaration hoisting. Copy the current lexical value at the declaration
position, bypassing with objects. Do not import later global/eval bridges or the
later implicit-arguments exclusion without an explicit edition review. Run
`sloppy-block-functions.js`, `TestSloppyBlockFunctions.c` and
`TestSloppyBlockWide.c`, including source/XDR, collection and wide atom operands.
The bridge advances the bytecode cache to 68; rebuild XPConnect loaders and
containing libraries before application checks. Keep its source-stack effect
balanced with the hidden POP during decompilation.
ES2015 literal `__proto__` initializers invoke the internal prototype operation,
without reading a public property or dispatching its setter. Preserve computed
keys, shorthand members, methods, accessors and earlier-edition behavior. Run
`literal-prototype.js` and `TestLiteralPrototype.c`, including rooted prototype
values across embedding callbacks, collection and saved-edition round trips.
