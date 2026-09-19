/* Frozen native fields preserve values and explicit legacy behavior.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "jsapi.h"
#include "jsxdrapi.h"
#include <stdio.h>
#include <string.h>
static unsigned checks;
static JSClass globalClass={"global",0,JS_PropertyStub,JS_PropertyStub,
 JS_PropertyStub,JS_PropertyStub,JS_EnumerateStub,JS_ResolveStub,JS_ConvertStub,
 JS_FinalizeStub,JSCLASS_NO_OPTIONAL_MEMBERS};
#define CHECK(c) do{++checks;if(!(c)){fprintf(stderr,"FAIL frozen native check %u\n",checks);goto out;}}while(0)
int main(void)
{
    JSRuntime *rt=JS_NewRuntime(8*1024*1024);
    JSContext *cx;
    JSObject *global=NULL,*scriptObject=NULL,*decodedObject=NULL;
    JSScript *script=NULL,*decoded=NULL;
    JSString *source=NULL;
    jsval result=JSVAL_VOID;
    JSXDRState *encoder=NULL,*decoder=NULL;
    void *data;
    uint32 length;
    int status=1;
    const char *program="function modernFrozen(){function f(){return Object.getOwnPropertyDescriptor(f,'arguments').value;}f.arguments;Object.defineProperty(f,'arguments',{writable:false,configurable:false});var old=Object.getOwnPropertyDescriptor(f,'arguments').value;return f()===old;}modernFrozen()";
    const char *initialized="function frozenCapture(){/(x)(y)/.exec('xy');Object.defineProperty(RegExp,'$1',{configurable:false});/(a)(b)/.exec('ab');return RegExp.$1==='x';}frozenCapture()";
    const char *legacy="function legacyProbe(){return Object.getOwnPropertyDescriptor(legacyProbe,'arguments').value;}function legacyFrozen(){legacyProbe.arguments;Object.defineProperty(legacyProbe,'arguments',{writable:false,configurable:false});var old=Object.getOwnPropertyDescriptor(legacyProbe,'arguments').value;return legacyProbe()!==old;}legacyFrozen()&&modernFrozen()";
    if(!rt)return 1;
    cx=JS_NewContext(rt,8192);if(!cx){JS_DestroyRuntime(rt);return 1;}
    JS_BeginRequest(cx);
    JS_AddNamedRoot(cx,&global,"frozen native global");JS_AddNamedRoot(cx,&scriptObject,"frozen native script");
    JS_AddNamedRoot(cx,&decodedObject,"frozen native decoded");JS_AddNamedRoot(cx,&source,"frozen native source");
    JS_SetVersion(cx,JSVERSION_ECMA_2015);
    global=JS_NewObject(cx,&globalClass,NULL,NULL);CHECK(global);
    JS_SetGlobalObject(cx,global);CHECK(JS_InitStandardClasses(cx,global));
    script=JS_CompileScript(cx,global,program,strlen(program),"frozen-native",1);
    CHECK(script&&(scriptObject=JS_NewScriptObject(cx,script))!=NULL);
    CHECK(JS_ExecuteScript(cx,global,script,&result)&&result==JSVAL_TRUE);
    CHECK(JS_EvaluateScript(cx,global,initialized,strlen(initialized),"initialized",1,&result)&&result==JSVAL_TRUE);
    encoder=JS_XDRNewMem(cx,JSXDR_ENCODE);decoder=JS_XDRNewMem(cx,JSXDR_DECODE);
    CHECK(encoder&&decoder&&JS_XDRScript(encoder,&script));
    data=JS_XDRMemGetData(encoder,&length);JS_XDRMemSetData(decoder,data,length);
    JS_SetVersion(cx,JSVERSION_1_7);CHECK(JS_XDRScript(decoder,&decoded));
    CHECK((decodedObject=JS_NewScriptObject(cx,decoded))!=NULL);
    JS_ClearNewbornRoots(cx);JS_GC(cx);
    CHECK(JS_ExecuteScript(cx,global,decoded,&result)&&result==JSVAL_TRUE);
    CHECK(JS_GetVersion(cx)==JSVERSION_1_7);
    CHECK(JS_EvaluateScript(cx,global,legacy,strlen(legacy),"legacy",1,&result)&&result==JSVAL_TRUE);
    source=JS_DecompileScript(cx,decoded,"frozen-native-source",0);CHECK(source);
    JS_SetVersion(cx,JSVERSION_ECMA_2015);
    CHECK(JS_EvaluateUCScript(cx,global,JS_GetStringChars(source),JS_GetStringLength(source),"roundtrip",1,&result)&&result==JSVAL_TRUE);
    CHECK(JS_EvaluateScript(cx,global,"legacyFrozen()&&modernFrozen()",strlen("legacyFrozen()&&modernFrozen()"),"cross-edition",1,&result)&&result==JSVAL_TRUE);
    JS_SetVersion(cx,JSVERSION_DEFAULT);
    CHECK(JS_EvaluateScript(cx,global,"frozenCapture()",strlen("frozenCapture()"),"default",1,&result)&&result==JSVAL_TRUE);
    printf("ES6-FROZEN-NATIVE checks=%u failures=0\n",checks);status=0;
out:
    if(decoder){JS_XDRMemSetData(decoder,NULL,0);JS_XDRDestroy(decoder);}
    if(encoder)JS_XDRDestroy(encoder);
    JS_RemoveRoot(cx,&source);JS_RemoveRoot(cx,&decodedObject);JS_RemoveRoot(cx,&scriptObject);JS_RemoveRoot(cx,&global);
    JS_EndRequest(cx);JS_DestroyContext(cx);JS_DestroyRuntime(rt);JS_ShutDown();return status;
}
