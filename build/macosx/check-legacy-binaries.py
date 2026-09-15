#!/usr/bin/env python3
"""Check packaged legacy Mach-O deployment metadata and library locations.

This is a static deployment check, not proof of target-OS runtime support.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess


def version(value):
    return tuple((list(map(int, value.split("."))) + [0, 0])[:3])


def check(runtime, minimum, sdk, tree=None, arch="i386", tool_prefix=""):
    tree = tree or runtime
    records = []
    for path in sorted(tree.rglob("*")):
        if not path.is_file():
            continue
        with path.open("rb") as binary:
            magic = binary.read(4)
        if magic not in (b"\xce\xfa\xed\xfe", b"\xcf\xfa\xed\xfe", b"\xfe\xed\xfa\xce",
                         b"\xca\xfe\xba\xbe", b"\xbe\xba\xfe\xca"):
            continue
        if arch == "ppc":
            # Old lipo distinguishes the G3 subtype (ppc750) from generic ppc.
            info = subprocess.check_output([tool_prefix + "lipo", "-info", str(path)],
                                           text=True)
            architectures = re.split(r"(?:architecture:|are:)\s*", info)[-1].split()
            if not any(value in ("ppc", "ppc750") for value in architectures):
                raise RuntimeError("No G3-compatible PowerPC slice: " + str(path))
        else:
            subprocess.run([tool_prefix + "lipo", str(path), "-verify_arch", arch], check=True)
        output = subprocess.check_output([tool_prefix + "otool", "-arch", arch, "-l", str(path)],
                                         text=True)
        record = {"file": str(path.relative_to(tree)), "dependencies": []}
        if version(minimum) < version("10.1"):
            header = subprocess.check_output(
                [tool_prefix + "otool", "-arch", arch, "-hv", str(path)], text=True)
            if "TWOLEVEL" in header.split():
                raise RuntimeError("Two-level namespaces require 10.1: " + str(path))
        for block in re.split(r"Load command \d+\n", output)[1:]:
            command = re.search(r"\bcmd (\S+)", block).group(1)
            if command == "LC_VERSION_MIN_MACOSX":
                value = re.search(r"\bversion (\S+)", block).group(1)
                record["minimum_os"] = value
                if version(value) > version(minimum):
                    raise RuntimeError("%s requires %s, above %s" % (path, value, minimum))
            if command in ("LC_BUILD_VERSION", "LC_DYLD_CHAINED_FIXUPS",
                           "LC_DYLD_EXPORTS_TRIE", "LC_RPATH"):
                raise RuntimeError("Unexpected legacy load command %s in %s" % (command, path))
            if command == "LC_MAIN" and version(minimum) < version("10.8"):
                raise RuntimeError("LC_MAIN requires 10.8: " + str(path))
            if command in ("LC_DYLD_INFO", "LC_DYLD_INFO_ONLY") and version(minimum) < version("10.6"):
                raise RuntimeError("Compressed dyld information requires 10.6: " + str(path))
            if command not in ("LC_LOAD_DYLIB", "LC_LOAD_WEAK_DYLIB", "LC_REEXPORT_DYLIB"):
                continue
            if command == "LC_LOAD_WEAK_DYLIB" and version(minimum) < version("10.2"):
                raise RuntimeError("Weak library loading requires 10.2: " + str(path))
            if command == "LC_REEXPORT_DYLIB" and version(minimum) < version("10.5"):
                raise RuntimeError("Library reexports require 10.5: " + str(path))
            dependency = re.search(r"\bname (.*?) \(offset", block).group(1)
            if dependency.startswith(("/System/Library/", "/usr/lib/")):
                resolved = sdk / dependency.lstrip("/")
            elif dependency.startswith("@executable_path/"):
                resolved = runtime / dependency[len("@executable_path/"):]
            elif dependency.startswith("@loader_path/"):
                if version(minimum) < version("10.4"):
                    raise RuntimeError("@loader_path requires 10.4: " + str(path))
                resolved = path.parent / dependency[len("@loader_path/"):]
            else:
                raise RuntimeError("Nonportable dependency %s in %s" % (dependency, path))
            if not dependency.startswith("/") and tree not in resolved.resolve().parents:
                raise RuntimeError("Dependency escapes runtime: " + dependency)
            if not resolved.exists():
                raise RuntimeError("Dependency absent from artifact or SDK: " + dependency)
            record["dependencies"].append(dependency)
        if "minimum_os" not in record:
            if arch == "ppc":
                # Apple's old PowerPC linker predates version load commands.
                # SDK/link checks remain useful, but must not invent metadata.
                record["minimum_os"] = None
                record["note"] = "Classic PowerPC binary; minimum OS not encoded"
                records.append(record)
                continue
            # These in-tree Java plug-ins predate version load commands.
            # Retain them, but report their minimum as unknown. Only permit
            # the exact bundled binaries, not arbitrary newly built files.
            bundled = {
                "plugins/JavaEmbeddingPlugin.bundle/Contents/MacOS/JavaEmbeddingPlugin",
                "plugins/MRJPlugin.plugin/Contents/MacOS/MRJPlugin",
            }
            relative = str(path.relative_to(runtime)) if runtime in path.parents else ""
            original = Path(__file__).resolve().parents[2] / "plugin/oji/JEP" / relative[8:]
            if (relative not in bundled or not original.is_file() or
                    hashlib.sha256(path.read_bytes()).digest() !=
                    hashlib.sha256(original.read_bytes()).digest()):
                raise RuntimeError("Missing deployment metadata: " + str(path))
            record["minimum_os"] = None
            record["note"] = "Unmodified bundled Java plug-in; minimum OS not encoded"
        records.append(record)
    if not records:
        raise RuntimeError("No Mach-O binaries found")
    return {"architecture": arch, "maximum_minimum_os": minimum,
            "runtime_tested": False, "binaries": records}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("runtime", type=Path)
    parser.add_argument("--minimum", required=True)
    parser.add_argument("--sdk", required=True, type=Path)
    parser.add_argument("--report", required=True, type=Path)
    parser.add_argument("--root", type=Path,
                        help="Audit this entire package, including plug-ins outside the runtime")
    parser.add_argument("--arch", choices=("i386", "ppc"), default="i386")
    parser.add_argument("--tool-prefix", default="",
                        help="Prefix for cross-toolchain lipo and otool")
    args = parser.parse_args()
    result = check(args.runtime.resolve(), args.minimum, args.sdk.resolve(),
                   args.root.resolve() if args.root else None,
                   args.arch, args.tool_prefix)
    args.report.write_text(json.dumps(result, indent=2) + "\n")
    print("Legacy deployment checks passed for %d binaries; runtime unverified" %
          len(result["binaries"]))
