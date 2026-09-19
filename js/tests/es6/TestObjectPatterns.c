/* Object-pattern coercibility and source notes through GC and XDR.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "jsapi.h"
#include "jsxdrapi.h"
#include <stdio.h>
#include <string.h>
static unsigned checks, collections;
static JSClass globalClass = {
    "global", 0,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};
static JSBool Collect(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{ ++collections; JS_GC(cx); *rval = JSVAL_VOID; return JS_TRUE; }
static void Report(JSContext *cx, const char *message, JSErrorReport *report)
{ fprintf(stderr,"object patterns: %s\n",message); }
#define CHECK(v) do { ++checks; if (!(v)) { fprintf(stderr,"FAIL object patterns check %u\n",checks); goto out; } } while (0)
int main(void)
{
    JSRuntime *rt = JS_NewRuntime(4 * 1024 * 1024);
    JSContext *cx;
    JSObject *global = NULL, *scriptObject = NULL, *decodedObject = NULL;
    JSScript *script = NULL, *decoded = NULL;
    JSString *source = NULL;
    jsval result = JSVAL_VOID;
    JSXDRState *encoder = NULL, *decoder = NULL;
    void *data;
    uint32 length;
    int status = 1;
    const char *program =
        "function pattern(o){var {nested:{x},blank:{}}=o;return x}"
        "function empty(o){({}=o);return true}"
        "var hits=0;try{empty(null)}catch(e){if(e instanceof TypeError)hits++}"
        "var copy=eval('('+empty.toString()+')');"
        "try{copy(undefined)}catch(e){if(e instanceof TypeError)hits++}"
        "function computed(o,a,b){var {[(a,b)]:x}=o;return x}"
        "var symbol=Symbol('key'),values={},key={};values[symbol]=21;"
        "key[Symbol.toPrimitive]=function(){gc();return symbol};"
        "pattern({get nested(){gc();return {x:9}},blank:3})===9 && hits===2 && "
        "computed(values,null,key)===21";
    const char *legacy = "function old(v){var [a,b]=v;return a+b}old([3,4])===7";
    if (!rt) return 1;
    cx = JS_NewContext(rt,8192);
    if (!cx) { JS_DestroyRuntime(rt); return 1; }
    JS_BeginRequest(cx); JS_SetErrorReporter(cx,Report);
    JS_AddNamedRoot(cx,&global,"pattern global");
    JS_AddNamedRoot(cx,&scriptObject,"pattern script");
    JS_AddNamedRoot(cx,&decodedObject,"pattern decoded script");
    JS_AddNamedRoot(cx,&source,"pattern source");
    JS_AddNamedRoot(cx,&result,"pattern result");
    JS_SetVersion(cx,JSVERSION_ECMA_2015);
    global = JS_NewObject(cx,&globalClass,NULL,NULL); CHECK(global);
    JS_SetGlobalObject(cx,global); CHECK(JS_InitStandardClasses(cx,global));
    CHECK(JS_DefineFunction(cx,global,"gc",Collect,0,0));
    script = JS_CompileScript(cx,global,program,strlen(program),"patterns",1);
    CHECK(script && (scriptObject=JS_NewScriptObject(cx,script))!=NULL);
    CHECK(JS_ExecuteScript(cx,global,script,&result) && result==JSVAL_TRUE);
    encoder=JS_XDRNewMem(cx,JSXDR_ENCODE); decoder=JS_XDRNewMem(cx,JSXDR_DECODE);
    CHECK(encoder && decoder && JS_XDRScript(encoder,&script));
    data=JS_XDRMemGetData(encoder,&length); JS_XDRMemSetData(decoder,data,length);
    JS_SetVersion(cx,JSVERSION_1_7);
    CHECK(JS_XDRScript(decoder,&decoded));
    CHECK((decodedObject=JS_NewScriptObject(cx,decoded))!=NULL);
    JS_GC(cx);
    CHECK(JS_ExecuteScript(cx,global,decoded,&result) && result==JSVAL_TRUE);
    CHECK(JS_GetVersion(cx)==JSVERSION_1_7);
    CHECK(JS_EvaluateScript(cx,global,legacy,strlen(legacy),"legacy-pattern",1,&result) && result==JSVAL_TRUE);
    JS_SetVersion(cx,JSVERSION_ECMA_2015);
    source=JS_DecompileScript(cx,decoded,"pattern-source",0);
    CHECK(source && JS_EvaluateUCScript(cx,global,JS_GetStringChars(source),JS_GetStringLength(source),"roundtrip",1,&result) && result==JSVAL_TRUE);
    CHECK(collections>=3);
    printf("ES6-OBJECT-PATTERN-EMBEDDING PASS checks=%u\n",checks); status=0;
  out:
    if (decoder) { JS_XDRMemSetData(decoder,NULL,0); JS_XDRDestroy(decoder); }
    if (encoder) JS_XDRDestroy(encoder);
    if (decoded && !decodedObject) JS_DestroyScript(cx,decoded);
    if (script && !scriptObject) JS_DestroyScript(cx,script);
    JS_RemoveRoot(cx,&result); JS_RemoveRoot(cx,&source);
    JS_RemoveRoot(cx,&decodedObject); JS_RemoveRoot(cx,&scriptObject); JS_RemoveRoot(cx,&global);
    JS_EndRequest(cx); JS_DestroyContext(cx); JS_DestroyRuntime(rt); JS_ShutDown();
    return status;
}
