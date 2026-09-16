#!/bin/sh
# Run under Xvfb inside the Linux cross-build image.
set -eu
test "$#" = 2 || { echo "Usage: $0 APP NEW_WORK_DIRECTORY" >&2; exit 2; }
zr_app=$1
case "$zr_app" in suite|browser|calendar|xulrunner) ;; *) exit 2 ;; esac
zr_checkout=`CDPATH= cd "$(dirname "$0")/../../.." && pwd`
mkdir -p "$2"
zr_work=`CDPATH= cd "$2" && pwd`
test ! -e "$zr_work/source" || { echo "Use a new work directory" >&2; exit 1; }
mkdir "$zr_work/source" "$zr_work/logs" "$zr_work/artifacts"
cp "$MSVC8_ROOT/PROVENANCE.txt" "$zr_work/logs/toolchain-provenance.txt"
{ uname -a; wine --version; cc --version; } > "$zr_work/logs/environment.txt" 2>&1
zr_log() {
  zr_label=$1; shift
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
  --exclude='.mozconfig*' --exclude='.DS_Store' "$zr_checkout/" "$zr_work/source/"
cd "$zr_work/source"
zr_scripts="$zr_work/source/build/win32/msvc8-cross"
zr_config=$zr_app
test "$zr_app" != suite || zr_config=suite-legacy
MOZCONFIG="$zr_work/source/mozconfigs/cross/win32-msvc8-$zr_config.mozconfig"
MSVC8_REQUIRE_STATIC_RTL=1
MSVC8_USE_PROCESS_HEAP=1
export MOZCONFIG MSVC8_REQUIRE_STATIC_RTL MSVC8_USE_PROCESS_HEAP
zr_obj="$zr_work/source/obj-zoolrunner-win32-msvc8-$zr_config"
# The bind-mounted parent belongs to the host runner, not necessarily this user.
# Wine requires an owned prefix, or an owned parent when creating the prefix.
mkdir -p "${WINEPREFIX:?Set WINEPREFIX to the build container Wine directory}"
zr_log wine wineboot -u
wineserver -w
zr_log toolchain sh "$zr_scripts/test-toolchain.sh"
zr_log build make -f client.mk build "MOZ_MAKE_FLAGS=-j${ZR_BUILD_JOBS:-3}"
zr_log package python3 "$zr_scripts/package-ci.py" "$zr_app" "$zr_obj" "$zr_work"
zr_log audit sh "$zr_scripts/audit-pe.sh" "$zr_work/runtime" "$zr_work/logs/pe-imports.tsv"
zr_log runtime python3 "$zr_scripts/tests/run-wine.py" "$zr_app" "$zr_obj" "$zr_work"
echo "$zr_app: Linux cross-build, package audit and Wine regressions passed"
