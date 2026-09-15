#!/bin/sh
# The installation CD does not run all installed-system security services.
# Run only inside the disposable original-OS QEMU guest.
set -eu
/sbin/ifconfig lo0 127.0.0.1 netmask 255.0.0.0 up
if ! /bin/ps -ax | awk '/[l]ookupd/ {found=1} END {exit !found}'; then
    /usr/sbin/lookupd > /tmp/lookupd.log 2>&1 < /dev/null &
    sleep 5
fi
if ! /bin/ps -ax | awk '/[S]ecurityServer/ {found=1} END {exit !found}'; then
    /System/Library/CoreServices/SecurityServer > /tmp/securityserver.log 2>&1 < /dev/null &
    sleep 5
fi
