# MSVC 2005 cross-build support

Windows builds are made from a **Linux or macOS host**, using genuine
**Microsoft Visual C++ 2005 (MSVC 8.0 / VC8)** tools through **Wine**. CrossOver
is a supported Wine provider on macOS. This directory connects those tools to
the existing Mozilla configure/make build; the wrappers are shared by both
hosts, and only runtime discovery is host-sensitive.

The minimum target operating systems are **Windows 95** and **Windows NT 4.0**.
These are compatibility requirements; see [COMPATIBILITY.md](COMPATIBILITY.md)
for actual validation results and unresolved runtime blockers.

The target is Windows x86.  Programs needed while building remain native to
the build host and use `HOST_CC`, `HOST_CXX`, `HOST_LD`, and `HOST_AR`.

## Toolchain layout

Set `MSVC8_ROOT` to the extracted toolchain root.  The installation used to
develop this support contains:

```text
MSVC8_ROOT/
  bin/                 cl.exe, link.exe, lib.exe, rc.exe, mt.exe, ml.exe
  include/             Visual C++ and CRT headers
  lib/                 Visual C++ and CRT libraries
  atlmfc/include/
  atlmfc/lib/
  PlatformSDK/Bin/
  PlatformSDK/Include/
  PlatformSDK/Lib/
  redist/x86/Microsoft.VC80.CRT/
```

The full Suite build requires the genuine Microsoft `midl.exe` and its
`midlc.exe` companion.  They may be placed in either `bin` or
`PlatformSDK/Bin`; the tested installation uses Microsoft MIDL 6.00.0366 from
the Windows Server 2003 SP1 Platform SDK.  Configure detects this older target
and supplies `-no_robust`, avoiding generated stubs which require Windows 2000.
The XULRunner configuration does not currently build a MIDL-using component.

The build does not use files from `redist`: target code uses the static
multithreaded CRT (`/MT`, or `/MTd` for an actual debug build).

The wrappers construct `INCLUDE`, `LIB`, and the Windows-side executable
search path explicitly for every invocation.  No Visual Studio registration,
`vcvars32.bat`, or persistent Wine-prefix modification is required.

## Runtime selection

The runtime launcher is selected in this order:

1. `MSVC8_WINE`
2. `WINE`
3. `wine` on `PATH`
4. the standard CrossOver application path on macOS

For CrossOver, set `MSVC8_WINE_BOTTLE` when a particular bottle is required.
`CX_BOTTLE` is also recognized.  Ordinary Wine ignores these CrossOver-only
settings.

Examples:

```sh
# macOS with CrossOver
MSVC8_ROOT=/path/to/msvc8.0 \
MSVC8_WINE=/Applications/CrossOver.app/Contents/SharedSupport/CrossOver/bin/wine \
MSVC8_WINE_BOTTLE=MyBottle \
build/win32/msvc8-cross/test-toolchain.sh

# Linux with Wine from PATH
MSVC8_ROOT=/path/to/msvc8.0 \
WINE=wine \
build/win32/msvc8-cross/test-toolchain.sh
```

The standalone test compiles and links a console program with genuine MSVC 8,
checks that it is PE32 when host tools permit, rejects a VC80 DLL-runtime
dependency, and runs it through the selected runtime.

## Building XULRunner or the Suite

Use the supplied mozconfig directly from the source root:

```sh
MSVC8_ROOT=/path/to/msvc8.0 \
WINE=wine \
MOZCONFIG=mozconfigs/cross/win32-msvc8-xulrunner.mozconfig \
make -f client.mk build
```

For the complete Suite, select
`mozconfigs/cross/win32-msvc8-suite.mozconfig` instead.  It uses eight make
jobs.  For Windows 95/NT 4 testing, use the static-component aggregate config:

```sh
MSVC8_ROOT=/path/to/msvc8.0 \
WINE=wine \
MOZCONFIG=mozconfigs/cross/win32-msvc8-suite-legacy.mozconfig \
make -f client.mk build

package_dir=`mktemp -d /tmp/zoolrunner-package.XXXXXX` || exit 1
make -C obj-zoolrunner-win32-msvc8-suite-legacy/xpinstall/packager dist \
  MOZ_PKG_DEST="$package_dir" MOZ_PKG_APPNAME=zoolrunner
```

