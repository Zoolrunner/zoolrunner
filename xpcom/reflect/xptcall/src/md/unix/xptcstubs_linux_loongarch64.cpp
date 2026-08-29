/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* ***** BEGIN LICENSE BLOCK *****
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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1999
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of any one of the MPL, the
 * GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

/* Implement shared vtbl methods. */

#include "xptcprivate.h"

#if !defined(LINUX) || !defined(__loongarch64)
#error "This code is for Linux LoongArch64 only."
#endif

const PRUint32 PARAM_BUFFER_COUNT = 16;
const PRUint32 GPR_COUNT = 7;
const PRUint32 FPR_COUNT = 8;

union FPRValue
{
    float f;
    double d;
};

extern "C" nsresult
PrepareAndDispatch(nsXPTCStubBase* self, PRUint32 methodIndex,
                   PRUint64* gprData, FPRValue* fprData,
                   PRUint64* stackData)
{
    nsXPTCMiniVariant paramBuffer[PARAM_BUFFER_COUNT];
    nsXPTCMiniVariant* dispatchParams = nsnull;
    nsIInterfaceInfo* iface_info = nsnull;
    const nsXPTMethodInfo* info;
    PRUint32 paramCount;
    PRUint32 nr_gpr = 0;
    PRUint32 nr_fpr = 0;
    PRUint32 nr_stack = 0;
    nsresult result = NS_ERROR_FAILURE;

    NS_ASSERTION(self, "no self");

    self->GetInterfaceInfo(&iface_info);
    NS_ASSERTION(iface_info, "no interface info");
    if (!iface_info)
        return NS_ERROR_UNEXPECTED;

    iface_info->GetMethodInfo(PRUint16(methodIndex), &info);
    NS_ASSERTION(info, "no method info");
    if (!info) {
        NS_RELEASE(iface_info);
        return NS_ERROR_UNEXPECTED;
    }

    paramCount = info->GetParamCount();
    dispatchParams = paramCount > PARAM_BUFFER_COUNT
                   ? new nsXPTCMiniVariant[paramCount]
                   : paramBuffer;
    if (!dispatchParams) {
        NS_RELEASE(iface_info);
        return NS_ERROR_OUT_OF_MEMORY;
    }

    for (PRUint32 i = 0; i < paramCount; i++) {
        const nsXPTParamInfo& param = info->GetParam(i);
        const nsXPTType& type = param.GetType();
        nsXPTCMiniVariant* dp = &dispatchParams[i];
        PRUint64 value = 0;

        if (!param.IsOut() && type == nsXPTType::T_DOUBLE) {
            if (nr_fpr < FPR_COUNT)
                dp->val.d = fprData[nr_fpr++].d;
            else
                dp->val.d = *((double*) &stackData[nr_stack++]);
            continue;
        }

        if (!param.IsOut() && type == nsXPTType::T_FLOAT) {
            if (nr_fpr < FPR_COUNT)
                dp->val.f = fprData[nr_fpr++].f;
            else
                dp->val.f = *((float*) &stackData[nr_stack++]);
            continue;
        }

        if (nr_gpr < GPR_COUNT)
            value = gprData[nr_gpr++];
        else
            value = stackData[nr_stack++];

        if (param.IsOut() || !type.IsArithmetic()) {
            dp->val.p = (void*) value;
            continue;
        }

        switch (type) {
        case nsXPTType::T_I8:     dp->val.i8  = (PRInt8) value;    break;
        case nsXPTType::T_I16:    dp->val.i16 = (PRInt16) value;   break;
        case nsXPTType::T_I32:    dp->val.i32 = (PRInt32) value;   break;
        case nsXPTType::T_I64:    dp->val.i64 = (PRInt64) value;   break;
        case nsXPTType::T_U8:     dp->val.u8  = (PRUint8) value;   break;
        case nsXPTType::T_U16:    dp->val.u16 = (PRUint16) value;  break;
        case nsXPTType::T_U32:    dp->val.u32 = (PRUint32) value;  break;
        case nsXPTType::T_U64:    dp->val.u64 = (PRUint64) value;  break;
        case nsXPTType::T_BOOL:   dp->val.b   = (PRBool) value;    break;
        case nsXPTType::T_CHAR:   dp->val.c   = (char) value;      break;
        case nsXPTType::T_WCHAR:  dp->val.wc  = (PRUnichar) value; break;
        default:
            NS_ASSERTION(0, "bad type");
            break;
        }
    }

    result = self->CallMethod((PRUint16) methodIndex, info, dispatchParams);

    NS_RELEASE(iface_info);

    if (dispatchParams != paramBuffer)
        delete [] dispatchParams;

    return result;
}

