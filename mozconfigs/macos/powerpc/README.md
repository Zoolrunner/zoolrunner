# Building for PowerPC Mac OS X

Use the supplied **Linux/386 Apple GCC cross-toolchain container**. It builds
Suite, Browser, Calendar and XULRunner for PowerPC G3 and Mac OS X 10.0.
An ARM host needs x86 emulation to run the compiler container.

## Mac OS X 10.0

Run from the repository root:

```sh
docker build --platform linux/386 -f build/macosx/powerpc.Dockerfile \
  -t zoolrunner-powerpc-toolchain build/macosx
mkdir -p /tmp/zoolrunner-powerpc-suite
docker run --rm --platform linux/386 \
  --mount "type=bind,source=$PWD,target=/source,readonly" \
  --mount "type=bind,source=/tmp/zoolrunner-powerpc-suite,target=/work" \
  -e ZR_BUILD_JOBS=3 zoolrunner-powerpc-toolchain \
  sh /source/build/macosx/build-10.0-ci.sh suite /work/build
```

Replace `suite` with `browser`, `calendar` or `xulrunner`. Use a new work
directory for each build; the driver refuses to overwrite an existing source
copy. Adjust `ZR_BUILD_JOBS` for parallelism.

The driver fetches pinned inputs, prepares the SDK, linker, startup objects and
private C++ runtime, then builds and packages the application. The adapted SDK
combines **10.1.5 headers with original Mac OS X 10.0 (4K78) libraries**.
The profiles are `cocoa_APP_10.0_gcc.mozconfig`.

Archives appear in `/tmp/zoolrunner-powerpc-suite/build/artifacts`, build logs
in `build/logs`, and the object directory in `build/obj-suite`. The driver also
prepares runtime-test media; guest execution is a separate workflow step.
Preserve archives and logs before removing completed temporary workspaces.

The [PowerPC workflow](../../../.github/workflows/macos-powerpc.yml) uses this
build procedure. SDK provenance, runtime constraints and results are in the
[10.0 deployment record](10.0-status.md).

## Intermediate Mac OS X 10.3.9 profiles

Profiles without `_10.0` select the separate Panther toolchain path. To build
one, start a fresh container from the same image with a writable checkout and
SDK directory, then run from the checkout root:

```sh
sh build/macosx/fetch-sdk.sh /sdks 10.3.9
export ZR_MACOS_SDK=/sdks/MacOSX10.3.9.sdk
sh build/macosx/prepare-ppc-startup.sh
export MOZCONFIG="$PWD/mozconfigs/macos/powerpc/cocoa_suite_gcc.mozconfig"
make -f client.mk build
python3 build/macosx/package-ci.py powerpc suite
```

Set `ZR_OBJDIR` to an absolute path or `ZR_BUILD_JOBS` to override the profile's
output directory or parallelism. Host tools remain native Linux programs.
The wrappers select Panther runtime libraries explicitly; the compiler's
built-in defaults point to Tiger. Keep the supplied wrappers and startup setup.

[Historical platform notes](NOTES.md) and [interactive guest setup](utm.md)
are separate from the build procedure.
