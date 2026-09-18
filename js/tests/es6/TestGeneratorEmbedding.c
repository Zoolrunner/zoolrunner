/* Generator ownership, native reentry and serialization. MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "jsapi.h"
#include "jsdbgapi.h"
#include "jsxdrapi.h"
#include <stdio.h>
#include <string.h>

static JSClass globalClass = {
    "global", 0,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};
static unsigned checks, allocations, reentries;
static JSBool inHook, hookFailed;
#define CHECK(v) do { ++checks; if (!(v)) { \
    fprintf(stderr,"FAIL generator embedding check %u\n",checks); goto out; \
} } while (0)

static void
Report(JSContext *cx, const char *message, JSErrorReport *report)
{ fprintf(stderr,"generator embedding: %s\n",message); }

static void
Allocation(JSContext *cx, JSObject *obj, JSBool creating, void *data)
{
    JSStackFrame *cursor = NULL, *frame;
    JSFunction *fun;
    JSString *name;
    jsval result;
    const char *source = "(function(){try{iterator.next();return false}"
                         "catch(e){return e instanceof TypeError}})()";
    if (!creating || inHook) return;
    inHook = JS_TRUE;
    ++allocations;
    JS_GC(cx);
    frame = JS_FrameIterator(cx, &cursor);
    fun = frame ? JS_GetFrameFunction(cx, frame) : NULL;
    name = fun ? JS_GetFunctionId(fun) : NULL;
    if (frame && JS_GetFramePC(cx, frame) && name &&
        !strcmp(JS_GetStringBytes(name), "generatorProbe")) {
        ++reentries;
        if (!JS_EvaluateInStackFrame(cx, frame, source, strlen(source),
                                    "generator-reentry", 1, &result) ||
            result != JSVAL_TRUE)
            hookFailed = JS_TRUE;
    }
    inHook = JS_FALSE;
}

int main(void)
{
    JSRuntime *rt;
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
        "var iterator,capture;"
        "function* generatorProbe(a){let x={value:a};yield ()=>x;"
        "yield* [a+1];try{yield a+2}finally{yield a+3}}"
        "iterator=generatorProbe(3);"
        "capture=iterator.next().value;"
        "capture().value===3 && iterator.next().value===4 && "
        "iterator.next().value===5 && iterator.return({value:9}).value===6";
    const char *finish =
        "(function(){var r=iterator.next();return r.done && r.value.value===9 && "
        "capture().value===3 && iterator.next().done})()";
    rt = JS_NewRuntime(4 * 1024 * 1024);
    if (!rt) return 1;
    cx = JS_NewContext(rt, 8192);
    if (!cx) {JS_DestroyRuntime(rt);return 1;}
    JS_BeginRequest(cx);
    JS_SetErrorReporter(cx, Report);
    JS_AddNamedRoot(cx, &global, "generator global");
    JS_AddNamedRoot(cx, &scriptObject, "generator script");
    JS_AddNamedRoot(cx, &decodedObject, "generator decoded script");
    JS_AddNamedRoot(cx, &source, "generator source");
    JS_AddNamedRoot(cx, &result, "generator result");
    JS_SetVersion(cx, JSVERSION_ECMA_2015);
    global = JS_NewObject(cx, &globalClass, NULL, NULL);
    CHECK(global != NULL);
    JS_SetGlobalObject(cx, global);
    CHECK(JS_InitStandardClasses(cx, global));
    JS_SetObjectHook(rt, Allocation, NULL);
    script = JS_CompileScript(cx, global, program, strlen(program), "generators", 1);
    CHECK(script && (scriptObject = JS_NewScriptObject(cx, script)) != NULL);
    CHECK(JS_ExecuteScript(cx, global, script, &result) && result == JSVAL_TRUE);
    CHECK(allocations > 0 && reentries > 0 && !hookFailed);
    JS_SetObjectHook(rt, NULL, NULL);
    JS_GC(cx);
    CHECK(JS_EvaluateScript(cx, global, finish, strlen(finish), "finish", 1, &result) && result == JSVAL_TRUE);
    encoder = JS_XDRNewMem(cx, JSXDR_ENCODE);
    decoder = JS_XDRNewMem(cx, JSXDR_DECODE);
    CHECK(encoder && decoder && JS_XDRScript(encoder, &script));
    data = JS_XDRMemGetData(encoder, &length);
    JS_XDRMemSetData(decoder, data, length);
    JS_SetVersion(cx, JSVERSION_DEFAULT);
    CHECK(JS_XDRScript(decoder, &decoded));
    CHECK((decodedObject = JS_NewScriptObject(cx, decoded)) != NULL);
    JS_GC(cx);
    CHECK(JS_ExecuteScript(cx, global, decoded, &result) && result == JSVAL_TRUE);
    CHECK(JS_GetVersion(cx) == JSVERSION_DEFAULT);
    JS_SetVersion(cx, JSVERSION_ECMA_2015);
    CHECK(JS_EvaluateScript(cx, global, finish, strlen(finish), "finish-decoded", 1, &result) && result == JSVAL_TRUE);
    source = JS_DecompileScript(cx, decoded, "generator-source", 0);
    CHECK(source && JS_EvaluateUCScript(cx, global, JS_GetStringChars(source),
          JS_GetStringLength(source), "generator-roundtrip", 1, &result) && result == JSVAL_TRUE);
    CHECK(JS_EvaluateScript(cx, global, finish, strlen(finish), "finish-roundtrip", 1, &result) && result == JSVAL_TRUE);
    status = 0;
    printf("ES6-GENERATOR-EMBEDDING PASS checks=%u\n",checks);
out:
    JS_SetObjectHook(rt, NULL, NULL);
    if (decoder) {JS_XDRMemSetData(decoder, NULL, 0);JS_XDRDestroy(decoder);}
    if (encoder) JS_XDRDestroy(encoder);
    if (decoded && !decodedObject) JS_DestroyScript(cx, decoded);
    if (script && !scriptObject) JS_DestroyScript(cx, script);
    JS_RemoveRoot(cx, &result);JS_RemoveRoot(cx, &source);
    JS_RemoveRoot(cx, &decodedObject);JS_RemoveRoot(cx, &scriptObject);
    JS_RemoveRoot(cx, &global);
    JS_EndRequest(cx);JS_DestroyContext(cx);JS_DestroyRuntime(rt);JS_ShutDown();
    return status;
}
