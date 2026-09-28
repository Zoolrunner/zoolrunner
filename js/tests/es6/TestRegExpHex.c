/* Regexp grammar policy across lazy classes, source roundtrips and XDR.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "jsapi.h"
#include "jsxdrapi.h"
#include <stdio.h>
#include <string.h>
static unsigned checks,collections;
static void Report(JSContext *cx,const char *message,JSErrorReport *report)
{fprintf(stderr,"Regexp hex: %s\n",message);}
static JSClass globalClass={"global",0,JS_PropertyStub,JS_PropertyStub,
 JS_PropertyStub,JS_PropertyStub,JS_EnumerateStub,JS_ResolveStub,JS_ConvertStub,
 JS_FinalizeStub,JSCLASS_NO_OPTIONAL_MEMBERS};
static JSBool Collect(JSContext *cx,JSObject *obj,uintN argc,jsval *argv,jsval *rval)
{++collections;JS_GC(cx);*rval=JSVAL_VOID;return JS_TRUE;}
static JSBool ObjectRoundtrip(JSContext *cx,JSObject *global,JSBool modern)
{
    JSXDRState *encoder=NULL,*decoder=NULL;
    JSObject *object=NULL,*decoded=NULL;
    jsval value=JSVAL_VOID,argument=JSVAL_VOID,result;
    JSString *input;
    void *data;
    uint32 length;
    JSBool ok=JS_FALSE;
    const char *pattern="/[\\xA]/";
    JS_AddNamedRoot(cx,&object,"hex object");JS_AddNamedRoot(cx,&decoded,"decoded hex object");
    JS_AddNamedRoot(cx,&argument,"hex argument");
    JS_AddNamedRoot(cx,&value,"hex serialized value");
    JS_SetVersion(cx,modern?JSVERSION_ECMA_2015:JSVERSION_1_7);
    if(!JS_EvaluateScript(cx,global,pattern,strlen(pattern),"object-source",1,&value))goto done;
    object=JSVAL_TO_OBJECT(value);
    encoder=JS_XDRNewMem(cx,JSXDR_ENCODE);decoder=JS_XDRNewMem(cx,JSXDR_DECODE);
    if(!encoder||!decoder||!JS_XDRValue(encoder,&value))goto done;
    data=JS_XDRMemGetData(encoder,&length);JS_XDRMemSetData(decoder,data,length);
    JS_SetVersion(cx,modern?JSVERSION_1_7:JSVERSION_ECMA_2015);
    value=JSVAL_VOID;
    if(!JS_XDRValue(decoder,&value))goto done;
    decoded=JSVAL_TO_OBJECT(value);
    JS_ClearNewbornRoots(cx);JS_GC(cx);
    input=JS_NewStringCopyZ(cx,modern?"x":"\n");if(!input)goto done;
    argument=STRING_TO_JSVAL(input);
    ok=JS_CallFunctionName(cx,decoded,"test",1,&argument,&result)&&result==JSVAL_TRUE;
 done:
    if(decoder){JS_XDRMemSetData(decoder,NULL,0);JS_XDRDestroy(decoder);}
    if(encoder)JS_XDRDestroy(encoder);
    JS_RemoveRoot(cx,&value);JS_RemoveRoot(cx,&argument);JS_RemoveRoot(cx,&decoded);JS_RemoveRoot(cx,&object);
    return ok;
}
#define CHECK(c) do{++checks;if(!(c)){fprintf(stderr,"FAIL regexp hex check %u\n",checks);goto out;}}while(0)
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
    const char *program="function hex(){var r=/[\\x]/,s=/\\uABC/;gc();return r.test('x')&&s.test('uABC')&&/[\\u-\\x]/.test('v')}hex()";
    const char *legacy="/\\x/.test('\\u0000') && /\\xA/.test('\\n') && /[\\xA]/.test('\\n') && !/[\\x]/.test('x')";
    if(!rt)return 1;
    cx=JS_NewContext(rt,8192);if(!cx){JS_DestroyRuntime(rt);return 1;}
    JS_BeginRequest(cx);JS_SetErrorReporter(cx,Report);
    JS_AddNamedRoot(cx,&global,"hex global");JS_AddNamedRoot(cx,&scriptObject,"hex script");
    JS_AddNamedRoot(cx,&decodedObject,"hex decoded");JS_AddNamedRoot(cx,&source,"hex source");
    JS_SetVersion(cx,JSVERSION_ECMA_2015);
    global=JS_NewObject(cx,&globalClass,NULL,NULL);CHECK(global);
    JS_SetGlobalObject(cx,global);CHECK(JS_InitStandardClasses(cx,global));
    CHECK(JS_DefineFunction(cx,global,"gc",Collect,0,0));
    script=JS_CompileScript(cx,global,program,strlen(program),"hex",1);
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
    JS_SetVersion(cx,JSVERSION_ECMA_2015);
    source=JS_DecompileScript(cx,decoded,"hex-source",0);
    CHECK(source&&JS_EvaluateUCScript(cx,global,JS_GetStringChars(source),JS_GetStringLength(source),"roundtrip",1,&result)&&result==JSVAL_TRUE);
    CHECK(collections==3);
    CHECK(ObjectRoundtrip(cx,global,JS_TRUE));
    CHECK(ObjectRoundtrip(cx,global,JS_FALSE));
    printf("ES6-REGEXP-HEX checks=%u failures=0\n",checks);status=0;
out:
    if(decoder){JS_XDRMemSetData(decoder,NULL,0);JS_XDRDestroy(decoder);}
    if(encoder)JS_XDRDestroy(encoder);
    JS_RemoveRoot(cx,&source);JS_RemoveRoot(cx,&decodedObject);JS_RemoveRoot(cx,&scriptObject);JS_RemoveRoot(cx,&global);
    JS_EndRequest(cx);JS_DestroyContext(cx);JS_DestroyRuntime(rt);JS_ShutDown();return status;
}
