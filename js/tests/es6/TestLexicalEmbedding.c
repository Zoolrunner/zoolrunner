/* Lexical initialization through the classic JSAPI and XDR.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "jsapi.h"
#include "jsxdrapi.h"
#include "jsdbgapi.h"
#include <stdio.h>
#include <string.h>

static JSClass globalClass = {
    "global", 0,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};
static unsigned checks, allocationCalls, reentries, constReentries, iterationReentries;
static unsigned forOfReentries;
static JSBool inAllocation, hookFailed;
#define CHECK(v) do { ++checks; if (!(v)) { \
    fprintf(stderr,"FAIL lexical embedding check %u\n",checks); goto out; \
} } while (0)

static JSBool
Evaluate(JSContext *cx, JSObject *global, const char *source, jsval *result)
{
    return JS_EvaluateScript(cx, global, source, strlen(source),
                             "lexical-embedding", 1, result);
}

static void
Allocation(JSContext *cx, JSObject *obj, JSBool creating, void *data)
{
    JSStackFrame *iterator = NULL, *frame;
    JSFunction *fun;
    JSString *name;
    JSClass *clasp;
    jsval result, phase;
    const char *program =
        "(function(){try{typeof x;return false}"
        "catch(e){return e instanceof ReferenceError}})()";
    if (!creating || inAllocation)
        return;
    inAllocation = JS_TRUE;
    ++allocationCalls;
    JS_GC(cx);
    clasp = JS_GetClass(cx, obj);
    frame = JS_FrameIterator(cx, &iterator);
    fun = frame ? JS_GetFrameFunction(cx, frame) : NULL;
    name = fun ? JS_GetFunctionId(fun) : NULL;
    if (clasp && !strcmp(clasp->name, "Function") && name &&
        (!strcmp(JS_GetStringBytes(name), "lexicalProbe") ||
         !strcmp(JS_GetStringBytes(name), "constProbe"))) {
        ++reentries;
        if (!strcmp(JS_GetStringBytes(name), "constProbe"))
            ++constReentries;
        if (!JS_EvaluateInStackFrame(cx, frame, program, strlen(program),
                                    "lexical-reentry", 1, &result) ||
            result != JSVAL_TRUE)
            hookFailed = JS_TRUE;
        JS_GC(cx);
    }
    if (clasp && !strcmp(clasp->name, "Block") && name &&
        !strcmp(JS_GetStringBytes(name), "loopProbe") &&
        JS_GetProperty(cx, JS_GetGlobalObject(cx), "phase", &phase) &&
        phase == JSVAL_TRUE) {
        const char *capture = "saved.push(()=>i);true";
        ++iterationReentries;
        if (!JS_EvaluateInStackFrame(cx, frame, capture, strlen(capture),
                                    "loop-reentry", 1, &result) ||
            result != JSVAL_TRUE)
            hookFailed = JS_TRUE;
        JS_GC(cx);
    }
    if (clasp && !strcmp(clasp->name, "For Of State") && name &&
        !strcmp(JS_GetStringBytes(name), "forOfProbe")) {
        const char *capture = "headCaptures.push(()=>x);true";
        ++forOfReentries;
        if (!JS_EvaluateInStackFrame(cx, frame, capture, strlen(capture),
                                    "for-of-reentry", 1, &result) ||
            result != JSVAL_TRUE)
            hookFailed = JS_TRUE;
        JS_GC(cx);
    }
    inAllocation = JS_FALSE;
}

static void
Report(JSContext *cx, const char *message, JSErrorReport *report)
{
    fprintf(stderr, "lexical embedding: %s\n", message);
}

int main(void)
{
    JSRuntime *rt;
    JSContext *cx;
    JSObject *global = NULL, *scriptObject = NULL, *decodedObject = NULL;
    JSString *source = NULL;
    JSScript *script = NULL, *decoded = NULL;
    JSXDRState *encoder = NULL, *decoder = NULL;
    jsval result = JSVAL_VOID;
    uint32 length;
    void *data;
    int status = 1;
    const char *program =
        "function lexicalProbe(){return inner;let x=7;"
        "function inner(){return x}}"
        "var escaped=lexicalProbe();"
        "function initialized(){let a=3,b=a+4,c;return ()=>a+b+(c===undefined)}"
        "var live=initialized();"
        "function caught(){try{escaped();return false}"
        "catch(e){return e instanceof ReferenceError}}"
        "function constProbe(){return inner;const x=7;"
        "function inner(){return x}}"
        "var escapedConst=constProbe();"
        "var immutable=(function(){const x=7;return ()=>{try{x=8;return false}"
        "catch(e){return e instanceof TypeError}}})();"
        "function caughtConst(){try{escapedConst();return false}"
        "catch(e){return e instanceof ReferenceError}}"
        "var phase=false,saved=[];"
        "function loopProbe(){var r=[];for(let i=0;i<3;i++){"
        "r.push(()=>i);phase=true}phase=false;return r}"
        "var loopValues=loopProbe();"
        "function values(r){return r.map(function(f){return f()}).join()}"
        "var keys=[];for(const key in {a:1,b:2})keys.push(()=>key);"
        "var headCaptures=[],forOfValues=[];"
        "function forOfProbe(){for(let x of [3,4])forOfValues.push(()=>x)}"
        "forOfProbe();"
        "function headPending(){try{headCaptures[0]();return false}"
        "catch(e){return e instanceof ReferenceError}}"
        "caught() && caughtConst() && immutable() && live()===11 && "
        "values(forOfValues)==='3,4' && "
        "values(loopValues)==='0,1,2' && values(keys)==='a,b'";

    rt = JS_NewRuntime(4 * 1024 * 1024);
    if (!rt) return 1;
    cx = JS_NewContext(rt, 8192);
    if (!cx) {JS_DestroyRuntime(rt);return 1;}
    JS_BeginRequest(cx);
    JS_SetErrorReporter(cx, Report);
    JS_AddNamedRoot(cx, &global, "lexical global");
    JS_AddNamedRoot(cx, &scriptObject, "lexical script");
    JS_AddNamedRoot(cx, &decodedObject, "decoded lexical script");
    JS_AddNamedRoot(cx, &source, "lexical decompiled source");
    JS_AddNamedRoot(cx, &result, "lexical result");
    JS_SetVersion(cx, JSVERSION_ECMA_2015);
    global = JS_NewObject(cx, &globalClass, NULL, NULL);
    CHECK(global != NULL);
    JS_SetGlobalObject(cx, global);
    CHECK(JS_InitStandardClasses(cx, global));
    script = JS_CompileScript(cx, global, program, strlen(program), "lexicals", 1);
    CHECK(script && (scriptObject = JS_NewScriptObject(cx, script)) != NULL);
    JS_SetObjectHook(rt, Allocation, NULL);
    CHECK(JS_ExecuteScript(cx, global, script, &result) && result == JSVAL_TRUE);
    JS_SetObjectHook(rt, NULL, NULL);
    CHECK(allocationCalls > 0 && reentries > 0 && !hookFailed);
    CHECK(constReentries > 0 && iterationReentries == 3);
    CHECK(forOfReentries == 1);
    CHECK(Evaluate(cx, global, "headPending()", &result) && result == JSVAL_TRUE);
    CHECK(Evaluate(cx, global, "values(saved)==='0,1,2'", &result) && result == JSVAL_TRUE);
    JS_GC(cx);
    CHECK(Evaluate(cx, global, "caught() && live()===11", &result) && result == JSVAL_TRUE);
    CHECK(Evaluate(cx, global, "caughtConst() && immutable()", &result) && result == JSVAL_TRUE);
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
    source = JS_DecompileScript(cx, decoded, "lexical-roundtrip", 0);
    CHECK(source && JS_EvaluateUCScript(cx, global, JS_GetStringChars(source),
          JS_GetStringLength(source), "decompiled-lexicals", 1, &result) && result == JSVAL_TRUE);
    JS_SetVersion(cx, JSVERSION_1_7);
    CHECK(Evaluate(cx, global,
          "(function(){var result;{result=x;let x=7}return result===undefined})()",
          &result) && result == JSVAL_TRUE);
    CHECK(Evaluate(cx, global,
          "(function(){var x=7;return let(x=x+1) x})()===8",
          &result) && result == JSVAL_TRUE);
    status = 0;
    printf("ES6-LEXICAL-EMBEDDING PASS checks=%u\n", checks);
out:
    JS_SetObjectHook(rt, NULL, NULL);
    if (decoder) {JS_XDRMemSetData(decoder, NULL, 0);JS_XDRDestroy(decoder);}
    if (encoder) JS_XDRDestroy(encoder);
    if (decoded && !decodedObject) JS_DestroyScript(cx, decoded);
    if (script && !scriptObject) JS_DestroyScript(cx, script);
    JS_RemoveRoot(cx, &result);JS_RemoveRoot(cx, &source);
    JS_RemoveRoot(cx, &decodedObject);JS_RemoveRoot(cx, &scriptObject);
    JS_RemoveRoot(cx, &global);
    JS_EndRequest(cx);
    JS_DestroyContext(cx);
    JS_DestroyRuntime(rt);
    JS_ShutDown();
    return status;
}
