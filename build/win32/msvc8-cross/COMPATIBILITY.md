# Windows 95 and Windows NT 4 compatibility status

Windows 95 and Windows NT 4.0 are independent minimum operating-system targets.
The current NT 4 VM reports Service Pack 6; the SP6/SP6a distinction has not
been independently verified. Do not infer results for other installations. A successful build,
PE version 4.0, or execution under Wine does not establish compatibility with
either operating system.

Windows builds are hosted on Linux or macOS and use genuine Microsoft Visual
C++ 2005 (MSVC 8.0 / VC8) through Wine, including CrossOver on macOS. See the
[build guide](README.md). The results below concern the previously audited
packages; subsequent source changes require renewed validation.

## Suite VM regression results — 2026-09-16

The MSVC2005/Wine aggregate Suite package passed all 17 groups in the
[guest regression payload](tests/README.md) on each of these UTM/QEMU guests:

| Guest | Result |
| --- | --- |
| Windows NT Workstation 4.0 build 1381, reporting Service Pack 6 | 17 passed, 0 failed |
| Windows Me 4.90.3000 | 17 passed, 0 failed |
| Windows 2000 build 2195, Service Pack 4 | 17 passed, 0 failed |

Coverage includes 51 native Expat/JSAPI checks, eight JavaScript regressions,
inline script execution, packaged interfaces, browser chrome and local HTML
navigation, three network error pages, window builtins, Composer editing and
undo, repeated application teardown, Address Book, Inspector, Venkman and
ChatZilla. The lifecycle fixture passed 24 checks per guest. This is targeted
Suite coverage, not full Test262 conformance or a live mail-server test.

The run found and fixed missing typelibs and `embed-sample.jar` in the static
package, aggregate `xpcshell` initialization and static-CRT file handling,
and account-wizard callbacks resuming after their owner window closed.
The refreshed arm64 and x86_64 macOS Suite packages also passed shell,
lifecycle and GUI regressions using SDK 11.3.

Local logs, screenshots, package hashes and import audits are saved under
`artifacts/windows-suite-validation/`; the runnable archive is
`artifacts/zoolrunner-win32-msvc8-suite-regression.zip`. Windows 95 was not
tested, and these results do not establish support for earlier NT4 service
packs. Keep the minimum Windows targets and MSVC2005/Wine host build policy
unchanged.

## Verified on the cross-build host

The release XULRunner and full Suite builds made with Visual C++ 8 and
`--enable-static-rtl` have been inspected.  For the aggregate packaged Suite:

* all 30 shipped EXE and DLL files are x86 PE32 images;
* all have PE operating-system and subsystem versions no later than 4.0;
* none imports MSVCR80.dll, MSVCP80.dll, MSVCR80D.dll, or MSVCP80D.dll;
* none contains a Microsoft.VC80.CRT deployment reference;
* the build logs contain /MT, with no target /MD or /MDd invocation;
* all non-system DLL imports resolve within the package;
* 30 images import `TlsAlloc`, below the 64-slot Win95/NT4 process limit;
* the unpacked packaged Suite remains running when launched under CrossOver.
* the aggregate preserves per-module lazy initialization rather than running
  every component module constructor on the first component request.

An earlier `/MT` package shipped 113 PE images, including 76 individual XPCOM
component DLLs.  Almost every image embedded a CRT that allocated a TLS slot,
so the Windows 95/NT 4 loader began rejecting otherwise valid components after
the process exhausted its 64 slots.  The historical `mozcomps.dll` facility
now aggregates ordinary XPCOM components while preserving required DLL
boundaries.  The post-build auditor enforces the TLS-image budget so this
failure cannot silently return.

The Suite's cross-generated LDAP `.def` files must name the actual shipped
DLLs (`nsldap32v50.dll` and `nsldappr32v50.dll`).  An earlier package instead
stamped `NSLIB.dll` into its LDAP import libraries, leaving two packaged DLLs
with an unsatisfied dependency.  The build now generates the correct names,
and `audit-pe.sh` rejects any non-system DLL import missing from the package.

audit-pe.sh produces the per-file import inventory. Every imported system API
must still be classified separately for Windows 95 and Windows NT 4 SP6a.

## Remaining Windows 95 import review

Visual C++ 2005's static libcmt.lib itself introduces these imports even in
minimal /MT binaries:

* GetEnvironmentStringsW
* FreeEnvironmentStringsW
* GetStringTypeW
* LCMapStringW

The symbol inventory in libcmt.lib confirms that these come from CRT helpers,
rather than Mozilla or a bundled library. Microsoft also documents that Visual
Studio 2005 removed CRT support for Windows 95, Windows 98, Windows Me, and
Windows NT 4:

<https://learn.microsoft.com/cpp/porting/visual-cpp-change-history-2003-2015>

The packaged Suite also statically imports `IsDebuggerPresent` and
`TryEnterCriticalSection`, among other APIs needing platform-by-platform
review. The current binaries are **not yet claimed compatible with Windows
95**, despite their correct PE 4.0 metadata. Windows 95 import blockers must
be resolved at the originating object/import level without reintroducing the
VC80 DLL runtime, then verified on that operating system. NT4, Me and 2000
guest results are recorded separately below.

Microsoft's Layer for Unicode is an optional Windows 9x update, not part of
Windows 95 itself. It must not be silently assumed:

<https://learn.microsoft.com/archive/msdn-magazine/2001/october/mslu-develop-unicode-applications-for-windows-9x-platforms-with-the-microsoft-layer-for-unicode>

## Required validation matrix

| Result | Windows 95 | Windows NT 4 SP6a |
| --- | --- | --- |
| Works | Present and usable as required | Present in SP6a and usable |
| Optional update | Name the exact update | Name the exact update |
| Requires investigation | Availability or behavior is unproven | Availability or behavior is unproven |
| Incompatible | Prevents loading or operation | Prevents loading or operation |

Windows 95 RTM, OSR2, and Windows 95 with official Microsoft updates must be
tracked separately. Neither Windows 95 nor Windows NT 4 support may be declared
until the finished application has executed on that operating system.