#if defined(__GXX_ABI_VERSION) && __GXX_ABI_VERSION >= 100
#define STUB_ENTRY(n) \
asm(".text\n" \
    ".align 2\n" \
    ".if " #n " < 10\n" \
    ".globl _ZN14nsXPTCStubBase5Stub" #n "Ev\n" \
    ".type _ZN14nsXPTCStubBase5Stub" #n "Ev, @function\n" \
    "_ZN14nsXPTCStubBase5Stub" #n "Ev:\n" \
    ".elseif " #n " < 100\n" \
    ".globl _ZN14nsXPTCStubBase6Stub" #n "Ev\n" \
    ".type _ZN14nsXPTCStubBase6Stub" #n "Ev, @function\n" \
    "_ZN14nsXPTCStubBase6Stub" #n "Ev:\n" \
    ".elseif " #n " < 1000\n" \
    ".globl _ZN14nsXPTCStubBase7Stub" #n "Ev\n" \
    ".type _ZN14nsXPTCStubBase7Stub" #n "Ev, @function\n" \
    "_ZN14nsXPTCStubBase7Stub" #n "Ev:\n" \
    ".else\n" \
    ".err \"stub number " #n " >= 1000 not yet supported\"\n" \
    ".endif\n" \
    "  addi.w $t0, $zero, " #n "\n" \
    "  b SharedStub\n");
#else
#error "Unsupported C++ ABI for LoongArch64 xptcall."
#endif

asm(
".text\n"
".align 2\n"
".type SharedStub, @function\n"
"SharedStub:\n"
"  addi.d  $sp, $sp, -160\n"
"  st.d    $ra, $sp, 152\n"
"  st.d    $fp, $sp, 144\n"
"  addi.d  $fp, $sp, 160\n"
"  st.d    $a1, $sp, 0\n"
"  st.d    $a2, $sp, 8\n"
"  st.d    $a3, $sp, 16\n"
"  st.d    $a4, $sp, 24\n"
"  st.d    $a5, $sp, 32\n"
"  st.d    $a6, $sp, 40\n"
"  st.d    $a7, $sp, 48\n"
"  fst.d   $fa0, $sp, 56\n"
"  fst.d   $fa1, $sp, 64\n"
"  fst.d   $fa2, $sp, 72\n"
"  fst.d   $fa3, $sp, 80\n"
"  fst.d   $fa4, $sp, 88\n"
"  fst.d   $fa5, $sp, 96\n"
"  fst.d   $fa6, $sp, 104\n"
"  fst.d   $fa7, $sp, 112\n"
"  or      $a1, $t0, $zero\n"
"  or      $a2, $sp, $zero\n"
"  addi.d  $a3, $sp, 56\n"
"  or      $a4, $fp, $zero\n"
"  bl      PrepareAndDispatch\n"
"  ld.d    $ra, $sp, 152\n"
"  ld.d    $fp, $sp, 144\n"
"  addi.d  $sp, $sp, 160\n"
"  jr      $ra\n"
".size SharedStub, .-SharedStub\n"
);

#define SENTINEL_ENTRY(n) \
nsresult nsXPTCStubBase::Sentinel##n() \
{ \
    NS_ASSERTION(0, "nsXPTCStubBase::Sentinel called"); \
    return NS_ERROR_NOT_IMPLEMENTED; \
}

#include "xptcstubsdef.inc"
