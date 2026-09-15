#!/bin/sh
# Build the classic linker for explicit Mac OS X 10.0 deployment.
# Input: upstream odcctools-20090808.tar.bz2 (same release as the PPC tools).
set -eu
if test "$#" != 2; then
  echo "Usage: $0 SOURCE_ARCHIVE OUTPUT_DIRECTORY" >&2
  exit 1
fi
if test "`uname -s`" != Linux || test "`getconf LONG_BIT`" != 32; then
  echo "Build in the disposable 32-bit Linux toolchain container" >&2
  exit 1
fi
zr_script_dir=`CDPATH= cd -- "$(dirname -- "$0")" && pwd`
zr_archive=$1
zr_output=$2
if test -e "$zr_output/ld"; then
  echo "Refusing to overwrite $zr_output/ld" >&2
  exit 1
fi
printf '%s  %s\n' \
  ef781cfba574cb5702ba7dea07bcfeb020a6502324418207210d142138560c80 \
  "$zr_archive" | sha256sum -c -
zr_work=`mktemp -d`
trap 'rm -rf "$zr_work"' 0
trap 'exit 1' HUP INT TERM
mkdir "$zr_work/source" "$zr_work/obj"
tar -xjf "$zr_archive" -C "$zr_work/source" --strip-components=1
patch -d "$zr_work/source" -p1 < "$zr_script_dir/odcctools-10.0.patch"
mkdir -p "$zr_output"
zr_output=`CDPATH= cd -- "$zr_output" && pwd`
cd "$zr_work/obj"
"$zr_work/source/configure" --target=powerpc-apple-darwin \
  --prefix="$zr_output" CC=gcc CXX=g++ CFLAGS='-O2 -fcommon -std=gnu89'
make -j"${ZR_BUILD_JOBS:-4}" ld
# Invoke this linker directly: the pinned GCC collect2 uses a hard-coded
# --with-ld path. Keep it separate from the default Panther toolchain.
cp ld/ld_classic "$zr_output/ld"
