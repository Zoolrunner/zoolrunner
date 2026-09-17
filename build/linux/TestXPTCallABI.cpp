/* Compiler-generated virtual calls are the ABI oracle for XPTCall.
 * Exercise independent integer/FP register banks, interleaved stack slots,
 * narrow signed/unsigned values, 64-bit values, pointers and an out parameter.
 * Both directions repeat to detect failure to restore the stack/callee state.
 */
#include "xptcall.h"
#include "prlong.h"
#include <stdio.h>
#include <string.h>

static int identity;
static unsigned calls;
static const nsresult returned = NS_ERROR_ABORT;
#define REFCOUNTED_STACK_OBJECT \
    NS_IMETHOD QueryInterface(REFNSIID, void** out) { *out = 0; return NS_NOINTERFACE; } \
    NS_IMETHOD_(nsrefcnt) AddRef() { return 2; } \
    NS_IMETHOD_(nsrefcnt) Release() { return 1; }

#define ARGUMENTS \
    PRInt8 a0, \
    PRUint16 a1, \
    PRInt32 a2, \
    PRUint64 a3, \
    PRBool a4, \
    char a5, \
    PRUnichar a6, \
    float a7, \
    double a8, \
    float a9, \
    double a10, \
    float a11, \
    double a12, \
    float a13, \
    double a14, \
    PRInt8 a15, \
    float a16, \
    PRInt16 a17, \
    PRUint8 a18, \
    double a19, \
    PRInt64 a20, \
    PRUint32 a21, \
    PRBool a22, \
    char a23, \
    PRUnichar a24, \
    void* a25, \
    PRUint32* out

class Interface : public nsISupports { public: NS_IMETHOD Mixed(ARGUMENTS) = 0; };

class Native : public Interface {
public:
    REFCOUNTED_STACK_OBJECT
    NS_IMETHOD Mixed(ARGUMENTS) {
        if (a0 != (PRInt8)(-101)) return NS_ERROR_FAILURE;
        if (a1 != (PRUint16)(60001)) return NS_ERROR_FAILURE;
        if (a2 != (PRInt32)(-1234567)) return NS_ERROR_FAILURE;
        if (a3 != (PRUint64)(PR_UINT64(0xfedcba9876543210))) return NS_ERROR_FAILURE;
        if (a4 != (PRBool)(PR_TRUE)) return NS_ERROR_FAILURE;
        if (a5 != (char)('Q')) return NS_ERROR_FAILURE;
        if (a6 != (PRUnichar)(0x4321)) return NS_ERROR_FAILURE;
        if (a7 != (float)(0.25)) return NS_ERROR_FAILURE;
        if (a8 != (double)(-1.25)) return NS_ERROR_FAILURE;
        if (a9 != (float)(2.25)) return NS_ERROR_FAILURE;
        if (a10 != (double)(-3.25)) return NS_ERROR_FAILURE;
        if (a11 != (float)(4.25)) return NS_ERROR_FAILURE;
        if (a12 != (double)(-5.25)) return NS_ERROR_FAILURE;
        if (a13 != (float)(6.25)) return NS_ERROR_FAILURE;
        if (a14 != (double)(-7.25)) return NS_ERROR_FAILURE;
        if (a15 != (PRInt8)(-97)) return NS_ERROR_FAILURE;
        if (a16 != (float)(-11.75)) return NS_ERROR_FAILURE;
        if (a17 != (PRInt16)(-30001)) return NS_ERROR_FAILURE;
        if (a18 != (PRUint8)(231)) return NS_ERROR_FAILURE;
        if (a19 != (double)(123456.125)) return NS_ERROR_FAILURE;
        if (a20 != (PRInt64)(-PR_INT64(0x123456789abcdef))) return NS_ERROR_FAILURE;
        if (a21 != (PRUint32)(0xfedcba98U)) return NS_ERROR_FAILURE;
        if (a22 != (PRBool)(PR_FALSE)) return NS_ERROR_FAILURE;
        if (a23 != (char)('z')) return NS_ERROR_FAILURE;
        if (a24 != (PRUnichar)(0xbeef)) return NS_ERROR_FAILURE;
        if (a25 != (void*)(&identity)) return NS_ERROR_FAILURE;
        *out = 0x1234abcd;
        ++calls;
        return returned;
    }
};

