/* Unicode regexp compilation, GC, cancellation and XDR.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "jsapi.h"
#include "jsxdrapi.h"
#include <stdio.h>
#include <string.h>
static unsigned checks, collections, ticks, stopAfter, compileTicks, reentries;
static JSBool reenter, inCallback;
static JSBool Branch(JSContext *cx, JSScript *script)
{
    jsval result;
    const char *nested = "/[a-z]/iu.test('K')";
    if (!script && reenter && !inCallback) {
        inCallback = JS_TRUE;
        ++reentries;
        JS_GC(cx);
        if (!JS_EvaluateScript(cx, JS_GetGlobalObject(cx), nested,
                               strlen(nested), "nested-regexp", 1, &result) ||
            result != JSVAL_TRUE) {
            inCallback = JS_FALSE;
            return JS_FALSE;
        }
        inCallback = JS_FALSE;
    }
    ++ticks;
    if (!script) ++compileTicks;
    if (stopAfter && !script && compileTicks >= stopAfter) return JS_FALSE;
    if (!(ticks % 31)) { ++collections; JS_GC(cx); }
    return JS_TRUE;
}
static JSClass globalClass = {
    "global", 0,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};
static JSBool Collect(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{ ++collections; JS_GC(cx); *rval = JSVAL_VOID; return JS_TRUE; }
static void Report(JSContext *cx, const char *message, JSErrorReport *report)
{ fprintf(stderr,"Unicode regexp: %s\n",message); }
#define CHECK(v) do { ++checks; if (!(v)) { fprintf(stderr,"FAIL Unicode regexp check %u\n",checks); goto out; } } while (0)
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
        "function unicode(s){gc();return /^[\\u{1F600}-\\u{1F64F}]+$/u.test(s)}"
        "var astral='\\uD83D\\uDE00';"
        "unicode(astral) && /[\\w]/iu.test('\\u212A') && "
        "!/[\\W]/iu.test('\\u017F') && "
        "/^(\\u{10400})\\1$/iu.test('\\uD801\\uDC00\\uD801\\uDC28') && "
        "eval('('+unicode.toString()+')')(astral)";
    const char *legacy = "/./.exec('\\uD83D\\uDE00')[0].length===1 && /\\a/.test('a')";
    if (!rt) return 1;
    cx = JS_NewContext(rt,8192);
    if (!cx) { JS_DestroyRuntime(rt); return 1; }
    JS_BeginRequest(cx); JS_SetErrorReporter(cx,Report);
    JS_AddNamedRoot(cx,&global,"regexp global");
    JS_AddNamedRoot(cx,&scriptObject,"regexp script");
    JS_AddNamedRoot(cx,&decodedObject,"regexp decoded script");
    JS_AddNamedRoot(cx,&source,"regexp source");
    JS_AddNamedRoot(cx,&result,"regexp result");
    JS_SetVersion(cx,JSVERSION_ECMA_2015);
    global = JS_NewObject(cx,&globalClass,NULL,NULL); CHECK(global);
    JS_SetGlobalObject(cx,global); CHECK(JS_InitStandardClasses(cx,global));
    CHECK(JS_DefineFunction(cx,global,"gc",Collect,0,0));
    JS_SetBranchCallback(cx,Branch);
    script = JS_CompileScript(cx,global,program,strlen(program),"regexp",1);
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
    CHECK(JS_EvaluateScript(cx,global,legacy,strlen(legacy),"legacy-regexp",1,&result) && result==JSVAL_TRUE);
    JS_SetVersion(cx,JSVERSION_ECMA_2015);
    source=JS_DecompileScript(cx,decoded,"regexp-source",0);
    CHECK(source && JS_EvaluateUCScript(cx,global,JS_GetStringChars(source),JS_GetStringLength(source),"roundtrip",1,&result) && result==JSVAL_TRUE);
    CHECK(collections>=3);
    reenter=JS_TRUE;
    { const char *nestedTest="/[a-z]/iu.test('K')";
      CHECK(JS_EvaluateScript(cx,global,nestedTest,strlen(nestedTest),"reentry",1,&result) && result==JSVAL_TRUE); }
    reenter=JS_FALSE;
    CHECK(reentries>=2);
    ticks=0; compileTicks=0; stopAfter=2;
    { const char *interrupted="new RegExp('[\\\\W]','iu')";
      CHECK(!JS_EvaluateScript(cx,global,interrupted,strlen(interrupted),"interrupted",1,&result)); }
    CHECK(compileTicks>=2);
    stopAfter=0; JS_ClearPendingException(cx);
    JS_SetVersion(cx,JSVERSION_DEFAULT);
    JS_SetGlobalObject(cx,NULL);
    global=JS_NewObject(cx,&globalClass,NULL,NULL); CHECK(global);
    CHECK(JS_SetParent(cx,global,NULL) && JS_SetPrototype(cx,global,NULL));
    JS_SetGlobalObject(cx,global); CHECK(JS_InitStandardClasses(cx,global));
    JS_SetVersion(cx,JSVERSION_ECMA_2015);
    { const char *mixed=
        "var face='\\uD83D\\uDE00';"
        "/^.$/u.test(face) && /[a-z]/iu.test('\\u212A') && "
        "/\\u{1F600}/u.unicode && face.match(/(?:)/ug).length===2 && "
        "face.replace(/(?:)/ug,'-')==='-'+face+'-' && "
        "(face+'x').split(/(?:)/u).join('|')===face+'|x' && "
        "new RegExp('\\\\u{1F600}','u').test(face)";
      CHECK(JS_EvaluateScript(cx,global,mixed,strlen(mixed),"mixed-window",1,&result) && result==JSVAL_TRUE); }
    JS_SetVersion(cx,JSVERSION_1_7);
    { const char *old="/a/('a')[0]==='a' && /./.exec(face)[0].length===1";
      CHECK(JS_EvaluateScript(cx,global,old,strlen(old),"legacy-after-modern",1,&result) && result==JSVAL_TRUE); }
    printf("ES6-REGEXP-UNICODE-EMBEDDING PASS checks=%u\n",checks); status=0;
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
