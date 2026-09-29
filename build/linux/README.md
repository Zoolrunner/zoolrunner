# Linux builds

The i686 and x86_64 bring-up uses **Oracle Linux 8** (`oraclelinux:8`) and
**GCC Toolset 14** for both local builds and CI. The amd64 container includes
Oracle's multilib development packages for i686. Host utilities remain
x86_64; the i686 target compiler uses `-m32 -march=i686` and target pkg-config
metadata from `/usr/lib/pkgconfig`.

Both architectures have GTK2 and Xlib mozconfigs for Suite, Browser, Calendar and
XULRunner. All four x86 Suite configurations pass the complete local workflow;
the full sixteen-entry x86 application matrix remains unverified. Native
LoongArch Suite passes both backends, and Browser, Calendar and XULRunner pass
GTK2 package/runtime checks. Their remaining Xlib validation is in progress;
see the Speedometer work notes.

The native LoongArch GCC profiles also preserve `-flifetime-dse=1` and
`-fno-strict-aliasing`. A GCC 15 Suite lifecycle run exposed an invalid frame
style context during Venkman startup without these settings. Validation of
the corrected profiles is tracked in the
[Speedometer work notes](../../layout/html/tests/speedometer21/README.md);
the flags alone do not establish a passing application matrix.

```sh
docker build --platform linux/amd64 -f build/linux/oraclelinux8.Dockerfile \
  -t zoolrunner-oraclelinux8-gcc14 build/linux
docker volume create zoolrunner-linux-x86_64-suite-gtk2
docker run --rm --platform linux/amd64 \
  --mount "type=bind,source=$PWD,target=/source,readonly" \
  --mount type=volume,source=zoolrunner-linux-x86_64-suite-gtk2,target=/work \
  zoolrunner-oraclelinux8-gcc14 sh /source/build/linux/build-ci.sh x86_64 suite gtk2
```

Use a separate work volume for each architecture/application/toolkit combination.
The workflow defaults to three compiler jobs. Local `act` runs can select a
different count with `--env ZR_BUILD_JOBS=8`; choose a count that fits the
host's CPU and memory capacity, including other running virtual machines.
The source is copied to the Linux filesystem, where the object directory and logs remain
available for incremental porting work. Replace both `x86_64` and `suite` in
the example to select another configuration. The third argument selects
`gtk2` (the default) or `xlib`; use a distinct volume for each. Xlib object directories have an
`-xlib` suffix, and archive names include the toolkit. On Apple Silicon the amd64 host
tools run through OrbStack/Rosetta; 32-bit runtime execution requires working
i386 emulation and must be checked separately from compilation.

The Linux workflow has 24 jobs (four applications × three targets × two toolkits).
Packages dereference build-tree symlinks and omit host utilities. Validation
checks ELF class/machine, dynamic dependencies, generated SpiderMonkey ABI
metadata, JavaScript regressions, native JSAPI/Expat probes, and GTK2/Xlib windows
under Xvfb. Suite also exercises application lifecycle and ChatZilla fixtures.
All applications exercise native font enumeration. Browser preferences checks
also require populated font menus, since the original font builder catches
enumeration errors. These checks exposed an Xlib font-context crash during the
native LoongArch matrix; see [font regression coverage](../../gfx/tests/README-font-enumeration.md).

The relocated-package runner sets both `LD_LIBRARY_PATH` and
`MOZILLA_FIVE_HOME` to the extracted runtime. Unix XPCOM uses the latter to
find its components; without it, launching `xpcshell` from the source checkout
fails with `failed to get nsJSRuntimeService!` even when all ELF dependencies
resolve. The native Expat probe also needs the target build's NSPR headers.

Local `act` validation on 2026-09-16–17 passed all four Suite jobs, including
fresh compilation, packaging, relocated-package runtime checks and both
artifact uploads. These runs used Oracle Linux 8 / GCC Toolset 14 containers
on Apple Silicon, with three compiler jobs for x86_64 GTK2 and eight for the
other Suite configurations. They are local results, not GitHub-hosted runs.

| Suite target | Compile | Target ABI | Package | Runtime | `act` / uploads |
| --- | --- | --- | --- | --- | --- |
| x86_64 GTK2 | PASS | PASS | PASS | PASS | PASS |
| i686 GTK2 | PASS | PASS | PASS | PASS | PASS |
| x86_64 Xlib | PASS | PASS | PASS | PASS | PASS |
| i686 Xlib | PASS | PASS | PASS | PASS | PASS |

