/* URI decoding observes the executing edition through XDR and native calls.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "jsapi.h"
#include "jsxdrapi.h"
#include <stdio.h>
#include <string.h>
static unsigned checks;
static JSClass globalClass={"global",0,JS_PropertyStub,JS_PropertyStub,
 JS_PropertyStub,JS_PropertyStub,JS_EnumerateStub,JS_ResolveStub,JS_ConvertStub,
 JS_FinalizeStub,JSCLASS_NO_OPTIONAL_MEMBERS};
#define CHECK(c) do{++checks;if(!(c)){fprintf(stderr,"FAIL URI decoding check %u\n",checks);goto out;}}while(0)
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
    const char *program="function modernURI(){try{decodeURIComponent('%ED%BF%BF');}catch(e){return e instanceof URIError;}return false;}modernURI()";
    const char *initialized="function convertedURI(){var count=0;try{decodeURI({toString:function(){++count;return '%C0%80';}});}catch(e){return e instanceof URIError&&count===1;}return false;}convertedURI()";
    const char *legacy="function legacyURI(){return decodeURIComponent('%ED%BF%BF').charCodeAt(0)===0xdfff&&decodeURI('%C0%80').charCodeAt(0)===0xfffd;}legacyURI()&&modernURI()";
    if(!rt)return 1;
    cx=JS_NewContext(rt,8192);if(!cx){JS_DestroyRuntime(rt);return 1;}
    JS_BeginRequest(cx);
    JS_AddNamedRoot(cx,&global,"URI decoding global");JS_AddNamedRoot(cx,&scriptObject,"URI decoding script");
    JS_AddNamedRoot(cx,&decodedObject,"URI decoding decoded");JS_AddNamedRoot(cx,&source,"URI decoding source");
    JS_SetVersion(cx,JSVERSION_ECMA_2015);
    global=JS_NewObject(cx,&globalClass,NULL,NULL);CHECK(global);
    JS_SetGlobalObject(cx,global);CHECK(JS_InitStandardClasses(cx,global));
    script=JS_CompileScript(cx,global,program,strlen(program),"uri-decoding",1);
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
    source=JS_DecompileScript(cx,decoded,"uri-decoding-source",0);CHECK(source);
    JS_SetVersion(cx,JSVERSION_ECMA_2015);
    CHECK(JS_EvaluateUCScript(cx,global,JS_GetStringChars(source),JS_GetStringLength(source),"roundtrip",1,&result)&&result==JSVAL_TRUE);
    CHECK(JS_EvaluateScript(cx,global,"legacyURI()&&modernURI()",strlen("legacyURI()&&modernURI()"),"cross-edition",1,&result)&&result==JSVAL_TRUE);
    JS_SetVersion(cx,JSVERSION_DEFAULT);
    CHECK(JS_EvaluateScript(cx,global,"convertedURI()",strlen("convertedURI()"),"default",1,&result)&&result==JSVAL_TRUE);
    printf("ES6-URI-DECODING checks=%u failures=0\n",checks);status=0;
out:
    if(decoder){JS_XDRMemSetData(decoder,NULL,0);JS_XDRDestroy(decoder);}
    if(encoder)JS_XDRDestroy(encoder);
    JS_RemoveRoot(cx,&source);JS_RemoveRoot(cx,&decodedObject);JS_RemoveRoot(cx,&scriptObject);JS_RemoveRoot(cx,&global);
    JS_EndRequest(cx);JS_DestroyContext(cx);JS_DestroyRuntime(rt);JS_ShutDown();return status;
}
