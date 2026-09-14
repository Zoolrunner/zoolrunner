#!/bin/sh
# Download the only SDK version currently validated for ZoolRunner's Cocoa port.
set -eu
if test "$#" -ne 1; then
  echo "usage: fetch-sdk.sh DESTINATION" >&2
  exit 1
fi
mkdir -p "$1"
zr_sdk_dir=`cd "$1" && pwd`
zr_sdk_archive="$zr_sdk_dir/MacOSX11.3.sdk.tar.xz"
curl --fail --location --retry 3 \
  --output "$zr_sdk_archive" \
  https://github.com/phracker/MacOSX-SDKs/releases/download/11.3/MacOSX11.3.sdk.tar.xz
printf '%s  %s\n' \
  cd4f08a75577145b8f05245a2975f7c81401d75e9535dcffbb879ee1deefcbf4 \
  "$zr_sdk_archive" | shasum -a 256 -c -
tar -xJf "$zr_sdk_archive" -C "$zr_sdk_dir"
test "`/usr/libexec/PlistBuddy -c 'Print :Version' "$zr_sdk_dir/MacOSX11.3.sdk/SDKSettings.plist"`" = 11.3
