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
| Windows x86 | Suite passes the complete local MSVC2005/Wine `act` build, package, audit, runtime and upload workflow. A separate September 16 Suite package passes 17 regression groups in NT 4.0, Me, and 2000 guests. | The new Wine-tested package has not been revalidated on original Windows releases. Windows 95 blockers and the remaining application matrix remain open; see the [Windows build guide](build/win32/msvc8-cross/README.md). |
| Linux LoongArch64 | GTK2 Browser and Suite builds are recorded; experimental Xlib Browser creates and paints windows. | Xlib remains less complete. Additional Calendar and Xlib application profiles have not been runtime-validated. |
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
Test262 suite passes **11,540 / 11,540 cases in each of the eight modern macOS
application packages**, with zero failures, crashes, timeouts, or harness errors.
This includes inherited earlier-edition coverage and annotated strict-mode
cases. Unmarked cases use the upstream non-strict default; Unicode source,
directive prologues, expected exceptions, and the required
`America/Los_Angeles` timezone are preserved by the runner.

The work includes strict execution, eval and invocation semantics, property
descriptors and integrity controls, JSON, function binding, parsing, and built-in
behavior. Passing a finite suite is evidence of progress, not exhaustive proof
of specification correctness or a result for every operating system. The
[ES5 testing guide](js/tests/es5/README.md) records the pinned revision, commands,
focused regressions, and embedding checks. Selected later JavaScript features
may be added when they fit the existing architecture.

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
