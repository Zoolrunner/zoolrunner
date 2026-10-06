# Building for Windows

Build Windows x86 applications from Linux or macOS using **MSVC 2005 through
Wine**. CrossOver is supported on macOS. Host utilities use the native compiler;
target code uses Microsoft's compiler and the static CRT.

Run commands from the repository root. Choose `suite`, `browser`, `calendar`
or `xulrunner`.

## Linux container build

The supplied image installs host dependencies, checksum-pinned MSVC tools,
Microsoft MIDL and Wine:

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

Use a new work directory for each build. Replace `suite` with the desired
application and adjust `ZR_BUILD_JOBS` for parallelism. The driver copies source,
builds, packages, audits imports and runs the CI checks. ZIP files appear in
`/tmp/zoolrunner-windows-suite/build/artifacts`; logs are in `build/logs`.

The [Windows workflow](../../../.github/workflows/windows.yml) uses this image.
On Apple Silicon, use an x86 Linux VM if Wine fails under container CPU emulation.

## Using an existing toolchain

Install native build tools (C/C++ compiler, make, autoconf, Perl, pkg-config,
GLib development files, Python 3, flex, bison and archive utilities) and Wine
or CrossOver. Bundled libIDL builds automatically.

Set `MSVC8_ROOT` to an extracted toolchain with this layout:

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
```

Suite also requires Microsoft's `midl.exe` and `midlc.exe` in `bin` or
`PlatformSDK/Bin`. The supplied container uses MIDL 6.00.0366 from the Windows
Server 2003 SP1 Platform SDK. No Visual Studio registration or `vcvars32.bat`
is needed; wrappers set tool paths, `INCLUDE` and `LIB`.

The Wine launcher is selected from `MSVC8_WINE`, then `WINE`, then `wine` on
`PATH`, then the standard macOS CrossOver path. For CrossOver, optionally set
`MSVC8_WINE_BOTTLE` (or `CX_BOTTLE`). Check the toolchain before building:

```sh
export MSVC8_ROOT=/path/to/msvc8.0
export WINE=wine
build/win32/msvc8-cross/test-toolchain.sh
```

For CrossOver, replace the Wine setting with:

```sh
export MSVC8_WINE=/Applications/CrossOver.app/Contents/SharedSupport/CrossOver/bin/wine
export MSVC8_WINE_BOTTLE=MyBottle
```

## Configure and build

With the toolchain environment above set:

```sh
export MOZCONFIG="$PWD/mozconfigs/cross/win32-msvc8-suite-legacy.mozconfig"
make -f client.mk build
```

Use `suite-legacy` for the aggregate Suite build targeting Windows 95/NT 4.0.
For other applications, select `win32-msvc8-browser.mozconfig`,
`win32-msvc8-calendar.mozconfig` or `win32-msvc8-xulrunner.mozconfig`.
Object directories are `obj-zoolrunner-win32-msvc8-PROFILE`, where Suite's
profile suffix is `suite-legacy`.

Keep the supplied static-CRT and process-heap settings. Run `client.mk` builds
sequentially in a checkout; they share `.mozconfig.mk`.

## Package

For the Suite build above, with the same toolchain environment:

```sh
zr_package_work=$(mktemp -d /tmp/zoolrunner-windows-package.XXXXXX)
mkdir -p "$zr_package_work/logs" "$zr_package_work/artifacts"
python3 build/win32/msvc8-cross/package-ci.py suite \
  obj-zoolrunner-win32-msvc8-suite-legacy "$zr_package_work"
build/win32/msvc8-cross/audit-pe.sh \
  "$zr_package_work/runtime" "$zr_package_work/pe-imports.tsv"
```

Use the matching application and object directory for other builds. The ZIP
is written to `$zr_package_work/artifacts`. Audit the staged runtime; set
`PE_OBJDUMP` if the host's default `objdump` cannot read PE files.

## Build troubleshooting

If compilation stops after `Building deps for ...`, inspect the adjacent
`.deps/*.pp.log` before diagnosing the target compiler. For manifest embedding
problems, `MSVC8_EMBED_MANIFEST=0` disables embedding for diagnosis only.

[Runtime tests](tests/README.md), [Windows compatibility](COMPATIBILITY.md) and
[implementation/workflow records](NOTES.md) are separate references.