Each Suite runtime run passed the shell regressions, native JSAPI/Expat probes,
application navigation, window bootstrap, all 24 lifecycle checks and ChatZilla.
Local logs, uploaded archives and package checksums are retained under
`artifacts/linux-act-validation`; `results-suites.json` records workflow exits.
x86_64 GTK2 Browser and Calendar also passed complete local workflows before
validation was narrowed to Suite. Other application combinations remain
unverified; Suite success does not establish their results.

The lifecycle checks exposed GCC optimization assumptions that conflict with
the classic implementation. The x86 profiles
now use `-flifetime-dse=1` to preserve arena zeroing before frame constructors
(including the empty frames used by HTML `wbr`), and `-fno-strict-aliasing` to
preserve `nsCOMPtr` typed output-pointer writes. Without the latter, interface
enumeration appended null entries and window globals lacked `NodeFilter`.
The Account Wizard opened by Address Book also exposed an Xlib queue-dispatch bug:
the Xt callback watched the nested queue's descriptor but drained the shell's
original queue. It now dispatches the subscribed queue, and input IDs retain
their `XtInputId` width during removal on LP64. Both Xlib Suite workflows pass
the modal startup and lifecycle paths. The runtime runner retains partial
subprocess logs on timeout.

XULRunner GUI tests select the fixture with `toolkit.defaultChromeURI` in the
disposable profile. Its default command-line handler does not implement the
Browser `-chrome` option. The fixture still opens the unchanged Simple app
and verifies its XPT, JavaScript and native C++ components.

