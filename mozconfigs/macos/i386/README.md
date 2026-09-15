# Experimental i386 builds from a modern Mac

This target uses Xcode 16.4's Clang and classic i386 linker on an arm64 or
x86_64 macOS host. The target SDK is **10.4u**; native build utilities use
**11.3**. The existing arm64/x86_64 CI profiles continue using SDK 11.3.

```sh
sh build/macosx/fetch-sdk.sh "$HOME/dev/macos-sdk" 10.4u
sh build/macosx/fetch-sdk.sh "$HOME/dev/macos-sdk" 11.3
export MOZCONFIG="$PWD/mozconfigs/macos/i386/cocoa_suite_clang.mozconfig"
make -f client.mk build
python3 build/macosx/package-ci.py i386 suite
```

`ZR_MACOS_SDK` overrides the target SDK path and `ZR_HOST_SDK` the host SDK.
Browser, Calendar, and XULRunner profiles use the corresponding application
names. The SDK download checks a pinned SHA-256. No installed Xcode SDK is
modified. The explicit deployment target is **10.4**, covering the first Intel
Mac OS X release (10.4.4). These archives are unsigned development builds.

The wrappers use the SDK's `libgcc_s.10.4` and `libstdc++.6` rather than current
Clang's default compiler runtime. The SDK includes Tiger's startup object, and
the binaries use `LC_UNIXTHREAD` instead of the 10.8 `LC_MAIN` entry mechanism.
Earlier SDK 10.6 experiments targeted 10.8 because that SDK archive omitted
its matching startup object; those archives are a separate historical baseline.

