#!/bin/sh
# Build GCC's matching C++ ABI support for the experimental 10.0 overlay.
set -eu
if test "$#" != 3; then
  echo "Usage: $0 GCC_SOURCE_ARCHIVE GCC4_HEADER_DIRECTORY NEW_OUTPUT_DIRECTORY" >&2
  exit 1
fi
if test "`uname -s`" != Linux || test "`getconf LONG_BIT`" != 32; then
  echo "Use the disposable 32-bit Linux PowerPC toolchain container" >&2
  exit 1
fi
: "${ZR_MACOS_SDK:?Set ZR_MACOS_SDK to the original-library 10.0 overlay}"
zr_scripts=`CDPATH= cd -- "$(dirname -- "$0")" && pwd`
zr_archive=$1
zr_headers=`CDPATH= cd -- "$2" && pwd`
printf '%s  %s\n' \
  38b4f890513dff0cc97119ae80c92e5252d0e2a94bb240d6961f7ba166c1abc7 \
  "$zr_archive" | sha256sum -c -
mkdir "$3"
zr_output=`CDPATH= cd -- "$3" && pwd`
zr_work=`mktemp -d`
trap 'rm -rf "$zr_work"' 0
trap 'exit 1' HUP INT TERM
mkdir "$zr_work/source" "$zr_work/obj"
tar -xzf "$zr_archive" -C "$zr_work/source" --strip-components=1 \
  gcc-5247/libstdc++-v3/libsupc++ gcc-5247/libiberty gcc-5247/include \
  gcc-5247/gcc/unwind-pe.h gcc-5247/COPYING
cp -R "$zr_headers" "$zr_work/headers"
cp "$zr_scripts/compat/zoolrunner-recursive-mutex.h" "$zr_work/headers/"
python3 "$zr_scripts/patch-early-cxx-headers.py" "$zr_work/headers"
ZR_RUNTIME_SOURCE="$zr_work/source"
ZR_RUNTIME_HEADERS="$zr_work/headers"
export ZR_RUNTIME_SOURCE ZR_RUNTIME_HEADERS ZR_MACOS_SDK
cd "$zr_work/obj"
make -f "$zr_scripts/early-cxx-runtime.mk" -j"${ZR_BUILD_JOBS:-4}"
cp libsupc++-10.0.a "$zr_output/"
cp "$zr_work/source/COPYING" "$zr_output/COPYING.GCC"
mv "$zr_work/headers" "$zr_output/include"
echo "Built experimental C++ ABI support; full libstdc++ and application validation remain outstanding"
