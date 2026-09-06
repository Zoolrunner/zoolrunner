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
 * The Original Code is mozilla.org Code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1999
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
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

// Implement shared vtbl methods for the Darwin x86-64 ABI.

#include "xptcprivate.h"

// Darwin x86-64 passes the first 6 integer parameters and the first 8
// floating-point parameters in registers.  Remaining parameters are passed
// in the caller's stack area.

const PRUint32 PARAM_BUFFER_COUNT = 16;
const PRUint32 GPR_COUNT = 6;
const PRUint32 FPR_COUNT = 8;

extern "C" nsresult
PrepareAndDispatch(nsXPTCStubBase * self, PRUint32 methodIndex,
                   PRUint64 * args, PRUint64 * gpregs, double * fpregs)
{
    nsXPTCMiniVariant paramBuffer[PARAM_BUFFER_COUNT];
    nsXPTCMiniVariant * dispatchParams = NULL;
    nsIInterfaceInfo * ifaceInfo = NULL;
    const nsXPTMethodInfo * info;
    PRUint32 paramCount;
    PRUint32 i;
    nsresult result = NS_ERROR_FAILURE;

    NS_ASSERTION(self, "no self");

    self->GetInterfaceInfo(&ifaceInfo);
    NS_ASSERTION(ifaceInfo, "no interface info");
    if (!ifaceInfo)
        return NS_ERROR_UNEXPECTED;

    ifaceInfo->GetMethodInfo(PRUint16(methodIndex), &info);
    NS_ASSERTION(info, "no method info");
    if (!info) {
        NS_RELEASE(ifaceInfo);
        return NS_ERROR_UNEXPECTED;
    }

    paramCount = info->GetParamCount();

    if (paramCount > PARAM_BUFFER_COUNT)
        dispatchParams = new nsXPTCMiniVariant[paramCount];
    else
        dispatchParams = paramBuffer;

    NS_ASSERTION(dispatchParams, "no place for params");
    if (!dispatchParams) {
        NS_RELEASE(ifaceInfo);
        return NS_ERROR_OUT_OF_MEMORY;
    }

    PRUint64 * ap = args;
    PRUint32 nrGpr = 1; // skip one GPR for 'self'
    PRUint32 nrFpr = 0;
    PRUint64 value;

    for (i = 0; i < paramCount; i++) {
        const nsXPTParamInfo& param = info->GetParam(i);
        const nsXPTType& type = param.GetType();
        nsXPTCMiniVariant * dp = &dispatchParams[i];

        if (!param.IsOut() && type == nsXPTType::T_DOUBLE) {
            if (nrFpr < FPR_COUNT)
                dp->val.d = fpregs[nrFpr++];
            else
                dp->val.d = *(double *)ap++;
            continue;
        }

        if (!param.IsOut() && type == nsXPTType::T_FLOAT) {
            if (nrFpr < FPR_COUNT) {
                // Only the low 32 bits of the SSE register contain the float.
                dp->val.f = *(float *)&fpregs[nrFpr++];
            } else {
                dp->val.f = *(float *)ap++;
            }
            continue;
        }

        if (nrGpr < GPR_COUNT)
            value = gpregs[nrGpr++];
        else
            value = *ap++;

        if (param.IsOut() || !type.IsArithmetic()) {
            dp->val.p = (void *)value;
            continue;
        }

        switch (type) {
        case nsXPTType::T_I8:    dp->val.i8 = (PRInt8)value; break;
        case nsXPTType::T_I16:   dp->val.i16 = (PRInt16)value; break;
        case nsXPTType::T_I32:   dp->val.i32 = (PRInt32)value; break;
        case nsXPTType::T_I64:   dp->val.i64 = (PRInt64)value; break;
        case nsXPTType::T_U8:    dp->val.u8 = (PRUint8)value; break;
        case nsXPTType::T_U16:   dp->val.u16 = (PRUint16)value; break;
        case nsXPTType::T_U32:   dp->val.u32 = (PRUint32)value; break;
        case nsXPTType::T_U64:   dp->val.u64 = (PRUint64)value; break;
        case nsXPTType::T_BOOL:  dp->val.b = (PRBool)value; break;
        case nsXPTType::T_CHAR:  dp->val.c = (char)value; break;
        case nsXPTType::T_WCHAR: dp->val.wc = (wchar_t)value; break;
        default:
            NS_ASSERTION(0, "bad type");
            break;
        }
    }

    result = self->CallMethod((PRUint16)methodIndex, info, dispatchParams);

    NS_RELEASE(ifaceInfo);

    if (dispatchParams != paramBuffer)
        delete [] dispatchParams;

    return result;
}