// The stub only needs GetMethodInfo. All other metadata operations reject
// accidental use, while the compiler checks the complete historical interface.
class UnusedInfo : public nsIInterfaceInfo {
public:
    NS_FORWARD_SAFE_NSIINTERFACEINFO(((nsIInterfaceInfo*)0))
};
class Info : public UnusedInfo {
public:
    REFCOUNTED_STACK_OBJECT
    XPTMethodDescriptor method;
    XPTParamDescriptor params[27];
    Info() {
        memset(&method, 0, sizeof(method));
        memset(params, 0, sizeof(params));
        method.params = params;
        method.num_args = 27;
        params[0].flags = XPT_PD_IN; params[0].type.prefix.flags = nsXPTType::T_I8;
        params[1].flags = XPT_PD_IN; params[1].type.prefix.flags = nsXPTType::T_U16;
        params[2].flags = XPT_PD_IN; params[2].type.prefix.flags = nsXPTType::T_I32;
        params[3].flags = XPT_PD_IN; params[3].type.prefix.flags = nsXPTType::T_U64;
        params[4].flags = XPT_PD_IN; params[4].type.prefix.flags = nsXPTType::T_BOOL;
        params[5].flags = XPT_PD_IN; params[5].type.prefix.flags = nsXPTType::T_CHAR;
        params[6].flags = XPT_PD_IN; params[6].type.prefix.flags = nsXPTType::T_WCHAR;
        params[7].flags = XPT_PD_IN; params[7].type.prefix.flags = nsXPTType::T_FLOAT;
        params[8].flags = XPT_PD_IN; params[8].type.prefix.flags = nsXPTType::T_DOUBLE;
        params[9].flags = XPT_PD_IN; params[9].type.prefix.flags = nsXPTType::T_FLOAT;
        params[10].flags = XPT_PD_IN; params[10].type.prefix.flags = nsXPTType::T_DOUBLE;
        params[11].flags = XPT_PD_IN; params[11].type.prefix.flags = nsXPTType::T_FLOAT;
        params[12].flags = XPT_PD_IN; params[12].type.prefix.flags = nsXPTType::T_DOUBLE;
        params[13].flags = XPT_PD_IN; params[13].type.prefix.flags = nsXPTType::T_FLOAT;
        params[14].flags = XPT_PD_IN; params[14].type.prefix.flags = nsXPTType::T_DOUBLE;
        params[15].flags = XPT_PD_IN; params[15].type.prefix.flags = nsXPTType::T_I8;
        params[16].flags = XPT_PD_IN; params[16].type.prefix.flags = nsXPTType::T_FLOAT;
        params[17].flags = XPT_PD_IN; params[17].type.prefix.flags = nsXPTType::T_I16;
        params[18].flags = XPT_PD_IN; params[18].type.prefix.flags = nsXPTType::T_U8;
        params[19].flags = XPT_PD_IN; params[19].type.prefix.flags = nsXPTType::T_DOUBLE;
        params[20].flags = XPT_PD_IN; params[20].type.prefix.flags = nsXPTType::T_I64;
        params[21].flags = XPT_PD_IN; params[21].type.prefix.flags = nsXPTType::T_U32;
        params[22].flags = XPT_PD_IN; params[22].type.prefix.flags = nsXPTType::T_BOOL;
        params[23].flags = XPT_PD_IN; params[23].type.prefix.flags = nsXPTType::T_CHAR;
        params[24].flags = XPT_PD_IN; params[24].type.prefix.flags = nsXPTType::T_WCHAR;
        params[25].flags = XPT_PD_IN; params[25].type.prefix.flags = nsXPTType::T_VOID | XPT_TDP_POINTER;
        params[26].flags = XPT_PD_OUT;
        params[26].type.prefix.flags = nsXPTType::T_U32;
    }
    NS_IMETHOD GetMethodInfo(PRUint16 index, const nsXPTMethodInfo** out) {
        if (index != 3) return NS_ERROR_FAILURE;
        *out = reinterpret_cast<const nsXPTMethodInfo*>(&method);
        return NS_OK;
    }
};