The legacy config builds normal XPCOM modules into `mozcomps.dll`.  It does
not turn the Suite into one executable: NSPR, NSS, XPCOM, JavaScript, SQLite,
LDAP, MAPI, plugins, and components that must remain separate still use their
normal DLL boundaries.  Aggregation prevents a separate statically linked
VC8 CRT in every small component DLL from exhausting the 64 process TLS slots
available on Windows 95 and Windows NT 4.

The bundled Expat entropy provider resolves `ADVAPI32!SystemFunction036`
dynamically instead of calling the VC8 CRT `rand_s()` entry point.  VC8's
`rand_s()` terminates the process through its invalid-parameter handler when
that system entry point is absent, as it is on Windows 95 and Windows NT 4.
Expat therefore keeps the same high-quality provider on newer Windows and
uses its existing time/process fallback on the legacy systems.

`mozcomps.dll` delegates component requests to the original generic modules
inside the aggregate.  Their constructors remain lazy and independent, as in
a normal component build; requesting one component does not eagerly initialize
unrelated editor, parser, layout, networking, image, and application modules.

On macOS, set `MSVC8_WINE` and optionally `MSVC8_WINE_BOTTLE` as shown above.
The remaining command is identical.

`--enable-static-rtl` selects `/MT` throughout the Mozilla target and the
separately configured NSPR and NSS builds.  In addition, the compiler wrapper
rejects `/MD` and `/MDd` whenever `MSVC8_REQUIRE_STATIC_RTL` is active.  This
detects a third-party sub-build that attempts to reintroduce the DLL runtime
instead of relying on option ordering.

The legacy Suite config also enables `MSVC8_USE_PROCESS_HEAP`.  A small object
linked into every target image implements the public CRT allocation entry
points on top of the Win32 process heap.  This preserves `/MT` and avoids a
VC80 runtime DLL while giving Mozilla's historically cross-DLL C and C++
allocations one ownership domain.  Without it, each static LIBCMT copy owns a
different private heap, so a buffer created in one Mozilla DLL can fail when
another DLL releases it.

The MSVC 8 manifest tool can parse and create manifests under current
CrossOver but may fail while updating a PE resource.  When a linker-generated
manifest exists, the shared linker wrapper embeds it with the genuine MSVC 8
resource compiler and relinks with `/MANIFEST:NO`.  Set
`MSVC8_EMBED_MANIFEST=0` only for diagnosis.  Static-CRT release binaries do
not require the `Microsoft.VC80.CRT` private assembly.

## PE audit

After a build, audit all shipped executables and DLLs:

```sh
build/win32/msvc8-cross/audit-pe.sh \
  obj-zoolrunner-win32-msvc8-xulrunner/dist/bin \
  obj-zoolrunner-win32-msvc8-xulrunner/pe-imports.tsv
```

The audit follows the build's installation symlinks, fails on VC80 release or
debug DLL-runtime imports and embedded VC80 CRT deployment references, checks
that every image is x86 PE32, that PE OS/subsystem versions do not exceed 4.0,
and rejects non-system DLL imports which are absent from the package.  It also
lists every PE that imports `TlsAlloc` and rejects a payload with 64 or more
such images.  It emits
every imported API plus companion metadata and compatibility-review TSV files
for independent Windows 95 and Windows NT 4 SP6a review.  Audit the staged
package directory, not just `dist/bin`, so the report describes exactly what is
shipped.  See
COMPATIBILITY.md for the current status. Set `PE_OBJDUMP` if the host's
`objdump` is not the desired PE-aware implementation.

Path conversion is centralized in `msvc8-tool.pl`.  It converts native paths
only in known path-bearing options and operands, including response files,
instead of treating every slash as a path.  Wine's own `winepath` supplies the
drive mapping, so the implementation does not assume that the host filesystem
is mounted as `Z:`.

## Guest regression payload

The [Suite regression payload](tests/README.md) stages the existing JavaScript
and GUI checks for legacy Windows VMs with a separate profile and saved logs.
Run it against the audited aggregate package; host Wine execution alone does
not establish NT4, Windows Me or Windows 2000 runtime compatibility.

