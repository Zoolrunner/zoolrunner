#!/bin/sh
# Exercise current native drawing and the pre-Tiger stroke fallback on macOS.
# This host regression check does not emulate an old OS or PowerPC hardware.
set -eu
if test "$#" -lt 2 || test "$#" -gt 3; then
  echo "Usage: $0 COCOA_OBJDIR MACOS_SDK [arm64|x86_64]" >&2
  exit 1
fi
zr_obj=`CDPATH= cd -- "$1" && pwd`
zr_sdk=`CDPATH= cd -- "$2" && pwd`
zr_arch=${3:-`uname -m`}
case "$zr_arch" in arm64|x86_64) ;; *) echo "Unsupported test architecture: $zr_arch" >&2; exit 1;; esac
zr_tests=`CDPATH= cd -- "$(dirname -- "$0")" && pwd`
zr_src=`CDPATH= cd -- "$zr_tests/../../.." && pwd`
zr_work=`mktemp -d`
trap 'rm -rf "$zr_work"' 0
trap 'exit 1' HUP INT TERM
for zr_mode in native panther-fallback cheetah-software; do
  zr_extra=
  cp "$zr_src/gfx/cairo/cairo/src/cairo-quartz2-surface.c" "$zr_work/source.c"
  if test "$zr_mode" = panther-fallback; then
    python3 - "$zr_work/source.c" <<'PY'
from pathlib import Path
import sys
path = Path(sys.argv[1])
source = path.read_text()
guard = '#if MAC_OS_X_VERSION_MAX_ALLOWED >= 1040'
if source.count(guard) != 1:
    raise SystemExit('Stroke fallback guard changed; review this test')
path.write_text(source.replace(guard, '#if 0 /* Exercise pre-Tiger stroke fallback */'))
PY
  fi
  if test "$zr_mode" = cheetah-software; then
    zr_extra=-DCAIRO_TEST_EARLY_QUARTZ
  fi
  xcrun clang -arch "$zr_arch" -std=gnu89 -fPIC -isysroot "$zr_sdk" \
    $zr_extra \
    -I"$zr_src/config/macos" -I"$zr_src/gfx/cairo/cairo/src" \
    -I"$zr_obj/gfx/cairo/cairo/src" -I"$zr_obj/dist/include" \
    -I"$zr_obj/dist/include/png" -I"$zr_obj/dist/include/zlib" \
    -I"$zr_obj/dist/include/libpixman" -include "$zr_obj/mozilla-config.h" \
    -c "$zr_work/source.c" -o "$zr_work/cairo-quartz2-surface.o"
  cp "$zr_obj/gfx/cairo/cairo/src/libmozcairo.a" "$zr_work/libmozcairo.a"
  xcrun ar r "$zr_work/libmozcairo.a" "$zr_work/cairo-quartz2-surface.o"
  xcrun clang -arch "$zr_arch" -isysroot "$zr_sdk" -I"$zr_src/gfx/cairo/cairo/src" \
    -I"$zr_obj/gfx/cairo/cairo/src" -I"$zr_obj/dist/include/cairo" \
    "$zr_tests/quartz-gradient-stroke.c" "$zr_work/libmozcairo.a" \
    "$zr_obj/gfx/cairo/libpixman/src/libmozlibpixman.a" \
    -framework ApplicationServices -framework Carbon -framework Cocoa \
    -o "$zr_work/check-strokes"
  echo "Checking $zr_mode"
  "$zr_work/check-strokes"
  if test "$zr_mode" = cheetah-software; then
    xcrun clang -arch "$zr_arch" -isysroot "$zr_sdk" -I"$zr_src/gfx/cairo/cairo/src" \
      -I"$zr_src/config/macos" \
      -I"$zr_obj/gfx/cairo/cairo/src" -I"$zr_obj/dist/include/cairo" \
      "$zr_tests/quartz-early.c" "$zr_work/libmozcairo.a" \
      "$zr_obj/gfx/cairo/libpixman/src/libmozlibpixman.a" \
      -framework ApplicationServices -framework Carbon -framework Cocoa \
      -o "$zr_work/check-early"
    "$zr_work/check-early"
  fi
done