A build on current macOS cannot establish runtime compatibility. Apple states
that [Mojave 10.14 was the last release supporting 32-bit applications](https://support.apple.com/en-ie/103076).
Test startup, chrome, JavaScript, plug-ins and printing on a compatible Intel
Mac or an appropriate macOS virtual machine before claiming runtime support.
The packaging check compiles target ABI assertions without executing target
code on the build host.

## PowerPC research

The installed Apple Clang 17 rejects `-arch ppc`; SDK 11.3 is also unsuitable
for a PowerPC target. This is a separate toolchain setup, not another value of
the modern Clang architecture flag.

A plausible route is a Darwin PowerPC cross-compiler with C, C++, Objective-C
and Objective-C++ support, a PPC-capable Mach-O assembler/linker, and SDK 10.4u
or 10.5. [Iain Sandoe's GCC 13 Darwin branch](https://github.com/iains/gcc-13-branch)
lists PowerPC and arm64 Darwin support; [darwin-xtools](https://github.com/iains/darwin-xtools)
provides legacy Darwin binary-tool work. A cross-build of those tools must be
validated on the chosen modern host before using it for ZoolRunner.

[XcodeLegacy](https://github.com/devernay/xcodelegacy) documents extracting
legacy compilers and SDKs from older Apple Xcode downloads. Its installation
procedure modifies Xcode; this experiment does not install it. Extracted tools
also need host-architecture checks before assuming they run on Apple Silicon.
[OSXCross](https://github.com/tpoechtrager/osxcross) targets Linux/BSD hosts and
keeps an older `ppc-test` branch; it is an alternative inside a VM rather than
a verified native macOS solution here.

No PowerPC ZoolRunner binary or PowerPC CI job has been validated in this work.

## CI and local workflow testing

The separate `.github/workflows/macos-i386.yml` workflow builds Suite, Browser,
Calendar and XULRunner on `macos-15` using Xcode 16.4. It downloads and verifies
both SDKs, builds through `client.mk`, and packages unsigned development
archives. i386 packaging checks every Mach-O file's architecture, removes the
host-only `nsinstall` utility, checks generated ABI sizes/alignments, and sets
the desktop app bundle's minimum OS metadata to 10.4. Runtime tests are not
executed on these modern hosts.

```sh
act push -W .github/workflows/macos-i386.yml \
  -P macos-15=-self-hosted --concurrent-jobs 1 \
  --matrix app:suite \
  --env "DEVELOPER_DIR=$(xcode-select -p)" \
  --artifact-server-addr 127.0.0.1 \
  --artifact-server-path /tmp/zool-act-i386-artifacts
```

Remove the matrix filter to build all four applications. act's native runner
uses your Mac's tools rather than reproducing GitHub's hosted image. act
0.2.87's artifact server rejects the `mime_type` field from upload-artifact v7;
see the [documented local upload limitation](../README.md#act-artifact-server-limitation).
Do not call a build/package pass a successful end-to-end upload or hosted run.

Local validation: clean Tiger Suite, Browser, Calendar and XULRunner `act`
builds and packaging passed, including target ABI assertions. The complete
package deployment audits cover 110 Suite, 83 Browser, 65 Calendar and 37 XULRunner Mach-O
binaries, including print extensions outside the runtime directory.
Runtime compatibility remains untested.
The two unmodified bundled Java plug-ins lack OS-version metadata; the audit
verifies their hashes against the in-tree copies and reports their minimum as
unknown rather than assigning them a tested minimum.

Before lowering the target, Suite, Browser and Calendar built and packaged
through `act` with SDK 10.6/minimum 10.8. XULRunner also built and packaged after
fixing missing OpenGL/AGL link flags in libxul, using the same local workspace.
The arm64 Suite regression build passed packaging and 182 compatibility
assertions. Local uploads hit the `act` server limitation described above.

## PowerPC deployment work in progress

The PowerPC goal is Mac OS X 10.0 onward. The Panther build is an intermediate
milestone; see [the PowerPC notes](../powerpc/README.md).
The GCC 13 Darwin branch and recent Linux Xtools document a 10.5 minimum and
therefore do not establish the requested older compatibility. An
[older Apple GCC 4.0.1/odcctools Linux cross-toolchain](https://sourceforge.net/projects/freeverb3-vst/files/dev/mac-cross-gcc-2009-12-01_gcc-5247%2Bodcctools-698.1od9/)
now runs in a 32-bit Linux container on the modern Mac and has compiled and
linked C, C++ and Cocoa Objective-C++ probes using SDK 10.3.9.

Its default library path still points at Tiger. It requires explicit Panther
library paths, the compiler's private headers and explicit Panther runtime
libraries to keep Tiger dependencies out. Using `-static-libgcc` alone is
insufficient: its bundled archive contains code referencing Tiger-only
long-double stubs, exposed when linking NSPR with `-all_load`. The full platform build is
under investigation. No PowerPC application or target-OS runtime test has
passed. Supporting 10.2 or pre-10.3.9 C++ may require an earlier compiler/runtime
and additional platform work.

Local `act` retains separate source trees, SDKs and object directories for
jobs. Check free space first; save each job's archives and logs and remove its
temporary workspace before starting the next job. Keep existing developer
builds and shared SDK installations intact.

## Portability fixes exercised by the build

The host-built JavaScript configuration generator now selects target word
sizes explicitly, rather than copying the arm64 host ABI into i386 headers.
NSS uses Clang's compiler headers and the same diagnostic policy as the modern
Mac builds. The 32-bit Cocoa backend links its Carbon APIs and retains its
cursor/AppleScript resources. The default plug-in builds from its existing
sources with make, avoiding the obsolete Xcode project format.

Cairo printing uses a Core Graphics context on both widths. Cocoa targets of
10.6 and later select the native print panel used by the 64-bit port; older
targets retain the Carbon print-dialog extension. This avoids importing an
extension API removed from SDK 10.6. Printing and old-system runtime behavior
remain unverified.

The Panther port uses the older Core Graphics print-session interfaces and a
pixel-copy fallback for bitmap snapshots. The snapshot helper passed an
ASan/UBSan pixel-lifetime test on the modern host; Panther execution remains
untested. SQLite selects ordinary POSIX file locking on pre-Tiger targets,
which lack the UUID interface expected by Apple's proxy locking implementation.
Linux resource builds use source-hash-verified copies generated with Apple
Rez/SDP; see `config/macos/resources/README.md` at the repository root.

## Runtime validation before claiming support

Move the archive to a clean target Mac and extract it away from the source
checkout. Record its checksum, the Mac model/CPU, exact OS build and installed
updates. Test the oldest supported OS first, then later releases; a pass on
Leopard does not establish a Tiger pass.

The i386 archive includes a test script and the three JavaScript smoke tests.
From the extracted archive directory, run (adjust the app name as needed):

```sh
sh runtime-tests/run.sh './ZoolRunner.app/Contents/MacOS'
# XULRunner archive:
sh runtime-tests/run.sh ./xulrunner
```

The script needs no Python or developer tools. It records system information
and each test's output in a new `zoolrunner-runtime-checks-*` directory, with
eager dynamic symbol binding enabled. The script itself passed all 164 checks
using a packaged arm64 build; that is not i386 or PowerPC runtime validation.

* Launch with a fresh profile, quit, and relaunch the same profile.
* Exercise menus, preferences, typing/selection/clipboard, file dialogs,
  downloads, local HTML/CSS, JavaScript and printing (including the Carbon PDE).
* In Suite, exercise MailNews, Composer, Address Book and ChatZilla.
* In Calendar, create/edit/save events, restart and verify persistence.
* In XULRunner, launch the included simple/layoutdebug applications and an
  unmodified historical external XUL application.
* Run the repository's `js/tests/es5/object-reflection.js`,
  `legacy-application.js` and `debugger-lifecycle.js` with the packaged
  `xpcshell -f` and retain their results. These are compatibility smoke tests,
  not the entire ECMAScript conformance suite.

The static report catches architecture, missing bundled/SDK libraries,
nonportable link paths and newer load-command requirements. It cannot prove
Objective-C selector availability, runtime-loaded dependencies, CPU execution,
GUI behavior or old-system compatibility. Record these as unverified until
actual target testing passes.
