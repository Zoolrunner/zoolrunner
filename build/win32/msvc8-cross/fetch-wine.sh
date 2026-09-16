#!/bin/sh
# Use WineHQ's fixed Wine 11 binaries in the 64-bit WoW64 mode.
set -eu
zr_tmp=`mktemp -d`
trap 'rm -rf "$zr_tmp"' EXIT HUP INT TERM
zr_get() {
    zr_file=$1
    zr_hash=$2
    curl -fL --retry 3 "https://dl.winehq.org/wine-builds/debian/pool/main/w/wine/$zr_file" -o "$zr_tmp/$zr_file"
    echo "$zr_hash  $zr_tmp/$zr_file" | sha256sum -c -
    dpkg-deb -x "$zr_tmp/$zr_file" /
}
zr_get wine-stable-amd64_11.0.0.0~bookworm-1_amd64.deb e9c3d15171cd8d3bb164d742cdab7af43c0159dd8a91cd7ef237c7e5c8259675
zr_get wine-stable-i386_11.0.0.0~bookworm-1_i386.deb aec8a5d52308813118cab9ce4b3d34cd287dc13ff208571821c5320157c27421
zr_get wine-stable_11.0.0.0~bookworm-1_amd64.deb 9d7d50a25006af1793901369bb2119d5810389c0cb74c56142b27de44135835d
# New WoW64 uses the i386 PE modules, but not the old 32-bit Unix modules.
rm -rf /opt/wine-stable/lib/wine/i386-unix
for zr_tool in wineboot winepath; do
    printf '#!/bin/sh\nexec /opt/wine-stable/bin/wine %s.exe "$@"\n' "$zr_tool" > "/usr/local/bin/$zr_tool"
    chmod +x "/usr/local/bin/$zr_tool"
done
