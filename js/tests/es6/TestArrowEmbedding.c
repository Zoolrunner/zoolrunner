/* Arrow lexical environments through the classic JSAPI and XDR.
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
static unsigned checks, allocationCalls;
static JSBool inAllocation, hookFailed;
static unsigned reentries;
#define CHECK(v) do { ++checks; if (!(v)) { \
    fprintf(stderr,"FAIL arrow embedding check %u\n",checks); goto out; \
} } while (0)

static JSBool
Evaluate(JSContext *cx, JSObject *global, const char *source, jsval *result)
{
    return JS_EvaluateScript(cx, global, source, strlen(source),
                             "arrow-embedding", 1, result);
}

static void
Allocation(JSContext *cx, JSObject *obj, JSBool creating, void *data)
{
    JSStackFrame *iterator = NULL, *frame;
    JSClass *clasp;
    jsval result;
    const char *program = "(()=>this)() === this && (()=>arguments[0])() === arguments[0]";
    if (!creating || inAllocation)
        return;
    inAllocation = JS_TRUE;
    ++allocationCalls;
    JS_GC(cx);
    clasp = JS_GetClass(cx, obj);
    if (clasp && !strcmp(clasp->name, "ArrowBinding")) {
        frame = JS_FrameIterator(cx, &iterator);
        if (frame && JS_GetFrameFunction(cx, frame)) {
            ++reentries;
            if (!JS_EvaluateInStackFrame(cx, frame, program, strlen(program),
                                        "arrow-reentry", 1, &result) ||
                result != JSVAL_TRUE)
                hookFailed = JS_TRUE;
            JS_GC(cx);
        }
    }
    inAllocation = JS_FALSE;
}

static void
Report(JSContext *cx, const char *message, JSErrorReport *report)
{
    fprintf(stderr, "arrow embedding: %s\n", message);
}

int main(void)
{
    JSRuntime *rt;
    JSContext *cx;
    JSObject *a = NULL, *b = NULL, *scriptObject = NULL, *decodedObject = NULL;
    JSObject *clone = NULL;
    JSString *source = NULL;
    JSScript *script = NULL, *decoded = NULL;
    JSXDRState *encoder = NULL, *decoder = NULL;
    jsval result, arrow = JSVAL_VOID, owner = JSVAL_VOID;
    uint32 length;
    void *data;
    int status = 1;
    const char *program =
        "var owner={};"
        "function make(v){var result={receiver:()=>this,argument:()=>arguments[0],"
        "target:()=>new.target,nested:()=>()=>this};v+=1;return result;}"
        "var bundle=make.call(owner,7);var arrow=bundle.receiver;"
        "var constructed=new make(11);"
        "bundle.receiver()===owner && bundle.argument()===8 && "
        "bundle.target()===undefined && bundle.nested()()===owner && "
        "constructed.target()===make && constructed.argument()===12 && "
        "(function(a){'use strict';var f=()=>arguments[0];a=9;return f;})(7)()===7 && "
        "(function(){'use strict';return ()=>eval('this');}).call(23)()===23 && "
        "(function(){function C(){return ()=>eval('new.target');}function D(){}"
        "return Reflect.construct(C,[],D)()===D;})()";

    rt = JS_NewRuntime(4 * 1024 * 1024);
    if (!rt) return 1;
    cx = JS_NewContext(rt, 8192);
    if (!cx) {JS_DestroyRuntime(rt);return 1;}
    JS_BeginRequest(cx);
    JS_SetErrorReporter(cx, Report);
    JS_AddNamedRoot(cx, &a, "arrow global A");
    JS_AddNamedRoot(cx, &b, "arrow global B");
    JS_AddNamedRoot(cx, &scriptObject, "arrow script");
    JS_AddNamedRoot(cx, &decodedObject, "decoded arrow script");
    JS_AddNamedRoot(cx, &clone, "cloned arrow");
    JS_AddNamedRoot(cx, &source, "arrow decompiled source");
    JS_AddNamedRoot(cx, &arrow, "saved arrow");
    JS_AddNamedRoot(cx, &owner, "saved lexical receiver");
    JS_SetVersion(cx, JSVERSION_ECMA_2015);
    a = JS_NewObject(cx, &globalClass, NULL, NULL);
    CHECK(a != NULL);
    JS_SetGlobalObject(cx, a);
    CHECK(JS_InitStandardClasses(cx, a));
    b = JS_NewObject(cx, &globalClass, NULL, NULL);
    CHECK(b && JS_SetParent(cx, b, NULL) && JS_SetPrototype(cx, b, NULL));
    JS_SetGlobalObject(cx, b);
    CHECK(JS_InitStandardClasses(cx, b));
    JS_SetGlobalObject(cx, a);
    script = JS_CompileScript(cx, a, program, strlen(program), "arrows", 1);
    CHECK(script && (scriptObject = JS_NewScriptObject(cx, script)) != NULL);
    JS_SetObjectHook(rt, Allocation, NULL);
    {
        JSBool executed = JS_ExecuteScript(cx, a, script, &result);
        if (!executed) JS_ReportPendingException(cx);
        if (!executed || result != JSVAL_TRUE)
            fprintf(stderr, "executed=%d result=%ld reentries=%u hookFailed=%d\n",
                    executed, (long)result, reentries, hookFailed);
        CHECK(executed && result == JSVAL_TRUE);
    }
    JS_SetObjectHook(rt, NULL, NULL);
    CHECK(allocationCalls > 0);
    CHECK(reentries > 0 && !hookFailed);
    CHECK(JS_GetProperty(cx, a, "arrow", &arrow) && JS_GetProperty(cx, a, "owner", &owner));
    JS_GC(cx);
    JS_SetGlobalObject(cx, b);
    CHECK(JS_CallFunctionValue(cx, b, arrow, 0, NULL, &result) && result == owner);
    clone = JS_CloneFunctionObject(cx, JSVAL_TO_OBJECT(arrow), b);
    CHECK(clone != NULL);
    JS_GC(cx);
    CHECK(JS_CallFunctionValue(cx, b, OBJECT_TO_JSVAL(clone), 0, NULL, &result) && result == owner);
    encoder = JS_XDRNewMem(cx, JSXDR_ENCODE);
    decoder = JS_XDRNewMem(cx, JSXDR_DECODE);
    CHECK(encoder && decoder && JS_XDRScript(encoder, &script));
    data = JS_XDRMemGetData(encoder, &length);
    JS_XDRMemSetData(decoder, data, length);
    JS_SetVersion(cx, JSVERSION_DEFAULT);
    CHECK(JS_XDRScript(decoder, &decoded));
    CHECK((decodedObject = JS_NewScriptObject(cx, decoded)) != NULL);
    JS_GC(cx);
    CHECK(JS_ExecuteScript(cx, b, decoded, &result) && result == JSVAL_TRUE);
    CHECK(JS_GetVersion(cx) == JSVERSION_DEFAULT);
    JS_SetVersion(cx, JSVERSION_ECMA_2015);
    source = JS_DecompileScript(cx, decoded, "arrow-roundtrip", 0);
    CHECK(source && JS_EvaluateUCScript(cx, b, JS_GetStringChars(source),
          JS_GetStringLength(source), "decompiled-arrows", 1, &result) && result == JSVAL_TRUE);
    CHECK(Evaluate(cx, b,"var f=(x)=>x; !f.hasOwnProperty('prototype') && "
          "!f.hasOwnProperty('arguments') && !f.hasOwnProperty('caller')",&result)
          && result == JSVAL_TRUE);
    JS_SetErrorReporter(cx, NULL);
    JS_SetVersion(cx, JSVERSION_1_7);
    CHECK(!Evaluate(cx, a, "var legacyArrow=()=>1;", &result));
    JS_ClearPendingException(cx);
    status = 0;
    printf("ES6-ARROW-EMBEDDING PASS checks=%u\n", checks);
out:
    JS_SetObjectHook(rt, NULL, NULL);
    if (decoder) {JS_XDRMemSetData(decoder, NULL, 0);JS_XDRDestroy(decoder);}
    if (encoder) JS_XDRDestroy(encoder);
    if (decoded && !decodedObject) JS_DestroyScript(cx, decoded);
    if (script && !scriptObject) JS_DestroyScript(cx, script);
    JS_RemoveRoot(cx, &owner);JS_RemoveRoot(cx, &arrow);
    JS_RemoveRoot(cx, &source);JS_RemoveRoot(cx, &clone);
    JS_RemoveRoot(cx, &decodedObject);JS_RemoveRoot(cx, &scriptObject);
    JS_RemoveRoot(cx, &b);JS_RemoveRoot(cx, &a);
    JS_EndRequest(cx);
    JS_DestroyContext(cx);
    JS_DestroyRuntime(rt);
    JS_ShutDown();
    return status;
}
