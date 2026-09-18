/* Adapt selected bundled kernels to the engine's target ABI and math policy.
 * Do not change existing ES3/ES5 transcendental implementations here. */
#ifndef zoolrunner_fdlibm_h___
#define zoolrunner_fdlibm_h___
#include "../jstypes.h"
#include "../jslibmath.h"
#include "../jsmathfd.h"

typedef union {
#ifdef IS_LITTLE_ENDIAN
    struct { JSUint32 lo; JSInt32 hi; } ints;
#else
    struct { JSInt32 hi; JSUint32 lo; } ints;
#endif
    double d;
} fd_twoints;

#define __HI(x) ((x).ints.hi)
#define __LO(x) ((x).ints.lo)
#define __ieee754_exp fd_exp
#define __ieee754_log fd_log
#define fd_expm1 js_math_expm1
#define fd_log1p js_math_log1p
#define fd_cbrt js_math_cbrt
#define fd_asinh js_math_asinh
#define fd_tanh js_math_tanh
#define __ieee754_acosh js_math_acosh
#define __ieee754_atanh js_math_atanh
#define __ieee754_cosh js_math_cosh
#define __ieee754_sinh js_math_sinh
#define __ieee754_log10 js_math_log10

#endif