#if defined(__GXX_ABI_VERSION) && __GXX_ABI_VERSION >= 100

#define STUB_ENTRY(n) \
asm(".section\t__TEXT,__text,regular,pure_instructions\n\t" \
    ".align\t2\n\t" \
    ".if\t" #n " < 10\n\t" \
    ".globl\t__ZN14nsXPTCStubBase5Stub" #n "Ev\n\t" \
    "__ZN14nsXPTCStubBase5Stub" #n "Ev:\n\t" \
    ".elseif\t" #n " < 100\n\t" \
    ".globl\t__ZN14nsXPTCStubBase6Stub" #n "Ev\n\t" \
    "__ZN14nsXPTCStubBase6Stub" #n "Ev:\n\t" \
    ".elseif\t" #n " < 1000\n\t" \
    ".globl\t__ZN14nsXPTCStubBase7Stub" #n "Ev\n\t" \
    "__ZN14nsXPTCStubBase7Stub" #n "Ev:\n\t" \
    ".else\n\t" \
    ".err\t\"stub number " #n " >= 1000 not yet supported\"\n\t" \
    ".endif\n\t" \
    "movl\t$" #n ", %eax\n\t" \
    "jmp\tSharedStub\n\t");

asm(".section\t__TEXT,__text,regular,pure_instructions\n\t"
    ".align\t2\n\t"
    "SharedStub:\n\t"
    "pushq\t%rbp\n\t"
    "movq\t%rsp,%rbp\n\t"
    // gpregs (48 bytes) followed by fpregs (64 bytes)
    "subq\t$112,%rsp\n\t"
    "movq\t%rdi,-112(%rbp)\n\t"
    "movq\t%rsi,-104(%rbp)\n\t"
    "movq\t%rdx, -96(%rbp)\n\t"
    "movq\t%rcx, -88(%rbp)\n\t"
    "movq\t%r8,  -80(%rbp)\n\t"
    "movq\t%r9,  -72(%rbp)\n\t"
    "leaq\t-112(%rbp),%rcx\n\t"
    "movsd\t%xmm0,-64(%rbp)\n\t"
    "movsd\t%xmm1,-56(%rbp)\n\t"
    "movsd\t%xmm2,-48(%rbp)\n\t"
    "movsd\t%xmm3,-40(%rbp)\n\t"
    "movsd\t%xmm4,-32(%rbp)\n\t"
    "movsd\t%xmm5,-24(%rbp)\n\t"
    "movsd\t%xmm6,-16(%rbp)\n\t"
    "movsd\t%xmm7, -8(%rbp)\n\t"
    "leaq\t-64(%rbp),%r8\n\t"
    // rdi already contains self
    "movl\t%eax,%esi\n\t"
    "leaq\t16(%rbp),%rdx\n\t"
    "call\t_PrepareAndDispatch\n\t"
    "leave\n\t"
    "ret\n\t");

#define SENTINEL_ENTRY(n) \
nsresult nsXPTCStubBase::Sentinel##n() \
{ \
    NS_ASSERTION(0, "nsXPTCStubBase::Sentinel called"); \
    return NS_ERROR_NOT_IMPLEMENTED; \
}

#include "xptcstubsdef.inc"

#else
#error "Unsupported compiler for Darwin x86-64."
#endif
