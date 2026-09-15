#!/usr/bin/env python3
"""Fix an overflowing integer constant in the isolated 10.6 SDK copy."""
from pathlib import Path
import sys

if len(sys.argv) != 2:
    raise SystemExit("usage: patch-sdk-10.6.py SDK_DIRECTORY")

header = (Path(sys.argv[1]) / "System/Library/Frameworks/Security.framework/Headers/SecKeychain.h")
old = "((x >> 24) | ((x >> 8) & 0xff00) | ((x << 8) & 0xff0000) | (x & 0xff) << 24)"
new = "(((unsigned)(x) >> 24) | (((unsigned)(x) >> 8) & 0xff00) | (((unsigned)(x) << 8) & 0xff0000) | ((unsigned)(x) & 0xff) << 24)"
text = header.read_bytes()
old = old.encode("ascii")
new = new.encode("ascii")
if old in text:
    header.write_bytes(text.replace(old, new))
elif new not in text:
    raise SystemExit("Unexpected SDK SecKeychain.h: refusing to patch")
