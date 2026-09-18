/* Rest parameter environments through the classic JSAPI and XDR.
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
    fprintf(stderr,"FAIL rest embedding check %u\n",checks); goto out; \
} } while (0)

static JSBool
Evaluate(JSContext *cx, JSObject *global, const char *source, jsval *result)
{
    return JS_EvaluateScript(cx, global, source, strlen(source),
                             "rest-embedding", 1, result);
}

static void
Allocation(JSContext *cx, JSObject *obj, JSBool creating, void *data)
{
    JSStackFrame *iterator = NULL, *frame;
    JSClass *clasp;
    jsval result;
    const char *program = "((...values)=>values[0])(19) === 19";
    if (!creating || inAllocation)
        return;
    inAllocation = JS_TRUE;
    ++allocationCalls;
    JS_GC(cx);
    clasp = JS_GetClass(cx, obj);
    if (clasp && !strcmp(clasp->name, "Array")) {
        frame = JS_FrameIterator(cx, &iterator);
        if (frame && JS_GetFrameFunction(cx, frame)) {
            ++reentries;
            if (!JS_EvaluateInStackFrame(cx, frame, program, strlen(program),
                                        "rest-reentry", 1, &result) ||
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
    fprintf(stderr, "rest embedding: %s\n", message);
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
    jsval result = JSVAL_VOID, rest = JSVAL_VOID, owner = JSVAL_VOID;
    jsval values[2] = {INT_TO_JSVAL(7), INT_TO_JSVAL(8)}, element;
    uint32 length;
    void *data;
    int status = 1;
    const char *program =
        "var owner={};var collector=(...r)=>r;"
        "function make(v,...tail){var result={receiver:()=>this,argument:()=>tail[0],"
        "target:()=>new.target,nested:()=>()=>this};tail[0]+=1;return result;}"
        "var bundle=make.call(owner,7,8);var rest=bundle.receiver;"
        "var constructed=new make(11,12);"
        "bundle.receiver()===owner && bundle.argument()===9 && "
        "bundle.target()===undefined && bundle.nested()()===owner && "
        "constructed.target()===make && constructed.argument()===13 && "
        "(function(a,...r){var args=arguments;args[0];a=9;return args;})(7)[0]===7 && "
        "(function(a,...r){arguments[0]=9;return a;})(7)===7 && "
        "(function(){'use strict';return ((...r)=>this)(1);}).call(23)===23 && "
        "(function(a,...r){return eval('arguments[0]');})(7)===7";

    rt = JS_NewRuntime(4 * 1024 * 1024);
    if (!rt) return 1;
    cx = JS_NewContext(rt, 8192);
    if (!cx) {JS_DestroyRuntime(rt);return 1;}
    JS_BeginRequest(cx);
    JS_SetErrorReporter(cx, Report);
    JS_AddNamedRoot(cx, &a, "rest global A");
    JS_AddNamedRoot(cx, &b, "rest global B");
    JS_AddNamedRoot(cx, &scriptObject, "rest script");
    JS_AddNamedRoot(cx, &decodedObject, "decoded rest script");
    JS_AddNamedRoot(cx, &clone, "cloned rest");
    JS_AddNamedRoot(cx, &source, "rest decompiled source");
    JS_AddNamedRoot(cx, &rest, "saved rest");
    JS_AddNamedRoot(cx, &owner, "saved lexical receiver");
    JS_AddNamedRoot(cx, &result, "rest call result");
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
    script = JS_CompileScript(cx, a, program, strlen(program), "rests", 1);
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
    CHECK(JS_GetProperty(cx, a, "rest", &rest) && JS_GetProperty(cx, a, "owner", &owner));
    JS_GC(cx);
    JS_SetGlobalObject(cx, b);
    CHECK(JS_CallFunctionValue(cx, b, rest, 0, NULL, &result) && result == owner);
    clone = JS_CloneFunctionObject(cx, JSVAL_TO_OBJECT(rest), b);
    CHECK(clone != NULL);
    JS_GC(cx);
    CHECK(JS_CallFunctionValue(cx, b, OBJECT_TO_JSVAL(clone), 0, NULL, &result) && result == owner);
    CHECK(JS_GetProperty(cx, a, "collector", &rest) &&
          Evaluate(cx, a, "Array.prototype", &owner));
    CHECK(JS_CallFunctionValue(cx, b, rest, 2, values, &result));
    JS_GC(cx);
    CHECK(JSVAL_IS_OBJECT(result) &&
          JS_GetPrototype(cx, JSVAL_TO_OBJECT(result)) == JSVAL_TO_OBJECT(owner) &&
          JS_GetElement(cx, JSVAL_TO_OBJECT(result), 1, &element) &&
          element == INT_TO_JSVAL(8));
    clone = JS_CloneFunctionObject(cx, JSVAL_TO_OBJECT(rest), b);
    CHECK(clone && JS_CallFunctionValue(cx, b, OBJECT_TO_JSVAL(clone), 2, values, &result) &&
          JS_GetElement(cx, JSVAL_TO_OBJECT(result), 0, &element) &&
          element == INT_TO_JSVAL(7));
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
    source = JS_DecompileScript(cx, decoded, "rest-roundtrip", 0);
    CHECK(source && JS_EvaluateUCScript(cx, b, JS_GetStringChars(source),
          JS_GetStringLength(source), "decompiled-rests", 1, &result) && result == JSVAL_TRUE);
    CHECK(Evaluate(cx, b,"var f=(x,...r)=>x; f.length===1 && !f.hasOwnProperty('prototype') && "
          "!f.hasOwnProperty('arguments') && !f.hasOwnProperty('caller')",&result)
          && result == JSVAL_TRUE);
    JS_SetErrorReporter(cx, NULL);
    JS_SetVersion(cx, JSVERSION_1_7);
    CHECK(!Evaluate(cx, a, "var legacyArrow=(...r)=>1;", &result));
    JS_ClearPendingException(cx);
    status = 0;
    printf("ES6-REST-EMBEDDING PASS checks=%u\n", checks);
out:
    JS_SetObjectHook(rt, NULL, NULL);
    if (decoder) {JS_XDRMemSetData(decoder, NULL, 0);JS_XDRDestroy(decoder);}
    if (encoder) JS_XDRDestroy(encoder);
    if (decoded && !decodedObject) JS_DestroyScript(cx, decoded);
    if (script && !scriptObject) JS_DestroyScript(cx, script);
    JS_RemoveRoot(cx, &result);
    JS_RemoveRoot(cx, &owner);JS_RemoveRoot(cx, &rest);
    JS_RemoveRoot(cx, &source);JS_RemoveRoot(cx, &clone);
    JS_RemoveRoot(cx, &decodedObject);JS_RemoveRoot(cx, &scriptObject);
    JS_RemoveRoot(cx, &b);JS_RemoveRoot(cx, &a);
    JS_EndRequest(cx);
    JS_DestroyContext(cx);
    JS_DestroyRuntime(rt);
    JS_ShutDown();
    return status;
}
