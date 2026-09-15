#!/bin/sh
# Download pinned inputs into a reusable CI cache; no OS files enter the checkout.
set -eu
if test "$#" != 1; then
  echo "Usage: $0 DOWNLOAD_DIRECTORY" >&2; exit 1
fi
mkdir -p "$1"
zr_download=`CDPATH= cd -- "$1" && pwd`
zr_partial=
trap 'test -z "$zr_partial" || rm -f "$zr_partial"' 0
trap 'exit 1' HUP INT TERM
zr_fetch() {
  zr_file="$zr_download/$1"
  zr_hash=$2
  if test ! -f "$zr_file"; then
    zr_partial="$zr_file.partial"
    curl --fail --location --retry 3 --connect-timeout 30 \
      --speed-limit 1024 --speed-time 120 --output "$zr_partial" "$3"
    printf '%s  %s\n' "$zr_hash" "$zr_partial" | sha256sum -c -
    mv "$zr_partial" "$zr_file"
    zr_partial=
  fi
  printf '%s  %s\n' "$zr_hash" "$zr_file" | sha256sum -c -
}
zr_release='https://downloads.sourceforge.net/project/freeverb3-vst/dev/mac-cross-gcc-2009-12-01_gcc-5247%2Bodcctools-698.1od9'
zr_fetch gcc-5247.tar.gz \
  38b4f890513dff0cc97119ae80c92e5252d0e2a94bb240d6961f7ba166c1abc7 \
  "$zr_release/gcc-5247.tar.gz"
zr_fetch odcctools-20090808.tar.bz2 \
  ef781cfba574cb5702ba7dea07bcfeb020a6502324418207210d142138560c80 \
  "$zr_release/odcctools-20090808.tar.bz2"
zr_fetch MacOSX10.0-original.cdr \
  e6fbc5c9ae633637777b5f2a9b9be3d8e3779cde02e2c7a2bec5dcc453951dc3 \
  'https://archive.org/download/cdrom_Mac_OS_X_Version_10.0_1Z691-2974-A/Mac%20OS%20X%20Version%2010.0%201Z691-2974-A.cdr'
