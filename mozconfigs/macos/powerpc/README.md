# PowerPC Mac OS X builds

The required deployment goal is Mac OS X **10.0 onward** on PowerPC.
The `cocoa_*_10.0_gcc.mozconfig` profiles build Suite, Browser, Calendar and
XULRunner from Linux. All four pass fresh local `act` builds, package audits,
original Mac OS X 10.0 runtime/GUI tests under PowerPC emulation, and artifact
uploads. See [10.0-status.md](10.0-status.md) for the reproducible workflow,
runtime probes and API constraints. GitHub-hosted and physical hardware runs
remain untested.

The profiles without `_10.0` reproduce the intermediate SDK 10.3.9 port. These
Panther profiles are **not validated deployment builds**. Their Linux cross-build
produced and packaged the Suite. All eight target executables were
relinked using original Panther startup code, and all 110 packaged Mach-O
binaries passed the SDK dependency-location audit. Target ABI assertions also
passed. This was an incremental Panther build; its fresh-container application
builds and Panther runtime execution remain outstanding. SDK 10.3.9 does not
establish support for earlier Panther releases, Jaguar (10.2), Puma (10.1), or
Cheetah (10.0). The separate 10.0 profiles use adapted headers, original 10.0
libraries, a private C++ runtime, rebuilt startup code and tested API fallbacks.

The following commands describe the intermediate Panther profiles. For the
validated 10.0 path, use the workflow linked in the deployment-status document.

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
