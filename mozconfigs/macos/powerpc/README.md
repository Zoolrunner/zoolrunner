# Experimental PowerPC Mac OS X builds

The required deployment goal is Mac OS X **10.0 onward** on PowerPC.
See [10.0-status.md](10.0-status.md) for the early-target probes and remaining
compiler, runtime and API constraints.

These profiles currently reproduce the intermediate SDK 10.3.9 port. They are **not yet
validated deployment builds**. The Linux cross-build has produced and packaged
the Suite with the current source fixes. All eight target executables were
relinked using original Panther startup code, and all 110 packaged Mach-O
binaries passed the SDK dependency-location audit. Target ABI assertions also
passed. This was an incremental build; fresh-container application builds and
target execution remain outstanding. Full applications have not run on PowerPC;
bounded 10.0 compiler/runtime probes now pass under emulation.
SDK 10.3.9 does not establish
support for earlier Panther releases, Jaguar (10.2), Puma (10.1), or
Cheetah (10.0). The minimum has not simply been relabeled: the older SDK,
C/C++ runtime, startup code and API fallbacks are still being investigated.

Use a disposable 32-bit Linux container, including when the physical host is
a modern Mac. The tested environment is Debian bookworm-slim, linux/386, with
build-essential, autoconf, libglib2.0-dev, pkg-config, python3, perl, zip, unzip,
bzip2, flex, bison, curl, ca-certificates, xz-utils and rsync installed. An x86_64
Linux host can execute these 32-bit tools directly; Apple Silicon needs Docker's
x86 emulation. The container should have a writable checkout and SDK directory.

Run `build/macosx/fetch-ppc-toolchain.sh` once in a fresh container. The script
installs the checksum-pinned Apple GCC 4.0.1 build 5247 and odcctools 20090808
distribution under its compiled-in `/opt/mac` prefix. The
[original distribution](https://sourceforge.net/projects/freeverb3-vst/files/dev/mac-cross-gcc-2009-12-01_gcc-5247%2Bodcctools-698.1od9/)
also provides the matching compiler and tool sources; retain their original
licenses when redistributing the tools. No toolchain binaries are vendored.

```sh
sh build/macosx/fetch-sdk.sh /sdks 10.3.9
export ZR_MACOS_SDK=/sdks/MacOSX10.3.9.sdk
sh build/macosx/prepare-ppc-startup.sh
export MOZCONFIG="$PWD/mozconfigs/macos/powerpc/cocoa_suite_gcc.mozconfig"
make -f client.mk build
```

Profiles also exist for Browser, Calendar and XULRunner for subsequent matrix
validation. `ZR_OBJDIR` and `ZR_BUILD_JOBS` override the output directory and
parallelism. Host tools remain native Linux programs. The target is G3,
without AltiVec, with 64-bit long double and the Panther system C++ runtime.
Wrappers explicitly select Panther libraries: the compiler's defaults can
otherwise introduce Tiger libraries despite selecting the older SDK.
The archived Panther SDK also omits `crt1.o`; the preparation script rebuilds
it from Apple's matching Panther Csu sources and records `/usr/lib/dyld` with
the classic linker. Its temporary dynamic-loader link stub is not shipped.

The Cairo library now also compiles against Panther. Bitmap snapshots copy
their pixels, and image/gradient strokes use Cairo's software fallback where
the native stroke-to-path API is unavailable. The package audit covers plug-ins
outside the main runtime directory as well. The old linker encodes no minimum-OS
metadata, and the static audit does not validate symbol binding or runtime APIs.

The gradient regression test now passes 500 strokes with both native Quartz
and the forced pre-Tiger software fallback on an arm64 host. It catches the
incorrect solid-source classification that previously rendered gradients
black. Run it against an existing native Cocoa build with:

```sh
sh build/macosx/tests/check-quartz-strokes.sh /path/to/native-objdir /path/to/MacOSX11.3.sdk
```

The test builds a temporary Cairo archive without modifying the developer's
object tree. It is a pixel regression check, not PowerPC runtime validation.

The existing make build uses verified classic resources from
`config/macos/resources` on Linux. Compiling and linking does not establish
correct rendering, printing, AppleScript, plug-in behavior or OS compatibility.
The separate [i386 notes](../i386/README.md) track completed Tiger builds and
the required target-Mac runtime checks.
