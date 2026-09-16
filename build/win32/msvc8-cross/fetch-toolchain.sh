#!/bin/sh
# Pinned portable VC8 plus Microsoft's matching x86 MIDL tools.
set -eu
zr_dest=${1:?Usage: fetch-toolchain.sh NEW_TOOLCHAIN_DIRECTORY}
test ! -e "$zr_dest" || { echo "Toolchain destination already exists" >&2; exit 1; }
zr_temp=`mktemp -d`
trap 'rm -rf "$zr_temp"' 0 1 2 3 15
zr_revision=936f93b9e9e92943ed5c9398d24712a32c2e9fe4
curl -fL --retry 3 -o "$zr_temp/msvc8.tar.gz" \
  "https://codeload.github.com/widberg/msvc8.0/tar.gz/$zr_revision"
curl -fL --retry 3 -o "$zr_temp/sdk.img" \
  'https://download.microsoft.com/download/7/5/e/75ec7f04-4c8c-4f38-b582-966e76602643/5.2.3790.1830.15.PlatformSDK_Svr2003SP1_rtm.img'
printf '%s  %s\n' \
  18e876d22285aa17be6676d765be6556cb30ffb26e0c9dad59ddd66e5ccf6afa "$zr_temp/msvc8.tar.gz" \
  7ef138b07a8ed2e008371d8602900eb68e86ac2a832d16b53f462a9e64f24d53 "$zr_temp/sdk.img" | sha256sum -c -
mkdir -p "$zr_dest"
tar -xzf "$zr_temp/msvc8.tar.gz" --strip-components=1 -C "$zr_dest"
7z e -y "-o$zr_temp" "$zr_temp/sdk.img" Setup/PSDK-SDK_Core_BIN-x86.0.cab >/dev/null
7z e -y "-o$zr_temp" "$zr_temp/PSDK-SDK_Core_BIN-x86.0.cab" \
  'Midl_Exe.*' 'MidlC_Exe.*' >/dev/null
cp "$zr_temp"/Midl_Exe.* "$zr_dest/PlatformSDK/Bin/midl.exe"
cp "$zr_temp"/MidlC_Exe.* "$zr_dest/PlatformSDK/Bin/midlc.exe"
printf 'widberg/msvc8.0 %s\nMicrosoft Platform SDK 3790.1830, x86 MIDL 6.00.0366\n' \
  "$zr_revision" > "$zr_dest/PROVENANCE.txt"
