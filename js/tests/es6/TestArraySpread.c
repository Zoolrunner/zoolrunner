/* Array literal spread through callbacks, GC and XDR.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "jsapi.h"
#include "jsxdrapi.h"
#include <stdio.h>
#include <string.h>
static unsigned checks, resolveCalls;
static JSBool inResolve, hookFailed;
static JSBool Resolve(JSContext *cx, JSObject *obj, jsval id)
{
    jsval result;
    const char *nested="(function(){return 17})()";
    if (!inResolve && JSVAL_IS_STRING(id) &&
        !strcmp(JS_GetStringBytes(JSVAL_TO_STRING(id)),"gdNative")) {
        inResolve=JS_TRUE;
        ++resolveCalls;
        JS_GC(cx);
        if (!JS_EvaluateScript(cx,obj,nested,strlen(nested),"resolve-reentry",1,&result) ||
            result!=INT_TO_JSVAL(17)) hookFailed=JS_TRUE;
        inResolve=JS_FALSE;
    }
    return !hookFailed;
}
static JSClass globalClass={
    "global",0,JS_PropertyStub,JS_PropertyStub,JS_PropertyStub,JS_PropertyStub,
    JS_EnumerateStub,Resolve,JS_ConvertStub,JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};
static JSBool ChangeEdition(JSContext *cx,JSObject *obj,uintN argc,jsval *argv,jsval *rval)
{JS_SetVersion(cx,JSVERSION_1_7);*rval=JSVAL_VOID;return JS_TRUE;}
static JSBool Collect(JSContext *cx,JSObject *obj,uintN argc,jsval *argv,jsval *rval)
{JS_GC(cx);*rval=JSVAL_VOID;return JS_TRUE;}
static unsigned branchCalls;
static JSBool inBranch, cancelBranch, branchFailed;
static JSBool Branch(JSContext *cx,JSScript *script)
{
    jsval result;
    const char *nested="[...[9]][0]";
    if(script || inBranch)return JS_TRUE;
    ++branchCalls;
    if(cancelBranch)return JS_FALSE;
    inBranch=JS_TRUE;JS_GC(cx);
    if(!JS_EvaluateScript(cx,JS_GetGlobalObject(cx),nested,strlen(nested),
                          "spread-reentry",1,&result) || result!=INT_TO_JSVAL(9))
        branchFailed=JS_TRUE;
    inBranch=JS_FALSE;
    return !branchFailed;
}
static void Report(JSContext *cx,const char *message,JSErrorReport *report)
{fprintf(stderr,"array spread: %s\n",message);}
#define CHECK(v) do {++checks;if(!(v)){fprintf(stderr,"FAIL array spread check %u\n",checks);goto out;}}while(0)
int main(void)
{
    JSRuntime *rt=JS_NewRuntime(4*1024*1024);
    JSContext *cx;
    JSObject *global=NULL,*scriptObject=NULL,*decodedObject=NULL;
    JSScript *script=NULL,*decoded=NULL;
    JSString *source=NULL;
    jsval result=JSVAL_VOID;
    JSXDRState *encoder=NULL,*decoder=NULL;
    void *data;
    uint32 length;
    int status=1;
    const char *program=
        "var gdOne=1;function gdNative(){return gdOne}"
        "var source={};source[Symbol.iterator]=function(){var n=0;return {"
        "next:function(){gcNow();return {done:n===3,value:n++}}}};"
        "var a=[,...source,,7];"
        "var restored=eval('(function(){return [,...[1,2],,3]})');"
        "a.length===6 && !(0 in a) && a[1]===0 && a[3]===2 && "
        "!(4 in a) && a[5]===7 && restored().join()===',1,2,,3' && gdNative()===1";
    const char *conflict="var gdAbsent;function NaN(){}";
    const char *absent="!Object.prototype.hasOwnProperty.call(this,'gdAbsent')";
    if(!rt)return 1;
    cx=JS_NewContext(rt,8192);
    if(!cx){JS_DestroyRuntime(rt);return 1;}
    JS_BeginRequest(cx);JS_SetErrorReporter(cx,Report);
    JS_AddNamedRoot(cx,&global,"declaration global");
    JS_AddNamedRoot(cx,&scriptObject,"declaration script");
    JS_AddNamedRoot(cx,&decodedObject,"decoded declaration script");
    JS_AddNamedRoot(cx,&source,"declaration source");
    JS_AddNamedRoot(cx,&result,"declaration result");
    JS_SetVersion(cx,JSVERSION_ECMA_2015);
    global=JS_NewObject(cx,&globalClass,NULL,NULL);CHECK(global);
    JS_SetGlobalObject(cx,global);CHECK(JS_InitStandardClasses(cx,global));
    CHECK(JS_DefineFunction(cx,global,"gcNow",Collect,0,0));
    script=JS_CompileScript(cx,global,program,strlen(program),"declarations",1);
    CHECK(script && (scriptObject=JS_NewScriptObject(cx,script))!=NULL);
    JS_SetBranchCallback(cx,Branch);
    CHECK(JS_ExecuteScript(cx,global,script,&result) && result==JSVAL_TRUE);
    CHECK(resolveCalls>0 && !hookFailed);
    CHECK(branchCalls>0 && !branchFailed);
    encoder=JS_XDRNewMem(cx,JSXDR_ENCODE);decoder=JS_XDRNewMem(cx,JSXDR_DECODE);
    CHECK(encoder && decoder && JS_XDRScript(encoder,&script));
    data=JS_XDRMemGetData(encoder,&length);JS_XDRMemSetData(decoder,data,length);
    JS_SetVersion(cx,JSVERSION_1_7);
    CHECK(JS_XDRScript(decoder,&decoded));
    CHECK((decodedObject=JS_NewScriptObject(cx,decoded))!=NULL);
    JS_GC(cx);
    CHECK(JS_ExecuteScript(cx,global,decoded,&result) && result==JSVAL_TRUE);
    CHECK(JS_GetVersion(cx)==JSVERSION_1_7);
    JS_SetVersion(cx,JSVERSION_ECMA_2015);
    source=JS_DecompileScript(cx,decoded,"declaration-source",0);
    CHECK(source && JS_EvaluateUCScript(cx,global,JS_GetStringChars(source),JS_GetStringLength(source),"roundtrip",1,&result) && result==JSVAL_TRUE);
    CHECK(!JS_EvaluateScript(cx,global,conflict,strlen(conflict),"conflict",1,&result));
    JS_ClearPendingException(cx);JS_GC(cx);
    CHECK(JS_EvaluateScript(cx,global,absent,strlen(absent),"absent",1,&result) && result==JSVAL_TRUE);
    CHECK(JS_DefineFunction(cx,global,"changeEdition",ChangeEdition,0,0));
    { const char *changing="(function(){try{return 7}finally{changeEdition()}})()===7";
      CHECK(JS_EvaluateScript(cx,global,changing,strlen(changing),"changing-edition",1,&result) && result==JSVAL_TRUE); }
    CHECK(JS_GetVersion(cx)==JSVERSION_1_7);
    { const char *legacy="var n=0;for(;;){if(n===3)break;else n++}";
      CHECK(JS_EvaluateScript(cx,global,legacy,strlen(legacy),"legacy-completion",1,&result) && result==INT_TO_JSVAL(2)); }
    { const char *modernOnly="[...[]]";
      CHECK(!JS_EvaluateScript(cx,global,modernOnly,strlen(modernOnly),"legacy-spread",1,&result)); }
    JS_ClearPendingException(cx);
    JS_SetVersion(cx,JSVERSION_ECMA_2015);
    cancelBranch=JS_TRUE;
    { const char *cancelled="var infinite={};infinite[Symbol.iterator]=function(){return {next:function(){return {value:1,done:false}}}};[...infinite]";
      CHECK(!JS_EvaluateScript(cx,global,cancelled,strlen(cancelled),"cancelled-spread",1,&result)); }
    cancelBranch=JS_FALSE;JS_ClearPendingException(cx);
    { const char *resumed="[...[1,2]].join()==='1,2'";
      CHECK(JS_EvaluateScript(cx,global,resumed,strlen(resumed),"after-cancel",1,&result) && result==JSVAL_TRUE); }
    CHECK(!branchFailed);
    printf("ARRAY-SPREAD-EMBEDDING PASS checks=%u\n",checks);status=0;
  out:
    if(decoder){JS_XDRMemSetData(decoder,NULL,0);JS_XDRDestroy(decoder);}
    if(encoder)JS_XDRDestroy(encoder);
    if(decoded && !decodedObject)JS_DestroyScript(cx,decoded);
    if(script && !scriptObject)JS_DestroyScript(cx,script);
    JS_RemoveRoot(cx,&result);JS_RemoveRoot(cx,&source);
    JS_RemoveRoot(cx,&decodedObject);JS_RemoveRoot(cx,&scriptObject);JS_RemoveRoot(cx,&global);
    JS_EndRequest(cx);JS_DestroyContext(cx);JS_DestroyRuntime(rt);JS_ShutDown();
    return status;
}
