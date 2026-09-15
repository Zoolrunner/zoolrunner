# Experimental C++ ABI runtime for Mac OS X 10.0

The matching GCC 4.0.1 support sources can be built for the original 10.0 library
overlay. A prototype executable passed five test categories on original 4K78
under QEMU mac99/G3 emulation: allocation/RTTI, array allocation, exception
unwinding with destructor cleanup, retry after a throwing static initializer,
and shared initialization by eight threads, including nested initialization.
The final checked-in build scripts, two-mutex helper and unwind bridge were
then rebuilt in a fresh Linux container and passed the same target-OS checks.
The executable printed `C++ runtime: checks=5 attempts=2 nested=1 live=0` and
exited 0. The Foundation and coalescing probes were also rebuilt and rerun
successfully. Local evidence is in `artifacts/powerpc-10.0-probes/`, including
`final-probes-run.png` with all three exit statuses.

This builds **libsupc++**, the C++ ABI support library, rather than all of
libstdc++. Standard-library integration, application builds, physical hardware,
and additional ABI cases remain outstanding. The archive is experimental and
is not yet used by a 10.0 application profile.

## Build

Use the pinned 32-bit Linux PowerPC toolchain. The source input is the matching
`gcc-5247.tar.gz`, SHA-256
`38b4f890513dff0cc97119ae80c92e5252d0e2a94bb240d6961f7ba166c1abc7`.
The GCC 4 headers come from the checksum-pinned 10.4u SDK's compiler directory;
OS headers and libraries come from the experimental early overlay described in
[10.0-status.md](10.0-status.md). The builder copies and configures the compiler
headers in its private output directory.

```sh
export ZR_MACOS_SDK=/sdks/experimental-10.0-overlay
export ZR_PPC_DEPLOYMENT_TARGET=10.0
export ZR_PPC_EARLY_LINKER=/tools/early-ppc/ld
sh build/macosx/prepare-ppc-startup.sh
sh build/macosx/build-early-cxx-runtime.sh /downloads/gcc-5247.tar.gz \
  /sdks/MacOSX10.4u.sdk/usr/include/c++/4.0.0 /tools/early-cxx
sh build/macosx/tests/link-early-runtime.sh /tools/early-cxx /build/runtime-probe
```

The build uses make, the original libsupc++ sources and the upstream demangler.
The test links GCC's static exception-unwind and arithmetic support archives
normally, allowing the linker to select needed members. Do not apply
`-all_load` to the entire prebuilt libgcc archive: it also contains newer-OS
and long-double support outside this target's 64-bit-long-double ABI.

## Compatibility changes and checks

The private compiler configuration disables declarations for unavailable C99
C-library features. It retains C++ exceptions and thread-safe static
initialization. Early Darwin lacks recursive pthread mutex attributes, so the
gthread adaptation uses a normal ownership mutex plus a separate mutex for
recursion bookkeeping. Locking introduces no condition-wait cancellation point.

PowerPC guard publication uses explicit acquire/release `sync` barriers.
Inspection of the pinned compiler's optimized output confirms that it calls
`__cxa_guard_acquire` on the ready path too. Physical SMP testing remains
necessary; a single-CPU emulator does not validate all hardware memory ordering.

The recursive-mutex helper passes host ThreadSanitizer tests for contention,
recursion, ownership errors, depth overflow and cancellation. On a native Mac:

```sh
xcrun clang++ -std=gnu++98 -g -fsanitize=thread -isysroot /path/to/MacOSX11.3.sdk \
  build/macosx/tests/recursive-mutex.cc -o /tmp/zool-recursive-mutex-test
/tmp/zool-recursive-mutex-test
```

The target runtime test uses original 10.0 Csu with GCC's pre-10.2 exception-frame
registration bridge. Its only OS library dependency is libSystem 50.0.0.
Successful bounded tests do not establish complete C++ standard-library or
ZoolRunner application compatibility.

## Next standard-library constraints

An exploratory compile of the matching libstdc++ sources with the private
compiler headers reaches further configuration mismatches: the early OS headers
lack `wchar.h` and `wctype.h`, while the Tiger compiler configuration enables
them. Its C++ `<cctype>` wrapper also removes the early C classification macros
without finding function declarations. The original libSystem exports the
ordinary `isalnum` and `isalpha` functions, so that declaration problem must be
distinguished from genuinely unavailable OS functions. The inspected original
library does not export `mbrtowc`, `wcslen` or `iswalpha`.

`build/macosx/build-early-stdlib.sh` now builds an explicit early-Darwin
configuration from the same source archive and compiler headers, using the
same three arguments as the ABI builder. It supplies `libzoolcxx.dylib` and a
private GCC support archive whose assertion helper uses the old printf ABI.
It preserves C++ exceptions, RTTI and thread-safe initialization. A shared-library
probe ran on original 10.0 and passed strings, vectors, input/output streams,
character classification and exceptions crossing the library boundary. The
checked-in builder subsequently passed a clean build against the reproducible
Linux-prepared SDK, and its library passed that runtime probe too.

This configuration explicitly omits GCC's wide-character stream interfaces,
which depend on C-library facilities absent from the original OS. It is not a
claim of complete C++ standard-library support. ZoolRunner's Unicode strings,
JavaScript, DOM and application-facing text APIs do not use those interfaces
and remain enabled. Do not reuse Tiger's feature configuration unchanged or
silently remove application-facing Unicode support. Full application validation
remains necessary.
