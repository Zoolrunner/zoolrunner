#!/usr/bin/env python3
"""Create disposable APM/HFS test media on Linux without mounts or privileges.

The first filesystem contains short-name input archives and a guest script.
The second partition is blank: the original guest's newfs_hfs creates its own
HFS+ scratch filesystem, so no modern host filesystem features are required.
"""
import argparse
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile
from hfsutils_lock import hfsutils_lock


def create(output, files, scratch_mb, home_mb=0):
    if output.exists():
        raise ValueError("Refusing to overwrite " + str(output))
    for name, source in files:
        if not source.is_file() or len(name) > 31 or ":" in name or "/" in name:
            raise ValueError("Invalid HFS transfer file: " + str(source))
    total = sum(source.stat().st_size for _, source in files)
    transfer_bytes = max(16, (total * 5 // 4 + 8*1024*1024) // (1024*1024) + 1) * 1024*1024
    if not 64 <= scratch_mb <= 2048:
        raise ValueError("Scratch size must be between 64 and 2048 MiB")
    if home_mb and not 16 <= home_mb <= 512:
        raise ValueError("Home size must be zero or between 16 and 512 MiB")
    home_blocks = home_mb * 2048
    transfer_blocks = transfer_bytes // 512
    scratch_blocks = scratch_mb * 2048
    header = bytearray(64 * 512)
    struct.pack_into(">HHI", header, 0, 0x4552, 512,
                     64 + transfer_blocks + scratch_blocks + home_blocks)
    parts = (
        (1, 1, 63, b"Apple", b"Apple_partition_map"),
        (2, 64, transfer_blocks, b"ZoolTransfer", b"Apple_HFS"),
        (3, 64 + transfer_blocks, scratch_blocks, b"ZoolScratch", b"Apple_HFS"),
    )
    if home_mb:
        parts += ((4, 64 + transfer_blocks + scratch_blocks, home_blocks,
                   b"ZoolHome", b"Apple_HFS"),)
    for index, start, count, name, kind in parts:
        offset = index * 512
        struct.pack_into(">HHIII", header, offset, 0x504d, 0, len(parts), start, count)
        header[offset+16:offset+16+len(name)] = name
        header[offset+48:offset+48+len(kind)] = kind
        struct.pack_into(">III", header, offset+80, 0, count, 0x37)
    with tempfile.TemporaryDirectory(prefix="zool-hfs-") as work:
        transfer = Path(work) / "transfer.hfs"
        with transfer.open("xb") as stream:
            stream.truncate(transfer_bytes)
        with hfsutils_lock():
            subprocess.run(["hformat", "-l", "ZoolTransfer", str(transfer)], check=True)
            try:
                for name, source in files:
                    subprocess.run(["hcopy", "-r", str(source), ":" + name], check=True)
            finally:
                subprocess.run(["humount"], check=True)
        with output.open("xb") as stream, transfer.open("rb") as source:
            stream.write(header)
            shutil.copyfileobj(source, stream, 1024*1024)
            stream.truncate((64 + transfer_blocks + scratch_blocks + home_blocks) * 512)
    print("Created disposable test disk: %d MiB transfer + %d MiB scratch + %d MiB home" %
          (transfer_bytes // (1024*1024), scratch_mb, home_mb))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("--file", action="append", nargs=2, metavar=("HFS_NAME", "SOURCE"), required=True)
    parser.add_argument("--scratch-mb", type=int, default=512)
    parser.add_argument("--home-mb", type=int, default=0)
    args = parser.parse_args()
    create(args.output, [(name, Path(source)) for name, source in args.file], args.scratch_mb, args.home_mb)
