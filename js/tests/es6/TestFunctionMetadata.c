/* Function metadata across classic embedding and cached scripts.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "jsapi.h"
#include "jsxdrapi.h"
#include <stdio.h>
#include <string.h>

static JSClass globalClass = {
    "global", 0,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};
static unsigned checks;
#define CHECK(condition) do { ++checks; if (!(condition)) { \
    fprintf(stderr, "FAIL function metadata embedding check %u\n", checks); \
    goto out; } } while (0)

int main(void)
{
    JSRuntime *rt = JS_NewRuntime(8 * 1024 * 1024);
    JSContext *cx;
    JSObject *global, *clone;
    JSScript *script = NULL, *decoded = NULL;
    JSXDRState *encoder = NULL, *decoder = NULL;
    jsval result;
    uint32 length;
    void *data;
    int status = 1;
    const char *source = "function named(a,b){var r=/a/;return typeof r==='object'?a+b:-1;}"
                         "var inferred=function(v){return /x/.test('x')?v:0;};"
                         "var accessor={get value(){return 42;}};"
                         "var parenFactory=function(){var target;(target)=function(){};return target;};"
                         "!parenFactory().hasOwnProperty('name') && "
                         "!eval('('+parenFactory.toString()+')')().hasOwnProperty('name') && "
                         "inferred.name==='inferred' && "
                         "Object.getOwnPropertyDescriptor(accessor,'value').get.name==='get value' && "
                         "!Object.getOwnPropertyDescriptor(accessor,'value').get.hasOwnProperty('prototype') && "
                         "named.length===2 && named.name==='named' && "
                         "Object.getOwnPropertyDescriptor(named,'length').configurable";
    const char *metadata = "cloned.length===2 && cloned.name==='named' && "
                           "cloned(20,22)===42 && "
                           "Object.getOwnPropertyDescriptor(cloned,'name').configurable";
    if (!rt) return 1;
    cx = JS_NewContext(rt, 8192);
    if (!cx) { JS_DestroyRuntime(rt); return 1; }
    JS_BeginRequest(cx);
    JS_SetVersion(cx, JSVERSION_ECMA_2015);
    global = JS_NewObject(cx, &globalClass, NULL, NULL);
    CHECK(global != NULL);
    JS_SetGlobalObject(cx, global);
    CHECK(JS_InitStandardClasses(cx, global));
    script = JS_CompileScript(cx, global, source, strlen(source), "metadata", 1);
    CHECK(script != NULL);
    encoder = JS_XDRNewMem(cx, JSXDR_ENCODE);
    decoder = JS_XDRNewMem(cx, JSXDR_DECODE);
    CHECK(encoder && decoder && JS_XDRScript(encoder, &script));
    data = JS_XDRMemGetData(encoder, &length);
    JS_XDRMemSetData(decoder, data, length);
    JS_SetVersion(cx, JSVERSION_DEFAULT);
    CHECK(JS_XDRScript(decoder, &decoded));
    CHECK(JS_ExecuteScript(cx, global, decoded, &result) && result == JSVAL_TRUE);
    CHECK(JS_GetVersion(cx) == JSVERSION_DEFAULT);
    CHECK(JS_GetProperty(cx, global, "named", &result) && JSVAL_IS_OBJECT(result));
    clone = JS_CloneFunctionObject(cx, JSVAL_TO_OBJECT(result), global);
    CHECK(clone && JS_DefineProperty(cx, global, "cloned", OBJECT_TO_JSVAL(clone),
                                    NULL, NULL, 0));
    JS_GC(cx);
    CHECK(JS_EvaluateScript(cx, global, metadata, strlen(metadata), "clone", 1,
                            &result) && result == JSVAL_TRUE);
    CHECK(JS_GetProperty(cx, global, "inferred", &result) && JSVAL_IS_OBJECT(result));
    clone = JS_CloneFunctionObject(cx, JSVAL_TO_OBJECT(result), global);
    CHECK(clone && JS_DefineProperty(cx, global, "inferredClone", OBJECT_TO_JSVAL(clone),
                                    NULL, NULL, 0));
    JS_GC(cx);
    {
        const char *checkInferred = "inferredClone.name==='inferred' && inferredClone(42)===42 && "
                                    "Object.getOwnPropertyDescriptor(inferredClone,'name').configurable";
        CHECK(JS_EvaluateScript(cx, global, checkInferred, strlen(checkInferred),
                                "inferred-clone", 1, &result) && result == JSVAL_TRUE);
    }
    JS_SetVersion(cx, JSVERSION_ECMA_2015);
    {
        const char *bind = "var bound=named.bind(null,20);bound";
        const char *checkBound = "boundClone(22)===42 && boundClone.length===1 && "
                                 "boundClone.name==='bound named'";
        CHECK(JS_EvaluateScript(cx, global, bind, strlen(bind), "bind", 1,
                                &result) && JSVAL_IS_OBJECT(result));
        clone = JS_CloneFunctionObject(cx, JSVAL_TO_OBJECT(result), global);
        CHECK(clone && JS_DefineProperty(cx, global, "boundClone",
                                         OBJECT_TO_JSVAL(clone), NULL, NULL, 0));
        JS_GC(cx);
        CHECK(JS_EvaluateScript(cx, global, checkBound, strlen(checkBound),
                                "bound-clone", 1, &result) && result == JSVAL_TRUE);
    }
    JS_XDRMemSetData(decoder, NULL, 0);
    JS_XDRDestroy(decoder);
    decoder = NULL;
    JS_XDRDestroy(encoder);
    encoder = NULL;
    JS_DestroyScript(cx, decoded);
    decoded = NULL;
    JS_DestroyScript(cx, script);
    script = NULL;
    JS_SetVersion(cx, JSVERSION_1_7);
    source = "function legacyCached(a,b){return legacyCached.arguments[0];}"
             "legacyCached(42)===42 && legacyCached.length===2 && "
             "legacyCached.name==='legacyCached' && "
             "!Object.getOwnPropertyDescriptor(legacyCached,'length').configurable";
    script = JS_CompileScript(cx, global, source, strlen(source), "legacy-cache", 1);
    CHECK(script != NULL);
    encoder = JS_XDRNewMem(cx, JSXDR_ENCODE);
    decoder = JS_XDRNewMem(cx, JSXDR_DECODE);
    CHECK(encoder && decoder && JS_XDRScript(encoder, &script));
    data = JS_XDRMemGetData(encoder, &length);
    JS_XDRMemSetData(decoder, data, length);
    JS_SetVersion(cx, JSVERSION_ECMA_2015);
    CHECK(JS_XDRScript(decoder, &decoded));
    CHECK(JS_ExecuteScript(cx, global, decoded, &result) && result == JSVAL_TRUE &&
          JS_GetVersion(cx) == JSVERSION_ECMA_2015);
    status = 0;
    printf("ES6-FUNCTION-METADATA-EMBEDDING checks=%u failures=0\n", checks);
out:
    if (decoder) { JS_XDRMemSetData(decoder, NULL, 0); JS_XDRDestroy(decoder); }
    if (encoder) JS_XDRDestroy(encoder);
    if (decoded) JS_DestroyScript(cx, decoded);
    if (script) JS_DestroyScript(cx, script);
    JS_EndRequest(cx);
    JS_DestroyContext(cx);
    JS_DestroyRuntime(rt);
    JS_ShutDown();
    return status;
}
