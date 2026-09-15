#!/usr/bin/env python3
"""Run a test payload on original Mac OS X 10.0 using Linux QEMU and hfsutils.

The payload is a trusted, locally built tar.gz containing test.sh at its root.
Only the generated disposable disk is writable. The OS CD stays read-only.
"""
import argparse
import gzip
import json
from pathlib import Path
import socket
import shutil
import subprocess
import time
from hfsutils_lock import hfsutils_lock


class Monitor:
    def __init__(self, path):
        self.socket = socket.socket(socket.AF_UNIX)
        self.socket.settimeout(10)
        self.socket.connect(str(path))
        self.stream = self.socket.makefile("rwb")
        json.loads(self.stream.readline())
        self.call("qmp_capabilities")

    def call(self, command, arguments=None):
        self.stream.write((json.dumps({"execute": command,
                                      "arguments": arguments or {}}) + "\n").encode())
        self.stream.flush()
        while True:
            result = json.loads(self.stream.readline())
            if "error" in result:
                raise RuntimeError(result["error"])
            if "return" in result:
                return result["return"]

    def type(self, text):
        keys = {" ": "spc", "/": "slash", "-": "minus", ".": "dot", "\n": "ret",
                "&": "shift-7", "|": "shift-backslash"}
        for char in text:
            key = keys.get(char, char)
            if char not in keys and not (char.isdigit() or "a" <= char <= "z"):
                raise ValueError("Unsupported console character: " + repr(char))
            self.call("send-key", {"keys": [{"type": "qcode", "data": part} for part in key.split("-")],
                                   "hold-time": 100})
            # Leave time for both key-down and key-up on busy emulation hosts.
            time.sleep(0.4)

    def close(self):
        self.stream.close()
        self.socket.close()


GUEST = """#!/bin/sh
set -eux
trap '' 1
zr_completed=no
finish() {
    zr_status=$?
    # Original zsh can expose the previous command's status in an EXIT trap
    # after an explicit exit. Only the completed payload may mark success.
    if test "$zr_completed" != yes; then zr_status=1; fi
    if test "$zr_status" -eq 0; then
        echo PASS > /tmp/result.txt
    else
        echo "FAIL $zr_status" > /tmp/result.txt
        if test -r /var/log/system.log; then
            cat /var/log/system.log || :
        fi
    fi
    sync
    /sbin/reboot
}
trap finish 0
exec > /tmp/test.log 2>&1
ZR_TEST_TRANSFER_DISK=$(/sbin/mount | awk '$3 == "/tmp" || $3 == "/private/tmp" {sub(/s2$/, "", $1); print $1}')
case "$ZR_TEST_TRANSFER_DISK" in /dev/disk0|/dev/disk1) ;; *) exit 2 ;; esac
export ZR_TEST_TRANSFER_DISK
/sbin/newfs_hfs -v ZoolScratch "/dev/r${ZR_TEST_TRANSFER_DISK#/dev/}s3"
mkdir /tmp/work
/sbin/mount -t hfs "${ZR_TEST_TRANSFER_DISK}s3" /tmp/work
if ! /sbin/mount | awk '$3 == "/.vol" {found=1} END {exit !found}'; then
    /sbin/mount_volfs /.vol
fi
cd /tmp/work
tar xf /tmp/payload.tar
sh ./test.sh
zr_completed=yes
"""


