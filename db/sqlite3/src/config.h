#ifndef _zoolrunner_sqlite3_config_h
#define _zoolrunner_sqlite3_config_h

#include "prcpucfg.h"

#define HAVE_USLEEP 1

#if defined(__APPLE__)
#include <AvailabilityMacros.h>
#if MAC_OS_X_VERSION_MIN_REQUIRED < 1040
/* Panther has no uuid_t/gethostuuid interface for Apple's proxy locking.
 * Use SQLite's ordinary POSIX file locking on these systems. */
#define SQLITE_ENABLE_LOCKING_STYLE 0
#endif
#if MAC_OS_X_VERSION_MIN_REQUIRED < 1020
#define SQLITE_ZOOLRUNNER_EARLY_DARWIN 1
/* The early SDK has no nanosleep declaration; select the supported API. */
#define HAVE_NANOSLEEP 0
#define SQLITE_WITHOUT_ZONEMALLOC 1
#define SQLITE_MEMORY_BARRIER __asm__ __volatile__("sync" ::: "memory")
#ifndef INFINITY
#define INFINITY HUGE_VAL
#endif
#endif
#endif

#if defined(_MSC_VER) && _MSC_VER < 1600
#undef HAVE_STDINT_H
#ifndef UINT64_C
#define UINT64_C(value) value##ui64
#endif
#ifndef INFINITY
#define INFINITY HUGE_VAL
#endif
#else
#define HAVE_STDINT_H 1
#endif

#if defined(_MSC_VER) && _MSC_VER < 1400
#define SQLITE_DISABLE_INTRINSIC 1
#define SQLITE_OMIT_SEH 1
#define SQLITE_WIN32_NO_WIDE 1
#define SQLITE_OS_WINNT 0
#define SQLITE_OMIT_WAL 1
#define SQLITE_MAX_MMAP_SIZE 0
#endif

#endif /* _zoolrunner_sqlite3_config_h */
