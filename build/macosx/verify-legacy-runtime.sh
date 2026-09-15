#!/bin/sh
# Run on the target Mac. No developer tools or Python are required.
set -u
if test "$#" -lt 1 || test "$#" -gt 2; then
  echo "usage: sh run.sh RUNTIME_DIRECTORY [NEW_REPORT_DIRECTORY]" >&2
  exit 2
fi
zr_runtime=`cd "$1" && pwd` || exit 2
case "$0" in
  */*) zr_test_dir=${0%/*} ;;
  *) zr_test_dir=. ;;
esac
zr_tests=`cd "$zr_test_dir" && pwd` || exit 2
zr_report=${2:-./zoolrunner-runtime-checks-$$}
test -x "$zr_runtime/xpcshell" || { echo "Missing xpcshell in $zr_runtime" >&2; exit 2; }
mkdir "$zr_report" || exit 2
{
  uname -a
  sw_vers
  sysctl -n hw.model
  file "$zr_runtime/xpcshell"
} > "$zr_report/system.txt" 2>&1
zr_failures=0
for zr_test in object-reflection legacy-application debugger-lifecycle; do
  case "$zr_test" in
    object-reflection) zr_marker='ES5-OBJECT-REFLECTION checks=101 failures=0' ;;
    legacy-application) zr_marker='LEGACY-APPLICATION checks=58 failures=0' ;;
    debugger-lifecycle) zr_marker='DEBUGGER-LIFECYCLE checks=5 failures=0' ;;
  esac
  # Force eager symbol binding to expose missing OS/library imports sooner.
  if DYLD_BIND_AT_LAUNCH=1 "$zr_runtime/xpcshell" -f "$zr_tests/$zr_test.js" \
      > "$zr_report/$zr_test.log" 2>&1; then
    zr_status=0
  else
    zr_status=$?
  fi
  if test "$zr_status" -eq 0 && awk -v marker="$zr_marker" \
      'index($0, marker) { found=1 } END { exit !found }' "$zr_report/$zr_test.log"; then
    echo "PASS $zr_test"
  else
    echo "FAIL $zr_test (exit $zr_status; see $zr_report/$zr_test.log)"
    zr_failures=$((zr_failures + 1))
  fi
done
echo "failures=$zr_failures" > "$zr_report/result.txt"
echo "Reports: $zr_report"
echo "These shell checks do not replace GUI, printing, or full conformance tests."
test "$zr_failures" -eq 0
