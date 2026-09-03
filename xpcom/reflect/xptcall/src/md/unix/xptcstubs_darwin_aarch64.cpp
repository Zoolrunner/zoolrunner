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

#if !defined(__APPLE__) || !(defined(__arm64__) || defined(__aarch64__))
#error "This code is for Darwin AArch64 only."
#endif

static const PRUint32 PARAM_BUFFER_COUNT = 16;
static const PRUint32 GPR_COUNT = 8;
static const PRUint32 FPR_COUNT = 8;

template<class T>
static void
GetStackValue(T* result, void*& stack)
{
    const size_t alignment = sizeof(T);
    PRUptrdiff address = ((PRUptrdiff)stack + alignment - 1) &
                         ~(PRUptrdiff)(alignment - 1);
    memcpy(result, (void*)address, sizeof(T));
    stack = (void*)(address + alignment);
}

extern "C" nsresult
PrepareAndDispatch(nsXPTCStubBase* self, PRUint32 methodIndex, void* stackData,
                   PRUint64* gprData, double* fprData)
{
    nsXPTCMiniVariant localBuffer[PARAM_BUFFER_COUNT];
    nsXPTCMiniVariant* dispatchParams;
    nsIInterfaceInfo* interfaceInfo = nsnull;
    const nsXPTMethodInfo* info = nsnull;
    PRUint32 gpr = 1; /* gprData[0] is the this pointer. */
    PRUint32 fpr = 0;
    nsresult result;

    NS_ASSERTION(self, "no self");
    self->GetInterfaceInfo(&interfaceInfo);
    if (!interfaceInfo)
        return NS_ERROR_UNEXPECTED;

    interfaceInfo->GetMethodInfo((PRUint16)methodIndex, &info);
    if (!info) {
        NS_RELEASE(interfaceInfo);
        return NS_ERROR_UNEXPECTED;
    }

    PRUint32 paramCount = info->GetParamCount();
    dispatchParams = paramCount > PARAM_BUFFER_COUNT
                   ? new nsXPTCMiniVariant[paramCount] : localBuffer;
    if (!dispatchParams) {
        NS_RELEASE(interfaceInfo);
        return NS_ERROR_OUT_OF_MEMORY;
    }

    void* stack = stackData;
    for (PRUint32 i = 0; i < paramCount; ++i) {
        const nsXPTParamInfo& param = info->GetParam(i);
        const nsXPTType& type = param.GetType();
        nsXPTCMiniVariant* destination = &dispatchParams[i];

        if (param.IsOut() || !type.IsArithmetic()) {
            if (gpr < GPR_COUNT)
                destination->val.p = (void*)gprData[gpr++];
            else
                GetStackValue(&destination->val.p, stack);
            continue;
        }

        switch (type) {
        case nsXPTType::T_FLOAT:
            if (fpr < FPR_COUNT)
                memcpy(&destination->val.f, &fprData[fpr++], sizeof(float));
            else
                GetStackValue(&destination->val.f, stack);
            break;
        case nsXPTType::T_DOUBLE:
            if (fpr < FPR_COUNT)
                memcpy(&destination->val.d, &fprData[fpr++], sizeof(double));
            else
                GetStackValue(&destination->val.d, stack);
            break;
        case nsXPTType::T_I8:
            if (gpr < GPR_COUNT) destination->val.i8 = (PRInt8)gprData[gpr++];
            else GetStackValue(&destination->val.i8, stack);
            break;
        case nsXPTType::T_I16:
            if (gpr < GPR_COUNT) destination->val.i16 = (PRInt16)gprData[gpr++];
            else GetStackValue(&destination->val.i16, stack);
            break;
        case nsXPTType::T_I32:
            if (gpr < GPR_COUNT) destination->val.i32 = (PRInt32)gprData[gpr++];
            else GetStackValue(&destination->val.i32, stack);
            break;
        case nsXPTType::T_I64:
            if (gpr < GPR_COUNT) destination->val.i64 = (PRInt64)gprData[gpr++];
            else GetStackValue(&destination->val.i64, stack);
            break;
        case nsXPTType::T_U8:
            if (gpr < GPR_COUNT) destination->val.u8 = (PRUint8)gprData[gpr++];
            else GetStackValue(&destination->val.u8, stack);
            break;
        case nsXPTType::T_U16:
            if (gpr < GPR_COUNT) destination->val.u16 = (PRUint16)gprData[gpr++];
            else GetStackValue(&destination->val.u16, stack);
            break;
        case nsXPTType::T_U32:
            if (gpr < GPR_COUNT) destination->val.u32 = (PRUint32)gprData[gpr++];
            else GetStackValue(&destination->val.u32, stack);
            break;
        case nsXPTType::T_U64:
            if (gpr < GPR_COUNT) destination->val.u64 = (PRUint64)gprData[gpr++];
            else GetStackValue(&destination->val.u64, stack);
            break;
        case nsXPTType::T_BOOL:
            if (gpr < GPR_COUNT) destination->val.b = (PRBool)gprData[gpr++];
            else GetStackValue(&destination->val.b, stack);
            break;
        case nsXPTType::T_CHAR:
            if (gpr < GPR_COUNT) destination->val.c = (char)gprData[gpr++];
            else GetStackValue(&destination->val.c, stack);
            break;
        case nsXPTType::T_WCHAR:
            if (gpr < GPR_COUNT) destination->val.wc = (PRUnichar)gprData[gpr++];
            else GetStackValue(&destination->val.wc, stack);
            break;
        default:
            NS_ASSERTION(0, "bad type");
            break;
        }
    }

    result = self->CallMethod((PRUint16)methodIndex, info, dispatchParams);
    NS_RELEASE(interfaceInfo);
    if (dispatchParams != localBuffer)
        delete [] dispatchParams;
    return result;
}

#if defined(__GXX_ABI_VERSION) && __GXX_ABI_VERSION >= 100
#define STUB_ENTRY(n)                                                   \
__asm__(".text\n\t"                                                    \
        ".align 2\n\t"                                                 \
        ".if " #n " < 10\n\t"                                        \
        ".globl __ZN14nsXPTCStubBase5Stub" #n "Ev\n\t"                \
        "__ZN14nsXPTCStubBase5Stub" #n "Ev:\n\t"                       \
        ".elseif " #n " < 100\n\t"                                   \
        ".globl __ZN14nsXPTCStubBase6Stub" #n "Ev\n\t"                \
        "__ZN14nsXPTCStubBase6Stub" #n "Ev:\n\t"                       \
        ".elseif " #n " < 1000\n\t"                                  \
        ".globl __ZN14nsXPTCStubBase7Stub" #n "Ev\n\t"                \
        "__ZN14nsXPTCStubBase7Stub" #n "Ev:\n\t"                       \
        ".else\n\t.err \"stub number too large\"\n\t.endif\n\t"          \
        "mov w17, #" #n "\n\tb SharedStub\n\t");
#else
#error "Unsupported C++ ABI for Darwin AArch64 xptcall."
#endif

#define SENTINEL_ENTRY(n)                         \
nsresult nsXPTCStubBase::Sentinel##n()             \
{                                                  \
    NS_ASSERTION(0, "nsXPTCStubBase::Sentinel called"); \
    return NS_ERROR_NOT_IMPLEMENTED;               \
}

#include "xptcstubsdef.inc"
