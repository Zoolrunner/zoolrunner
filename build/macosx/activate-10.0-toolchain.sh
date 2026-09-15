#!/bin/sh
# Change only the toolchain inside the disposable early-Darwin build container.
set -eu
if test "`uname -s`" != Linux || test "`getconf LONG_BIT`" != 32; then
  echo "Use the disposable 32-bit Linux PowerPC toolchain container" >&2; exit 1
fi
: "${ZR_PPC_EARLY_LINKER:?Build the classic 10.0 linker first}"
: "${ZR_MACOS_SDK:?Prepare the original-library overlay first}"
zr_scripts=`CDPATH= cd -- "$(dirname -- "$0")" && pwd`
zr_driver_linker=/opt/mac/bin/powerpc-apple-darwin8-ld
if test ! -f "$zr_driver_linker.before-10.0"; then
  cp "$zr_driver_linker" "$zr_driver_linker.before-10.0"
fi
# GCC's configured collect2 path and Apple's libtool both ignore -B for this.
if ! cmp -s "$ZR_PPC_EARLY_LINKER" "$zr_driver_linker"; then
  cp "$ZR_PPC_EARLY_LINKER" "$zr_driver_linker"
  chmod 755 "$zr_driver_linker"
fi
ZR_PPC_DEPLOYMENT_TARGET=10.0
export ZR_PPC_DEPLOYMENT_TARGET
sh "$zr_scripts/prepare-ppc-startup.sh"