class Stub : public nsXPTCStubBase {
    Info info;
public:
    REFCOUNTED_STACK_OBJECT
    NS_IMETHOD GetInterfaceInfo(nsIInterfaceInfo** out) {
        *out = &info; info.AddRef(); return NS_OK;
    }
    NS_IMETHOD CallMethod(PRUint16 index, const nsXPTMethodInfo* method,
                          nsXPTCMiniVariant* args) {
        if (index != 3 || method->GetParamCount() != 27) return NS_ERROR_FAILURE;
        if (args[0].val.i8 != (PRInt8)(-101)) return NS_ERROR_FAILURE;
        if (args[1].val.u16 != (PRUint16)(60001)) return NS_ERROR_FAILURE;
        if (args[2].val.i32 != (PRInt32)(-1234567)) return NS_ERROR_FAILURE;
        if (args[3].val.u64 != (PRUint64)(PR_UINT64(0xfedcba9876543210))) return NS_ERROR_FAILURE;
        if (args[4].val.b != (PRBool)(PR_TRUE)) return NS_ERROR_FAILURE;
        if (args[5].val.c != (char)('Q')) return NS_ERROR_FAILURE;
        if (args[6].val.wc != (PRUnichar)(0x4321)) return NS_ERROR_FAILURE;
        if (args[7].val.f != (float)(0.25)) return NS_ERROR_FAILURE;
        if (args[8].val.d != (double)(-1.25)) return NS_ERROR_FAILURE;
        if (args[9].val.f != (float)(2.25)) return NS_ERROR_FAILURE;
        if (args[10].val.d != (double)(-3.25)) return NS_ERROR_FAILURE;
        if (args[11].val.f != (float)(4.25)) return NS_ERROR_FAILURE;
        if (args[12].val.d != (double)(-5.25)) return NS_ERROR_FAILURE;
        if (args[13].val.f != (float)(6.25)) return NS_ERROR_FAILURE;
        if (args[14].val.d != (double)(-7.25)) return NS_ERROR_FAILURE;
        if (args[15].val.i8 != (PRInt8)(-97)) return NS_ERROR_FAILURE;
        if (args[16].val.f != (float)(-11.75)) return NS_ERROR_FAILURE;
        if (args[17].val.i16 != (PRInt16)(-30001)) return NS_ERROR_FAILURE;
        if (args[18].val.u8 != (PRUint8)(231)) return NS_ERROR_FAILURE;
        if (args[19].val.d != (double)(123456.125)) return NS_ERROR_FAILURE;
        if (args[20].val.i64 != (PRInt64)(-PR_INT64(0x123456789abcdef))) return NS_ERROR_FAILURE;
        if (args[21].val.u32 != (PRUint32)(0xfedcba98U)) return NS_ERROR_FAILURE;
        if (args[22].val.b != (PRBool)(PR_FALSE)) return NS_ERROR_FAILURE;
        if (args[23].val.c != (char)('z')) return NS_ERROR_FAILURE;
        if (args[24].val.wc != (PRUnichar)(0xbeef)) return NS_ERROR_FAILURE;
        if (args[25].val.p != (void*)(&identity)) return NS_ERROR_FAILURE;
        *static_cast<PRUint32*>(args[26].val.p) = 0x1234abcd;
        ++calls;
        return returned;
    }
};

// Stub entries deliberately have erased C++ signatures. Call the vtable entry
// with the real prototype, as native interface consumers do, without claiming
// that Stub derives from Interface (which would permit invalid devirtualization).
typedef nsresult (*MixedFunction)(void*, ARGUMENTS);
static nsresult Call(nsXPTCStubBase* target, PRUint32* out) {
    void** vtable = *reinterpret_cast<void***>(target);
    MixedFunction method = reinterpret_cast<MixedFunction>(vtable[3]);
    return method(target,
        (PRInt8)(-101),
        (PRUint16)(60001),
        (PRInt32)(-1234567),
        (PRUint64)(PR_UINT64(0xfedcba9876543210)),
        (PRBool)(PR_TRUE),
        (char)('Q'),
        (PRUnichar)(0x4321),
        (float)(0.25),
        (double)(-1.25),
        (float)(2.25),
        (double)(-3.25),
        (float)(4.25),
        (double)(-5.25),
        (float)(6.25),
        (double)(-7.25),
        (PRInt8)(-97),
        (float)(-11.75),
        (PRInt16)(-30001),
        (PRUint8)(231),
        (double)(123456.125),
        (PRInt64)(-PR_INT64(0x123456789abcdef)),
        (PRUint32)(0xfedcba98U),
        (PRBool)(PR_FALSE),
        (char)('z'),
        (PRUnichar)(0xbeef),
        (void*)(&identity), out);
}

