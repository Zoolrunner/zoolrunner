/* Modern method fields retain saved editions, clone flags and cache/source round trips.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "jsapi.h"
#include "jsxdrapi.h"
#include <stdio.h>
#include <string.h>
static unsigned checks;
static JSClass globalClass={"global",0,JS_PropertyStub,JS_PropertyStub,
 JS_PropertyStub,JS_PropertyStub,JS_EnumerateStub,JS_ResolveStub,JS_ConvertStub,
 JS_FinalizeStub,JSCLASS_NO_OPTIONAL_MEMBERS};
#define CHECK(c) do{++checks;if(!(c)){fprintf(stderr,"FAIL method fields check %u\n",checks);goto out;}}while(0)
int main(void)
{
    JSRuntime *rt=JS_NewRuntime(8*1024*1024);
    JSContext *cx;
    JSObject *global=NULL,*clone=NULL,*scriptObject=NULL,*decodedObject=NULL;
    JSScript *script=NULL,*decoded=NULL;
    JSString *source=NULL;
    jsval result=JSVAL_VOID;
    JSXDRState *encoder=NULL,*decoder=NULL;
    void *data;
    uint32 length;
    int status=1;
    const char *program="var specimen={method(){return 3;}}.method;function modernMethod(){return !specimen.hasOwnProperty('caller')&&!specimen.hasOwnProperty('arguments')&&specimen()===3;}modernMethod()";
    const char *initialized="function modernGetter(){var o={get x(){return 4;}};var f=Object.getOwnPropertyDescriptor(o,'x').get;return !f.hasOwnProperty('caller')&&!f.hasOwnProperty('arguments');}modernGetter()";
    const char *legacy="function legacyField(){return 7;}legacyField.hasOwnProperty('caller')&&legacyField.hasOwnProperty('arguments')&&modernMethod()";
    if(!rt)return 1;
    cx=JS_NewContext(rt,8192);if(!cx){JS_DestroyRuntime(rt);return 1;}
    JS_BeginRequest(cx);
    JS_AddNamedRoot(cx,&global,"method fields global");JS_AddNamedRoot(cx,&scriptObject,"method fields script");
    JS_AddNamedRoot(cx,&decodedObject,"method fields decoded");JS_AddNamedRoot(cx,&source,"method fields source");
    JS_AddNamedRoot(cx,&clone,"method clone");
    JS_SetVersion(cx,JSVERSION_ECMA_2015);
    global=JS_NewObject(cx,&globalClass,NULL,NULL);CHECK(global);
    JS_SetGlobalObject(cx,global);CHECK(JS_InitStandardClasses(cx,global));
    script=JS_CompileScript(cx,global,program,strlen(program),"method-fields",1);
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
    source=JS_DecompileScript(cx,decoded,"method-fields-source",0);CHECK(source);
    JS_SetVersion(cx,JSVERSION_ECMA_2015);
    CHECK(JS_EvaluateUCScript(cx,global,JS_GetStringChars(source),JS_GetStringLength(source),"roundtrip",1,&result)&&result==JSVAL_TRUE);
    CHECK(JS_EvaluateScript(cx,global,"legacyField()===7&&modernMethod()",strlen("legacyField()===7&&modernMethod()"),"cross-edition",1,&result)&&result==JSVAL_TRUE);
    JS_SetVersion(cx,JSVERSION_DEFAULT);
    CHECK(JS_EvaluateScript(cx,global,"modernGetter()",strlen("modernGetter()"),"default",1,&result)&&result==JSVAL_TRUE);
    CHECK(JS_GetProperty(cx,global,"specimen",&result)&&JSVAL_IS_OBJECT(result));
    clone=JS_CloneFunctionObject(cx,JSVAL_TO_OBJECT(result),global);CHECK(clone);
    CHECK(JS_DefineProperty(cx,global,"clonedMethod",OBJECT_TO_JSVAL(clone),NULL,NULL,0));
    JS_GC(cx);
    JS_SetVersion(cx,JSVERSION_ECMA_2015);
    CHECK(JS_EvaluateScript(cx,global,"!clonedMethod.hasOwnProperty('caller')&&!clonedMethod.hasOwnProperty('arguments')&&clonedMethod()===3",strlen("!clonedMethod.hasOwnProperty('caller')&&!clonedMethod.hasOwnProperty('arguments')&&clonedMethod()===3"),"clone",1,&result)&&result==JSVAL_TRUE);
    printf("ES6-METHOD-FIELDS checks=%u failures=0\n",checks);status=0;
out:
    if(decoder){JS_XDRMemSetData(decoder,NULL,0);JS_XDRDestroy(decoder);}
    if(encoder)JS_XDRDestroy(encoder);
    JS_RemoveRoot(cx,&clone);
    JS_RemoveRoot(cx,&source);JS_RemoveRoot(cx,&decodedObject);JS_RemoveRoot(cx,&scriptObject);JS_RemoveRoot(cx,&global);
    JS_EndRequest(cx);JS_DestroyContext(cx);JS_DestroyRuntime(rt);JS_ShutDown();return status;
}