def run(args):
    work = args.work.resolve()
    work.mkdir()  # Never reuse disks or stale PASS files from another run.
    scripts = Path(__file__).resolve().parent
    guest = work / "run.sh"
    guest_script = GUEST
    if args.graphical:
        # The original CD uses zsh as its single-user shell. Keep the test
        # alive while that shell exits and init starts the normal desktop.
        guest_script = guest_script.replace("sh ./test.sh", "sleep 90\nsh ./test.sh")
        # The boot CD's real root-user home is read-only. Give profile and
        # preferences APIs a writable home filesystem at the unchanged path.
        guest_script = guest_script.replace("cd /tmp/work", """/sbin/newfs_hfs -v ZoolHome "/dev/r${ZR_TEST_TRANSFER_DISK#/dev/}s4"
/sbin/mount -t hfs "${ZR_TEST_TRANSFER_DISK}s4" /private/var/root
mkdir -p /private/var/root/Library/Preferences
cd /tmp/work""")
    guest.write_text(guest_script)
    disk = work / "test.img"
    payload = work / "payload.tar"
    # The installation CD has tar but no gzip executable.
    with gzip.open(args.payload, "rb") as source, payload.open("xb") as output:
        shutil.copyfileobj(source, output)
    subprocess.run(["python3", str(scripts / "make-10.0-test-disk.py"), str(disk),
                    "--file", "run.sh", str(guest), "--file", "payload.tar",
                    str(payload), "--scratch-mb", str(args.scratch_mb),
                    "--home-mb", "64" if args.graphical else "0"],
                   check=True)
    payload.unlink()
    extra_drives = []
    if args.prepare_system:
        # Create a new owned copy; never write the source OS image.
        with args.cd.open("rb") as source, args.prepare_system.open("xb") as target:
            shutil.copyfileobj(source, target, 1024 * 1024)
        extra_drives = ["-drive", "file=%s,index=1,format=raw" % args.prepare_system.resolve()]
    monitor_path = work / "qmp.sock"
    command = ["qemu-system-ppc", "-M", "mac99", "-cpu", "G3", "-m", "256",
               "-drive", "file=%s,media=cdrom,index=2,format=raw,readonly=on" % args.cd.resolve(),
               "-drive", "file=%s,index=0,format=raw" % disk,
               "-net", "none", "-display", "none", "-no-reboot",
               "-serial", "file:" + str(work / "serial.log"),
               "-qmp", "unix:%s,server=on,wait=off" % monitor_path,
               "-monitor", "unix:%s,server=on,wait=off" % (work / "monitor.sock"),
               "-prom-env", "boot-device=cd:2,\\SystemFolderX\\BootX",
               "-prom-env", "boot-args=-s"]
    monitor = None
    command += extra_drives
    failure = None
    with (work / "qemu.log").open("wb") as log:
        process = subprocess.Popen(command, stdout=log, stderr=subprocess.STDOUT)
        try:
            deadline = time.monotonic() + 30
            while not monitor_path.exists():
                if process.poll() is not None or time.monotonic() > deadline:
                    raise RuntimeError("QEMU did not start; see " + str(work / "qemu.log"))
                time.sleep(0.2)
            monitor = Monitor(monitor_path)
            # Allow slow emulation hosts to reach the single-user prompt.
            time.sleep(args.boot_wait)
            monitor.call("screendump", {"filename": str(work / "boot.ppm")})
            monitor.type("/sbin/mount -t hfs /dev/disk0s2 /private/tmp\n")
            time.sleep(5)
            if args.prepare_system:
                # Original IOKit can enumerate the two writable disks in
                # either order. Identify transfer media by its input script.
                monitor.type("test -f /private/tmp/run.sh || /sbin/umount /private/tmp\n")
                monitor.type("test -f /private/tmp/run.sh || /sbin/mount -t hfs /dev/disk1s2 /private/tmp\n")
            monitor.type("cd /private/tmp\n")
            if args.graphical:
                # First exit acknowledges zsh's running-job warning; the
                # second returns to init. The payload waits for GUI startup.
                monitor.type("sh run.sh &\nexit\nexit\n")
            else:
                monitor.type("sh run.sh\n")
            deadline = time.monotonic() + args.timeout
            frame = 0
            while process.poll() is None:
                try:
                    monitor.call("screendump", {"filename": str(work / "current.ppm")})
                    if args.graphical:
                        shutil.copyfile(work / "current.ppm", work / ("frame-%03d.ppm" % frame))
                        frame += 1
                except (OSError, ValueError):
                    if process.poll() is None:
                        raise
                if time.monotonic() > deadline:
                    raise RuntimeError("Guest tests timed out; see " + str(work))
                time.sleep(10)
            result = process.returncode
            if result:
                raise RuntimeError("QEMU failed with exit status %d" % result)
        except Exception as error:
            failure = error
        finally:
            if process.poll() is None:
                process.terminate()
                try:
                    process.wait(timeout=10)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait()
            if monitor:
                monitor.close()
    try:
        with hfsutils_lock():
            subprocess.run(["hmount", str(disk), "1"], check=True)
            try:
                for name in ("test.log", "result.txt"):
                    subprocess.run(["hcopy", "-r", ":" + name, str(work / name)],
                                   check=failure is None)
            finally:
                subprocess.run(["humount"], check=True)
    except Exception:
        if failure is None:
            raise
    if (work / "test.log").exists():
        print((work / "test.log").read_text(errors="replace"))
    if failure is not None:
        raise failure
    if (work / "result.txt").read_text().strip() != "PASS":
        raise RuntimeError("Original Mac OS X 10.0 guest tests failed")
    print("Original Mac OS X 10.0 guest tests: PASS")
    if not args.keep_disk:
        disk.unlink()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cd", type=Path, required=True, help="APM-wrapped original 10.0 CD")
    parser.add_argument("--payload", type=Path, required=True)
    parser.add_argument("--work", type=Path, required=True, help="New result directory")
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--prepare-system", type=Path,
                      help="Create a new writable CD copy for the original-provider preparation payload")
    mode.add_argument("--graphical", action="store_true",
                      help="Run tests after the original system's normal graphical startup")
    parser.add_argument("--boot-wait", type=int, default=90)
    parser.add_argument("--timeout", type=int, default=600)
    parser.add_argument("--scratch-mb", type=int, default=512)
    parser.add_argument("--keep-disk", action="store_true")
    run(parser.parse_args())