int main() {
    Native native;
    Stub stub;
    nsXPTCVariant args[27];
    memset(args, 0, sizeof(args));
    PRUint32 out = 0;
    args[0].type = nsXPTType::T_I8; args[0].val.i8 = (PRInt8)(-101);
    args[1].type = nsXPTType::T_U16; args[1].val.u16 = (PRUint16)(60001);
    args[2].type = nsXPTType::T_I32; args[2].val.i32 = (PRInt32)(-1234567);
    args[3].type = nsXPTType::T_U64; args[3].val.u64 = (PRUint64)(PR_UINT64(0xfedcba9876543210));
    args[4].type = nsXPTType::T_BOOL; args[4].val.b = (PRBool)(PR_TRUE);
    args[5].type = nsXPTType::T_CHAR; args[5].val.c = (char)('Q');
    args[6].type = nsXPTType::T_WCHAR; args[6].val.wc = (PRUnichar)(0x4321);
    args[7].type = nsXPTType::T_FLOAT; args[7].val.f = (float)(0.25);
    args[8].type = nsXPTType::T_DOUBLE; args[8].val.d = (double)(-1.25);
    args[9].type = nsXPTType::T_FLOAT; args[9].val.f = (float)(2.25);
    args[10].type = nsXPTType::T_DOUBLE; args[10].val.d = (double)(-3.25);
    args[11].type = nsXPTType::T_FLOAT; args[11].val.f = (float)(4.25);
    args[12].type = nsXPTType::T_DOUBLE; args[12].val.d = (double)(-5.25);
    args[13].type = nsXPTType::T_FLOAT; args[13].val.f = (float)(6.25);
    args[14].type = nsXPTType::T_DOUBLE; args[14].val.d = (double)(-7.25);
    args[15].type = nsXPTType::T_I8; args[15].val.i8 = (PRInt8)(-97);
    args[16].type = nsXPTType::T_FLOAT; args[16].val.f = (float)(-11.75);
    args[17].type = nsXPTType::T_I16; args[17].val.i16 = (PRInt16)(-30001);
    args[18].type = nsXPTType::T_U8; args[18].val.u8 = (PRUint8)(231);
    args[19].type = nsXPTType::T_DOUBLE; args[19].val.d = (double)(123456.125);
    args[20].type = nsXPTType::T_I64; args[20].val.i64 = (PRInt64)(-PR_INT64(0x123456789abcdef));
    args[21].type = nsXPTType::T_U32; args[21].val.u32 = (PRUint32)(0xfedcba98U);
    args[22].type = nsXPTType::T_BOOL; args[22].val.b = (PRBool)(PR_FALSE);
    args[23].type = nsXPTType::T_CHAR; args[23].val.c = (char)('z');
    args[24].type = nsXPTType::T_WCHAR; args[24].val.wc = (PRUnichar)(0xbeef);
    args[25].type = nsXPTType::T_VOID | XPT_TDP_POINTER; args[25].val.p = (void*)(&identity);
    args[26].type = nsXPTType::T_U32;
    args[26].ptr = &out;
    args[26].SetPtrIsData();
    for (unsigned i = 0; i < 1000; ++i) {
        out = 0;
        if (XPTC_InvokeByIndex(&native, 3, 27, args) != returned || out != 0x1234abcd)
            return fprintf(stderr, "FAIL: invoke iteration %u\n", i), 1;
        out = 0;
        if (Call(&stub, &out) != returned || out != 0x1234abcd)
            return fprintf(stderr, "FAIL: stub iteration %u\n", i), 1;
    }
    if (calls != 2000) return 1;
    puts("XPTCALL-ABI checks=2000 failures=0");
    return 0;
}
