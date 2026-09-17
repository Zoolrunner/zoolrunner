# Linux builds

The i686 and x86_64 bring-up uses **Oracle Linux 8** (`oraclelinux:8`) and
**GCC Toolset 14** for both local builds and CI. The amd64 container includes
Oracle's multilib development packages for i686. Host utilities remain
x86_64; the i686 target compiler uses `-m32 -march=i686` and target pkg-config
metadata from `/usr/lib/pkgconfig`.

Both architectures have GTK2 and Xlib mozconfigs for Suite, Browser, Calendar and
XULRunner. All four x86 Suite configurations pass the complete local workflow;
the full sixteen-entry application matrix remains unverified. The additional LoongArch Calendar and Xlib
application profiles have not been runtime-tested on this host.

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
and repeated calls. Big-endian AArch64 and ILP32 are outside this port's scope.

Every ARM runtime job checks 2,000 ABI calls and runs the complete pinned ES5.1
Test262 required-mode suite (11,540 cases, America/Los_Angeles). It fetches the
exact upstream revision, preserves Unicode transport preflight, and retains the
full JSON report. Calendar additionally runs its eight unit suites and all four
views. These supplement the shared relocated-package application tests.

Validation is in progress. The first GTK2 Suite compiles and passes the shared
packaged runtime checks; full ARM matrix and extended regression results are
not yet established. GitHub-hosted execution remains unverified.
