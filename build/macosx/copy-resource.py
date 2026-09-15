#!/usr/bin/env python3
"""Copy a source-verified classic Mac resource on hosts without Rez/SDP."""
import hashlib
import json
from pathlib import Path
import shutil
import sys


def main():
    root = Path(__file__).resolve().parents[2]
    directory = root / "config/macos/resources"
    name, destination = sys.argv[1:]
    record = json.loads((directory / "manifest.json").read_text())[name]
    for path, checksum in ((root / record["source"], record["source_sha256"]),
                           (directory / name, record["resource_sha256"])):
        if hashlib.sha256(path.read_bytes()).hexdigest() != checksum:
            raise SystemExit("Stale resource: regenerate on macOS; see "
                             "config/macos/resources/README.md: " + str(path))
    shutil.copyfile(directory / name, destination)


if __name__ == "__main__":
    main()
