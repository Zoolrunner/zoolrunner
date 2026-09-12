# MSVC 8 cross-build support

This directory lets the existing Mozilla configure/make build invoke the
32-bit Microsoft Visual C++ 8.0 tools through either Wine on Unix or CrossOver
on macOS.  The wrappers are shared by both hosts; only runtime discovery is
host-sensitive.

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
