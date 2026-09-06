/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied.
 *
 * The Original Code is Mozilla xptcall Darwin x86-64 support.
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL").
 * ***** END LICENSE BLOCK ***** */

#include "xptcprivate.h"
#include <stddef.h>
#include <string.h>

#if !defined(__APPLE__) || !defined(__x86_64__)
#error "This code is for Darwin x86-64 only."
#endif

const PRUint32 GPR_COUNT = 6;
const PRUint32 FPR_COUNT = 8;

/* Keep this layout in sync with xptcinvoke_asm_x86_64_darwin.s. */
struct XPTCInvokeData
{
    PRUint64 method;
    PRUint64 gpregs[GPR_COUNT];
    PRUint64 fpregs[FPR_COUNT];
    PRUint64* stack;
    PRUint32 stackCount;
};

typedef char XPTCInvokeMethodOffset[(offsetof(XPTCInvokeData, method) == 0) ? 1 : -1];
typedef char XPTCInvokeGPROffset[(offsetof(XPTCInvokeData, gpregs) == 8) ? 1 : -1];
typedef char XPTCInvokeFPROffset[(offsetof(XPTCInvokeData, fpregs) == 56) ? 1 : -1];
typedef char XPTCInvokeStackOffset[(offsetof(XPTCInvokeData, stack) == 120) ? 1 : -1];
typedef char XPTCInvokeCountOffset[(offsetof(XPTCInvokeData, stackCount) == 128) ? 1 : -1];

extern "C" nsresult
xptc_invoke_x86_64_darwin(const XPTCInvokeData* data);

static void
invoke_count_words(PRUint32 paramCount, nsXPTCVariant* source,
                   PRUint32& nrGpr, PRUint32& nrFpr, PRUint32& nrStack)
{
    nrGpr = 1; /* The first integer register contains the this pointer. */
    nrFpr = 0;
    nrStack = 0;

    for (PRUint32 i = 0; i < paramCount; ++i, ++source) {
        if (!source->IsPtrData() &&
            (source->type == nsXPTType::T_FLOAT ||
             source->type == nsXPTType::T_DOUBLE)) {
            if (nrFpr < FPR_COUNT)
                ++nrFpr;
            else
                ++nrStack;
        } else {
            if (nrGpr < GPR_COUNT)
                ++nrGpr;
            else
                ++nrStack;
        }
    }
}

static PRUint64
float_bits(const float& value)
{
    PRUint64 result = 0;
    memcpy(&result, &value, sizeof(value));
    return result;
}

static PRUint64
double_bits(const double& value)
{
    PRUint64 result;
    memcpy(&result, &value, sizeof(value));
    return result;
}

static PRUint64
integer_value(const nsXPTCVariant& source)
{
    if (source.IsPtrData())
        return (PRUint64)source.ptr;

    switch (source.type) {
    case nsXPTType::T_I8:    return (PRUint64)source.val.i8;
    case nsXPTType::T_I16:   return (PRUint64)source.val.i16;
    case nsXPTType::T_I32:   return (PRUint64)source.val.i32;
    case nsXPTType::T_I64:   return (PRUint64)source.val.i64;
    case nsXPTType::T_U8:    return (PRUint64)source.val.u8;
    case nsXPTType::T_U16:   return (PRUint64)source.val.u16;
    case nsXPTType::T_U32:   return (PRUint64)source.val.u32;
    case nsXPTType::T_U64:   return (PRUint64)source.val.u64;
    case nsXPTType::T_BOOL:  return (PRUint64)source.val.b;
    case nsXPTType::T_CHAR:  return (PRUint64)source.val.c;
    case nsXPTType::T_WCHAR: return (PRUint64)source.val.wc;
    default:                 return (PRUint64)source.val.p;
    }
}

static void
invoke_copy_arguments(XPTCInvokeData& data, PRUint32 paramCount,
                      nsXPTCVariant* source)
{
    PRUint32 nrGpr = 1;
    PRUint32 nrFpr = 0;
    PRUint64* stack = data.stack;

    for (PRUint32 i = 0; i < paramCount; ++i, ++source) {
        if (!source->IsPtrData() && source->type == nsXPTType::T_DOUBLE) {
            PRUint64 value = double_bits(source->val.d);
            if (nrFpr < FPR_COUNT)
                data.fpregs[nrFpr++] = value;
            else
                *stack++ = value;
        } else if (!source->IsPtrData() &&
                   source->type == nsXPTType::T_FLOAT) {
            PRUint64 value = float_bits(source->val.f);
            if (nrFpr < FPR_COUNT)
                data.fpregs[nrFpr++] = value;
            else
                *stack++ = value;
        } else {
            PRUint64 value = integer_value(*source);
            if (nrGpr < GPR_COUNT)
                data.gpregs[nrGpr++] = value;
            else
                *stack++ = value;
        }
    }
}

XPTC_PUBLIC_API(nsresult)
XPTC_InvokeByIndex(nsISupports* that, PRUint32 methodIndex,
                   PRUint32 paramCount, nsXPTCVariant* params)
{
    PRUint32 nrGpr;
    PRUint32 nrFpr;
    PRUint32 nrStack;
    XPTCInvokeData data;

    invoke_count_words(paramCount, params, nrGpr, nrFpr, nrStack);

    /* The stack must be 16-byte aligned at the indirect call instruction. */
    nrStack = (nrStack + 1) & ~PRUint32(1);
    data.stack = nrStack
        ? (PRUint64*)__builtin_alloca(nrStack * sizeof(PRUint64))
        : nsnull;
    data.stackCount = nrStack;
    memset(data.gpregs, 0, sizeof(data.gpregs));
    memset(data.fpregs, 0, sizeof(data.fpregs));
    if (data.stack)
        memset(data.stack, 0, nrStack * sizeof(PRUint64));

    data.gpregs[0] = (PRUint64)that;
    invoke_copy_arguments(data, paramCount, params);
    data.gpregs[0] = (PRUint64)that;
    data.method = (*(PRUint64**)that)[methodIndex];

    return xptc_invoke_x86_64_darwin(&data);
}
