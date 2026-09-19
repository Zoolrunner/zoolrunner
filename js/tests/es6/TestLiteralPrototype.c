/* Prototype literal syntax bypasses public setters and preserves embedding hooks.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "jsapi.h"
#include "jsxdrapi.h"
#include <stdio.h>
#include <string.h>
static unsigned checks, writes;
static JSBool deny;
static JSBool Access(JSContext *cx,JSObject *obj,jsval id,JSAccessMode mode,jsval *vp)
{
    if ((mode & JSACC_TYPEMASK) != JSACC_PROTO || !(mode & JSACC_WRITE))
        return JS_TRUE;
    if (!JSVAL_IS_STRING(id) || strcmp(JS_GetStringBytes(JSVAL_TO_STRING(id)),"__proto__"))
        return JS_TRUE;
    ++writes;
    if (deny) { JS_ReportError(cx,"embedding rejects prototype"); return JS_FALSE; }
    /* Scripted-setter checks use the same mode and pass the callable itself.
     * Preserve that callable when exercising the explicit legacy path. */
    if (JS_TypeOfValue(cx,*vp)!=JSTYPE_FUNCTION) *vp=JSVAL_NULL;
    JS_GC(cx);
    return JS_TRUE;
}
static JSClass globalClass={"global",0,JS_PropertyStub,JS_PropertyStub,
 JS_PropertyStub,JS_PropertyStub,JS_EnumerateStub,JS_ResolveStub,JS_ConvertStub,
 JS_FinalizeStub,JSCLASS_NO_OPTIONAL_MEMBERS};
#define CHECK(c) do{++checks;if(!(c)){fprintf(stderr,"FAIL literal prototype check %u\n",checks);goto out;}}while(0)
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
    const char *program="var calls=0;Object.defineProperty(Object.prototype,\"__proto__\",{set:function(){++calls;throw Error(\"setter\")},configurable:true});function modernBlock(){var before=calls,p={marker:9},x={__proto__:p};return Object.getPrototypeOf(x)===p&&x.marker===9&&calls===before;}modernBlock()";
    const char *initialized="function modernASI(){return Object.getPrototypeOf({__proto__:null})===null;}modernASI()";
    const char *legacy="function legacyComment(){var before=calls;try{var observed={__proto__:{}};return observed===null;}catch(e){return calls===before+1}return false;}legacyComment()&&modernBlock()";
    if(!rt)return 1;
    cx=JS_NewContext(rt,8192);if(!cx){JS_DestroyRuntime(rt);return 1;}
    JS_BeginRequest(cx);
    JS_AddNamedRoot(cx,&global,"literal prototype global");JS_AddNamedRoot(cx,&scriptObject,"literal prototype script");
    JS_AddNamedRoot(cx,&decodedObject,"literal prototype decoded");JS_AddNamedRoot(cx,&source,"literal prototype source");
    JS_SetVersion(cx,JSVERSION_ECMA_2015);
    global=JS_NewObject(cx,&globalClass,NULL,NULL);CHECK(global);
    JS_SetGlobalObject(cx,global);CHECK(JS_InitStandardClasses(cx,global));
    JS_SetCheckObjectAccessCallback(rt,Access);
    script=JS_CompileScript(cx,global,program,strlen(program),"literal-prototype",1);
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
    source=JS_DecompileScript(cx,decoded,"literal-prototype-source",0);CHECK(source);
    JS_SetVersion(cx,JSVERSION_ECMA_2015);
    CHECK(JS_EvaluateUCScript(cx,global,JS_GetStringChars(source),JS_GetStringLength(source),"roundtrip",1,&result)&&result==JSVAL_TRUE);
    CHECK(JS_EvaluateScript(cx,global,"legacyComment()&&modernBlock()",strlen("legacyComment()&&modernBlock()"),"cross-edition",1,&result)&&result==JSVAL_TRUE);
    JS_SetVersion(cx,JSVERSION_DEFAULT);
    CHECK(JS_EvaluateScript(cx,global,"modernASI()",strlen("modernASI()"),"default",1,&result)&&result==JSVAL_TRUE);
    CHECK(writes>0);
    deny=JS_TRUE;
    CHECK(JS_EvaluateScript(cx,global,"var denied=false;try{modernBlock()}catch(e){denied=true}denied",strlen("var denied=false;try{modernBlock()}catch(e){denied=true}denied"),"denied",1,&result)&&result==JSVAL_TRUE);
    printf("ES6-LITERAL-PROTOTYPE checks=%u failures=0\n",checks);status=0;
out:
    JS_SetCheckObjectAccessCallback(rt,NULL);
    if(decoder){JS_XDRMemSetData(decoder,NULL,0);JS_XDRDestroy(decoder);}
    if(encoder)JS_XDRDestroy(encoder);
    JS_RemoveRoot(cx,&source);JS_RemoveRoot(cx,&decodedObject);JS_RemoveRoot(cx,&scriptObject);JS_RemoveRoot(cx,&global);
    JS_EndRequest(cx);JS_DestroyContext(cx);JS_DestroyRuntime(rt);JS_ShutDown();return status;
}
