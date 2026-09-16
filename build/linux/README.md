# Linux x86 builds

The i686 and x86_64 bring-up uses **Oracle Linux 8** (`oraclelinux:8`) and
**GCC Toolset 14** for both local builds and CI. The amd64 container includes
Oracle's multilib development packages for i686. Host utilities remain
x86_64; the i686 target compiler uses `-m32 -march=i686` and target pkg-config
metadata from `/usr/lib/pkgconfig`.

Both architectures have GTK2 and Xlib mozconfigs for Suite, Browser, Calendar and
XULRunner. These are bring-up configurations: full build and runtime
validation is in progress. The additional LoongArch Calendar and Xlib
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

The Linux workflow has sixteen jobs (four applications × two x86 targets × two toolkits).
Packages dereference build-tree symlinks and omit host utilities. Validation
checks ELF class/machine, dynamic dependencies, generated SpiderMonkey ABI
metadata, JavaScript regressions, native JSAPI/Expat probes, and GTK2/Xlib windows
under Xvfb. Suite also exercises application lifecycle and ChatZilla fixtures.

The relocated-package runner sets both `LD_LIBRARY_PATH` and
`MOZILLA_FIVE_HOME` to the extracted runtime. Unix XPCOM uses the latter to
find its components; without it, launching `xpcshell` from the source checkout
fails with `failed to get nsJSRuntimeService!` even when all ELF dependencies
resolve. The native Expat probe also needs the target build's NSPR headers.

Local revalidation on 2026-09-16 used an existing x86_64 GTK2 Suite archive
in the Oracle Linux 8 container on Apple Silicon. It reproduced the component
lookup failure without `MOZILLA_FIVE_HOME`. With the runner fixes, shell
regressions, native JSAPI and Expat probes, application navigation and window
bootstrap passed. The Suite lifecycle test subsequently exited with code 11
during Venkman startup. An isolated rebuild identified two GCC optimization
assumptions that conflict with the classic implementation. The x86 profiles
now use `-flifetime-dse=1` to preserve arena zeroing before frame constructors
(including the empty frames used by HTML `wbr`), and `-fno-strict-aliasing` to
preserve `nsCOMPtr` typed output-pointer writes. Without the latter, interface
enumeration appended null entries and window globals lacked `NodeFilter`.
With both settings, the isolated x86_64 GTK2 Suite package passes all packaged
checks, including all 24 lifecycle checks and ChatZilla. This is not yet a
fresh complete matrix or a GitHub-hosted run.

The first fresh x86_64 Xlib Suite `act` run compiled, passed the target ABI
check, and packaged successfully, but timed out during lifecycle testing.
The Account Wizard opened by Address Book exposed an Xlib queue-dispatch bug:
the Xt callback watched the nested queue's descriptor but drained the shell's
original queue. It now dispatches the subscribed queue, and input IDs retain
their `XtInputId` width during removal on LP64. An isolated rebuild passes all
packaged Xlib Suite checks, including the modal startup path, all 24 lifecycle
checks, and ChatZilla. The runtime runner retains partial subprocess logs on
timeout. Full fresh matrix validation remains in progress.

XULRunner GUI tests select the fixture with `toolkit.defaultChromeURI` in the
disposable profile. Its default command-line handler does not implement the
Browser `-chrome` option. The fixture still opens the unchanged Simple app
and verifies its XPT, JavaScript and native C++ components.

For local artifact uploads, use an
`act` build with the v7 artifact-server compatibility fix described in the
[macOS guide](../../mozconfigs/macos/README.md#act-artifact-server-limitation);
stock act 0.2.87 rejects the production upload action's `mime_type` field.

Bring-up has exposed and addressed host/target libIDL metadata selection,
the group-box paint override's i686 interface calling convention,
missing multilib development packages, Perl 5.26 literal-brace handling in
LDAP header generation, SQLite and GDK shared-library header visibility,
missing calling-convention annotations in interface overrides, and a
host-word-size leak in SpiderMonkey's i686 CPU header generator. The generator
now passes size/alignment checks against actual GCC 32-bit and 64-bit types.
Full application validation is still pending; these fixes are not a claim
that all sixteen jobs have passed.

The Xlib app shell's `Run()` override uses `NS_IMETHOD` / `NS_IMETHODIMP`
to preserve the interface calling convention on i686. On 2026-09-16, a
targeted rebuild in the existing i686 Xlib Suite build volume with Oracle
Linux 8 and GCC Toolset 14 reproduced the original conflicting-type-attributes
error and compiled the corrected app-shell, widget and window objects as
32-bit ELF. The widget-library link was blocked by a missing
`libxpwidgets_s.a` in that volume; this check does not establish a complete
application build, package or runtime result.
