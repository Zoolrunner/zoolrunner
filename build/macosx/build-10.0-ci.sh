#!/bin/sh
# Run inside powerpc.Dockerfile's disposable linux/386 container.
set -eu
if test "$#" != 2; then
  echo "Usage: $0 APPLICATION NEW_WORK_DIRECTORY" >&2; exit 2
fi
zr_app=$1
case "$zr_app" in suite|browser|calendar|xulrunner) ;; *) exit 2 ;; esac
zr_checkout=`CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd`
mkdir -p "$2"
zr_work=`CDPATH= cd -- "$2" && pwd`
if test -e "$zr_work/source"; then
  echo "Use a fresh work directory; refusing to overwrite $zr_work/source" >&2
  exit 2
fi
mkdir "$zr_work/source" "$zr_work/logs"
zr_log() {
  zr_label=$1
  shift
  echo "Starting $zr_label"
  if "$@" > "$zr_work/logs/$zr_label.log" 2>&1; then
    echo "Passed $zr_label"
  else
    zr_status=$?
    tail -n 80 "$zr_work/logs/$zr_label.log"
    exit "$zr_status"
  fi
}
rsync -a --exclude='.git' --exclude='obj-*' --exclude='artifacts' \
  --exclude='.mozconfig*' --exclude='.DS_Store' \
  "$zr_checkout/" "$zr_work/source/"
zr_scripts="$zr_work/source/build/macosx"
zr_log inputs sh "$zr_scripts/fetch-early-ppc-inputs.sh" "$zr_work/downloads"
zr_log headers sh "$zr_scripts/fetch-sdk.sh" "$zr_work/headers" 10.1.5
zr_log gcc-headers sh "$zr_scripts/fetch-sdk.sh" "$zr_work/headers" 10.4u
ZR_MACOS_SDK="$zr_work/target-sdk"
ZR_PPC_EARLY_LINKER="$zr_work/linker/ld"
ZR_EARLY_CXX_RUNTIME="$zr_work/cxx-runtime"
ZR_PPC_DEPLOYMENT_TARGET=10.0
ZR_OBJDIR="$zr_work/obj-$zr_app"
ZR_BUILD_JOBS=${ZR_BUILD_JOBS:-3}
export ZR_MACOS_SDK ZR_PPC_EARLY_LINKER ZR_EARLY_CXX_RUNTIME
export ZR_PPC_DEPLOYMENT_TARGET ZR_OBJDIR ZR_BUILD_JOBS
zr_log sdk python3 "$zr_scripts/prepare-10.0-sdk.py" \
  "$zr_work/downloads/MacOSX10.0-original.cdr" \
  "$zr_work/headers/MacOSX10.1.5.sdk" "$ZR_MACOS_SDK"
zr_log linker sh "$zr_scripts/build-early-ppc-linker.sh" \
  "$zr_work/downloads/odcctools-20090808.tar.bz2" "$zr_work/linker"
zr_log startup sh "$zr_scripts/activate-10.0-toolchain.sh"
zr_log cxx-runtime sh "$zr_scripts/build-early-stdlib.sh" \
  "$zr_work/downloads/gcc-5247.tar.gz" \
  "$zr_work/headers/MacOSX10.4u.sdk/usr/include/c++/4.0.0" \
  "$ZR_EARLY_CXX_RUNTIME"
MOZCONFIG="$zr_work/source/mozconfigs/macos/powerpc/cocoa_${zr_app}_10.0_gcc.mozconfig"
export MOZCONFIG
cd "$zr_work/source"
zr_log build make -f client.mk build
zr_log package python3 "$zr_scripts/package-ci.py" powerpc "$zr_app"
mv "$zr_work/source/artifacts" "$zr_work/artifacts"
set -- "$zr_work/artifacts/"*.tar.gz
if test "$#" != 1 || test ! -f "$1"; then
  echo "Expected exactly one application archive" >&2; exit 1
fi
zr_log target-probes python3 "$zr_scripts/tests/prepare-10.0-tests.py" \
  "$1" "$ZR_OBJDIR" "$zr_work/runtime-tests.tar.gz"
zr_log boot-media python3 "$zr_scripts/tests/wrap-10.0-cd.py" \
  "$zr_work/downloads/MacOSX10.0-original.cdr" "$zr_work/runtime-cd.img"
zr_log test-system-files python3 "$zr_scripts/tests/prepare-10.0-system-files.py" \
  "$zr_work/downloads/MacOSX10.0-original.cdr" "$zr_work/runtime-system-files.tar.gz"
cp "$ZR_MACOS_SDK/PROVENANCE.json" "$zr_work/logs/sdk-provenance.json"
echo "Packaged $zr_app; target runtime validation is a separate required CI step."
