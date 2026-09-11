/* SPDX-License-Identifier: MIT */
/* C99 integer and boolean definitions for Microsoft compilers before VS 2010. */
#ifndef EXPAT_MSVC_COMPAT_H
#define EXPAT_MSVC_COMPAT_H

#if defined(_MSC_VER) && _MSC_VER < 1600
#  include <stddef.h>
#  include <float.h>
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef signed short int16_t;
typedef unsigned short uint16_t;
typedef signed int int32_t;
typedef unsigned int uint32_t;
typedef signed __int64 int64_t;
typedef unsigned __int64 uint64_t;
typedef __w64 signed int intptr_t;
typedef __w64 unsigned int uintptr_t;
#  define UINT64_MAX ((uint64_t)0xffffffffffffffffui64)
#  ifndef SIZE_MAX
#    define SIZE_MAX ((size_t)-1)
#  endif
#  ifndef isnan
#    define isnan _isnan
#  endif
#  ifndef __cplusplus
typedef unsigned char bool;
#    define true 1
#    define false 0
#  endif
#else
#  include <stdbool.h>
#  include <stdint.h>
#endif

#endif /* EXPAT_MSVC_COMPAT_H */
