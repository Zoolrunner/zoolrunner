#!/usr/bin/env python3
"""Adapt an obsolete linkage spelling in a private copy of the early SDK."""
from pathlib import Path
import sys

sdk = Path(sys.argv[1])
header = sdk / "System/Library/Frameworks/Foundation.framework/Headers/NSJavaSetup.h"
data = header.read_bytes()
old = b'extern "Objective-C" {'
new = b'extern "C" { /* ZoolRunner: GCC 4 no longer accepts Objective-C linkage. */'
if old in data:
    if data.count(old) != 1:
        raise SystemExit("Unexpected NSJavaSetup.h linkage declarations")
    header.write_bytes(data.replace(old, new))
elif new not in data:
    raise SystemExit("Unrecognized NSJavaSetup.h; refusing to modify it")
