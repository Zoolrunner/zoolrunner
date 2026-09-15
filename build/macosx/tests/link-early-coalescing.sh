#!/bin/sh
# Build a bounded test of C++ template state shared across three Mach-O images.
set -eu
if test "$#" != 1; then
  echo "Usage: $0 NEW_OUTPUT_DIRECTORY" >&2
  exit 1
fi
: "${ZR_MACOS_SDK:?Set ZR_MACOS_SDK to the original-library 10.0 overlay}"
: "${ZR_PPC_EARLY_LINKER:?Set ZR_PPC_EARLY_LINKER to the rebuilt classic linker}"
zr_tests=`CDPATH= cd -- "$(dirname -- "$0")" && pwd`
# Deliberately require a fresh directory so prior outputs cannot mask failures.
mkdir "$1"
zr_output=`CDPATH= cd -- "$1" && pwd`
zr_work=`mktemp -d`
trap 'rm -rf "$zr_work"' 0
trap 'exit 1' HUP INT TERM
for zr_file in a b main; do
  /opt/mac/bin/powerpc-apple-darwin8-g++ -isysroot "$ZR_MACOS_SDK" \
    -mmacosx-version-min=10.0 -mcpu=G3 -mlong-double-64 \
    -fno-exceptions -fno-rtti -c "$zr_tests/coalescing/$zr_file.cc" \
    -o "$zr_work/$zr_file.o"
done
for zr_file in a b; do
  "$ZR_PPC_EARLY_LINKER" -dylib -arch ppc -macosx_version_min 10.0 \
    -syslibroot "$ZR_MACOS_SDK" -flat_namespace \
    -install_name "@executable_path/lib$zr_file.dylib" \
    /opt/mac/lib/zoolrunner-cheetah/dylib1.o "$zr_work/$zr_file.o" \
    -L"$ZR_MACOS_SDK/usr/lib" -lSystem -o "$zr_output/lib$zr_file.dylib"
done
"$ZR_PPC_EARLY_LINKER" -dynamic -arch ppc -macosx_version_min 10.0 \
  -syslibroot "$ZR_MACOS_SDK" -flat_namespace \
  /opt/mac/lib/zoolrunner-cheetah/crt1.o "$zr_work/main.o" \
  -L"$zr_output" -la -lb -L"$ZR_MACOS_SDK/usr/lib" -lSystem \
  -o "$zr_output/coalescing"
/opt/mac/bin/powerpc-apple-darwin8-otool -hv \
  "$zr_output/coalescing" "$zr_output/liba.dylib" "$zr_output/libb.dylib"
echo "Linked coalescing probe; run with both libraries on the target OS"
