/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

/*
 * JS math package.
 */
#include "jsstddef.h"
#include "jslibmath.h"
#include <stdlib.h>
#include "jstypes.h"
#include "jslong.h"
#include "prmjtime.h"
#include "jsapi.h"
#include "jsfun.h"
#include "jsatom.h"
#include "jscntxt.h"
#include "jsconfig.h"
#include "jslock.h"
#include "jsmath.h"
#include "jsmathfd.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jssymbol.h"

#ifndef M_E
#define M_E             2.7182818284590452354
#endif
#ifndef M_LOG2E
#define M_LOG2E         1.4426950408889634074
#endif
#ifndef M_LOG10E
#define M_LOG10E        0.43429448190325182765
#endif
#ifndef M_LN2
#define M_LN2           0.69314718055994530942
#endif
#ifndef M_LN10
#define M_LN10          2.30258509299404568402
#endif
#ifndef M_PI
#define M_PI            3.14159265358979323846
#endif
#ifndef M_SQRT2
#define M_SQRT2         1.41421356237309504880
#endif
#ifndef M_SQRT1_2
#define M_SQRT1_2       0.70710678118654752440
#endif

static JSConstDoubleSpec math_constants[] = {
    {M_E,       "E",            0, {0,0,0}},
    {M_LOG2E,   "LOG2E",        0, {0,0,0}},
    {M_LOG10E,  "LOG10E",       0, {0,0,0}},
    {M_LN2,     "LN2",          0, {0,0,0}},
    {M_LN10,    "LN10",         0, {0,0,0}},
    {M_PI,      "PI",           0, {0,0,0}},
    {M_SQRT2,   "SQRT2",        0, {0,0,0}},
    {M_SQRT1_2, "SQRT1_2",      0, {0,0,0}},
    {0,0,0,{0,0,0}}
};

