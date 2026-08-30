/* ZoolRunner libjpeg-turbo configuration. */

#ifndef JCONFIG_INCLUDED
#define JCONFIG_INCLUDED

/* Preserve the historical libjpeg 6b-facing API/ABI expected by Mozilla. */
#define JPEG_LIB_VERSION  62

/* libjpeg-turbo 3.2.0 */
#define LIBJPEG_TURBO_VERSION  3.2.0
#define LIBJPEG_TURBO_VERSION_NUMBER  3002000

/* Keep optional arithmetic coding enabled; this is no longer patent-limited. */
#define C_ARITH_CODING_SUPPORTED  1
#define D_ARITH_CODING_SUPPORTED  1

/* libjpeg-turbo's in-memory source/destination managers are internal API
 * additions and do not alter ZoolRunner's existing JPEG decoder interface.
 */
#define MEM_SRCDST_SUPPORTED  1

/* SIMD is intentionally disabled in the default bundled build. */
#undef WITH_SIMD

#ifndef BITS_IN_JSAMPLE
#define BITS_IN_JSAMPLE  8
#endif

#ifdef _WIN32

#undef RIGHT_SHIFT_IS_UNSIGNED

#ifndef __RPCNDR_H__
typedef unsigned char boolean;
#endif
#define HAVE_BOOLEAN

#if !(defined(_BASETSD_H_) || defined(_BASETSD_H))
typedef short INT16;
typedef signed int INT32;
#endif
#define XMD_H

#else

#undef RIGHT_SHIFT_IS_UNSIGNED

#endif

#endif /* JCONFIG_INCLUDED */
