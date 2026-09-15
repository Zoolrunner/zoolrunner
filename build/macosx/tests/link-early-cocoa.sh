#!/bin/sh
# Link only. Execute separately on the actual target OS to test startup.
set -eu
if test "$#" != 1; then
  echo "Usage: $0 OUTPUT_BINARY" >&2
  exit 1
fi
: "${ZR_MACOS_SDK:?Set ZR_MACOS_SDK to the original-library 10.0 overlay}"
: "${ZR_PPC_EARLY_LINKER:?Set ZR_PPC_EARLY_LINKER to the rebuilt 10.0 linker}"
zr_tests=`CDPATH= cd -- "$(dirname -- "$0")" && pwd`
if test -e "$1"; then
  echo "Refusing to overwrite probe output: $1" >&2
  exit 1
fi
zr_work=`mktemp -d`
trap 'rm -rf "$zr_work"' 0
trap 'exit 1' HUP INT TERM
/opt/mac/bin/powerpc-apple-darwin8-g++ \
  -isysroot "$ZR_MACOS_SDK" -mmacosx-version-min=10.0 \
  -DMAC_OS_X_VERSION_MIN_REQUIRED=1000 -DMAC_OS_X_VERSION_MAX_ALLOWED=1000 \
  -mcpu=G3 -mno-altivec -mlong-double-64 -fno-exceptions -fno-rtti \
  -c "$zr_tests/early-cocoa.mm" -o "$zr_work/probe.o"
# collect2 was configured with an absolute --with-ld path and ignores -B for
# linker selection. Invoke the rebuilt classic linker directly; ld64's common
# symbol treatment produced startup definitions rejected by the 10.0 loader.
"$ZR_PPC_EARLY_LINKER" -dynamic -arch ppc -macosx_version_min 10.0 \
  -syslibroot "$ZR_MACOS_SDK" -flat_namespace \
  /opt/mac/lib/zoolrunner-cheetah/crt1.o "$zr_work/probe.o" \
  -L"$ZR_MACOS_SDK/usr/lib" -framework Cocoa -lSystem -o "$1"
/opt/mac/bin/powerpc-apple-darwin8-otool -hvL "$1"
echo "Linked early Cocoa probe; target runtime has not been tested"