JSClass js_MathClass = {
    js_Math_str,
    JSCLASS_HAS_CACHED_PROTO(JSProto_Math),
    JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub,   JS_ConvertStub,   JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSBool
math_abs(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    jsdouble x, z;

    if (!js_ValueToNumber(cx, argv[0], &x))
        return JS_FALSE;
    z = fd_fabs(x);
    return js_NewNumberValue(cx, z, rval);
}

static JSBool
math_acos(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    jsdouble x, z;

    if (!js_ValueToNumber(cx, argv[0], &x))
        return JS_FALSE;
    z = fd_acos(x);
    return js_NewNumberValue(cx, z, rval);
}

static JSBool
math_asin(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    jsdouble x, z;

    if (!js_ValueToNumber(cx, argv[0], &x))
        return JS_FALSE;
    z = fd_asin(x);
    return js_NewNumberValue(cx, z, rval);
}

static JSBool
math_atan(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    jsdouble x, z;

    if (!js_ValueToNumber(cx, argv[0], &x))
        return JS_FALSE;
    z = fd_atan(x);
    return js_NewNumberValue(cx, z, rval);
}

static JSBool
math_atan2(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    jsdouble x, y, z;

    if (!js_ValueToNumber(cx, argv[0], &x))
        return JS_FALSE;
    if (!js_ValueToNumber(cx, argv[1], &y))
        return JS_FALSE;
#if !JS_USE_FDLIBM_MATH && defined(_MSC_VER)
    /*
     * MSVC's atan2 does not yield the result demanded by ECMA when both x
     * and y are infinite.
     * - The result is a multiple of pi/4.
     * - The sign of x determines the sign of the result.
     * - The sign of y determines the multiplicator, 1 or 3.
     */
    if (JSDOUBLE_IS_INFINITE(x) && JSDOUBLE_IS_INFINITE(y)) {
        z = fd_copysign(M_PI / 4, x);
        if (y < 0)
            z *= 3;
        return js_NewDoubleValue(cx, z, rval);
    }
#endif
    z = fd_atan2(x, y);
    return js_NewNumberValue(cx, z, rval);
}

static JSBool
math_ceil(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    jsdouble x, z;

    if (!js_ValueToNumber(cx, argv[0], &x))
        return JS_FALSE;
    z = fd_ceil(x);
    return js_NewNumberValue(cx, z, rval);
}

static JSBool
math_cos(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    jsdouble x, z;

    if (!js_ValueToNumber(cx, argv[0], &x))
        return JS_FALSE;
    z = fd_cos(x);
    return js_NewNumberValue(cx, z, rval);
}

static JSBool
math_exp(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    jsdouble x, z;

    if (!js_ValueToNumber(cx, argv[0], &x))
        return JS_FALSE;
#ifdef _WIN32
    if (!JSDOUBLE_IS_NaN(x)) {
        if (x == *cx->runtime->jsPositiveInfinity) {
            *rval = DOUBLE_TO_JSVAL(cx->runtime->jsPositiveInfinity);
            return JS_TRUE;
        }
        if (x == *cx->runtime->jsNegativeInfinity) {
            *rval = JSVAL_ZERO;
            return JS_TRUE;
        }
    }
#endif
    z = fd_exp(x);
    return js_NewNumberValue(cx, z, rval);
}

static JSBool
math_floor(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    jsdouble x, z;

    if (!js_ValueToNumber(cx, argv[0], &x))
        return JS_FALSE;
    z = fd_floor(x);
    return js_NewNumberValue(cx, z, rval);
}

static JSBool
math_log(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    jsdouble x, z;

    if (!js_ValueToNumber(cx, argv[0], &x))
        return JS_FALSE;
    z = fd_log(x);
    return js_NewNumberValue(cx, z, rval);
}

static JSBool
math_max(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    jsdouble x, z = *cx->runtime->jsNegativeInfinity;
    uintN i;
    JSBool sawNaN = JS_FALSE;
    JSBool standard = JSVERSION_NUMBER(cx) == JSVERSION_DEFAULT ||
                      JS_VERSION_IS_ES2015(cx);

    if (argc == 0) {
        *rval = DOUBLE_TO_JSVAL(cx->runtime->jsNegativeInfinity);
        return JS_TRUE;
    }
    for (i = 0; i < argc; i++) {
        if (!js_ValueToNumber(cx, argv[i], &x))
            return JS_FALSE;
        if (JSDOUBLE_IS_NaN(x)) {
            /* Standard editions still convert subsequent arguments. Preserve
             * explicit legacy callers' historical early return. */
            if (!standard) {
                *rval = DOUBLE_TO_JSVAL(cx->runtime->jsNaN);
                return JS_TRUE;
            }
            sawNaN = JS_TRUE;
            continue;
        }
        if (x == 0 && x == z && fd_copysign(1.0, z) == -1)
            z = x;
        else
            z = (x > z) ? x : z;
    }
    if (sawNaN) {
        *rval = DOUBLE_TO_JSVAL(cx->runtime->jsNaN);
        return JS_TRUE;
    }
    return js_NewNumberValue(cx, z, rval);
}

static JSBool
math_min(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    jsdouble x, z = *cx->runtime->jsPositiveInfinity;
    uintN i;
    JSBool sawNaN = JS_FALSE;
    JSBool standard = JSVERSION_NUMBER(cx) == JSVERSION_DEFAULT ||
                      JS_VERSION_IS_ES2015(cx);

    if (argc == 0) {
        *rval = DOUBLE_TO_JSVAL(cx->runtime->jsPositiveInfinity);
        return JS_TRUE;
    }
    for (i = 0; i < argc; i++) {
        if (!js_ValueToNumber(cx, argv[i], &x))
            return JS_FALSE;
        if (JSDOUBLE_IS_NaN(x)) {
            /* Standard editions still convert subsequent arguments. Preserve
             * explicit legacy callers' historical early return. */
            if (!standard) {
                *rval = DOUBLE_TO_JSVAL(cx->runtime->jsNaN);
                return JS_TRUE;
            }
            sawNaN = JS_TRUE;
            continue;
        }
        if (x == 0 && x == z && fd_copysign(1.0,x) == -1)
            z = x;
        else
            z = (x < z) ? x : z;
    }
    if (sawNaN) {
        *rval = DOUBLE_TO_JSVAL(cx->runtime->jsNaN);
        return JS_TRUE;
    }
    return js_NewNumberValue(cx, z, rval);
}

static JSBool
math_pow(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    jsdouble x, y, z;

    if (!js_ValueToNumber(cx, argv[0], &x))
        return JS_FALSE;
    if (!js_ValueToNumber(cx, argv[1], &y))
        return JS_FALSE;
#if !JS_USE_FDLIBM_MATH
    /*
     * Because C99 and ECMA specify different behavior for pow(),
     * we need to wrap the libm call to make it ECMA compliant.
     */
    if (!JSDOUBLE_IS_FINITE(y) && (x == 1.0 || x == -1.0)) {
        *rval = DOUBLE_TO_JSVAL(cx->runtime->jsNaN);
        return JS_TRUE;
    }
    /* pow(x, +-0) is always 1, even for x = NaN. */
    if (y == 0) {
        *rval = JSVAL_ONE;
        return JS_TRUE;
    }
#endif
    z = fd_pow(x, y);
    return js_NewNumberValue(cx, z, rval);
}

/*
 * Math.random() support, lifted from java.util.Random.java.
 */
static void
random_setSeed(JSRuntime *rt, int64 seed)
{
    int64 tmp;

    JSLL_I2L(tmp, 1000);
    JSLL_DIV(seed, seed, tmp);
    JSLL_XOR(tmp, seed, rt->rngMultiplier);
    JSLL_AND(rt->rngSeed, tmp, rt->rngMask);
}

static void
random_init(JSRuntime *rt)
{
    int64 tmp, tmp2;

    /* Do at most once. */
    if (rt->rngInitialized)
        return;
    rt->rngInitialized = JS_TRUE;

    /* rt->rngMultiplier = 0x5DEECE66DL */
    JSLL_ISHL(tmp, 0x5, 32);
    JSLL_UI2L(tmp2, 0xDEECE66DL);
    JSLL_OR(rt->rngMultiplier, tmp, tmp2);

    /* rt->rngAddend = 0xBL */
    JSLL_I2L(rt->rngAddend, 0xBL);

    /* rt->rngMask = (1L << 48) - 1 */
    JSLL_I2L(tmp, 1);
    JSLL_SHL(tmp2, tmp, 48);
    JSLL_SUB(rt->rngMask, tmp2, tmp);

    /* rt->rngDscale = (jsdouble)(1L << 53) */
    JSLL_SHL(tmp2, tmp, 53);
    JSLL_L2D(rt->rngDscale, tmp2);

    /* Finally, set the seed from current time. */
    random_setSeed(rt, PRMJ_Now());
}

static uint32
random_next(JSRuntime *rt, int bits)
{
    int64 nextseed, tmp;
    uint32 retval;

    JSLL_MUL(nextseed, rt->rngSeed, rt->rngMultiplier);
    JSLL_ADD(nextseed, nextseed, rt->rngAddend);
    JSLL_AND(nextseed, nextseed, rt->rngMask);
    rt->rngSeed = nextseed;
    JSLL_USHR(tmp, nextseed, 48 - bits);
    JSLL_L2I(retval, tmp);
    return retval;
}

static jsdouble
random_nextDouble(JSRuntime *rt)
{
    int64 tmp, tmp2;
    jsdouble d;

    JSLL_ISHL(tmp, random_next(rt, 26), 27);
    JSLL_UI2L(tmp2, random_next(rt, 27));
    JSLL_ADD(tmp, tmp, tmp2);
    JSLL_L2D(d, tmp);
    return d / rt->rngDscale;
}

static JSBool
math_random(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSRuntime *rt;
    jsdouble z;

    rt = cx->runtime;
    JS_LOCK_RUNTIME(rt);
    random_init(rt);
    z = random_nextDouble(rt);
    JS_UNLOCK_RUNTIME(rt);
    return js_NewNumberValue(cx, z, rval);
}

#if defined _WIN32 && !defined WINCE && _MSC_VER < 1400
/* Try to work around apparent _copysign bustage in VC6 and VC7. */
double
js_copysign(double x, double y)
{
    jsdpun xu, yu;

    xu.d = x;
    yu.d = y;
    xu.s.hi &= ~JSDOUBLE_HI32_SIGNBIT;
    xu.s.hi |= yu.s.hi & JSDOUBLE_HI32_SIGNBIT;
    return xu.d;
}
#endif

static JSBool
math_round(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    jsdouble x, z;

    if (!js_ValueToNumber(cx, argv[0], &x))
        return JS_FALSE;
    /* Adding 0.5 first can round across a boundary before floor sees it.
     * Values at least 2^52 are already integral. Preserve NaN and signed zero. */
    if (!JSDOUBLE_IS_FINITE(x) || x == 0 || fd_fabs(x) >= 4503599627370496.0) {
        z = x;
    } else {
        z = fd_floor(x);
        if (x - z >= 0.5)
            z += 1.0;
        z = fd_copysign(z, x);
    }
    return js_NewNumberValue(cx, z, rval);
}

static JSBool
math_sign(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    jsdouble x;

    if (!js_ValueToNumber(cx, argv[0], &x))
        return JS_FALSE;
    if (x != 0 && !JSDOUBLE_IS_NaN(x))
        x = x < 0 ? -1 : 1;
    return js_NewNumberValue(cx, x, rval);
}

static JSBool
math_trunc(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    jsdouble x;

    if (!js_ValueToNumber(cx, argv[0], &x))
        return JS_FALSE;
    return js_NewNumberValue(cx, fd_copysign(fd_floor(fd_fabs(x)), x), rval);
}

static JSBool
math_clz32(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    uint32 x, bit;
    jsint count;

    if (!js_ValueToECMAUint32(cx, argv[0], &x))
        return JS_FALSE;
    count = 0;
    for (bit = (uint32)0x80000000; bit && !(x & bit); bit >>= 1)
        ++count;
    *rval = INT_TO_JSVAL(count);
    return JS_TRUE;
}

static JSBool
math_imul(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    uint32 x, y, product;
    jsdouble result;

    if (!js_ValueToECMAUint32(cx, argv[0], &x) ||
        !js_ValueToECMAUint32(cx, argv[1], &y))
        return JS_FALSE;
    /* Unsigned multiplication supplies the required modulo 2^32 result.
     * Convert the sign without an implementation-defined unsigned cast. */
    product = x * y;
    result = (jsdouble)product;
    if (product & (uint32)0x80000000)
        result -= 4294967296.0;
    return js_NewNumberValue(cx, result, rval);
}

/* The new transcendental kernels use the bundled fdlibm algorithms. */
#define MATH_KERNEL_WRAPPER(name)                                             \
static JSBool                                                                \
math_##name(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)  \
{                                                                            \
    jsdouble x;                                                              \
    if (!js_ValueToNumber(cx, argv[0], &x))                                    \
        return JS_FALSE;                                                     \
    return js_NewNumberValue(cx, js_math_##name(x), rval);                     \
}

MATH_KERNEL_WRAPPER(expm1)
MATH_KERNEL_WRAPPER(log1p)
MATH_KERNEL_WRAPPER(cbrt)
MATH_KERNEL_WRAPPER(asinh)
MATH_KERNEL_WRAPPER(tanh)
MATH_KERNEL_WRAPPER(acosh)
MATH_KERNEL_WRAPPER(atanh)
MATH_KERNEL_WRAPPER(cosh)
MATH_KERNEL_WRAPPER(sinh)
MATH_KERNEL_WRAPPER(log10)
#undef MATH_KERNEL_WRAPPER

static JSBool
math_log2(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    jsdouble x, fraction, result;
    int exponent;

    if (!js_ValueToNumber(cx, argv[0], &x))
        return JS_FALSE;
    if (x > 0 && JSDOUBLE_IS_FINITE(x)) {
        fraction = frexp(x, &exponent);
        /* Powers of two, including subnormals, have exact integer results. */
        result = fraction == 0.5 ? (jsdouble)(exponent - 1)
                                : fd_log(x) * M_LOG2E;
    } else {
        result = fd_log(x);
    }
    return js_NewNumberValue(cx, result, rval);
}

jsdouble
js_RoundToFloat32(JSContext *cx, jsdouble x)
{
    jsdouble magnitude, fraction, scaled, integral, result;
    int exponent, shift;

    if (!JSDOUBLE_IS_FINITE(x) || x == 0)
        return x;
    magnitude = fd_fabs(x);
    fraction = frexp(magnitude, &exponent);
    if (exponent > 128) {
        result = *cx->runtime->jsPositiveInfinity;
    } else {
        /* Round the binary32 significand explicitly. This avoids dependence
         * on a host float cast's overflow behavior or x87 excess precision. */
        if (exponent < -125) {
            scaled = ldexp(magnitude, 149);
            shift = -149;
        } else {
            scaled = ldexp(fraction, 24);
            shift = exponent - 24;
        }
        integral = fd_floor(scaled);
        fraction = scaled - integral;
        if (fraction > 0.5 || (fraction == 0.5 && ((uint32)integral & 1)))
            integral += 1.0;
        result = ldexp(integral, shift);
        if (result >= 340282366920938463463374607431768211456.0) /* 2^128 */
            result = *cx->runtime->jsPositiveInfinity;
    }
    return fd_copysign(result, x);
}

static JSBool
math_fround(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    jsdouble x;
    if (!js_ValueToNumber(cx, argv[0], &x))
        return JS_FALSE;
    return js_NewNumberValue(cx, js_RoundToFloat32(cx, x), rval);
}

static JSBool
math_hypot(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    jsdouble x, maximum, sum, correction, ratio, term, adjusted, next, result;
    JSBool infinite, nan;
    uintN i;

    maximum = sum = correction = 0;
    infinite = nan = JS_FALSE;
    for (i = 0; i < argc; ++i) {
        /* Every conversion is observable, even after infinity or NaN. */
        if (!js_ValueToNumber(cx, argv[i], &x))
            return JS_FALSE;
        if (JSDOUBLE_IS_NaN(x)) {
            nan = JS_TRUE;
            continue;
        }
        if (!JSDOUBLE_IS_FINITE(x)) {
            infinite = JS_TRUE;
            continue;
        }
        x = fd_fabs(x);
        if (x == 0)
            continue;
        if (x > maximum) {
            ratio = maximum / x;
            sum *= ratio * ratio;
            correction *= ratio * ratio;
            maximum = x;
            term = 1;
        } else {
            ratio = x / maximum;
            term = ratio * ratio;
        }
        adjusted = term - correction;
        next = sum + adjusted;
        correction = (next - sum) - adjusted;
        sum = next;
    }
    if (infinite)
        result = *cx->runtime->jsPositiveInfinity;
    else if (nan)
        result = *cx->runtime->jsNaN;
    else
        result = maximum * fd_sqrt(sum);
    return js_NewNumberValue(cx, result, rval);
}

static JSBool
math_sin(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    jsdouble x, z;

    if (!js_ValueToNumber(cx, argv[0], &x))
        return JS_FALSE;
    z = fd_sin(x);
    return js_NewNumberValue(cx, z, rval);
}

static JSBool
math_sqrt(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    jsdouble x, z;

    if (!js_ValueToNumber(cx, argv[0], &x))
        return JS_FALSE;
    z = fd_sqrt(x);
    return js_NewNumberValue(cx, z, rval);
}

static JSBool
math_tan(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    jsdouble x, z;

    if (!js_ValueToNumber(cx, argv[0], &x))
        return JS_FALSE;
    z = fd_tan(x);
    return js_NewNumberValue(cx, z, rval);
}

#if JS_HAS_TOSOURCE
static JSBool
math_toSource(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
              jsval *rval)
{
    *rval = ATOM_KEY(CLASS_ATOM(cx, Math));
    return JS_TRUE;
}
#endif

static JSFunctionSpec math_static_methods[] = {
#if JS_HAS_TOSOURCE
    {js_toSource_str,   math_toSource,          0, 0, 0},
#endif
    {"abs",             math_abs,               1, 0, 0},
    {"acos",            math_acos,              1, 0, 0},
    {"asin",            math_asin,              1, 0, 0},
    {"atan",            math_atan,              1, 0, 0},
    {"atan2",           math_atan2,             2, 0, 0},
    {"ceil",            math_ceil,              1, 0, 0},
    {"clz32",           math_clz32,             1, 0, 0},
    {"cos",             math_cos,               1, 0, 0},
    {"exp",             math_exp,               1, 0, 0},
    {"floor",           math_floor,             1, 0, 0},
    {"imul",            math_imul,              2, 0, 0},
    {"log",             math_log,               1, 0, 0},
    {"max",             math_max,               2, 0, 0},
    {"min",             math_min,               2, 0, 0},
    {"pow",             math_pow,               2, 0, 0},
    {"random",          math_random,            0, 0, 0},
    {"round",           math_round,             1, 0, 0},
    {"sign",            math_sign,              1, 0, 0},
    {"sin",             math_sin,               1, 0, 0},
    {"sqrt",            math_sqrt,              1, 0, 0},
    {"tan",             math_tan,               1, 0, 0},
    {"trunc",           math_trunc,             1, 0, 0},
    {"expm1",            math_expm1,             1, 0, 0},
    {"log1p",            math_log1p,             1, 0, 0},
    {"cbrt",             math_cbrt,              1, 0, 0},
    {"asinh",            math_asinh,             1, 0, 0},
    {"tanh",             math_tanh,              1, 0, 0},
    {"acosh",            math_acosh,             1, 0, 0},
    {"atanh",            math_atanh,             1, 0, 0},
    {"cosh",             math_cosh,              1, 0, 0},
    {"sinh",             math_sinh,              1, 0, 0},
    {"log10",            math_log10,             1, 0, 0},
    {"log2",             math_log2,              1, 0, 0},
    {"fround",           math_fround,            1, 0, 0},
    {"hypot",            math_hypot,             2, 0, 0},
    {0,0,0,0,0}
};

JSObject *
js_InitMathClass(JSContext *cx, JSObject *obj)
{
    JSObject *Math, *proto = NULL;
    JSBool standard = JSVERSION_NUMBER(cx) == JSVERSION_DEFAULT ||
                      JS_VERSION_IS_ES2015(cx);

    /* A non-constructor intrinsic must not resolve its own class as its
     * prototype. Eager initialization otherwise recursively creates Math. */
    if (standard &&
        (!js_GetClassPrototype(cx, obj, INT_TO_JSID(JSProto_Object), &proto) ||
         !proto))
        return NULL;
    Math = JS_DefineObject(cx, obj, js_Math_str, &js_MathClass, proto, 0);
    if (!Math)
        return NULL;
    if (!JS_DefineFunctions(cx, Math, math_static_methods) ||
        !js_SetBuiltinMethodFlags(cx, Math, math_static_methods, JSFUN_NO_CONSTRUCT))
        return NULL;
    if (!JS_DefineConstDoubles(cx, Math, math_constants) ||
        !js_DefineBuiltinTag(cx, Math, "Math") ||
        (standard && !js_SetClassObject(cx, obj, JSProto_Math, Math)))
        return NULL;
    return Math;
}
