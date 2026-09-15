#!/usr/bin/env python3
"""Wrap the archived 4K78 HFS CD filesystem in an Apple partition map.

OpenBIOS can boot the mapped image. The filesystem payload is copied unchanged;
no kernel, library, boot-loader or installer file is patched. The output takes
about 565 MiB in addition to the input image. This script does not download or
redistribute the operating system.
"""
import argparse
import hashlib
from pathlib import Path
import shutil
import struct


def wrap(source, destination):
    if source.stat().st_size != 592384000:
        raise ValueError("Expected the 592384000-byte archived 10.0 CD filesystem")
    digest = hashlib.sha1()
    with source.open("rb") as image:
        for data in iter(lambda: image.read(1024 * 1024), b""):
            digest.update(data)
    if digest.hexdigest() != "23d00b0d6ec621a997b423d89dedd52db1da119b":
        raise ValueError("Input does not match the archived 4K78 CD checksum")
    blocks = source.stat().st_size // 512
    header = bytearray(64 * 512)
    struct.pack_into(">HHI", header, 0, 0x4552, 512, blocks + 64)
    partitions = (
        (1, 1, 63, b"Apple", b"Apple_partition_map"),
        (2, 64, blocks, b"Mac OS X CD", b"Apple_HFS"),
    )
    for index, start, count, name, kind in partitions:
        offset = index * 512
        struct.pack_into(">HHIII", header, offset, 0x504d, 0, 2, start, count)
        header[offset + 16:offset + 16 + len(name)] = name
        header[offset + 48:offset + 48 + len(kind)] = kind
        struct.pack_into(">III", header, offset + 80, 0, count, 0x37)
    # Exclusive creation protects both the source and existing experiment data.
    with destination.open("xb") as output, source.open("rb") as image:
        output.write(header)
        shutil.copyfileobj(image, output, 1024 * 1024)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("destination", type=Path)
    args = parser.parse_args()
    wrap(args.source, args.destination)
    print("Created APM wrapper with unchanged 10.0 filesystem payload")
