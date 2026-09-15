#!/usr/bin/env python3
"""Build the experimental 10.0 library overlay in Linux, without mounting disks."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import plistlib
import shutil
import subprocess
import tempfile

CD_SHA256 = "e6fbc5c9ae633637777b5f2a9b9be3d8e3779cde02e2c7a2bec5dcc453951dc3"
VOLUME = "Mac OS X Install CD"
MACH_MAGICS = (b"\xfe\xed\xfa\xce", b"\xce\xfa\xed\xfe",
               b"\xca\xfe\xba\xbe", b"\xbe\xba\xfe\xca")


def digest(path):
    result = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            result.update(block)
    return result.hexdigest()


def macho(path):
    if path.is_symlink() or not path.is_file():
        return False
    with path.open("rb") as source:
        return source.read(4) in MACH_MAGICS


def replace_link(path, target):
    if path.is_symlink() or path.exists():
        path.unlink()
    path.parent.mkdir(parents=True, exist_ok=True)
    path.symlink_to(target)


def copy_headers(source, destination):
    # Do not stat newly copied symlinks: some container bind mounts follow them
    # during metadata operations before the target has been copied.
    destination.mkdir()
    for directory, dirs, files in os.walk(source):
        base = Path(directory)
        target = destination / base.relative_to(source)
        for name in dirs + files:
            path = base / name
            if path.is_symlink():
                (target / name).symlink_to(os.readlink(path))
            elif path.is_dir():
                (target / name).mkdir()
            else:
                shutil.copyfile(path, target / name)


def prepare(cd, headers, output):
    if digest(cd) != CD_SHA256:
        raise SystemExit("This tool requires the pinned original 10.0 build 4K78 CD")
    if output.exists():
        raise SystemExit("Refusing to overwrite SDK output: " + str(output))
    # Headers must first be fetched and patched by fetch-sdk.sh, which verifies
    # the 10.1.5 archive. They are explicitly later headers, not a 10.0 SDK.
    if not (headers / "System/Library/Frameworks/Foundation.framework/Headers/NSJavaSetup.h").exists():
        raise SystemExit("Missing prepared 10.1.5 SDK headers")
    with tempfile.TemporaryDirectory(prefix="zool-10.0-sdk-") as temporary:
        work = Path(temporary)
        extracted = work / "cd"
        extracted.mkdir()
        result = subprocess.run([
            "7z", "x", "-y", "-o" + str(extracted), str(cd),
            VOLUME + "/usr/lib/*", VOLUME + "/System/Library/Frameworks/*",
            VOLUME + "/System/Library/PrivateFrameworks/*",
            VOLUME + "/System/Library/CoreServices/SystemVersion.plist",
            VOLUME + "/System/Installation/Packages/Essentials.pkg/Contents/Resources/Essentials.pax.gz",
        ], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        # Keep the extractor's link protection enabled. These two links in the
        # pinned image are recovered explicitly below, inside the overlay.
        allowed_errors = {
            "ERROR: Dangerous link via another link was ignored : " + VOLUME +
            "/usr/lib/libIOKit.dylib : libIOKit.A.dylib",
            "ERROR: Dangerous link path was ignored : " + VOLUME +
            "/System/Library/Frameworks/System.framework/Versions/B/System : "
            "../../../../../../usr/lib/libSystem.B.dylib",
        }
        errors = {line.strip() for line in result.stdout.splitlines()
                  if line.startswith("ERROR:")}
        if result.returncode and (result.returncode != 2 or not errors or
                                  not errors.issubset(allowed_errors)):
            raise SystemExit(result.stdout)
        root = extracted / VOLUME
        version = plistlib.loads((root / "System/Library/CoreServices/SystemVersion.plist").read_bytes())
        if version.get("ProductVersion") != "10.0" or version.get("ProductBuildVersion") != "4K78":
            raise SystemExit("Unexpected original OS version")
        essentials = work / "essentials"
        essentials.mkdir()
        subprocess.run([
            "bsdtar", "-xzf", str(root / "System/Installation/Packages/Essentials.pkg/Contents/Resources/Essentials.pax.gz"),
            "-C", str(essentials), "./System/Library/Frameworks/AGL.framework",
            "./System/Library/Frameworks/QuickTime.framework",
        ], check=True)
        copy_headers(headers, output)
        removed = []
        for path in output.rglob("*"):
            if macho(path):
                removed.append(str(path.relative_to(output)))
                path.unlink()
        binaries = {}
        for source_root in (root, essentials):
            for path in source_root.rglob("*"):
                relative = path.relative_to(source_root)
                destination = output / relative
                if macho(path):
                    destination.parent.mkdir(parents=True, exist_ok=True)
                    if destination.is_symlink() or destination.exists():
                        destination.unlink()
                    shutil.copyfile(path, destination)
                    binaries[str(relative)] = digest(destination)
                elif path.is_symlink():
                    # Read the original link bytes: 7z rewrites absolute links
                    # during extraction. Never adopt host-dependent targets.
                    target = os.readlink(path)
                    if source_root == root and target.startswith("/"):
                        target = subprocess.check_output([
                            "7z", "e", "-so", str(cd), VOLUME + "/" + str(relative)
                        ]).decode()
                    if target.startswith("/"):
                        target = os.path.relpath(output / target.lstrip("/"), destination.parent)
                    normalized = Path(os.path.normpath(str(destination.parent / target)))
                    if os.path.commonpath((output, normalized)) != str(output):
                        raise SystemExit("Link leaves SDK: " + str(relative))
                    replace_link(destination, target)
        replace_link(output / "usr/lib/libIOKit.dylib", "libIOKit.A.dylib")
        replace_link(output / "System/Library/Frameworks/System.framework/Versions/B/System",
                     "../../../../../../usr/lib/libSystem.B.dylib")
        (output / "PROVENANCE.json").write_text(json.dumps({
            "headers": "checksum-pinned MacOSX10.1.5 SDK with documented linkage, enum-tag and structure-packing adaptations",
            "libraries": "original Mac OS X 10.0 build 4K78 CD and its Essentials package",
            "cd_sha256": CD_SHA256, "removed_sdk_binaries": sorted(removed),
            "original_binaries_sha256": dict(sorted(binaries.items())),
            "runtime_compatibility_established": False,
        }, indent=2) + "\n")
        print("Prepared original-library overlay with %d binaries; later headers remain experimental" % len(binaries))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("original_cd", type=Path)
    parser.add_argument("prepared_10_1_5_sdk", type=Path)
    parser.add_argument("new_output", type=Path)
    args = parser.parse_args()
    prepare(args.original_cd.resolve(), args.prepared_10_1_5_sdk.resolve(), args.new_output.absolute())
