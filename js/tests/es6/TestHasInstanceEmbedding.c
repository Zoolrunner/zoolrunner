/* Preserve classic native instance checks beside modern Symbol dispatch.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "jsapi.h"
#include <stdio.h>
#include <string.h>

static unsigned checks, nativeCalls;
static JSClass globalClass = {
    "global", 0,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};
static JSClass nativeClass = {
    "NativeType", 0,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};
static JSBool
NativeHasInstance(JSContext *cx, JSObject *obj, jsval value, JSBool *result)
{
    ++nativeCalls;
    JS_GC(cx);
    *result = value == INT_TO_JSVAL(42);
    return JS_TRUE;
}
static JSBool
Evaluate(JSContext *cx, JSObject *global, const char *source)
{
    jsval result;
    return JS_EvaluateScript(cx, global, source, strlen(source),
                             "has-instance-embedding", 1, &result) &&
           result == JSVAL_TRUE;
}
#define CHECK(condition) do { ++checks; if (!(condition)) { \
    fprintf(stderr, "FAIL native hasInstance check %u\n", checks); goto out; \
} } while (0)

int main(void)
{
    JSRuntime *rt = JS_NewRuntime(8 * 1024 * 1024);
    JSContext *cx;
    JSObject *global, *native;
    JSBool result;
    unsigned before;
    int status = 1;
    if (!rt) return 1;
    cx = JS_NewContext(rt, 8192);
    if (!cx) { JS_DestroyRuntime(rt); return 1; }
    JS_BeginRequest(cx);
    nativeClass.hasInstance = NativeHasInstance;
    global = JS_NewObject(cx, &globalClass, NULL, NULL);
    CHECK(global);
    JS_SetGlobalObject(cx, global);
    CHECK(JS_InitStandardClasses(cx, global));
    native = JS_DefineObject(cx, global, "nativeType", &nativeClass, NULL, 0);
    CHECK(native);
    CHECK(Evaluate(cx, global,
        "function legacy(v){return v instanceof nativeType;}"
        "legacy(42) && !legacy(1)"));
    JS_SetVersion(cx, JSVERSION_1_7);
    CHECK(Evaluate(cx, global,
        "function legacy170(v){return v instanceof nativeType;}legacy170(42)"));
    JS_SetVersion(cx, JSVERSION_ECMA_2015);
    before = nativeCalls;
    CHECK(Evaluate(cx, global,
        "nativeType[Symbol.hasInstance]=function(v){return v===7;};"
        "7 instanceof nativeType && !(42 instanceof nativeType)"));
    CHECK(nativeCalls == before);
    CHECK(Evaluate(cx, global, "legacy(42) && legacy170(42)"));
    CHECK(nativeCalls == before + 2);
    CHECK(JS_HasInstance(cx, native, INT_TO_JSVAL(42), &result) && result);
    CHECK(nativeCalls == before + 3);
    JS_GC(cx);
    CHECK(Evaluate(cx, global,
        "7 instanceof nativeType && !Function.prototype[Symbol.hasInstance].call(nativeType,42)"));
    CHECK(JS_GetVersion(cx) == JSVERSION_ECMA_2015);
    printf("ES6-HAS-INSTANCE-EMBEDDING checks=%u failures=0\n", checks);
    status = 0;
  out:
    JS_EndRequest(cx);
    JS_DestroyContext(cx);
    JS_DestroyRuntime(rt);
    JS_ShutDown();
    return status;
}