The aggregate build's `xpcshell` loads `mozcomps` at runtime and supplies its
module table to XPCOM, matching Suite initialization without creating a clean
build dependency cycle. In static MSVC CRT builds, the shell reads script files
with its own CRT and passes source bytes to JSAPI; a `FILE*` cannot safely cross
between the executable's and JavaScript DLL's independent CRTs. Both `-f` and
`load()` follow this path. Inline `-e` scripts use the shell's script principal,
as file scripts do, and evaluation failures return a failing exit status.

Static packaging must retain the application-facing typelibs separately from
`mozcomps.dll`. The Suite manifest includes startup, Composer, protocol and
other enabled platform interfaces; their omission can leave a browser window
working while breaking shutdown or other Suite applications. The payload
checks required interfaces before running the GUI regressions.

## Linux GitHub Actions builds

`.github/workflows/windows.yml` builds **Suite, Browser, Calendar and
XULRunner** as separate Windows x86 jobs on Ubuntu. The container uses genuine
MSVC 2005 through Wine 11 WoW64; host tools use the native Linux compiler.
The minimum target requirements remain **Windows 95 and Windows NT 4.0**.

`fetch-toolchain.sh` downloads a pinned, SHA-256-checked snapshot of
[widberg/msvc8.0](https://github.com/widberg/msvc8.0), plus the missing MIDL
compiler from Microsoft's Windows Server 2003 SP1 Platform SDK. The image
also pins WineHQ binary packages and verifies their hashes. No separately
installed Visual Studio or GitHub toolchain action is required.

To reproduce a job from the checkout root:

```sh
docker build --platform linux/amd64 \
  -f build/win32/msvc8-cross/Dockerfile \
  -t zoolrunner-msvc8-linux build/win32/msvc8-cross
mkdir -p /tmp/zoolrunner-windows-suite
docker run --rm --platform linux/amd64 \
  --mount "type=bind,source=$PWD,target=/source,readonly" \
  --mount "type=bind,source=/tmp/zoolrunner-windows-suite,target=/work" \
  -e ZR_BUILD_JOBS=3 zoolrunner-msvc8-linux \
  sh /source/build/win32/msvc8-cross/with-display.sh \
  sh /source/build/win32/msvc8-cross/build-ci.sh suite /work/build
```

Use a new work directory for each build. Replace `suite` with `browser`,
`calendar` or `xulrunner` as appropriate. Before starting Wine, the build script
creates the configured `WINEPREFIX` (`/work/wine`) as the container user.
GitHub's runner owns the bind-mounted `/work` directory; Wine refuses to create
a missing prefix beneath that differently owned parent. Creating the prefix
first leaves ownership of the host work directory unchanged.
The build copies the source into the
work directory, builds and packages the application, audits PE imports, and
runs packaged native, JavaScript and application-window tests in Wine/Xvfb.
Suite uses the aggregate component library; Toolkit applications use libxul.
Calendar's `xpfe/components/build2` application component must declare
`LIBXUL_LIBRARY` so it is archived into libxul with its translated module entry
point. Building it as a separate `appcomps.dll` instead fails during compilation
with a missing `dist/lib/xpcom.lib` prerequisite, before libxul is linked.
All profiles use the static CRT and process-heap allocation support.

Each job uploads its application ZIP and diagnostic logs. Wine regression
results do not replace tests in Windows 95, NT 4.0 or other actual Windows
installations. The Linux/Wine 11 toolchain has passed its compile/link/PE32/execution
smoke test in an x86_64 Debian VM, including process-heap support. The GNU
PE auditor also passes on the previously validated Suite package. Full
application jobs are undergoing local `act` validation. The first Suite job
identified an unnecessary X11-header dependency in the host `mkdepend` tool;
Windows builds now select its existing no-X11 mode and compilation has
progressed past that step. Full build/package/runtime validation remains
pending. A GitHub-hosted run failed at Wine prefix creation; the script now
addresses that ownership mismatch, but a successful hosted rerun remains
unverified. On this Apple Silicon host, Wine
failed under container CPU emulation, so local validation uses a full x86
Linux VM. This is a local testing requirement, not a requirement for native
x86_64 Linux CI runners.
