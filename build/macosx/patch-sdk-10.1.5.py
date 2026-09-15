#!/usr/bin/env python3
"""Adapt obsolete declarations in a private copy of the early SDK."""
from pathlib import Path
import sys

sdk = Path(sys.argv[1])
# Mozilla uses 16-bit wchar_t internally. Darwin's rune tables retain their
# 32-bit system ABI even in a translation unit compiled with -fshort-wchar.
# These early headers incorrectly derive rune_t from the compiler's wchar_t.
header = sdk / "usr/include/ppc/ansi.h"
data = header.read_bytes()
old = b"#define _BSD_RUNE_T_    __WCHAR_TYPE__          /* rune_t */"
new = b"#define _BSD_RUNE_T_    int /* ZoolRunner: fixed Darwin rune ABI. */"
if data.count(old) == 1:
    header.write_bytes(data.replace(old, new))
elif new not in data:
    raise SystemExit("Unrecognized ppc/ansi.h rune declaration")
for name, whitespace in (("runetype.h", b" "), ("stdlib.h", b"\t")):
    header = sdk / "usr/include" / name
    data = header.read_bytes()
    old = b"typedef" + whitespace + b"_BSD_WCHAR_T_\trune_t;"
    new = b"typedef _BSD_RUNE_T_\trune_t; /* ZoolRunner: independent of wchar_t. */"
    if data.count(old) == 1:
        header.write_bytes(data.replace(old, new))
    elif new not in data:
        raise SystemExit("Unrecognized " + name + " rune declaration")

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

# The C declaration originally used an enum tag named bool. GCC's stdbool.h
# now defines bool as _Bool, making that tag invalid. Keep an enum (and its
# original integer ABI), rather than changing dyld's return type to C99 bool.
header = sdk / "usr/include/mach-o/dyld.h"
data = header.read_bytes()
old = b"#define DYLD_BOOL bool"
new = b"#define DYLD_BOOL zr_dyld_bool /* ZoolRunner: avoid the stdbool macro. */"
if data.count(old) == 1:
    header.write_bytes(data.replace(old, new))
elif new not in data:
    raise SystemExit("Unrecognized dyld.h boolean declaration")

# This header predates consistent FOUNDATION_EXPORT use and otherwise gives
# its C functions C++ linkage when included by Objective-C++ applications.
header = sdk / "System/Library/Frameworks/Foundation.framework/Headers/NSHFSFileTypes.h"
data = header.read_bytes()
for declaration in (b"NSString *NSFileTypeForHFSTypeCode(",
                    b"OSType NSHFSTypeCodeFromFileType(",
                    b"NSString *NSHFSTypeOfFile("):
    old = b"\n" + declaration
    new = b"\nFOUNDATION_EXPORT " + declaration
    if data.count(old) == 1:
        data = data.replace(old, new)
    elif new not in data:
        raise SystemExit("Unrecognized NSHFSFileTypes.h declaration")
header.write_bytes(data)

# GCC 4 does not implement Metrowerks' mac68k4byte alignment mode; ignoring
# that push leaves the later align=reset unbalanced. Express the documented
# four-byte structure packing with the push/pop pragma GCC supports.
header = sdk / "System/Library/Frameworks/Kerberos.framework/Frameworks/CredentialsCache.framework/Headers/CredentialsCache.h"
data = header.read_bytes()
for old, new in ((b"#pragma options align=mac68k4byte", b"#pragma pack(push, 4) /* ZoolRunner: GCC-compatible mac68k4byte. */"),
                 (b"#pragma options align=reset", b"#pragma pack(pop) /* ZoolRunner: restore structure packing. */")):
    if data.count(old) == 1:
        data = data.replace(old, new)
    elif new not in data:
        raise SystemExit("Unrecognized CredentialsCache.h structure packing")
header.write_bytes(data)
