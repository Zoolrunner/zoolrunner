#!/usr/bin/env python3
"""List PowerPC imports absent from a runtime and an original-library overlay.

This is an exploratory API work list, not a loader or deployment validator.
An export elsewhere in the overlay does not establish that a binary links it.
Objective-C methods and symbols looked up dynamically are not covered.
"""
import argparse
import json
from pathlib import Path
import re
import subprocess


def binaries(root):
    seen = set()
    for path in sorted(root.rglob("*")):
        if not path.is_file():
            continue
        real = path.resolve()
        if real in seen:
            continue
        seen.add(real)
        with path.open("rb") as binary:
            magic = binary.read(4)
        if magic in (b"\xfe\xed\xfa\xce", b"\xca\xfe\xba\xbe"):
            yield path


def symbols(path, nm):
    output = subprocess.check_output([nm, "-arch", "ppc", "-g", str(path)],
                                     text=True)
    defined, undefined = set(), set()
    for line in output.splitlines():
        # Classic nm lacks -U. Parse its address/type/name format instead,
        # ignoring archive/module headings and other descriptive lines.
        match = re.fullmatch(r"\s*(?:[0-9a-fA-F]+\s+)?([A-Za-z?])\s+(\S+)", line)
        if match:
            kind, name = match.groups()
            if kind == "U":
                undefined.add(name)
            elif kind.isupper() and kind != "?":
                defined.add(name)
    return defined, undefined


def survey(runtime, libraries, nm):
    system_exports = set()
    for path in binaries(libraries):
        defined, _ = symbols(path, nm)
        system_exports.update(defined)
    application_exports = set()
    imports = {}
    for path in binaries(runtime):
        defined, undefined = symbols(path, nm)
        application_exports.update(defined)
        imports[str(path.relative_to(runtime))] = undefined
    if not system_exports or not imports:
        raise RuntimeError("Both paths must contain PowerPC Mach-O binaries")
    candidates = {}
    for path, undefined in imports.items():
        missing = sorted(undefined - system_exports - application_exports)
        if missing:
            candidates[path] = missing
    return {"runtime_tested": False, "binding_validated": False,
            "runtime": str(runtime), "libraries": str(libraries),
            "system_export_count": len(system_exports),
            "application_binary_count": len(imports),
            "unavailable_symbol_candidates": candidates}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("runtime", type=Path)
    parser.add_argument("libraries", type=Path)
    parser.add_argument("--nm", required=True)
    parser.add_argument("--report", type=Path, required=True)
    args = parser.parse_args()
    result = survey(args.runtime.resolve(), args.libraries.resolve(), args.nm)
    args.report.write_text(json.dumps(result, indent=2) + "\n")
    print("Surveyed %d runtime binaries; %d have unavailable-symbol candidates" %
          (result["application_binary_count"],
           len(result["unavailable_symbol_candidates"])))
