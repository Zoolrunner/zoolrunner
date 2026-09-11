#ifndef _zoolrunner_sqlite3_config_h
#define _zoolrunner_sqlite3_config_h

#include "prcpucfg.h"

#define HAVE_USLEEP 1

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
