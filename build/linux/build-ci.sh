#!/bin/sh
set -eu
# Parse the driver before building so edits in a mounted checkout cannot
# change the remaining commands of an in-progress local porting run.
main() {
zr_arch=${1:?architecture required}
zr_app=${2:?application required}
zr_toolkit=${3:-gtk2}
case "$zr_toolkit" in gtk2|xlib) ;; *) exit 2 ;; esac
zr_suffix=
if test "$zr_toolkit" = xlib; then zr_suffix=-xlib; fi
case "$zr_arch" in i686|x86_64|aarch64) ;; *) exit 2 ;; esac
case "$zr_app" in suite|browser|calendar|xulrunner) ;; *) exit 2 ;; esac
mkdir -p /work/source /work/logs /work/artifacts
rsync -a --exclude=.git --exclude='obj-*' --exclude=artifacts \
  --exclude='.mozconfig*' --exclude=.DS_Store /source/ /work/source/
cd /work/source
export MOZCONFIG="/work/source/mozconfigs/linux/$zr_arch/${zr_toolkit}_${zr_app}_gcc.mozconfig"
{ cat /etc/oracle-release; gcc --version; rpm -q gcc-toolset-14-gcc; } > /work/logs/environment.txt
# Re-read the shared mozconfig even when its changes do not alter .mozconfig.mk.
if { make -f client.mk configure &&
     make -f client.mk build "MOZ_MAKE_FLAGS=-j${ZR_BUILD_JOBS:-3}"; } > /work/logs/build.log 2>&1; then
  echo "$zr_arch $zr_toolkit $zr_app build passed"
else
  grep -n -E 'error:|undefined reference|hidden symbol' /work/logs/build.log | tail -n 20 || :
  tail -n 70 /work/logs/build.log
  exit 1
fi
run_logged() {
  zr_log=$1
  shift
  if "$@" > "/work/logs/$zr_log.log" 2>&1; then
    echo "$zr_arch $zr_toolkit $zr_app $zr_log passed"
  else
    tail -n 70 "/work/logs/$zr_log.log"
    return 1
  fi
}
run_logged cpucfg sh build/linux/test-cpucfg.sh /work/source "/work/source/obj-zoolrunner-linux-$zr_arch-$zr_app$zr_suffix"
run_logged package python3 build/linux/package-ci.py "$zr_arch" "$zr_app" /work --toolkit "$zr_toolkit"
run_logged runtime sh build/linux/with-display.sh python3 build/linux/test-package.py "$zr_arch" "$zr_app" /work --toolkit "$zr_toolkit"
}
main "$@"
