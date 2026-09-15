#!/bin/sh
# Run inside the original 4K78 CD guest after mounting writable /tmp and volfs.
# Follow the CD's rc.cdrom service ordering, then run our test application.
# No installer, kernel or original system library is modified.
set -eu
PATH=/bin:/sbin:/usr/bin:/usr/sbin:/usr/libexec:/System/Library/CoreServices
export PATH
/usr/libexec/kextd
/sbin/ifconfig lo0 127.0.0.1 netmask 255.0.0.0 up
/sbin/mount -u -o ro /
/System/Library/Frameworks/ApplicationServices.framework/Frameworks/ATS.framework/Support/StartATSServer
sleep 5
"/System/Library/Frameworks/ApplicationServices.framework/Frameworks/CoreGraphics.framework/Resources/Window Manager"
sleep 15
/System/Library/CoreServices/pbs > /tmp/pbs.log 2>&1 < /dev/null &
