/* ZoolRunner libjpeg-turbo internal configuration. */

#define BUILD  "20260630"

#define HIDDEN

#if defined(_MSC_VER)
#define INLINE  __inline
#elif defined(__GNUC__)
#define INLINE  __inline__
#else
#define INLINE
#endif

#define THREAD_LOCAL

#define PACKAGE_NAME  "libjpeg-turbo"
#define VERSION  "3.2.0"

#if defined(_WIN64) || defined(__LP64__) || defined(_LP64) || \
    defined(__64BIT__) || defined(__arch64__) || defined(__sparcv9)
#define SIZEOF_SIZE_T  8
#else
#define SIZEOF_SIZE_T  4
#endif

#undef HAVE_BUILTIN_CTZL
#undef HAVE_INTRIN_H

#define FALLTHROUGH

#ifndef BITS_IN_JSAMPLE
#define BITS_IN_JSAMPLE  8
#endif

#define C_ARITH_CODING_SUPPORTED  1
#define D_ARITH_CODING_SUPPORTED  1
#undef WITH_SIMD

#undef WITH_PROFILE