For local artifact uploads, use an
`act` build with the v7 artifact-server compatibility fix described in the
[macOS guide](../../mozconfigs/macos/README.md#act-artifact-server-limitation);
stock act 0.2.87 rejects the production upload action's `mime_type` field.
The validated runs used the local `act` build with the upstream PR 6115
artifact-server fixes. Commit source fixes before rerunning a native `act`
job: reusing its cache at the same HEAD can retain the previous checkout.
Verify the copied source when diagnosing a rerun.

Bring-up has exposed and addressed host/target libIDL metadata selection,
the group-box paint and MathML reflow overrides' i686 interface calling conventions,
missing multilib development packages, Perl 5.26 literal-brace handling in
LDAP header generation, SQLite and GDK shared-library header visibility,
missing calling-convention annotations in interface overrides, and a
host-word-size leak in SpiderMonkey's i686 CPU header generator. The generator
now passes size/alignment checks against actual GCC 32-bit and 64-bit types.
The Xlib app shell's `Run()` override uses `NS_IMETHOD` / `NS_IMETHODIMP`
to preserve the interface calling convention on i686. The group-box paint
and MathML dirty-reflow declarations likewise use `NS_IMETHOD` to match their
base interfaces. All are covered by the completed Suite workflows above.

## AArch64 bring-up

The little-endian AArch64 LP64 port uses native Oracle Linux 8 and GCC Toolset
14, with eight profiles in `mozconfigs/linux/aarch64`. GTK2 and Xlib each have
Suite, Browser, Calendar and XULRunner profiles. The CI matrix uses GitHub's
[native `ubuntu-24.04-arm` runner](https://docs.github.com/en/actions/reference/runners/github-hosted-runners)
and `build/linux/oraclelinux8-aarch64.Dockerfile`; x86 jobs retain their existing
amd64/multilib environment. ARM host tools are native, with no x86 multilib flags.

```sh
docker build --platform linux/arm64 -f build/linux/oraclelinux8-aarch64.Dockerfile \
  -t zoolrunner-oraclelinux8-gcc14-aarch64 build/linux
docker volume create zoolrunner-linux-aarch64-suite-gtk2
docker run --rm --platform linux/arm64 \
  --mount "type=bind,source=$PWD,target=/source,readonly" \
  --mount type=volume,source=zoolrunner-linux-aarch64-suite-gtk2,target=/work \
  zoolrunner-oraclelinux8-gcc14-aarch64 \
  sh /source/build/linux/build-ci.sh aarch64 suite gtk2
```

Linux XPCOM uses a separate
[AAPCS64](https://github.com/ARM-software/abi-aa/blob/main/aapcs64/aapcs64.rst)
implementation: integer and floating-point arguments use independent register
banks, and spilled scalars occupy eight-byte slots. Darwin's compact stack
layout is not applicable. ELF stubs declare their function type and size.
`TestXPTCallABI.cpp` checks native invocation and incoming stubs with mixed
register/stack arguments, narrow scalars, 64-bit values, pointers, out parameters
and repeated calls. The probe links against the packaged XPCOM library:
`libxul` for XULRunner, or `libxpcom_core` for the separate-library builds.
The combined Xlib `libxul` link also retains `MOZ_XLIB_LDFLAGS`, including
libXext for the drag cursor's X Shape calls, just as the separate widget library does.
Big-endian AArch64 and ILP32 are outside this port's scope.

Every ARM runtime job checks 2,000 ABI calls and runs the complete pinned ES5.1
Test262 required-mode suite (11,540 cases, America/Los_Angeles). It fetches the
exact upstream revision, preserves Unicode transport preflight, and retains the
full JSON report. Calendar additionally runs its eight unit suites and all four
views. These supplement the shared relocated-package application tests.

All eight AArch64 jobs pass fresh local `act` compilation, CPU-ABI checks,
ELF/package audits, relocated-package runtime tests and both artifact uploads.
These runs used native ARM containers on Apple Silicon with eight compiler jobs.
GitHub-hosted execution and other Linux distributions remain unverified.

| Application | GTK2 compile/package/runtime/`act` | Xlib compile/package/runtime/`act` |
| --- | --- | --- |
| Suite | PASS | PASS |
| Browser | PASS | PASS |
| Calendar | PASS | PASS |
| XULRunner | PASS | PASS |

Each job passed all 11,540 pinned ES5.1 cases (10,894 non-strict and 646 strict),
2,000 native XPCOM ABI calls, 17 window-bootstrap checks, 18 native embedding
checks and 30 Expat checks, alongside the focused JavaScript regressions and
application fixtures. Both Suite jobs passed all 24 lifecycle checks and
ChatZilla startup. Both Calendar jobs passed eight unit suites and all four
views, including navigation and the script-console check. XULRunner exercised
the unchanged Simple application and its JavaScript and native C++ components.

Additional tests against each of the eight extracted packages passed the
existing `build/macosx/tests/early-nss.c` and `early-sqlite.c` probes: SHA-256,
ChaCha20-Poly1305, P-256 signing and verification, SQL/DBM database startup,
SQLite recursive locking and 800 concurrent inserts. These are supplemental
local checks; the workflow's automated regression coverage is listed above.

The [ARM conformance record](../../js/tests/es5/linux-aarch64-results.json)
records each validated commit, report/package hashes and separate workflow
stages. Archives, uploaded ZIP files and complete diagnostic logs are retained
locally under `artifacts/linux-aarch64-validation`; `results-final.json` gives
the eight successful workflow exits. Earlier failures remain in the history.
The XULRunner ABI-probe link failure was fixed by selecting its packaged
`libxul`, and the conformance runners use Python 3.6-compatible subprocess
arguments for Oracle Linux 8. No upstream ES5 tests or assertions were changed.

This validates the listed application and component regressions, not every
historical application's behavior or exhaustive ECMAScript conformance.

## ES2015 regression gate

The runtime stage now also calls `test-es6.py` for every application, architecture
and backend. Both container recipes install Oracle Linux's Python YAML package
for the pinned ES2015 runner. The gate runs the shared focused fixture table,
native probes and all 28,582 required modes at Test262 revision
`5e653f2e6ca14ac1ad8e801955a709cae7ac8a11`, in America/Los_Angeles. It uses four
workers and a 60-second per-case limit. Reports are retained under `logs/es6`;
no subset or reviewed later-edition result substitutes for the complete run.

Most native probes link against the relocated package's `libmozjs.so`. Five
unit probes intentionally call private engine interfaces: ClassRuntime,
GlobalLexicalStore, MethodHome, SuperReference and TypedArrays. Those link a
temporary archive of the production Makefile's exact `OBJS` into standalone
test executables. This does not export private functions or link a second
engine into an application. The gate records object hashes and requires the
packaged engine to match the build providing them. Generated target headers
and target compiler flags are used, including `-m32` for i686.

The expanded workflow passes native aarch64 Suite GTK2 through local `act` for
the discarded-derived-this correction: build, target ABI, package, runtime,
11,540/11,540 ES5.1 cases, 28,582/28,582 ES2015 modes, 117 focused fixtures,
69 native probes and both artifact uploads. Logs, original reports and the
source snapshot are retained under `artifacts/es6/linux-this-effects-act-suite-gtk2`
and `linux-this-effects-snapshot`. Other entries have not yet been validated with
this expanded gate. The earlier eight-job table above records the ES5-era gate,
not this new ES2015 gate. Supplemental
testing of the Suite GTK2 package at `57e7022a` passes the full ES2015 corpus.
Its first attempt recorded four exhaustive URI-decoding timeouts with the
10-second default. The unchanged corpus passed in full with the 60-second
limit; both reports remain under `artifacts/es6/linux-date-act-suite-gtk2`.
The parser follow-up's Suite workflow at `b7551120` also passes its existing
gate. Its supplemental run passed 116 focused fixtures and 63 packaged native
probes, but five private-probe links failed and the external harness encountered
a Git worktree-path error. Those failures remain recorded; they motivated
running internal probes while production build objects are still available.

For the subsequent discarded-operation/arguments-detachment correction, native
aarch64 Suite, Browser and Calendar GTK2 each pass the expanded local `act` workflow:
build, ABI, package, runtime, 11,540 ES5.1 cases, 28,582 ES2015 modes, 122 focused
fixtures, 70 native probes and artifact uploads. The unchanged packaged engine
hash is `df4c535ffecd820f568f445fc097825e7927f0a6c9e9450401d6b16260cee42e`.
The frozen source and reports are under `artifacts/es6/linux-discarded-effects-*`.
XULRunner GTK2 also passes after the retry documented below; the four Xlib
entries are still running; this is not a complete new
matrix result and does not establish Linux x86 or GitHub-hosted validation.

The XULRunner GTK2 continuation recorded 11,538 ES5.1 passes and two exhaustive
URI-decoding timeouts at the original 10-second per-case limit. The exact
packaged engine matches the first three applications' hash. Both unchanged cases
pass alone in about nine seconds (`linux-es5-uri-timeout-investigation`); these
subset diagnostics do not replace the failed full report. The Linux ES5 gate
now uses the same uniform 60-second per-case limit as ES6, retaining every case,
mode and assertion. The full XULRunner retry passes all 11,540 ES5 cases with
zero failures or timeouts, followed by all 28,582 ES6 cases, all 122 focused
fixtures, 70 native probes, runtime checks and both uploads. Its complete workflow
passes; the remaining Xlib entries are still pending. Runner reports now include the per-case limit and worker count.

The native ES2015 driver accepts `loongarch64` as well as the existing Linux
architectures. The Speedometer Suite development runtime passes its 139 focused
fixtures, 83 native probes and all 28,582 pinned ES2015 modes. This result does
not establish package validation or results for other LoongArch applications;
see the [Suite work notes](../../layout/html/tests/speedometer21/README.md).

The package and relocated-runtime drivers also accept `loongarch64`. For native
builds whose object directory differs from the CI naming convention, pass
`--objdir /absolute/path/to/object-directory` to both drivers. The work directory
must contain a `source` checkout (or symlink), `logs`, and `artifacts`. For example:

```sh
python3 build/linux/package-ci.py loongarch64 suite "$work" --objdir "$obj"
sh build/linux/with-display.sh python3 build/linux/test-package.py loongarch64 suite "$work" --objdir "$obj"
```

Use `--toolkit xlib` for an Xlib package. Architecture validation checks ELF64,
little endian, machine 258. LoongArch runs the full pinned ES5.1 and ES2015 gates,
the compiler-generated XPTCall argument/return ABI probe, native embedding,
application windows, and Calendar unit/four-view coverage when testing Calendar.
Adding this driver support does not establish that the application matrix passes;
results remain tracked in the Speedometer work notes.

The first LoongArch GTK2 Suite package attempt passed all 11,540 ES5.1 cases,
then failed the XPTCall ABI probe at its first stack-passed float. The LoongArch
invoke bridge now copies spilled FP bits, and its stub bridge consumes available
integer registers before stack slots after exhausting FP registers. Unsigned
32-bit register arguments receive the sign extension required by the
[LoongArch procedure call standard](https://github.com/loongson/la-abi-specs/blob/release/lapcs.adoc).
Both native probes pass 2,000 calls each after the correction. `TestXPTCallFP.cpp`
adds coverage where FP registers run out first; the existing ABI probe covers
interleaved stack arguments. Full package validation is being rerun; these
focused passes do not substitute for it.

The native LoongArch GTK2 Suite package retry passes the complete gate: 11,540
ES5.1 cases, 28,582 ES2015 modes, 139 focused fixtures, 83 native ES2015 probes,
both 2,000-call ABI probes, embedding, window and Suite application checks.
Reports are retained in `artifacts/speedometer21/package-suite-gtk2-abi-fixed`.
The package predates subsequent DOM propagation and string-key corrections;
see the Speedometer notes for those focused results. Xlib's full gate is pending.
The driver now also runs `TestStringKeyLength.cpp` (19 checks), and resolves
native checkout symlinks when rejecting build-tree library dependencies.

Native LoongArch Xlib Suite also passes the complete relocated-package gate:
11,540 ES5.1 cases, 28,582 ES2015 modes, 139 focused fixtures, 83 native ES2015
probes, both ABI probes, native embedding and Suite windows/lifecycle/ChatZilla.
Reports are in `artifacts/speedometer21/package-suite-xlib-propagation`. This
package includes event propagation fixes but predates the string-key correction.
The later Xlib build passes all 927 content assertions and its 400-by-300
box-sizing painting region matches the Chromium reference pixel-for-pixel.
These are Suite results, not a completed four-application/backend matrix.

Session-history validation also runs `docshell/test/history-entry-state.js`
against the packaged xpcshell. The ten assertions check additive payload storage,
clone retention/replacement and preservation of historical entry identifiers,
URI and title. Native LoongArch Suite development builds pass on both backends;
packaged validation of this new check is pending the next package run.

Suite History state checks can be run separately with
`python3 layout/html/tests/speedometer21/run-history-probes.py --runtime OBJ/dist/bin --reports REPORT_DIR`.
The runner serves the unchanged fixtures locally and uses disposable Suite
profiles. Native LoongArch GTK2 and Xlib each pass 120 assertions and all 24
Suite lifecycle checks for the initial pushState/replaceState/state integration.
The expanded traversal/popstate fixtures pass 300 assertions on GTK2 over HTTP
and on Xlib with HTTP plus file reload/legacy checks. They cover queued
same-document and child traversal, event-state lifetime, reload and explicit
legacy calls. Cross-document events and other History behavior remain
incomplete. See the Speedometer test guide for limitations and the separate
full benchmark status.

For the other native applications, add `--application browser`, `calendar` or
`xulrunner` to the History and HTTP probe drivers. Individual content fixtures
use `layout/html/tests/speedometer21/run-toolkit.py`; it registers the test chrome
only in a disposable runtime copy and uses a private HOME/profile. This supplements
the relocated-package checks, including Calendar's unchanged unit and four-view
tests; content probes alone are not application compatibility validation.

The first complete Speedometer 2.1 native LoongArch GTK2 Suite run passed on
2026-09-29: 16 enabled workloads, ten iterations, all 480 workload checks and
the original completion callback. The frozen initial History-binding runtime
predates subsequent fixes; see `layout/html/tests/speedometer21/README.md` for
its exact provenance. A current Xlib package rerun and Browser/Calendar/XULRunner
validation are in progress. Missing rendering features remain unfinished, so
benchmark execution is not a full standards or application compatibility claim.

Current Suite packages at `989aaa00` pass the complete native LoongArch GTK2
and Xlib package gates. Each passes 11,540 ES5.1 cases, 28,582 ES2015 cases,
139 focused fixtures, 85 native probes, five internal probes, both ABI probes,
native embedding, window bootstrap, lifecycle and ChatZilla checks. Reports
are in `artifacts/speedometer21/history-package-{gtk2,xlib}/`. These package
results are distinct from the earlier frozen GTK2 benchmark pass.

Browser package checks now include all seven configured preference panes,
homepage editing and the existing restore-default modal alert through
`browser/components/preferences/tests/lifecycle.xul`. Native Browser GTK2
passes its 28 compatibility assertions, including a separate run from the
relocated package. The report explicitly records a pre-existing limitation:
the literal `about:home` default is requested as an `nsIPrefLocalizedString`,
so the default read fails and the old restore handler clears the homepage.
The test preserves that legacy null-to-empty conversion; it does not claim
the application's default-localization mismatch was fixed. Application sources
and the frozen preference interfaces remain unchanged.
