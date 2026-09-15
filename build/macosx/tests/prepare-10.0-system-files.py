#!/usr/bin/env python3
"""Prepare original installation files for QEMU.

The boot CD omits AppleCSP, QuickTime and AGL from the installed OS. Extract
them from its Essentials package, without substituting newer OS binaries.
The disposable image drops the already-extracted installer archive to make
room. Original OS runtime files are preserved. No OS image is packaged with
the application.
"""
import argparse
import hashlib
import io
import json
from pathlib import Path
import subprocess
import tarfile
import sys
import tempfile

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from legacy_tar import LegacyTarInfo

CD_SHA256 = "e6fbc5c9ae633637777b5f2a9b9be3d8e3779cde02e2c7a2bec5dcc453951dc3"
VOLUME = "Mac OS X Install CD/"


def digest(path):
    result = hashlib.sha256()
    with path.open("rb") as source:
        for data in iter(lambda: source.read(1024 * 1024), b""):
            result.update(data)
    return result.hexdigest()


def prepare(cd, output):
    if output.exists():
        raise ValueError("Refusing to overwrite " + str(output))
    if digest(cd) != CD_SHA256:
        raise ValueError("Expected the pinned original Mac OS X 10.0 build 4K78 CD")
    with tempfile.TemporaryDirectory(prefix="zool-test-system-") as temporary:
        work = Path(temporary)
        package = work / "Essentials.pax.gz"
        with package.open("xb") as stream:
            subprocess.run(["7z", "e", "-so", str(cd), VOLUME +
                            "System/Installation/Packages/Essentials.pkg/Contents/Resources/Essentials.pax.gz"],
                           stdout=stream, check=True)
        subprocess.run(["bsdtar", "-xzf", str(package), "-C", str(work),
                        "./System/Library/Security",
                        "./System/Library/Frameworks/QuickTime.framework",
                        "./System/Library/Frameworks/AGL.framework"], check=True)
        script = b"""#!/bin/sh
set -ex
mkdir /tmp/test-system
case "$ZR_TEST_TRANSFER_DISK" in
    /dev/disk0) zr_system=/dev/disk1s2 ;;
    /dev/disk1) zr_system=/dev/disk0s2 ;;
    *) exit 2 ;;
esac
/sbin/mount -t hfs "$zr_system" /tmp/test-system
test -f /tmp/test-system/System/Library/CoreServices/SystemVersion.plist
rm /tmp/test-system/System/Installation/Packages/Essentials.pkg/Contents/Resources/Essentials.pax.gz
for item in Security Frameworks/QuickTime.framework Frameworks/AGL.framework; do
    test ! -e "/tmp/test-system/System/Library/$item"
    cp -Rp "System/Library/$item" "/tmp/test-system/System/Library/$item"
done
sync
/sbin/umount /tmp/test-system
echo Original 10.0 installed-system files installed
"""
        system = work / "System"
        hashes = {str(p.relative_to(work)): digest(p)
                  for p in sorted(system.rglob("*")) if p.is_file()}
        provenance = json.dumps({"original_cd_sha256": CD_SHA256,
                                 "original_installed_files_sha256": hashes,
                                 "purpose": "Disposable QEMU tests; original runtime files preserved",
                                 "removed_installer_archive":
                                 "System/Installation/Packages/Essentials.pkg/Contents/Resources/Essentials.pax.gz"},
                                indent=2).encode() + b"\n"
        with output.open("xb") as stream:
            with tarfile.open(fileobj=stream, mode="w:gz", format=tarfile.USTAR_FORMAT,
                              tarinfo=LegacyTarInfo) as tar:
                # Original tar chowns a symlink's target while extracting it.
                # Install files first, then inner framework aliases before
                # aliases which traverse Versions/Current.
                paths = [system] + sorted(system.rglob("*"))
                for path in paths:
                    if not path.is_symlink():
                        tar.add(path, arcname=str(path.relative_to(work)), recursive=False)
                for path in sorted((p for p in paths if p.is_symlink()),
                                   key=lambda p: (-len(p.parts), str(p))):
                    tar.add(path, arcname=str(path.relative_to(work)), recursive=False)
                for name, data in (("test.sh", script),
                                   ("system-provenance.json", provenance)):
                    entry = tarfile.TarInfo(name)
                    entry.mode = 0o644 if name.endswith(".json") else 0o755
                    entry.size = len(data)
                    tar.addfile(entry, io.BytesIO(data))
    print("Prepared original 10.0 installed-system files")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("original_cd", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    prepare(args.original_cd, args.output)
