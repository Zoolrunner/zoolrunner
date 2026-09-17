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
 * The Original Code is Mozilla xptcall AArch64 support.
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL").
 * ***** END LICENSE BLOCK ***** */

#include "xptcprivate.h"
#include <string.h>

#if !defined(__linux__) || !defined(__aarch64__) || defined(__AARCH64EB__) || defined(__ILP32__)
#error "This code is for Linux AArch64 LP64 only."
#endif

template<class T>
static PRUint64
NormalizeArg(const T& value)
{
    return (PRUint64)value;
}

template<>
PRUint64
NormalizeArg<float>(const float& value)
{
    PRUint64 result = 0;
    memcpy(&result, &value, sizeof(value));
    return result;
}

template<>
PRUint64
NormalizeArg<double>(const double& value)
{
    PRUint64 result = 0;
    memcpy(&result, &value, sizeof(value));
    return result;
}

template<class T>
static void
AllocArg(PRUint64*& regArgs, PRUint64* regArgsEnd, void*& stackArgs,
         const T* data)
{
    if (regArgs < regArgsEnd) {
        *regArgs++ = NormalizeArg(*data);
    } else {
        // AAPCS64 C.5/C.14-C.17: every scalar stack slot occupies 8 bytes.
        // Darwin's compact stack layout must not be used on Linux.
        PRUint64 value = NormalizeArg(*data);
        memcpy(stackArgs, &value, sizeof(value));
        stackArgs = (char*)stackArgs + sizeof(value);
    }
}

extern "C" void
invoke_copy_to_stack(PRUint64* stack, PRUint64* end,
                     PRUint32 paramCount, nsXPTCVariant* source)
{
    PRUint64* intArgs = stack;
    PRUint64* intEnd = intArgs + 8;
    PRUint64* floatArgs = intEnd;
    PRUint64* floatEnd = floatArgs + 8;
    void* stackArgs = floatEnd;

    ++intArgs; /* x0 contains the this pointer. */

    for (PRUint32 i = 0; i < paramCount; ++i, ++source) {
        if (source->IsPtrData()) {
            AllocArg(intArgs, intEnd, stackArgs, &source->ptr);
            continue;
        }

        switch (source->type) {
        case nsXPTType::T_FLOAT:
            AllocArg(floatArgs, floatEnd, stackArgs, &source->val.f); break;
        case nsXPTType::T_DOUBLE:
            AllocArg(floatArgs, floatEnd, stackArgs, &source->val.d); break;
        case nsXPTType::T_I8:
            AllocArg(intArgs, intEnd, stackArgs, &source->val.i8); break;
        case nsXPTType::T_I16:
            AllocArg(intArgs, intEnd, stackArgs, &source->val.i16); break;
        case nsXPTType::T_I32:
            AllocArg(intArgs, intEnd, stackArgs, &source->val.i32); break;
        case nsXPTType::T_I64:
            AllocArg(intArgs, intEnd, stackArgs, &source->val.i64); break;
        case nsXPTType::T_U8:
            AllocArg(intArgs, intEnd, stackArgs, &source->val.u8); break;
        case nsXPTType::T_U16:
            AllocArg(intArgs, intEnd, stackArgs, &source->val.u16); break;
        case nsXPTType::T_U32:
            AllocArg(intArgs, intEnd, stackArgs, &source->val.u32); break;
        case nsXPTType::T_U64:
            AllocArg(intArgs, intEnd, stackArgs, &source->val.u64); break;
        case nsXPTType::T_BOOL:
            AllocArg(intArgs, intEnd, stackArgs, &source->val.b); break;
        case nsXPTType::T_CHAR:
            AllocArg(intArgs, intEnd, stackArgs, &source->val.c); break;
        case nsXPTType::T_WCHAR:
            AllocArg(intArgs, intEnd, stackArgs, &source->val.wc); break;
        default:
            AllocArg(intArgs, intEnd, stackArgs, &source->val.p); break;
        }
    }

    NS_ASSERTION((PRUint64*)stackArgs <= end, "xptcall stack overflow");
}

extern "C" nsresult
_XPTC_InvokeByIndex(nsISupports* that, PRUint32 methodIndex,
                    PRUint32 paramCount, nsXPTCVariant* params);

XPTC_PUBLIC_API(nsresult)
XPTC_InvokeByIndex(nsISupports* that, PRUint32 methodIndex,
                   PRUint32 paramCount, nsXPTCVariant* params)
{
    return _XPTC_InvokeByIndex(that, methodIndex, paramCount, params);
}
