# Building on Linux

Run commands from the repository root. Profiles are available for `suite`,
`browser`, `calendar` and `xulrunner`, using either `gtk2` or `xlib`.

## x86 and x86_64

Use Docker with the supplied Oracle Linux 8 / GCC Toolset 14 image. It includes
native x86_64 host tools and the i686 multilib dependencies.

```sh
docker build --platform linux/amd64 -f build/linux/oraclelinux8.Dockerfile \
  -t zoolrunner-oraclelinux8-gcc14 build/linux
docker volume create zoolrunner-linux-x86_64-suite-gtk2
docker run --rm --platform linux/amd64 \
  --mount "type=bind,source=$PWD,target=/source,readonly" \
  --mount type=volume,source=zoolrunner-linux-x86_64-suite-gtk2,target=/work \
  zoolrunner-oraclelinux8-gcc14 sh /source/build/linux/build-ci.sh x86_64 suite gtk2
```

Replace `x86_64` with `i686`, `suite` with the application, and `gtk2` with `xlib`
as needed. Use a separate work volume for each combination. The driver copies
source into `/work/source`, builds, packages and runs the CI checks. Build logs
are in `/work/logs`, staged packages in `/work/package` and archives in
`/work/artifacts` inside the volume.

The default is three compiler jobs; pass `-e ZR_BUILD_JOBS=8` to `docker run`
before the image name to change it. An ARM host needs x86 emulation for the
container and i386 execution support for 32-bit package checks.

## ARM64

Use the native ARM64 Oracle Linux 8 / GCC Toolset 14 image:

```sh
docker build --platform linux/arm64 -f build/linux/oraclelinux8-aarch64.Dockerfile \
  -t zoolrunner-oraclelinux8-gcc14-aarch64 build/linux
docker volume create zoolrunner-linux-aarch64-suite-gtk2
docker run --rm --platform linux/arm64 \
  --mount "type=bind,source=$PWD,target=/source,readonly" \
  --mount type=volume,source=zoolrunner-linux-aarch64-suite-gtk2,target=/work \
  zoolrunner-oraclelinux8-gcc14-aarch64 \
  sh /source/build/linux/build-ci.sh aarch64 suite gtk2
```

Application/toolkit selection, job count and output locations are the same as
above. These profiles target little-endian AArch64 LP64.

## LoongArch64

Build natively with GCC, make, autoconf, Perl, pkg-config and GLib development
files, plus GTK2 development files for GTK2 or X11/Xt development files for
Xlib. The profiles build bundled libIDL automatically.

```sh
export MOZCONFIG="$PWD/mozconfigs/linux/loongarch64/gtk2_suite_gcc.mozconfig"
make -f client.mk build
```

Choose another application or replace `gtk2` with `xlib` in the profile name.
The Suite profiles produce `obj-zoolrunner-suite` and
`obj-zoolrunner-suite-xlib`; other profiles specify their own `MOZ_OBJDIR`.
Keep the profiles' compiler compatibility flags.

To package a native Suite build:

```sh
zr_package_work=$(mktemp -d /tmp/zoolrunner-linux-package.XXXXXX)
mkdir -p "$zr_package_work/logs" "$zr_package_work/artifacts"
ln -s "$PWD" "$zr_package_work/source"
python3 build/linux/package-ci.py loongarch64 suite "$zr_package_work" \
  --objdir "$PWD/obj-zoolrunner-suite"
```

For Xlib, add `--toolkit xlib` and use the matching object directory. The
archive is written to `$zr_package_work/artifacts`.

## Incremental builds

Within a configured container or native build environment, use an absolute
`MOZCONFIG` path and `make -f client.mk build`. Run `client.mk` configurations
sequentially in a checkout; they share `.mozconfig.mk`. Reconfigure after
changing compiler options. Object directories for the container profiles are
`obj-zoolrunner-linux-ARCH-APP`, with `-xlib` appended for Xlib.

[Runtime checks, platform details and recorded results](NOTES.md) are maintained
separately from these build instructions.
