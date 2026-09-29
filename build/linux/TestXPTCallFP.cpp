/* Verify FP-bank exhaustion while integer argument registers remain available.
 * The compiler supplies the ABI oracle in each direction. */
#include "xptcall.h"
#include <stdio.h>
#include <string.h>

#define SIGNATURE double a, double b, double c, double d, double e, double f, \
                  double g, double h, double i, float j, PRUint32 k, PRUint32* out
#define REFCOUNTED \
    NS_IMETHOD QueryInterface(REFNSIID, void** out) { *out=0; return NS_NOINTERFACE; } \
    NS_IMETHOD_(nsrefcnt) AddRef() { return 2; } \
    NS_IMETHOD_(nsrefcnt) Release() { return 1; }
static const double values[9] = { .25, -1.25, 2.25, -3.25, 4.25, -5.25, 6.25, -7.25, 8.25 };
static unsigned calls;
class Interface : public nsISupports { public: NS_IMETHOD Mixed(SIGNATURE) = 0; };
class Native : public Interface {
public:
    REFCOUNTED
    NS_IMETHOD Mixed(SIGNATURE) {
        const double received[9] = {a,b,c,d,e,f,g,h,i};
        for (unsigned n=0;n<9;++n)
            if (received[n]!=values[n]) return NS_ERROR_FAILURE;
        if (j!=-9.75f || k!=0xfedcba98U) return NS_ERROR_FAILURE;
        *out=0x1234abcd;
        ++calls;
        return NS_ERROR_ABORT;
    }
};
class UnusedInfo : public nsIInterfaceInfo {
public:
    NS_FORWARD_SAFE_NSIINTERFACEINFO(((nsIInterfaceInfo*)0))
};
class Info : public UnusedInfo {
public:
    REFCOUNTED
    XPTMethodDescriptor method;
    XPTParamDescriptor params[12];
    Info() {
        memset(&method,0,sizeof(method));
        memset(params,0,sizeof(params));
        method.params=params;
        method.num_args=12;
        for (unsigned n=0;n<12;++n) {
            params[n].flags=n==11 ? XPT_PD_OUT : XPT_PD_IN;
            params[n].type.prefix.flags=n<9 ? nsXPTType::T_DOUBLE :
                n==9 ? nsXPTType::T_FLOAT : nsXPTType::T_U32;
        }
    }
    NS_IMETHOD GetMethodInfo(PRUint16 index,const nsXPTMethodInfo** out) {
        if (index!=3) return NS_ERROR_FAILURE;
        *out=reinterpret_cast<const nsXPTMethodInfo*>(&method);
        return NS_OK;
    }
};
class Stub : public nsXPTCStubBase {
    Info info;
public:
    REFCOUNTED
    NS_IMETHOD GetInterfaceInfo(nsIInterfaceInfo** out) {
        *out=&info; info.AddRef(); return NS_OK;
    }
    NS_IMETHOD CallMethod(PRUint16 index,const nsXPTMethodInfo* method,nsXPTCMiniVariant* args) {
        if (index!=3 || method->GetParamCount()!=12) return NS_ERROR_FAILURE;
        for (unsigned n=0;n<9;++n)
            if (args[n].val.d!=values[n]) return NS_ERROR_FAILURE;
        if (args[9].val.f!=-9.75f || args[10].val.u32!=0xfedcba98U) return NS_ERROR_FAILURE;
        *static_cast<PRUint32*>(args[11].val.p)=0x1234abcd;
        ++calls;
        return NS_ERROR_ABORT;
    }
};
typedef nsresult (*Method)(void*,SIGNATURE);
int main() {
    Native native;
    Stub stub;
    nsXPTCVariant args[12];
    memset(args,0,sizeof(args));
    PRUint32 out;
    for (unsigned n=0;n<9;++n) {
        args[n].type=nsXPTType::T_DOUBLE;
        args[n].val.d=values[n];
    }
    args[9].type=nsXPTType::T_FLOAT; args[9].val.f=-9.75f;
    args[10].type=nsXPTType::T_U32; args[10].val.u32=0xfedcba98U;
    args[11].type=nsXPTType::T_U32;
    args[11].flags=nsXPTCVariant::PTR_IS_DATA; args[11].ptr=&out;
    Method method=reinterpret_cast<Method>((*reinterpret_cast<void***>(&stub))[3]);
    for (unsigned n=0;n<1000;++n) {
        out=0;
        if (XPTC_InvokeByIndex(&native,3,12,args)!=NS_ERROR_ABORT || out!=0x1234abcd) {
            fprintf(stderr,"FAIL FP invoke iteration %u\n",n); return 1;
        }
        out=0;
        if (method(&stub,.25,-1.25,2.25,-3.25,4.25,-5.25,6.25,-7.25,8.25,-9.75f,
                   0xfedcba98U,&out)!=NS_ERROR_ABORT || out!=0x1234abcd) {
            fprintf(stderr,"FAIL FP stub iteration %u\n",n); return 1;
        }
    }
    if (calls!=2000) return 1;
    puts("XPTCALL-FP checks=2000 failures=0");
    return 0;
}
