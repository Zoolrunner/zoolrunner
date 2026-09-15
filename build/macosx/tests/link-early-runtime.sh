#!/bin/sh
# Link the early C++ ABI probe; execution is a separate target-OS test.
set -eu
if test "$#" != 2; then
  echo "Usage: $0 CXX_RUNTIME_DIRECTORY NEW_OUTPUT_BINARY" >&2
  exit 1
fi
: "${ZR_MACOS_SDK:?Set ZR_MACOS_SDK to the original-library 10.0 overlay}"
: "${ZR_PPC_EARLY_LINKER:?Set ZR_PPC_EARLY_LINKER to the rebuilt classic linker}"
zr_runtime=`CDPATH= cd -- "$1" && pwd`
zr_tests=`CDPATH= cd -- "$(dirname -- "$0")" && pwd`
if test -e "$2"; then
  echo "Refusing to overwrite probe output: $2" >&2
  exit 1
fi
zr_work=`mktemp -d`
trap 'rm -rf "$zr_work"' 0
trap 'exit 1' HUP INT TERM
zr_gcc=/opt/mac/lib/gcc/powerpc-apple-darwin8/4.0.1
/opt/mac/bin/powerpc-apple-darwin8-g++ -isystem "$zr_gcc/include" \
  -isysroot "$ZR_MACOS_SDK" -mmacosx-version-min=10.0 \
  -mcpu=G3 -mno-altivec -mlong-double-64 -O2 -nostdinc++ \
  -I"$zr_runtime/include" -I"$zr_runtime/include/powerpc-apple-darwin8" \
  -c "$zr_tests/early-runtime.cc" -o "$zr_work/probe.o"
"$ZR_PPC_EARLY_LINKER" -dynamic -arch ppc -macosx_version_min 10.0 \
  -syslibroot "$ZR_MACOS_SDK" -flat_namespace \
  /opt/mac/lib/zoolrunner-cheetah/crt1.o "$zr_work/probe.o" \
  "$zr_runtime/libsupc++-10.0.a" "$zr_gcc/libgcc_eh.a" "$zr_gcc/libgcc.a" \
  -L"$ZR_MACOS_SDK/usr/lib" -lSystem -o "$2"
/opt/mac/bin/powerpc-apple-darwin8-otool -hvL "$2"
echo "Linked C++ ABI probe; full platform compatibility is not established"
