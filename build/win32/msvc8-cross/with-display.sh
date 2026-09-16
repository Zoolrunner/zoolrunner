#!/bin/sh
# Xvfb's displayfd handshake also works with 32-bit Linux under CPU emulation.
set -eu
zr_display_dir=`mktemp -d`
Xvfb -displayfd 3 -screen 0 1280x1024x24 -nolisten tcp \
  3>"$zr_display_dir/number" >"$zr_display_dir/server.log" 2>&1 &
zr_display_pid=$!
trap 'kill "$zr_display_pid" 2>/dev/null || :; rm -rf "$zr_display_dir"' 0 1 2 3 15
zr_wait=0
while test ! -s "$zr_display_dir/number"; do
  if ! kill -0 "$zr_display_pid" 2>/dev/null || test "$zr_wait" -ge 30; then
    cat "$zr_display_dir/server.log" >&2
    echo 'Xvfb did not become ready' >&2
    exit 1
  fi
  sleep 1
  zr_wait=$((zr_wait + 1))
done
DISPLAY=:`cat "$zr_display_dir/number"`
export DISPLAY
"$@"
