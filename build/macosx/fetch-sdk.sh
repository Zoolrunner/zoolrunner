#!/bin/sh
# Download a pinned SDK for the modern or experimental i386 Cocoa build.
set -eu
if test "$#" -lt 1 || test "$#" -gt 2; then
  echo "usage: fetch-sdk.sh DESTINATION [11.3|10.6|10.4u|10.3.9|10.1.5]" >&2
  exit 1
fi
zr_version=${2:-11.3}
case "$zr_version" in
  11.3) zr_checksum=cd4f08a75577145b8f05245a2975f7c81401d75e9535dcffbb879ee1deefcbf4 ;;
  10.6) zr_checksum=ad93f3e25c1ee71e457ced1a9c106c2e6741afb96252f259a5b7c625d1156fe8 ;;
  10.4u) zr_checksum=d28276ce6b0f14af30f3bfdc171f8ef4033652da593578568f8be189506c1021 ;;
  10.3.9) zr_checksum=c91b4e510e8a4898637f89984fe0068c3f4067467e36642f83304846666ce6a6 ;;
  10.1.5) zr_checksum=cf939f34716cbf5a6046c650a54cf26ec67d102b856155e7f1de111d314e8f8a ;;
  *) echo "Unsupported SDK version: $zr_version" >&2; exit 1 ;;
esac
mkdir -p "$1"
zr_sdk_dir=`cd "$1" && pwd`
zr_sdk_archive="$zr_sdk_dir/MacOSX${zr_version}.sdk.tar.xz"
curl --fail --location --retry 3 \
  --output "$zr_sdk_archive" \
  https://github.com/phracker/MacOSX-SDKs/releases/download/11.3/MacOSX${zr_version}.sdk.tar.xz
printf '%s  %s\n' \
  "$zr_checksum" \
  "$zr_sdk_archive" | shasum -a 256 -c -
tar -xJf "$zr_sdk_archive" -C "$zr_sdk_dir"
python3 - "$zr_sdk_dir/MacOSX${zr_version}.sdk/SDKSettings.plist" "$zr_version" <<'PY'
import plistlib
import re
import sys
with open(sys.argv[1], 'rb') as source:
    data = source.read()
try:
    actual = plistlib.loads(data).get('Version')
except plistlib.InvalidFileException:
    # Early SDKs use OpenStep property-list syntax. Extract only the root
    # version field from the checksum-pinned file; do not rewrite the SDK.
    match = re.search(rb'^\s*Version\s*=\s*"([0-9.]+)"\s*;', data, re.MULTILINE)
    actual = match.group(1).decode('ascii') if match else None
expected = {'10.4u': '10.4', '10.1.5': '10.1'}.get(sys.argv[2], sys.argv[2])
if actual != expected:
    raise SystemExit('Unexpected SDK version: %r (expected %s)' % (actual, expected))
PY
if test "$zr_version" = 10.6; then
  python3 "$(dirname "$0")/patch-sdk-10.6.py" "$zr_sdk_dir/MacOSX10.6.sdk"
fi
if test "$zr_version" = 10.1.5; then
  python3 "$(dirname "$0")/patch-sdk-10.1.5.py" "$zr_sdk_dir/MacOSX10.1.5.sdk"
fi
rm -f "$zr_sdk_archive"
