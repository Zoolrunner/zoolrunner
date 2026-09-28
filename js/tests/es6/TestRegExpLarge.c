/* Large RegExp counts retain editions, cache round trips and callback cancellation.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "jsapi.h"
#include "jsxdrapi.h"
#include <stdio.h>
#include <string.h>
static unsigned checks, callbacks;
static JSBool nested, callbackFailed, shouldCancel=JS_TRUE;
static JSBool InterruptLarge(JSContext *cx, JSScript *script)
{
    jsval result;
    const char *probe="/z/.test('z')";
    if (script || nested) return JS_TRUE;
    ++callbacks;
    nested=JS_TRUE;
    JS_GC(cx);
    if (!JS_EvaluateScript(cx,JS_GetGlobalObject(cx),probe,strlen(probe),"reentrant",1,&result) || result!=JSVAL_TRUE)
        callbackFailed=JS_TRUE;
    nested=JS_FALSE;
    return (!shouldCancel || callbacks<5) && !callbackFailed;
}
static JSClass globalClass={"global",0,JS_PropertyStub,JS_PropertyStub,
 JS_PropertyStub,JS_PropertyStub,JS_EnumerateStub,JS_ResolveStub,JS_ConvertStub,
 JS_FinalizeStub,JSCLASS_NO_OPTIONAL_MEMBERS};
#define CHECK(c) do{++checks;if(!(c)){fprintf(stderr,"FAIL regexp large check %u\n",checks);goto out;}}while(0)
int main(void)
{
    JSRuntime *rt=JS_NewRuntime(8*1024*1024);
    JSContext *cx;
    JSObject *global=NULL,*other=NULL,*scriptObject=NULL,*decodedObject=NULL;
    JSScript *script=NULL,*decoded=NULL;
    JSString *source=NULL;
    jsval result=JSVAL_VOID;
    JSXDRState *encoder=NULL,*decoder=NULL;
    void *data;
    uint32 length;
    int status=1;
    const char *program="function modernLarge(){return /a{4294967296}/.test('a')===false && /(?:){18446744073709551616}/.test('');}modernLarge()";
    const char *legacy="var rejected=false;try{eval('/a{65536}/')}catch(e){rejected=e instanceof SyntaxError}rejected&&new RegExp('a{65536}').test('a')===false&&modernLarge()";
    const char *expensive="/(a?){18446744073709551616}/.test('')";
    const char *minimal="/(a?){18446744073709551616}?/.test('')";
    const char *recovery="/c/.test('c')&&RegExp.lastMatch==='c'";
    if(!rt)return 1;
    cx=JS_NewContext(rt,8192);if(!cx){JS_DestroyRuntime(rt);return 1;}
    JS_BeginRequest(cx);
    JS_AddNamedRoot(cx,&global,"regexp large global");JS_AddNamedRoot(cx,&scriptObject,"regexp large script");
    JS_AddNamedRoot(cx,&decodedObject,"regexp large decoded");JS_AddNamedRoot(cx,&source,"regexp large source");
    JS_AddNamedRoot(cx,&other,"regexp large second global");
    JS_SetVersion(cx,JSVERSION_ECMA_2015);
    global=JS_NewObject(cx,&globalClass,NULL,NULL);CHECK(global);
    JS_SetGlobalObject(cx,global);CHECK(JS_InitStandardClasses(cx,global));
    script=JS_CompileScript(cx,global,program,strlen(program),"regexp-large",1);
    CHECK(script&&(scriptObject=JS_NewScriptObject(cx,script))!=NULL);
    CHECK(JS_ExecuteScript(cx,global,script,&result)&&result==JSVAL_TRUE);
    encoder=JS_XDRNewMem(cx,JSXDR_ENCODE);decoder=JS_XDRNewMem(cx,JSXDR_DECODE);
    CHECK(encoder&&decoder&&JS_XDRScript(encoder,&script));
    data=JS_XDRMemGetData(encoder,&length);JS_XDRMemSetData(decoder,data,length);
    JS_SetVersion(cx,JSVERSION_1_7);CHECK(JS_XDRScript(decoder,&decoded));
    CHECK((decodedObject=JS_NewScriptObject(cx,decoded))!=NULL);
    JS_ClearNewbornRoots(cx);JS_GC(cx);
    CHECK(JS_ExecuteScript(cx,global,decoded,&result)&&result==JSVAL_TRUE);
    CHECK(JS_GetVersion(cx)==JSVERSION_1_7);
    CHECK(JS_EvaluateScript(cx,global,legacy,strlen(legacy),"legacy",1,&result)&&result==JSVAL_TRUE);
    source=JS_DecompileScript(cx,decoded,"regexp-large-source",0);CHECK(source);
    JS_SetVersion(cx,JSVERSION_ECMA_2015);
    CHECK(JS_EvaluateUCScript(cx,global,JS_GetStringChars(source),JS_GetStringLength(source),"roundtrip",1,&result)&&result==JSVAL_TRUE);
    JS_SetBranchCallback(cx,InterruptLarge);
    CHECK(!JS_EvaluateScript(cx,global,expensive,strlen(expensive),"cancel-greedy",1,&result));
    JS_SetBranchCallback(cx,NULL);
    CHECK(callbacks==5&&!callbackFailed);
    JS_ClearPendingException(cx);
    CHECK(JS_EvaluateScript(cx,global,recovery,strlen(recovery),"recovery",1,&result)&&result==JSVAL_TRUE);
    callbacks=0;
    JS_SetBranchCallback(cx,InterruptLarge);
    CHECK(!JS_EvaluateScript(cx,global,minimal,strlen(minimal),"cancel-minimal",1,&result));
    JS_SetBranchCallback(cx,NULL);
    CHECK(callbacks==5&&!callbackFailed);
    JS_ClearPendingException(cx);
    CHECK(JS_EvaluateScript(cx,global,recovery,strlen(recovery),"recovery",1,&result)&&result==JSVAL_TRUE);
    callbacks=0;shouldCancel=JS_FALSE;
    JS_SetBranchCallback(cx,InterruptLarge);
    CHECK(JS_EvaluateScript(cx,global,
          "var largeText=new Array(65537).join('a'); /^(a){65536}$/.test(largeText)&&RegExp.lastMatch===largeText&&RegExp.$1==='a'",
          strlen("var largeText=new Array(65537).join('a'); /^(a){65536}$/.test(largeText)&&RegExp.lastMatch===largeText&&RegExp.$1==='a'"),
          "successful-reentrancy",1,&result)&&result==JSVAL_TRUE);
    JS_SetBranchCallback(cx,NULL);
    CHECK(callbacks>5&&!callbackFailed);
    other=JS_NewObject(cx,&globalClass,NULL,NULL);CHECK(other);
    JS_SetGlobalObject(cx,other);CHECK(JS_InitStandardClasses(cx,other));
    JS_SetVersion(cx,JSVERSION_1_7);
    JS_ClearNewbornRoots(cx);JS_GC(cx);
    CHECK(JS_ExecuteScript(cx,other,decoded,&result)&&result==JSVAL_TRUE);
    CHECK(JS_GetVersion(cx)==JSVERSION_1_7);
    JS_SetVersion(cx,JSVERSION_DEFAULT);
    printf("ES6-REGEXP-LARGE checks=%u failures=0\n",checks);status=0;
out:
    JS_SetBranchCallback(cx,NULL);
    if(decoder){JS_XDRMemSetData(decoder,NULL,0);JS_XDRDestroy(decoder);}
    if(encoder)JS_XDRDestroy(encoder);
    JS_RemoveRoot(cx,&other);
    JS_RemoveRoot(cx,&source);JS_RemoveRoot(cx,&decodedObject);JS_RemoveRoot(cx,&scriptObject);JS_RemoveRoot(cx,&global);
    JS_EndRequest(cx);JS_DestroyContext(cx);JS_DestroyRuntime(rt);JS_ShutDown();return status;
}
