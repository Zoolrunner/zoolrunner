# ZoolRunner

ZoolRunner preserves and modernizes the classic Mozilla application platform.
It continues RetroZilla, which was derived from Mozilla 1.8.1, and provides a
runtime for standalone XUL applications and Mozilla-based desktop software.

An application can use XUL for its interface, JavaScript for its behavior, and
XPCOM components for native services, with Gecko supplying document rendering
and Necko supplying networking. Profiles, preferences, extensions, themes, and
application packaging are part of the same platform. Keeping that combination
usable—on contemporary development systems and historical operating systems—is
the reason ZoolRunner exists.

The project also maintains Browser, the full Mozilla Suite, and Calendar. These
are useful applications in their own right and practical tests of the runtime.
Work focuses on application compatibility, crash and memory-safety fixes,
portability, selected standards improvements, and maintainable dependencies.
Full compatibility with the contemporary Web is outside the project's scope.

[Build guides](#building) · [Platform status](#platform-status) ·
[Application compatibility](#application-and-javascript-compatibility) ·
[Contributing](#contributing)

## The platform and its applications

The classic Mozilla technologies remain intentional parts of ZoolRunner:

| Layer | What it provides |
| --- | --- |
| XUL and XBL | Desktop interface markup, widgets, and reusable bindings |
| Gecko and SpiderMonkey | HTML/CSS layout, DOM behavior, and JavaScript execution |
| XPCOM, XPConnect, XPIDL/XPT | Component interfaces, native services, and JavaScript access to them |
| Necko, NSS, and NSPR | Networking, cryptography, and operating-system abstractions |
| Chrome packages and application services | Privileged application resources, profiles, preferences, extensions, themes, and RDF-based infrastructure |

Historical application-facing APIs are preserved wherever practical. Internal
changes should let an existing application continue using its Mozilla contract
IDs, interfaces, namespaces, chrome URLs, and XULRunner conventions. The project
does not rename these interfaces simply to replace Mozilla branding.

Four application targets form the build and CI matrix:

| Target | Purpose and coverage |
| --- | --- |
| **XULRunner** | Runs standalone applications using an `application.ini` manifest, chrome packages, and optional native components. Modern macOS packages also include the Simple example and standalone Layout Debugger. |
| **Suite** | Navigator, Mail & News, Composer, and Address Book, with extensions such as ChatZilla, Venkman, and DOM Inspector in configured builds. Exercises editing, mail infrastructure, and application services beyond browser navigation. |
| **Browser** | A standalone browser front end. Exercises rendering, networking, profiles, preferences, downloads, and native window behavior together. |
| **Calendar** | The standalone Sunbird calendar application. Exercises XUL/XBL interfaces, event components, and memory/SQLite calendar providers. |

These are the maintained build targets. Historical configure options and other
source remnants do not establish that an additional application is complete or
buildable. Third-party XULRunner applications are valuable compatibility tests
because they exercise interfaces independently of applications in this tree.

## Building

ZoolRunner uses the inherited configure/make build system. A **mozconfig** selects
the application, target architecture, toolkit, and build options. Start with the
[profile index](mozconfigs/README.md), then follow the appropriate guide to set
up its compiler, SDK, and host dependencies:

| Target | Build guide and requirements |
| --- | --- |
| Linux i686 / x86_64 | [Linux build guide](build/linux/README.md): Oracle Linux 8 containers, GCC Toolset 14, GTK2 or Xlib |
| Linux aarch64 | [Linux build guide](build/linux/README.md#aarch64-bring-up): native Oracle Linux 8, GCC Toolset 14, GTK2 or Xlib |
| Linux LoongArch64 | [Checked-in profiles](mozconfigs/linux/loongarch64): GCC, GTK2 or experimental Xlib |
| macOS arm64 / x86_64 | [macOS build guide](mozconfigs/macos/README.md): Clang, Cocoa, and **SDK 11.3** |
| Mac OS X i386 | [i386 cross-build guide](mozconfigs/macos/i386/README.md): SDK 10.4u for target code, SDK 11.3 for native host tools |
| Mac OS X PowerPC | [10.0 build and runtime notes](mozconfigs/macos/powerpc/10.0-status.md): Linux-hosted cross-toolchain, prepared early SDK, and private C++ runtime |
| Windows x86 | [Windows cross-build guide](build/win32/msvc8-cross/README.md): genuine **MSVC 2005 through Wine**, hosted on Linux or macOS; CrossOver is supported on macOS |

After preparing the environment, select a profile with an absolute `MOZCONFIG`
path. For example, on a configured LoongArch64 Linux host:

```sh
export MOZCONFIG="$PWD/mozconfigs/linux/loongarch64/gtk2_browser_gcc.mozconfig"
make -f client.mk build
```

This profile builds into `obj-zoolrunner-linux`. Launch its browser with:

```sh
cd obj-zoolrunner-linux/dist/bin
sh ./run-mozilla.sh ./zrbrowser
```

The LoongArch64 GTK2 Suite profile uses `obj-zoolrunner-suite`; from its
`dist/bin` directory, run `LD_LIBRARY_PATH=. MOZ_NO_REMOTE=1 ./zoolrunner -browser`.
Use `-mail` or `-edit` for MailNews or Composer. Other profiles have their own
object directories and platform-specific launch and packaging procedures.

The macOS guide includes SDK preparation and package commands. Its XULRunner
archives contain a runtime and example applications; from the extracted archive,
launch the Simple example with:

```sh
xulrunner/xulrunner-bin applications/simple/application.ini
```

Run `client.mk` configurations sequentially within one checkout: they share
`.mozconfig.mk`. Once a full build succeeds, incremental work can use `make`
in the corresponding object-directory subdirectory. Bundled dependencies build
through the existing infrastructure. Profiles can enable the in-tree libIDL
bootstrap with `BOOTSTRAP_IN_TREE_LIBIDL`; configurations that leave it unset
continue to use system libIDL.

## Platform status

The following summarizes recorded development results as of **2026-09-16**.
Build, packaging, runtime, and workflow results are distinct: a mozconfig or a
successful cross-build alone does not establish operating-system compatibility.
The linked guides retain the detailed test scope and reproduction procedures.

| Platform | Recorded results | Remaining limits |
| --- | --- | --- |
| macOS arm64 / x86_64 | All four applications pass local `act` build, package, runtime, and artifact-upload jobs. GUI regressions and the pinned ES5 Test262 suite pass on arm64 and on x86_64 through Rosetta. | Tested on macOS 15.7.1. GitHub-hosted runs, physical Intel hardware, and minimum deployment OS execution remain unverified. |
| Mac OS X PowerPC | All four applications pass the local `act` build/package/runtime/upload matrix, including GUI and platform probes on original Mac OS X 10.0 under emulation. | Physical PowerPC hardware and GitHub-hosted runs remain untested. See the [10.0 status](mozconfigs/macos/powerpc/10.0-status.md) and [interactive UTM guide](mozconfigs/macos/powerpc/utm.md). |
| Mac OS X i386 | All four applications pass local `act` compilation and packaging checks with an explicit 10.4 deployment target. | Runtime compatibility is unverified; recorded artifact uploads encountered a local server limitation. |
| Windows x86 | Suite and Browser pass complete local MSVC2005/Wine `act` workflows; Suite, Browser and XULRunner also pass the GitHub-hosted build, package, audit, runtime and upload jobs. A separate September 16 Suite package passes 17 regression groups in NT 4.0, Me, and 2000 guests. | The new Wine-tested package has not been revalidated on original Windows releases. Calendar passes hosted build/package/audit; its runtime rerun remains pending after fixing omitted JavaScript component files. Windows 95 blockers remain open; see the [Windows build guide](build/win32/msvc8-cross/README.md). |
| Linux LoongArch64 | GTK2 Browser and Suite builds are recorded; experimental Xlib Browser creates and paints windows. | Xlib remains less complete. Additional Calendar and Xlib application profiles have not been runtime-validated. |
| Linux aarch64 | All eight Suite/Browser/Calendar/XULRunner × GTK2/Xlib jobs pass local `act` compilation, packaging, runtime tests and uploads. Each passes all 11,540 pinned ES5.1 cases. | Native Oracle Linux 8 / GCC Toolset 14 containers; GitHub-hosted runs and other distributions remain unverified. See the [Linux guide](build/linux/README.md#aarch64-bring-up). |
| Linux i686 / x86_64 | Suite passes all four local `act` build, ABI, package, runtime and upload jobs across GTK2/Xlib and both architectures. x86_64 GTK2 Browser and Calendar also pass complete local workflows. | Other application combinations and GitHub-hosted runs remain unverified. See the [Linux status](build/linux/README.md). |

Modern macOS profiles currently target 11.0 for arm64 and 10.6 for x86_64. The
broader legacy goals remain Intel Mac OS X 10.4 onward and PowerPC Mac OS X 10.0
onward; these goals should not be confused with the tested combinations above.

The minimum Windows targets are **Windows 95 and Windows NT 4.0**, treated as
independent requirements. The recorded Suite guest tests used NT 4.0 reporting
SP6 (SP6 versus SP6a was not independently verified), Windows Me 4.90.3000, and
Windows 2000 SP4. They do not establish Windows 95 compatibility, earlier NT4
service-pack support, or validation of every application. The
[Windows compatibility record](build/win32/msvc8-cross/COMPATIBILITY.md) tracks
these limits and the remaining CRT/import review. Windows builds use the static
CRT and existing component aggregation without a VC80 runtime DLL dependency.

Inherited OS/2, other Unix, and historical toolkit code remains in the tree.
Its presence is not a current support claim. Unix development centers on GTK2
and the lower-dependency Xlib path; GTK3/GTK4 migration is outside the project
direction.

## Application and JavaScript compatibility

SpiderMonkey is being modernized within the existing engine and embedding APIs.
Full ECMAScript 5.1 correctness is a required target. The pinned historical
Test262 suite passes **11,540 / 11,540 required-mode cases** in the latest
macOS arm64 engine validation, with zero failures, crashes, timeouts, or harness
errors. All four macOS arm64 application packages pass their compatibility
checks; earlier platform results do not revalidate the current ES6 changes.
This includes inherited earlier-edition coverage and annotated strict-mode
cases. Unmarked cases use the upstream non-strict default; Unicode source,
directive prologues, expected exceptions, and the required
`America/Los_Angeles` timezone are preserved by the runner.

The work includes strict execution, eval and invocation semantics, property
descriptors and integrity controls, JSON, function binding, parsing, and built-in
behavior. Passing a finite suite is evidence of progress, not exhaustive proof
of specification correctness or a result for every operating system. The
[ES5 testing guide](js/tests/es5/README.md) records the pinned revision, commands,
focused regressions, and embedding checks. Full ECMAScript 2015 (ES6) compliance
is the next required target, with historical XUL applications and legacy
JavaScript compatibility preserved. This work is in progress; the engine is
not yet ES6 compliant. The [ES6 testing guide](js/tests/es6/README.md) records
the baseline corpus, runner limitations and remaining implementation work.
The complete pinned historical ES6 corpus now passes **28,582 / 28,582 modes**,
with zero failures or unsupported cases. Later coverage is still under review,
and further edge-case and platform validation remains unfinished. This historical pass does not complete
the full ES2015 target.

Implemented areas include lexical bindings and temporal dead zones, per-iteration
environments, destructuring, arrows, default/rest parameters, spread, classes,
inheritance, `super`, `new.target`, generators, iteration, templates, Symbols,
collections, Promises, Proxy/Reflect, binary data, typed arrays, Unicode regular
expressions, standard-library additions and native modules. Focused regressions
cover callbacks, garbage collection, Unicode source, decompilation and wide
operands. Later tests continue to identify edge cases in these implementations.

Modern global initialization is opt-in through `xpcshell -E` or selection of
ES2015 before native standard-class initialization. Script edition changes alone
do not replace a global's built-ins. Existing application scripts retain their
historical defaults and embedding APIs; explicit legacy versions preserve their
grammar and compatibility behavior. The module JSAPI adds explicit compilation,
dependency linking and evaluation without changing classic XUL/component loaders.
It does not imply an HTML module-script loader or ordinary module XDR caching.

The detailed entries below record implementation milestones chronologically.
Use the latest integrated result in the ES6 testing guide for current validation
scope; historical failure counts and feature gaps in older entries describe
those earlier milestones.

Application compatibility is tested separately from language conformance:

* Unchanged ChatZilla initializes in Suite and a temporary standalone XULRunner
  wrapper after fixes to built-in initialization in window globals. This covers
  startup, not IRC network operation.
* Calendar retains its historical accessor syntax and XBL method receivers.
  Its eight existing unit tests pass, and GUI checks exercise startup and day,
  week, multiweek, and month views.
* Composer retains the legacy script syntax it needs. Edit/undo/close cycles,
  application teardown, and debugger callbacks have dedicated
  [lifecycle regressions](editor/composer/tests/README.md).
* Native embedding and chrome/content window tests check behaviors that shell
  tests cannot cover, including constructor initialization and object receivers.

The [macOS packaged runtime tests](build/macosx/tests/README.md) cover relocated
applications, navigation, graphics, and these application regressions. Native
input also needs its own checks: see the
[startup keyboard regression](mozconfigs/macos/arm64/README.md#startup-keyboard-regression)
for typing immediately after the first window opens, shortcuts, and reactivation.

## HTML and CSS compatibility

Selective rendering improvements support application interfaces, MailNews HTML,
and compatible web content. Implemented additions include structural HTML block
elements, standard `inline-block` behavior, circular `border-radius` aliases,
viewport width/height/orientation media queries with resize restyling,
single-layer `background-size`, and `linear-gradient()` rendering. Native opacity
groups use an optional rendering-context interface while historical backends
retain their existing blender path.

The Basilisk and Pale Moon websites are required, bounded rendering regression
targets. Their shared needs drive engine changes rather than site-specific
exceptions. The [layout probes](layout/html/tests/style/README-probes.md) cover
169 assertions alongside painting fixtures, responsive menus, FAQ toggles, and
resize behavior. Passing parser or CSSOM assertions does not establish complete
site rendering; layout and painting must also be inspected in Browser and Suite.

Remaining gaps include flexbox sizing and alignment, multiple background layers,
shadows, transitions, and keyframe animation. Background shorthand `/` sizing,
radial/repeating gradient functions, and newer gradient syntax are also outside
the implemented subset. Downloadable fonts and external font libraries are
excluded from this work. Complete rendering of either site is not yet claimed.

## Bundled dependencies

Dependencies are updated individually behind the existing Mozilla interfaces.
The bundled versions below describe this source tree, including private copies
that must not be assumed interchangeable with their main-tree counterparts.

| Dependency | Version | Source / integration notes |
| --- | --- | --- |
| NSS | 3.42, customized beta marker | [Version header](security/nss/lib/nss/nss.h) |
| NSPR | 4.7.7 | [Version header](nsprpub/pr/include/prinit.h) |
| SQLite, Mozilla storage | 3.53.4 | [Header](db/sqlite3/src/sqlite3.h); updated from 3.3.5 |
| SQLite, NSS private copy | 3.7.15 | [Header](security/nss/lib/sqlite/sqlite3.h) |
| Expat | 2.8.4 | [Gecko pause/replay adapter and regressions](parser/expat/README.zoolrunner.md) |
| libpng | 1.6.58 | [Header](modules/libimg/png/png.h) |
| zlib, main tree | 1.3.2 | [Header](modules/zlib/src/zlib.h) |
| zlib, NSS private copy | 1.2.5 | [Header](security/nss/lib/zlib/zlib.h) |
| bzip2/libbzip2 | 1.0.8 | [Version header](modules/libbz2/src/bzlib_private.h); updated from 1.0.3 |
| libjpeg-turbo | 3.2.0 | [Bundled source](jpeg); replaces IJG 6b while retaining its API compatibility level (`JPEG_LIB_VERSION 62`) |
| Cairo | 1.1.1 in the version header | [Header](gfx/cairo/cairo/src/cairo-features.h.in); the historical [integration notes](gfx/cairo/README) still identify 1.0.2 |
| libIDL | 0.8.14 | [Bundled source](build/unix/libIDL); final upstream release, retained for XPIDL |

Updates must account for historical APIs, local library modifications, MSVC
2005, and minimum operating-system targets. NSPR, NSS, Cairo/pixman, and
SpiderMonkey's fdlibm require particular care because of their platform and
runtime assumptions. Vendored libIDL keeps builds possible where distributions
no longer supply it; it does not replace XPIDL.

## Contributing

**Preserve the platform. Modernize the implementation. Fix the bugs.**

High-priority work includes reproducible crashes, use-after-free defects,
allocation and integer errors, pointer truncation, malformed-input handling,
and assumptions about word size, alignment, or byte order. Modern compiler
diagnostics and sanitizers help find these problems. Unsupported content should
fail gracefully without crashing or corrupting the process. ZoolRunner remains
a historical codebase undergoing stabilization; it is not claimed to be audited,
memory-safe, or a hardened replacement for a current mainstream browser.

Contributions should explain the affected application behavior or platform API,
keep changes focused, and preserve historical compatibility where practical.
Retain the configure/make build system, portable shell conventions, and existing
OS and architecture paths. Avoid broad mechanical warning cleanup or dependency
upgrades without understanding the underlying behavior.

Read [AGENTS.md](AGENTS.md) for development requirements. Include relevant
runtime checks as well as compilation, use disposable test profiles, and record
the exact application, architecture, OS, and test scope. Update the appropriate
build or test guide alongside behavior changes; keep this README's overview and
limitations aligned with those records.

## History, license, and credits

The source lineage is **Mozilla 1.8.1 → RetroZilla → ZoolRunner**. Mozilla
provided the application platform and included applications; RetroZilla carried
it forward with an emphasis on legacy Windows and practical maintenance.
ZoolRunner continues that work with the application runtime as its central focus.

Mozilla and RetroZilla authorship, copyright, and attribution remain part of the
project. See [LICENSE](LICENSE), [LEGAL](LEGAL), and individual source and
third-party license files for applicable terms and credits.


On the ES6 development branch, modern array/object literal construction now
uses private per-global intrinsic constructors, including classic embedding
globals without reserved slots. Legacy scripts retain their historical binding
lookup. This milestone used bytecode cache version 37; see the latest entry
for the current version. All four macOS arm64 applications passed the packaged compatibility checks;
the full ES5 run passes 11,540 cases, and ES2015 remains at 23,528 passes with
5,038 failures, 14 unsupported cases and two harness errors. Other platforms
have not been revalidated for this change. See `js/tests/es6/README.md`.

`Array.of` is implemented with generic construction, intrinsic fallback and
throwing own-property creation. All four macOS arm64 applications pass the
packaged compatibility checks. ES2015 is at 23,550 passes, 5,016 failures,
14 unsupported cases and two harness errors; all 11,540 ES5 cases pass. Other
platforms have not been revalidated for this change.

Symbol primitives include identity, property keys, reflection, conversions and
a runtime-wide registry. The full macOS arm64 ES2015 run passes 23,781 cases,
with 4,785 failures, 14 unsupported cases and two harness errors. All 11,540 ES5
cases pass. All four macOS arm64 applications pass build, package and runtime
checks; other platforms have not been revalidated for this batch. Several
well-known-symbol protocols remain unimplemented; this is not full ES2015
compliance. See `js/tests/es6/README.md`.

String `includes`, `startsWith` and `endsWith` now respect Symbol.match-based
RegExp classification. The full macOS arm64 ES2015 run passes 23,787 cases,
with 4,779 failures, 14 unsupported cases and two harness errors; all 11,540
ES5 cases pass. All four applications pass packaged compatibility checks.
Other platforms have not been revalidated for this batch.

Fresh RegExp literal evaluation is implemented in the explicitly selected
ES2015 edition; legacy editions retain their historical literal identity.
Execution, decompilation, realm and XDR checks pass on macOS arm64, as do all
four applications' packaged compatibility checks. The complete ES2015 result
remains 23,787 passes and 4,779 failures, with 14 unsupported cases and two
harness errors; all 11,540 ES5 cases pass. Other platforms have not been
revalidated for this batch.

Symbol.hasInstance custom dispatch and Function.prototype's ordinary instance
check are implemented. The full macOS arm64 ES2015 run passes 23,811 cases,
with 4,755 failures, 14 unsupported cases and two harness errors. All 11,540
ES5 cases and all four applications' packaged compatibility checks pass.
Legacy script modes and the public JS_HasInstance API preserve their native
dispatch. Other platforms have not been revalidated for this batch.

Math/JSON Symbol.toStringTag properties and modern default-tag corrections are
implemented, preserving legacy native class names. The full macOS arm64 ES2015
run passes 23,815 cases, with 4,751 failures, 14 unsupported cases and two harness
errors. All 11,540 ES5 cases and all four applications' packaged compatibility
checks pass. Other platforms have not been revalidated for this batch.


Array/String iterator methods and modern arguments' Symbol.iterator are
implemented while preserving classic iteration. On macOS arm64, 62 focused
checks and 23 native embedding checks pass. The full ES2015 run passes 23,909
cases, with 4,657 failures, 14 unsupported cases and two harness errors; all
11,540 ES5 cases pass. All four applications pass packaged compatibility checks.
Iteration syntax and consumers remain separate work. Other platforms have not
been revalidated for this batch.


Array.from is implemented for iterable and array-like sources, with generic
constructors, mapping and iterator cleanup. On macOS arm64, 36 focused checks
and 17 embedding checks pass. The full ES2015 run passes 23,975 cases, with
4,591 failures, 14 unsupported cases and two harness errors; all 11,540 ES5
cases pass. All four applications pass packaged compatibility checks. Other
platforms have not been revalidated for this batch.


Symbol.unscopables and Array's standard exclusion table are implemented for
modern with statements. Legacy scripts and embedding/global scopes retain their
lookup behavior. On macOS arm64, 37 focused checks, 14 embedding checks and
all 155 upstream with-statement cases pass. The full ES2015 run passes 23,982
cases, with 4,584 failures, 14 unsupported cases and two harness errors; all
11,540 ES5 cases pass. All four applications pass packaged compatibility checks.
Other platforms have not been revalidated for this batch.


ES2015 object literals support computed data properties, ordinary concise
methods, computed accessors and shorthand properties, preserving legacy parsing.
The bytecode cache is version 38. On macOS arm64, 58 focused checks, two
extended-atom checks and 14 edition/embedding checks pass. The full ES2015 run
passes 24,063 cases, with 4,503 failures, 14 unsupported module cases and two
harness errors; all 11,540 ES5 cases pass. All four applications pass packaged
compatibility checks. Other platforms remain unverified for this batch.
Generator methods, super and computed destructuring remain separate work.

Map and Set support ordered storage, iterable construction, mutation-safe
iteration and callback/GC handling. On macOS arm64, the full ES2015 run passes
24,708 cases, with 3,858 failures, 14 unsupported module cases and two harness
errors. All 11,540 ES5 cases and all four packaged application checks pass.
Arrow functions remain separate work; other platforms have not been revalidated
for this batch. Weak collections are covered below.

Reflect implements the 2015 operations,
including `enumerate` and alternate-target construction, and passes 53 focused
checks and 43 native embedding checks on macOS arm64. The pinned Reflect
subset passed 266/288 cases before Proxy work; the remaining 22 required Proxy. The full ES2015
run passes 25,330 cases, with 3,236 failures, 14 unsupported module cases and
two harness errors, gaining 266 passes without regressions. All 11,540 ES5.1
cases pass. All four macOS arm64 applications pass build, package and desktop
checks, including Calendar views, Composer and ChatZilla. Other platforms have
not been revalidated for this batch; see the
[ES6 validation notes](js/tests/es6/README.md).

Proxy now supports forwarding, property/prototype/extensibility traps,
call/construction, revocation and ES2015 enumeration. On macOS arm64, all 398
pinned Proxy cases and all 288 Reflect cases pass. The full ES2015 run passes
25,767 cases, with 2,799 failures, 14 unsupported module cases and two harness
errors; 437 cases were gained without losing earlier passes. All 11,540 ES5.1
cases and all four packaged application checks pass. This remains partial ES6
support: native host wrapping, other language features and other-platform
validation still need work. The Annex B follow-up addresses __proto__ receivers. See the
[Proxy validation notes](js/tests/es6/README.md#proxy).

The Annex B follow-up adds standard __proto__ accessors to ES2015-initialized
globals, correct HTML helper quoting/conversion order in ES2015 code, and
persistent deletion of modern global String helpers. Legacy globals and
cross-edition virtual ownership remain covered. Its full macOS arm64
ES2015 run passes **25,783 cases**, with **2,783 failures**, 14 unsupported module
cases and two harness errors: 16 gained, zero lost. All 11,540 ES5.1 cases and
all four applications' build/package/desktop checks pass. Other platforms have
not been revalidated for this batch; see the
[Annex B validation notes](js/tests/es6/README.md#annex-b-built-ins).

The RegExp follow-up implements ES2015 prototype accessors, escaped source,
generic flags/toString and own lastIndex in modern globals. Legacy globals
retain their historical fields and matcher-bearing prototype. Native creation,
cache decoding, failure cleanup and mixed-edition lazy initialization are
covered. Its full macOS arm64 ES2015 run passes **25,839 cases**, with
**2,727 failures**, 14 unsupported module cases and two harness errors:
56 gained, zero lost. All 11,540 ES5.1 cases and all four applications' build,
package and desktop checks pass. Unicode/sticky matching, RegExp protocols and
other ES6 work remain; other platforms have not been revalidated for this batch.
See [RegExp validation notes](js/tests/es6/README.md#regexp-prototype-fields).

The match/search follow-up adds generic RegExp test, Symbol.match/Symbol.search,
String dispatch, observable ES2015 exec property access and result-realm handling.
Native loops remain interruptible; legacy globals retain their original methods.
The match/search full macOS arm64 ES2015 run passes **25,969 cases**, with **2,597
failures**, 14 unsupported module cases and two harness errors: 130 gained, zero
lost. All 11,540 ES5.1 cases and all four applications' build, package and desktop
checks pass. See [match/search validation notes](js/tests/es6/README.md#regexp-matchsearch-protocols)
for remaining limitations and platform scope.

The constructor/sticky follow-up adds the `y` flag, ES2015 RegExp construction,
species, newTarget realm handling and direct RegExpCreate for String fallback.
Legacy source grammar and initialized legacy globals retain their behavior.
The constructor/sticky full macOS arm64 ES2015 run passes **26,061 cases**, with **2,505
failures**, 14 unsupported module cases and two harness errors: 92 gained,
zero lost. All 11,540 ES5.1 cases and all four application build, package and
desktop checks pass. Cache version 39 invalidates older component caches.
Unicode `u`, replacement/split protocols and other ES6 work remain incomplete;
other operating systems and architectures have not been revalidated. See
[constructor validation notes](js/tests/es6/README.md#regexp-constructors-and-sticky-flags).

The split follow-up adds RegExp species construction, Symbol.split dispatch,
capture preservation, result-realm handling and the corrected ES2015 limit
conversion. The split full macOS arm64 ES2015 run passes **26,143 cases**, with
**2,423 failures**, 14 unsupported module cases and two harness errors: 82 gained,
zero lost. All 11,540 ES5.1 cases pass. All four application builds and package
checks pass, and the completed desktop run passes. An earlier intermittent
Inspector lifecycle error is retained in the [split validation notes](js/tests/es6/README.md#regexp-and-string-split-protocols).
Unicode matching, replacement protocols and other ES6 work remain incomplete;
other platforms have not been revalidated.

The replacement follow-up adds Symbol.replace, String dispatch, callback and
capture ordering, checked substitution buffers and legacy-method isolation.
The replacement full macOS arm64 ES2015 run passes **26,249 cases**, with **2,317
failures**, 14 unsupported module cases and two harness errors: 106 gained,
zero lost. All 11,540 ES5.1 cases and all four application build, package and
desktop checks pass. Unicode RegExp matching and other ES6 work remain incomplete;
other platforms have not been revalidated. See [replacement validation notes](js/tests/es6/README.md#regexp-and-string-replacement-protocols).

The Date follow-up adds Symbol.toPrimitive, modern prototype/constructor behavior,
zero-argument ordinary conversion and legacy-global isolation. All 898 Date
cases pass. The Date full macOS arm64 ES2015 run passes **26,281 cases**, with
**2,285 failures**, 14 unsupported module cases and two harness errors: 32 gained,
zero lost. All 11,540 ES5.1 cases and all four build, package and desktop checks
pass. Full ES6 and other-platform validation remain incomplete. See
[Date validation notes](js/tests/es6/README.md#date-conversion-and-prototype).

The concat follow-up adds Symbol.isConcatSpreadable, Array species construction,
sparse results, safe-integer indices and method-realm handling. Legacy globals
retain their historical concat. The concat full macOS arm64 ES2015 run passes
**26,330 cases**, with **2,236 failures**, 14 unsupported module cases and two
harness errors: 49 gained, zero lost. All 11,540 ES5.1 cases and all four build,
package and desktop checks pass. Full ES6 and other-platform validation remain
incomplete. See [concat validation notes](js/tests/es6/README.md#array-concat-and-species).

The callback-array follow-up adds ES2015 ToLength, map/filter species results,
sparse/live traversal and native-loop interruption while preserving legacy
methods. All 3,048 cases across seven methods pass. The callback-method full macOS arm64
ES2015 run passes **26,394 cases**, with **2,172 failures**, 14 unsupported module
cases and two harness errors: 64 gained, zero lost. All 11,540 ES5.1 cases and all
four build, package and desktop checks pass. Full ES6 and other-platform
validation remain incomplete. See [iteration validation notes](js/tests/es6/README.md#array-callback-methods).

The indexed-array follow-up modernizes push/pop, shift/unshift, reverse,
slice/splice and indexOf/lastIndexOf while retaining the legacy implementations.
All 1,144 diagnostic cases pass. The indexed-method full macOS arm64 ES2015 run passes
**26,452 cases**, with **2,114 failures**, 14 unsupported module cases and two
harness errors: 58 gained, zero lost. All 11,540 ES5.1 cases and all four build,
package and desktop checks pass. Full ES6 and other-platform validation remain
incomplete. See [indexed-method validation notes](js/tests/es6/README.md#array-indexed-methods).

The Array string/sort follow-up adds modern string dispatch, primitive locale
forwarding, cycle handling without enumeration, and interruptible sparse sorting.
All 141 method-subset cases pass. The string/sort full macOS arm64 ES2015 run passes
**26,462 cases**, with **2,104 failures**, 14 unsupported module cases and two
harness errors: 10 gained, zero lost. All 11,540 ES5.1 cases and all four build,
package and desktop checks pass. Full ES6 and other-platform validation remain
incomplete. See [string/sort validation notes](js/tests/es6/README.md#array-string-conversion-and-sorting).

The Error follow-up implements modern constructor/prototype relationships,
message deletion and conversion order while preserving native reports and legacy
behavior. All 148 diagnostic cases pass. The Error full macOS arm64 ES2015 run
passes **26,482 cases**, with **2,084 failures**, 14 unsupported module cases and
two harness errors: 20 gained, zero lost. All 11,540 ES5.1 cases and all four
build, package and desktop checks pass. Full ES6 and other-platform validation
remain incomplete. See [Error validation notes](js/tests/es6/README.md#error-construction-and-prototypes).

The binary-data follow-up adds ArrayBuffer and DataView, including species,
endian-aware numeric access and detachment-safe callbacks. All 146 diagnostic
cases pass. The binary-data full macOS arm64 ES2015 run passes **26,630 cases**, with
**1,936 failures**, 14 unsupported module cases and two harness errors: 148
gained, zero lost. All 11,540 ES5.1 cases and all four build, package and desktop
checks pass. Typed arrays, full ES6 and other-platform validation remain
incomplete. See [binary-data validation notes](js/tests/es6/README.md#arraybuffer-and-dataview).

Modern String casing uses Unicode 18.0.0 full mappings and context-sensitive
sigma while preserving legacy methods and embedding locale callbacks. All 204
diagnostic cases and 4,456,448 UCD comparisons pass. The casing full macOS arm64
ES2015 run passes **26,648 cases**, with **1,918 failures**, 14 unsupported module
cases and two harness errors: 18 gained, zero lost. All 11,540 ES5.1 cases and all
four build, package and desktop checks pass. Full ES6 and other-platform validation
remain incomplete. See [casing validation notes](js/tests/es6/README.md#modern-unicode-casing).

The invocation follow-up fixes modern apply length conversion, call/apply
receiver checks and foreign native exception realms while preserving legacy
methods. All 715 Function cases pass. The invocation full macOS arm64 ES2015 run
preserves **26,648 passes**, **1,918 failures**, 14 unsupported module cases and
two harness errors, with zero lost passes. All 11,540 ES5.1 cases and all four
build, package and desktop checks pass. Full ES6 and other-platform validation
remain incomplete. See [invocation validation notes](js/tests/es6/README.md#modern-function-invocation).

The reflection follow-up fixes modern key ordering and foreign result-array
realms while preserving legacy script behavior and proxy order. All 5,984 Object
and 208 JSON cases pass. The reflection macOS arm64 ES2015 run passes **26,654
cases**, with **1,912 failures**, 14 unsupported module cases and two harness
errors: six gained, zero lost. All 11,540 ES5.1 cases and all four build, package
and desktop checks pass. Full ES6 and other-platform validation remain incomplete.
See [reflection validation notes](js/tests/es6/README.md#reflection-key-ordering-and-array-realms).

WeakMap and WeakSet include garbage-collector support for weak key/value
reachability, native finalizer checks and legacy generator cleanup. On macOS
arm64, all 322 pinned built-in cases pass; the full ES2015 run passes 25,064
cases, with 3,502 failures, 14 unsupported module cases and two harness errors.
All 11,540 ES5 cases and all four packaged application checks pass. Other
platforms have not been revalidated for this batch.

The ES6 job queue foundation adds explicit embedding checkpoints and shell
execution tests. Queue support alone is not Promise conformance. The corrected
macOS arm64 queue-foundation run preserves 26,654 ES6 passes and all 11,540 ES5 passes; all four application checks
pass. Application testing found and fixed a context-migration teardown crash;
an isolated lifecycle timeout remains recorded in the validation notes. See the
[job queue notes](js/tests/es6/README.md#ecmascript-job-queue-foundation) for
ownership, teardown and validation limits.


Promise construction, reactions, then/catch, resolve/reject, all/race and
application job checkpoints are implemented. That batch’s full macOS arm64 run
passes **27,036 ES2015 cases**, with **1,530 failures**, 14 unsupported module
cases and two harness errors: 382 gained, zero lost, no crashes/timeouts.
All **11,540 ES5.1 cases** and all four application build, package and desktop
checks pass. Application testing also fixed job principal lookup for sandbox
callbacks while preserving content security restrictions. Full ES6 and
other-platform validation remain incomplete. See the
[Promise validation notes](js/tests/es6/README.md#promise-and-application-checkpoints).

ES2015 ordinary-function `new.target` supports strict/direct eval, alternate
constructors and serialized scripts. The final macOS arm64 checks preserve all
27,036 ES6 passes and 11,540 ES5 passes, with all four application build, package
and desktop checks passing. Bytecode cache version 40 invalidates older component
caches. Arrow functions and classes remain unfinished; see the [new.target notes](js/tests/es6/README.md#function-environment-newtarget).

ES2015 property-reference ordering and large-script property/decompilation fixes
pass **27,064 ES6 cases**, with **1,502 failures**, 14 unsupported module cases
and two harness errors: 28 gained, zero lost. All 11,540 ES5 cases pass.
The fixes preserve computed keys across RHS evaluation and retain primitive
setter receivers. All four macOS arm64 applications pass build, package and
desktop checks, including Calendar views, browser navigation and ChatZilla.
Cache version 41 invalidates older serialized code. See the [property reference notes](js/tests/es6/README.md#property-assignment-references)
for scope and validation status.

ES2015 identifier-reference retention and wide-name operations raise the full
macOS arm64 result to **27,100 ES6 passes**, with **1,466 failures**, 14 unsupported
module cases and two harness errors: 36 gained, zero lost. All 11,540 ES5 cases
pass, and the compound-assignment group passes all 703 cases. All four
applications pass build, package and desktop checks with the additional
large-script native assignment/debugger correction. Its 38 native checks and
existing debugger lifecycle checks pass. See the [identifier reference notes](js/tests/es6/README.md#identifier-assignment-references).

ES2015 ordinary and tagged templates raise the full macOS arm64 result to
**27,217 ES6 passes**, with **1,349 failures**, 14 unsupported module cases and
two harness errors: 117 gained, zero lost, no crashes/timeouts. All 11,540 ES5
cases pass. All four applications pass build, package and desktop checks,
including Calendar views, browser navigation/layout and ChatZilla. Their engine
binaries match the frozen conformance runtime. Focused checks cover raw Unicode,
decompilation/XDR, realm identity, reentrant native allocation/GC and long lines.
See the [template notes](js/tests/es6/README.md#template-literals) for the
ES2015-specific cache rules and remaining validation limits.

ES2015 arrows with simple parameters raise the full macOS arm64 result to
**27,296 ES6 passes**, with **1,270 failures**, 14 unsupported module cases and
two harness errors: 79 gained, zero lost, no crashes/timeouts. All 11,540 ES5
cases pass. Lexical `this`, `arguments` and `new.target`, constructor rejection,
metadata and XDR/decompilation are covered, including native callback/GC reentry.
The callback tests also exposed and fixed argument lifetime and debugger/eval
receiver defects. All four applications pass build, package and relocated
desktop checks, including Calendar views, browser navigation/layout and ChatZilla;
their engine binaries match the frozen conformance runtime.
Default/destructured parameters and class/super behavior remain incomplete. See the
[arrow notes](js/tests/es6/README.md#arrow-functions).

Named rest parameters are implemented for ordinary/arrow functions, methods and
`Function`, with independent arguments snapshots, intrinsic array creation and
source/XDR reconstruction. The final macOS arm64 suite passes **27,316 ES6
cases**, with **1,250 failures**, 14 unsupported modules and two harness errors:
20 gained, zero lost, no crashes/timeouts. All 11,540 ES5 cases and the 56 focused
and 24 native checks pass. All four applications pass build, package and relocated
desktop checks, including Calendar views, browser navigation/layout and ChatZilla;
their engine binaries match the frozen conformance runtime. Default parameters
and rest patterns remain unfinished. See the [rest notes](js/tests/es6/README.md#rest-parameters).


The block-lexical initialization batch passes **27,338 ES2015 cases** on
macOS arm64, with **1,228 failures**, 14 unsupported modules and two harness
errors (**22 gained, zero lost**). All **11,540 ES5.1 cases** pass. Modern block
and function-body `let` accesses now reject uninitialized bindings, including
captured closures after frame exit, while selected legacy let semantics are
preserved. Global lexical storage remains unfinished; the following batch adds
block const scoping and per-iteration loop bindings. All four macOS arm64 applications pass build,
package and desktop checks, including Calendar views, browser navigation and
ChatZilla. Other platforms have not been revalidated for these changes; see
[the ES6 validation record](js/tests/es6/README.md#block-lexical-initialization).


The lexical-scope batch implements modern block/function-body `const`
and fresh named `let`/`const` loop bindings while preserving selected legacy
semantics. The complete macOS arm64 run passes **27,377 ES2015 cases**, with
**1,189 failures**, 14 unsupported modules and two harness errors
(**39 gained, zero lost**). All **11,540 ES5.1 cases** still pass. Global lexical
environments, `for-of` and broader destructuring remain unfinished. All four
macOS arm64 applications pass build, package and desktop checks, including
Calendar views, Browser navigation/layout and ChatZilla. Other platforms have
not been revalidated for this batch; see [the detailed validation record](js/tests/es6/README.md#block-const-and-per-iteration-bindings).


The subsequent strict declaration-position fix brings the complete macOS arm64
run to **27,383 ES2015 passes**, with **1,183 failures**, 14 unsupported module
cases and two harness errors (**six gained, zero lost**). All **11,540 ES5.1
cases** pass. Strict statement bodies reject bare function declarations while
valid block declarations and selected legacy syntax remain available. All four
macOS arm64 applications pass build, package and desktop checks, including
Calendar views, Browser navigation/layout and ChatZilla. Other platforms remain
unvalidated for this change. See the [validation record](js/tests/es6/README.md#strict-function-declaration-positions).


Modern `for…of` now supports the Symbol.iterator protocol, named per-iteration
`let`/`const` bindings, assignment targets and iterator cleanup through abrupt
completion. Historical for-in/for-each and selected-edition XML wildcard syntax
remain available. The implementation preserves source reconstruction and cached
bytecode round-trips. See [the for-of validation record](js/tests/es6/README.md#for-of-iteration)
for conformance totals, compatibility checks and remaining dependencies.

The complete corrected macOS arm64 run passes **27,485 ES2015 cases**, with
**1,081 failures**, 14 unsupported modules and two harness errors:
**102 gained, zero lost**, with no crashes or timeouts. All **11,540 ES5.1 cases**
pass. All four applications pass build, package and desktop checks, including
Calendar's four views, Browser navigation/layout and Suite/XULRunner ChatZilla.
This is progress toward full ES2015 compliance, not completion. Other platforms
have not been revalidated for this batch.


Modern Unicode identifier parsing now accepts brace escapes and supplementary
characters using separate pinned Unicode tables, while legacy editions retain
their old rules. Identifier source reconstruction also round-trips non-ASCII
names. The full macOS arm64 run passes **27,499 ES2015 modes**, with **1,067
failures**, 14 unsupported modules and two harness errors (**14 gained, zero
lost**). All 11,540 ES5 cases and all four application build/package/runtime
checks pass. Other platforms have not been revalidated for this batch. See
[the validation record](js/tests/es6/README.md#unicode-identifier-code-points).


Modern generators now implement function-star and generator methods, delegated
yield, return/throw completion and a separate modern iterator protocol while
preserving classic generator APIs. The complete macOS arm64 run passes
**27,878 ES2015 modes**, with **688 failures**, 14 unsupported modules and two
harness errors (**379 gained, zero lost**). All **11,540 ES5 cases** and all
four application build/package/runtime checks pass. Full ES2015 compliance
remains unfinished. See [the generator validation record](js/tests/es6/README.md#modern-generators).


All nine ES2015 typed-array types now provide integer-indexed views, native
methods, species construction and shared ArrayBuffer storage. The latest full
macOS arm64 result is **27,940 passing ES2015 modes**, **628 failures** and
**14 unsupported modules**, with no harness errors, crashes or timeouts
(**62 gained, zero lost**). All **11,540 ES5 cases** and all four application
build/package/runtime checks pass. Native tests exercise GC, detachment and
reflection during allocation callbacks. Full ES2015 compliance remains
unfinished; other platforms have not been revalidated for this batch. See
[the typed-array validation record](js/tests/es6/README.md#typed-arrays).


Basic ES2015 object patterns now support shorthand bindings and consistent
null/undefined checks, with preserved decompiled source. Invalid update targets
report the original ES2015 early errors. The latest full macOS arm64 run passes
**27,997 ES2015 modes**, with **571 failures** and **14 unsupported modules**
(**57 gained, zero lost**), and all **11,540 ES5 cases**. All four application
build/package/runtime checks pass. Full ES2015 compliance remains unfinished.
See [the pattern validation record](js/tests/es6/README.md#basic-object-patterns-and-update-targets).

Computed object-pattern keys now support Symbols, conversion callbacks and
generator suspension, with source and bytecode-cache round trips. The latest
full macOS arm64 run passes **28,002 ES2015 modes**, with **566 failures** and
**14 unsupported modules** (**five gained, zero lost**), no harness errors,
crashes or timeouts, and all **11,540 ES5 cases**. All four application
build/package/runtime checks pass. Full ES2015 compliance remains unfinished;
other platforms have not been revalidated for this batch. See
[the computed-pattern validation record](js/tests/es6/README.md#computed-object-pattern-keys).

Destructuring defaults now run only for undefined values and preserve source,
lexical initialization, generator suspension and bytecode-cache round trips.
The latest full macOS arm64 result is **28,085 ES2015 passes**, **483 failures**
and **14 unsupported modules**, with no harness errors, crashes or timeouts
(**83 gained, zero lost**). All **11,540 ES5 cases** and all four application
build/package/runtime checks pass. Full ES2015 compliance remains unfinished;
other platforms remain unvalidated for this batch. See
[the defaults validation record](js/tests/es6/README.md#destructuring-defaults).

ES2015 array patterns now consume iterators, support rest elements, retain
exhaustion and close unfinished iterators. Object and array assignments capture
the destination before source getters or defaults. Legacy selected-edition
patterns retain indexed access. The latest full macOS arm64 result is
**28,147 ES2015 passes**, **421 failures**, **14 unsupported modules**, and no
harness errors, crashes or timeouts (**62 gained, zero lost**). All **11,540 ES5
cases** and all four application build/package/runtime checks pass. Full ES2015
compliance remains unfinished; other platforms remain unvalidated for this
batch. See [the iterator-pattern validation record](js/tests/es6/README.md#iterator-based-patterns-and-captured-assignment-targets).

ES2015 Unicode regular expressions pass macOS arm64 validation. The full run
reaches **28,193 passes**, **375 failures**, and **14 unsupported modules**,
with no harness errors, crashes or timeouts (46 gained, zero lost). The `u` flag
adds code-point matching, strict escape grammar and Unicode simple case folding.
Focused, native callback/cache and all pinned case-fold mapping checks pass;
all **11,540 ES5 cases** and all four application build/package/desktop checks
pass. Full ES2015 compliance remains unfinished; other platforms have not been
revalidated for this batch. See the
[Unicode regexp notes](js/tests/es6/README.md#unicode-regular-expressions).

Global declaration preflight and ES2015 statement completion values pass macOS
arm64 validation: **28,199 ES2015 passes**, **369 failures**, **14 unsupported
modules**, no harness errors, crashes or timeouts (**six gained, zero lost**).
All **11,540 ES5 cases** and all four application build/package/desktop checks
pass. The changes preserve explicitly selected legacy behavior and exercise
embedding callbacks, source reconstruction, XDR and real window globals. Full
ES2015 compliance remains unfinished; other platforms are not revalidated for
this batch. See the [validation notes](js/tests/es6/README.md#global-declarations-and-statement-completion-values).

Array literals, function calls and constructors support iterator-based spread.
Focused, native embedding, wide-argument, cache/source and differential checks
pass. macOS arm64 validation retains **28,199 ES2015 passes**, **369 failures**
and **14 unsupported modules**, with no lost passes, harness errors, crashes or
timeouts. All **11,540 ES5 cases** and all four application build/package/desktop
checks pass. Full ES2015 compliance remains unfinished; other platforms have
not been revalidated for this batch. See the
[spread validation notes](js/tests/es6/README.md#array-call-and-constructor-spread).

Persistent global and fresh eval lexical environments pass macOS arm64
validation: **28,213 ES2015 passes**, **355 failures**, and **14 unsupported
modules**, with 14 gained and zero lost passes, harness errors, crashes or
timeouts. All **11,540 ES5 cases** and all four application build/package/desktop
checks pass. Full ES2015 compliance remains unfinished; other platforms have not
been revalidated for this batch. See the
[lexical binding notes](js/tests/es6/README.md#persistent-global-and-eval-lexical-bindings).


Object-method `super` properties now use traced home objects,
receiver-preserving calls and setters, arrows, direct eval and reconstructed
source/cache support. The complete integrated macOS arm64 run records 28,229 ES2015
passes, 339 failures and 14 unsupported modules (16 gained, zero lost), with all
11,540 ES5 cases passing. All four applications pass build, package and relocated
desktop checks, including Calendar views, Browser navigation/layout and Suite /
XULRunner ChatZilla; see `js/tests/es6/README.md`. Classes, full parameter
environments and modules remain unfinished. Other platforms have not been
revalidated for this batch.


Scripted setter assignment results now follow standard-edition behavior while
preserving explicitly selected legacy behavior. The complete macOS arm64 run
passes **28,231 ES2015 cases**, with **337 failures**, **14 unsupported modules**,
and no harness errors, crashes or timeouts (two gained, zero lost). All **11,540
ES5 cases** and all four applications' build/package/desktop checks pass,
including Calendar views, Browser navigation/layout and Suite/XULRunner ChatZilla.
Full ES2015 compliance remains unfinished; other platforms are not revalidated
for this batch.


ES2015 classes and derived constructors now include class source/cache round
trips, inherited methods and lexical `super()` calls. The integrated macOS arm64
run passes **28,519 cases**, with **49 failures**, **14 unsupported modules**,
zero harness errors/crashes/timeouts, and **288 gained, zero lost**. All **11,540
ES5 cases** and all four applications' build/package/desktop checks pass,
including Calendar views, Browser navigation/layout and Suite/XULRunner ChatZilla.
See `js/tests/es6/README.md`. Full parameter environments, module execution and
full ES2015 compliance remain unfinished; other platforms are not revalidated
for this batch.


ES2015 catch-variable instantiation and duplicate block-function errors now
pass integrated macOS arm64 validation: **28,522 ES6 passes, 46 failures and
14 unsupported modules**, with three gained and zero lost. All 11,540 ES5 cases
and all four applications' build/package/desktop checks pass, including Calendar
views, Browser navigation/layout and Suite/XULRunner ChatZilla. Full ES2015
compliance remains unfinished. Other platforms were not revalidated for this
batch; see `js/tests/es6/README.md`.


ES2015 default parameters and parameter environments pass integrated macOS arm64
validation: **28,568 ES6 passes, zero failures, 14 unsupported module cases**,
with 46 gained and zero lost. ES5 remains **11,540/11,540**. All four applications
pass build/package/desktop checks, including Calendar views, Browser
navigation/layout, and Suite/XULRunner ChatZilla. Module support is still required
for completion. Other platforms were not revalidated for this batch; see
`js/tests/es6/README.md`.


The native module implementation reaches **28,582/28,582 pinned ES6 modes**,
with zero failures or unsupported cases, and preserves **11,540/11,540 ES5**.
All four macOS arm64 applications pass build/package/desktop checks, including
Calendar views, Browser navigation/layout and Suite/XULRunner ChatZilla. This
is the complete historical corpus, not the completion of the broader ES2015
coverage review: later tests and newly found edge cases remain under review
and implementation. Other platforms were not revalidated for this batch. See
`js/tests/es6/README.md` for APIs, reports and remaining work.


The module-context follow-up corrects literal `as`/`from` parsing, export-list
semicolon insertion, and lexical `new.target` availability through arrows and
direct eval. Public namespace retrieval now propagates failed module records.
The integrated macOS arm64 run preserves **28,582/28,582 ES6** and
**11,540/11,540 ES5**, with zero failures or unsupported cases. All four
applications pass builds, packages and relocated desktop checks, including
Calendar views, Browser navigation/layout and Suite/XULRunner ChatZilla.
Reports: `artifacts/es6/module-context-final-*`. Focused coverage includes
9 contextual-keyword, 8 module-production and 12 `new.target` checks, plus
24 native module checks across five scripts. C89 checks pass. Other platforms
remain unvalidated for this batch; the later-test review remains open.


The later built-in/statement edge batch preserves **28,582/28,582 pinned ES6**
and **11,540/11,540 ES5**, with zero failures, unsupported cases, crashes,
timeouts or harness errors on macOS arm64. It corrects Number.toString and
RegExp.compile arity in modern globals, undefined substr lengths in ES2015,
Symbol subclass definition versus constructor rejection, statement-position
function restrictions, and duplicate catch-binding SyntaxErrors. Explicit
legacy statement forms still pass in JavaScript 1.7. The local statement
fixture now distinguishes Annex B's permitted positions from legacy extensions;
no upstream tests were changed. There are 21 new built-in checks and 24 new
statement checks, plus 39 updated statement/edition checks.

All four applications pass root builds, packages and relocated desktop checks,
Calendar's four views, Browser navigation/layout and Suite/XULRunner ChatZilla.
Reports use `artifacts/es6/later-edges-final-*`. C89 checks pass; adding the
missing realm-helper header produces byte-identical libraries across all four
applications and the frozen conformance runtime (SHA-256
`0edb98fd8de17b868885d7125a17ad309d79dca89ea76375ad2700ea48e6cc9c`).
The later 5,852-mode diagnostic with isolated realms improves to 5,820 passes
and 32 failures, without harness errors. That diagnostic still needs edition
review and phase checking and is not a full ES2015 conformance result. Further
pattern/scope fixes are being validated separately. Other platforms have not
been revalidated for this batch.


The pattern/realm/strict-block follow-up preserves **28,582/28,582 pinned ES6**
and **11,540/11,540 ES5**, plus all four macOS arm64 build/package/desktop gates,
Calendar views, Browser navigation/layout and Suite/XULRunner ChatZilla. Strict
block functions now have lexical scope and entry initialization; pattern,
contextual-arrow, foreign-realm eval and RegExp identity-escape cases are fixed.
The phase-aware later-ID diagnostic improves to 5,833 passes and 19 failures;
those are not conformance counts. Broader coverage confirms that proper tail
calls remain unfinished. Cache version is 63. See `js/tests/es6/README.md` for
reports, host APIs and the remaining coverage review. Other platforms were not
revalidated for this batch.


The original-ES2015 RegExp range and buffer-detachment follow-up preserves
**28,582/28,582 pinned ES6** and **11,540/11,540 ES5**, with all four macOS arm64
build/package/desktop gates, Calendar views, Browser navigation/layout and
Suite/XULRunner ChatZilla passing. Modern RegExp grammar follows its constructor
realm even when called from legacy code. The native `JS_DetachArrayBuffer` API
now backs the isolated Test262 host. The later diagnostic runner recognizes
upstream's informational `generated` flag; 18 runner controls pass. See the
ES6 guide for reports and edition differences. Full ES2015 completion remains
open, and this batch does not revalidate other platforms.


The conversion/built-in follow-up adds modern Number parser aliases, corrects
DataView arity and omitted-offset handling, and fixes conversion fallback,
exception propagation and `isPrototypeOf` receiver ordering. Legacy conversion
conventions and native embedding hooks have explicit cross-realm coverage.
Final validation again passes **28,582/28,582 pinned ES6**, **11,540/11,540 ES5**
and all four macOS arm64 build/package/application gates, including Calendar
views and Browser/ChatZilla integration. See the ES6 guide for exact reports,
expanded diagnostic limitations and unfinished tail-call/coverage work.
This is not a new Windows or Linux validation result.


The tail-call follow-up adds strict ES2015 frame retirement across ordinary,
arrow, bound, spread, Proxy and native forwarding calls, including constructor
return handling. Direct eval and explicit legacy editions keep their existing
behavior. It also rejects invalid generator `yield` shorthand and corrects
idle-debugger hook handling and strict-function stack visibility. Final macOS arm64 validation passes **28,582/28,582 pinned ES6** and
**11,540/11,540 ES5**, all four builds/packages/relocated desktop checks, Calendar
views, Browser navigation/layout and Suite/XULRunner ChatZilla. Focused coverage
adds 101 shell checks and 20 native checks; C89 checks pass. Other operating
systems have not been revalidated for this batch. See [tail-call validation](js/tests/es6/README.md#tail-call-execution-and-generator-shorthand).


The property-conversion follow-up fixes key/receiver conversion ordering and
array-length callbacks that change writability during coercion. Modern
assignments preserve their original value while native storage and legacy
JSAPI conventions remain intact. Both complete pinned suites pass (28,582 ES6 and 11,540 ES5 cases), as do
all four macOS arm64 application builds, packages and relocated runtime checks,
Calendar’s four views, Browser navigation and Suite/XULRunner ChatZilla.
Other operating systems have not been revalidated for this batch.
See [property-conversion validation](js/tests/es6/README.md#property-queries-and-reentrant-array-length-conversion).


The ES2015 property-order batch preserves creation order during
descriptor replacement, creates interpreted constructor metadata in the
original ES2015 order, and keeps dynamic-function compiler metadata out of
reflection. Both complete pinned suites pass with zero failures, along with
all four macOS arm64 builds, packages and relocated runtime checks;
see [property ordering](js/tests/es6/README.md#property-replacement-and-function-creation-order).


The ES2015 parser/index batch corrects catch parameter/body environments,
string code-point escapes, generator `let` parsing and negative-zero typed-array
indices. Both complete pinned suites pass with zero failures, as do focused
and native checks, all four macOS arm64 builds/packages and relocated application
checks, including Calendar’s four views. Other operating systems have not been
revalidated for this batch. See
[parser and index validation](js/tests/es6/README.md#catch-environments-unicode-strings-and-signed-zero-indices).

Generator-method parsing and decompilation now preserve valid `get`/`set` names
and reject invalid accessor forms. Modern caller reflection handles strict
callers while retaining explicitly selected legacy behavior. Both complete
pinned suites and all four macOS arm64 build/package/runtime checks pass;
see [generator and caller validation](js/tests/es6/README.md#generator-method-grammar-and-caller-reflection).
Broader later-Test262 edition review and validation on other operating systems
remain unfinished.

The current environment edge-case batch fixes switch discriminant closure
scope, default-only switch decompilation and ES2015 `with` binding reads after
observable lookup. It retains explicit legacy scope behavior and extends the
native cache/source round-trip checks; bytecode cache version is 64. Both
complete pinned suites pass with zero failures, along with all four macOS arm64
build/package/runtime checks and the expanded native collection/cache probe.
The broader edition review and other-platform validation remain unfinished; see
[environment validation](js/tests/es6/README.md#switch-discriminant-environments).

JSON reviver definitions now reject non-configurable replacements atomically
while preserving callback exceptions. Both pinned suites and all four macOS
arm64 build/package/runtime gates pass; a Suite lifecycle retry and the
separately reproduced Inspector startup race are documented in the
[JSON reviver validation](js/tests/es6/README.md#json-reviver-descriptor-rejection).
Broader edition review and other-platform validation remain unfinished.

Modern JSON now preserves method-realm allocations and wide Proxy-array
lengths, while legacy methods retain their historical length conversion.
Inspector startup also handles browser loads arriving before viewer readiness.
Both full pinned suites and all four macOS arm64 application gates pass; see
[JSON realm and length validation](js/tests/es6/README.md#json-method-realms-and-array-lengths).

Canonical String comparison and incomplete non-Unicode RegExp escapes now
pass both complete pinned suites and all four macOS arm64 application gates,
with legacy method behavior and grammar retained. The ES6 result required a
complete retry after one shell-bootstrap error; both reports are retained.
RegExp cache records now preserve their compilation edition (cache version 65).
See [String/RegExp edge validation](js/tests/es6/README.md#canonical-comparison-and-incomplete-regexp-escapes).
