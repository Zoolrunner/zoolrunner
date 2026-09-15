#!/bin/sh
# Install only inside the disposable Linux build container: upstream tools
# contain /opt/mac paths and are not a relocatable host installation.
set -eu
if test "`uname -s`" != Linux || test "`getconf LONG_BIT`" != 32; then
  echo "This toolchain requires a disposable 32-bit Linux environment" >&2
  exit 1
fi
if test -e /opt/mac; then
  echo "Refusing to overwrite /opt/mac; use a fresh build container" >&2
  exit 1
fi
zr_download=`mktemp -d`
trap 'rm -rf "$zr_download"' EXIT HUP INT TERM
zr_archive="$zr_download/toolchain.tar.bz2"
curl --fail --location --retry 3 --output "$zr_archive" \
  'https://downloads.sourceforge.net/project/freeverb3-vst/dev/mac-cross-gcc-2009-12-01_gcc-5247%2Bodcctools-698.1od9/odcctools-20090808-gcc-5247-bin.tar.bz2'
printf '%s  %s\n' \
  e34e46eee8a6e9b9292cf0ebb7c8914dcecab7090d8566469f5a68363aac79b0 \
  "$zr_archive" | sha256sum -c -
tar -xjf "$zr_archive" -C /
# Only these self-contained register-save routines are needed from libgcc.a.
# Linking the full archive with Mozilla's -all_load imports Tiger-only stubs.
/opt/mac/bin/powerpc-apple-darwin8-ar p \
  /opt/mac/lib/gcc/powerpc-apple-darwin8/4.0.1/libgcc.a darwin-fpsave.o \
  > /opt/mac/lib/zoolrunner-fpsave.o
test -s /opt/mac/lib/zoolrunner-fpsave.o
