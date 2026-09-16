#!/bin/sh
# Pixel regressions for decoded images, sprite states, and opacity groups.
set -eu
if test "$#" != 3; then
  echo "Usage: $0 COCOA_OBJDIR MACOS_SDK arm64|x86_64" >&2
  exit 2
fi
zr_obj=`cd "$1" && pwd`
zr_sdk=`cd "$2" && pwd`
zr_arch=$3
case "$zr_arch" in arm64|x86_64) ;; *) exit 2 ;; esac
zr_root=`cd "$(dirname "$0")/../../.." && pwd`
zr_work=`mktemp -d`
trap 'rm -rf "$zr_work"' 0
trap 'exit 1' HUP INT TERM
for zr_test in Images Opacity; do
  xcrun clang -arch "$zr_arch" -isysroot "$zr_sdk" \
    -I"$zr_obj/dist/include/cairo" -include "$zr_root/config/macos/CarbonCompat.h" \
    "$zr_root/gfx/tests/TestQuartz$zr_test.c" \
    "$zr_obj/gfx/cairo/cairo/src/libmozcairo.a" \
    "$zr_obj/gfx/cairo/libpixman/src/libmozlibpixman.a" \
    -framework Cocoa -framework Carbon -o "$zr_work/check"
  "$zr_work/check"
done
