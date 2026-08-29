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
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

/* Platform specific code to invoke XPCOM methods on native objects. */

#include "xptcprivate.h"

#if !defined(LINUX) || !defined(__loongarch64)
#error "This code is for Linux LoongArch64 only."
#endif

const PRUint32 GPR_COUNT = 8;
const PRUint32 FPR_COUNT = 8;

union FPRValue
{
    float f;
    double d;
};

extern "C" PRUint32
_xptc_loongarch64_invoke(PRUint64 methodAddress, PRUint64* gprData,
                         FPRValue* fprData, PRUint64* stackData,
                         PRUint32 stackCount);

static PRUint32
invoke_copy_to_stack(PRUint64* stackData, PRUint64* gprData,
                     FPRValue* fprData, PRUint32 paramCount,
                     nsXPTCVariant* s)
{
    PRUint32 nr_gpr = 1; /* skip a0 for 'that' */
    PRUint32 nr_fpr = 0;
    PRUint32 nr_stack = 0;
    PRUint64 value = 0;

    for (PRUint32 i = 0; i < paramCount; i++, s++) {
        if (s->IsPtrData()) {
            value = (PRUint64) s->ptr;
        } else {
            switch (s->type) {
            case nsXPTType::T_FLOAT:
                if (nr_fpr < FPR_COUNT) {
                    fprData[nr_fpr++].f = s->val.f;
                    continue;
                }
                break;
            case nsXPTType::T_DOUBLE:
                if (nr_fpr < FPR_COUNT) {
                    fprData[nr_fpr++].d = s->val.d;
                    continue;
                }
                break;
            case nsXPTType::T_I8:     value = s->val.i8;  break;
            case nsXPTType::T_I16:    value = s->val.i16; break;
            case nsXPTType::T_I32:    value = s->val.i32; break;
            case nsXPTType::T_I64:    value = s->val.i64; break;
            case nsXPTType::T_U8:     value = s->val.u8;  break;
            case nsXPTType::T_U16:    value = s->val.u16; break;
            case nsXPTType::T_U32:    value = s->val.u32; break;
            case nsXPTType::T_U64:    value = s->val.u64; break;
            case nsXPTType::T_BOOL:   value = s->val.b;   break;
            case nsXPTType::T_CHAR:   value = s->val.c;   break;
            case nsXPTType::T_WCHAR:  value = s->val.wc;  break;
            default:                  value = (PRUint64) s->val.p; break;
            }
        }

        if (nr_gpr < GPR_COUNT)
            gprData[nr_gpr++] = value;
        else
            stackData[nr_stack++] = value;
    }

    return nr_stack;
}

XPTC_PUBLIC_API(nsresult)
XPTC_InvokeByIndex(nsISupports* that, PRUint32 methodIndex,
                   PRUint32 paramCount, nsXPTCVariant* params)
{
    PRUint64* vtable = *((PRUint64**) that);
    PRUint64 methodAddress;
    PRUint32 stackCount;
    PRUint32 alignedStackCount;
    PRUint64* stackData;
    PRUint64 gprData[GPR_COUNT];
    FPRValue fprData[FPR_COUNT];

#if defined(__GXX_ABI_VERSION) && __GXX_ABI_VERSION >= 100
    methodAddress = vtable[methodIndex];
#else
    methodAddress = vtable[methodIndex + 2];
#endif

    stackData = (PRUint64*) __builtin_alloca((paramCount + 1) *
                                             sizeof(PRUint64));
    for (PRUint32 i = 0; i < GPR_COUNT; i++)
        gprData[i] = 0;
    for (PRUint32 j = 0; j < FPR_COUNT; j++)
        fprData[j].d = 0.0;

    gprData[0] = (PRUint64) that;
    stackCount = invoke_copy_to_stack(stackData, gprData, fprData,
                                      paramCount, params);
    alignedStackCount = (stackCount + 1) & ~1;

    return (nsresult) _xptc_loongarch64_invoke(methodAddress, gprData,
                                               fprData, stackData,
                                               alignedStackCount);
}

asm(
".text\n"
".align 2\n"
".globl _xptc_loongarch64_invoke\n"
".type _xptc_loongarch64_invoke, @function\n"
"_xptc_loongarch64_invoke:\n"
"  addi.d  $sp, $sp, -48\n"
"  st.d    $ra, $sp, 40\n"
"  st.d    $fp, $sp, 32\n"
"  addi.d  $fp, $sp, 48\n"
"  slli.d  $t3, $a4, 3\n"
"  sub.d   $sp, $sp, $t3\n"
"  beqz    $a4, 2f\n"
"  or      $t4, $sp, $zero\n"
"1:\n"
"  ld.d    $t5, $a3, 0\n"
"  st.d    $t5, $t4, 0\n"
"  addi.d  $a3, $a3, 8\n"
"  addi.d  $t4, $t4, 8\n"
"  addi.w  $a4, $a4, -1\n"
"  bnez    $a4, 1b\n"
"2:\n"
"  or      $t0, $a0, $zero\n"
"  or      $t1, $a1, $zero\n"
"  or      $t2, $a2, $zero\n"
"  fld.d   $fa0, $t2, 0\n"
"  fld.d   $fa1, $t2, 8\n"
"  fld.d   $fa2, $t2, 16\n"
"  fld.d   $fa3, $t2, 24\n"
"  fld.d   $fa4, $t2, 32\n"
"  fld.d   $fa5, $t2, 40\n"
"  fld.d   $fa6, $t2, 48\n"
"  fld.d   $fa7, $t2, 56\n"
"  ld.d    $a7, $t1, 56\n"
"  ld.d    $a6, $t1, 48\n"
"  ld.d    $a5, $t1, 40\n"
"  ld.d    $a4, $t1, 32\n"
"  ld.d    $a3, $t1, 24\n"
"  ld.d    $a2, $t1, 16\n"
"  ld.d    $a1, $t1, 8\n"
"  ld.d    $a0, $t1, 0\n"
"  jirl    $ra, $t0, 0\n"
"  or      $sp, $fp, $zero\n"
"  ld.d    $ra, $sp, -8\n"
"  ld.d    $fp, $sp, -16\n"
"  jr      $ra\n"
".size _xptc_loongarch64_invoke, .-_xptc_loongarch64_invoke\n"
);
