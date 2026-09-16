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
during Venkman startup. This was not a fresh build or a GitHub-hosted matrix
run; i686, Xlib and the other application packages remain unverified by this
recheck.

Bring-up has exposed and addressed host/target libIDL metadata selection,
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
