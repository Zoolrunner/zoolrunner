#!/usr/bin/env python3
"""Configure a private GCC 4 header copy for the experimental 10.0 runtime."""
from pathlib import Path
import sys


def replace_once(text, old, new):
    if text.count(old) != 1:
        raise RuntimeError("Unexpected GCC header revision: " + old[:70])
    return text.replace(old, new)


def write(path, text):
    # Atomic replacement also avoids partial reads through a container mount.
    temporary = path.with_name(path.name + ".zoolrunner-new")
    temporary.write_text(text)
    temporary.replace(path)


def patch(headers):
    bits = headers / "powerpc-apple-darwin8/bits"
    path = bits / "c++config.h"
    text = path.read_text()
    for macro in ("_GLIBCXX_USE_C99", "_GLIBCXX_USE_C99_MATH",
                  "_GLIBCXX_USE_C99_COMPLEX"):
        text = replace_once(text, "#define " + macro + " 1",
                            "/* " + macro + " unavailable in the 10.0 C library */")
    write(path, text)

    path = bits / "gthr-default.h"
    text = replace_once(path.read_text(),
                        "typedef pthread_mutex_t __gthread_recursive_mutex_t;",
                        "#include <zoolrunner-recursive-mutex.h>\n"
                        "typedef zr_recursive_mutex __gthread_recursive_mutex_t;")
    start = text.index("#if defined(PTHREAD_RECURSIVE_MUTEX_INITIALIZER)")
    end = text.index("#endif", start) + len("#endif")
    text = text[:start] + ("#define __GTHREAD_RECURSIVE_MUTEX_INIT "
                           "ZR_RECURSIVE_MUTEX_INITIALIZER") + text[end:]
    start = text.index("#ifndef PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP\n"
                       "static inline int\n__gthread_recursive_mutex_init_function")
    end = text.index("#endif /* _LIBOBJC */", start)
    text = text[:start] + """static inline int
__gthread_recursive_mutex_lock (__gthread_recursive_mutex_t *mutex)
{ return zr_recursive_lock(mutex, 0); }
static inline int
__gthread_recursive_mutex_trylock (__gthread_recursive_mutex_t *mutex)
{ return zr_recursive_lock(mutex, 1); }
static inline int
__gthread_recursive_mutex_unlock (__gthread_recursive_mutex_t *mutex)
{ return zr_recursive_unlock(mutex); }

""" + text[end:]
    write(path, text)

    path = bits / "cxxabi_tweaks.h"
    # GCC's PowerPC code calls __cxa_guard_acquire even on its ready path.
    # The published guard needs an acquire load and release store on real SMP
    # PowerPC hardware; compiler-only barriers do not provide those semantics.
    text = replace_once(path.read_text(),
        "#define _GLIBCXX_GUARD_TEST(x) (*(char *) (x) != 0)",
        '#define _GLIBCXX_GUARD_TEST(x) __extension__ ({ unsigned char zr_ready = '
        '*(volatile unsigned char *)(x); __asm __volatile ("sync" ::: "memory"); '
        'zr_ready != 0; })')
    text = replace_once(text,
        "#define _GLIBCXX_GUARD_SET(x) *(char *) (x) = 1",
        '#define _GLIBCXX_GUARD_SET(x) do { __asm __volatile ("sync" ::: "memory"); '
        '*(volatile unsigned char *)(x) = 1; } while (0)')
    write(path, text)


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit("Usage: patch-early-cxx-headers.py PRIVATE_HEADER_DIRECTORY")
    patch(Path(sys.argv[1]))
