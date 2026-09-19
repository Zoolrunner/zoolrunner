/* Modern conversion through legacy globals, preserving embedding hooks.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "jsapi.h"
#include <stdio.h>
#include <string.h>

static unsigned checks, conversions;
static JSClass globalClass = {
    "global", 0,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};
static JSBool Convert(JSContext *cx, JSObject *obj, JSType type, jsval *value)
{
    ++conversions;
    *value = INT_TO_JSVAL(42);
    return JS_TRUE;
}
static JSClass hostClass = {
    "conversionHost", 0,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, Convert, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};
static JSBool Evaluate(JSContext *cx, JSObject *global, const char *source)
{
    jsval result;
    return JS_EvaluateScript(cx, global, source, strlen(source),
                             "conversion-realms", 1, &result) && result == JSVAL_TRUE;
}
#define CHECK(c) do { ++checks; if (!(c)) { fprintf(stderr, "FAIL conversion realms %u\n", checks); goto out; } } while (0)
int main(void)
{
    JSRuntime *rt = JS_NewRuntime(8 * 1024 * 1024);
    JSContext *legacy = NULL, *modern = NULL;
    JSObject *oldGlobal, *newGlobal, *host;
    jsval value;
    int status = 1;
    if (!rt) return 1;
    legacy = JS_NewContext(rt, 8192);
    if (!legacy) goto out;
    JS_BeginRequest(legacy);
    JS_SetVersion(legacy, JSVERSION_1_7);
    oldGlobal = JS_NewObject(legacy, &globalClass, NULL, NULL);
    CHECK(oldGlobal);
    JS_SetGlobalObject(legacy, oldGlobal);
    CHECK(JS_InitStandardClasses(legacy, oldGlobal));
    modern = JS_NewContext(rt, 8192);
    CHECK(modern);
    JS_BeginRequest(modern);
    JS_SetVersion(modern, JSVERSION_ECMA_2015);
    newGlobal = JS_NewObject(modern, &globalClass, NULL, NULL);
    CHECK(newGlobal);
    JS_SetGlobalObject(modern, newGlobal);
    CHECK(JS_InitStandardClasses(modern, newGlobal));
    CHECK(JS_GetProperty(modern, newGlobal, "Number", &value));
    CHECK(JS_DefineProperty(legacy, oldGlobal, "ModernNumber", value, NULL, NULL, 0));
    CHECK(JS_GetProperty(modern, newGlobal, "String", &value));
    CHECK(JS_DefineProperty(legacy, oldGlobal, "ModernString", value, NULL, NULL, 0));
    CHECK(Evaluate(legacy, oldGlobal,
        "var x={valueOf:function(){return arguments.length;}};"
        "+x===1 && ModernNumber(x)===0 && +x===1"));
    CHECK(Evaluate(legacy, oldGlobal,
        "ModernString({toString:{},valueOf:function(){return 'ok';}})==='ok'"));
    CHECK(Evaluate(legacy, oldGlobal,
        "var sentinel={};var caught=false;"
        "try{ModernNumber({get valueOf(){throw sentinel;}});}"
        "catch(e){caught=e===sentinel;}caught"));
    CHECK(Evaluate(modern, newGlobal,
        "Number.parseInt===parseInt && Number.parseFloat===parseFloat"));
    host = JS_NewObject(modern, &hostClass, NULL, newGlobal);
    CHECK(host);
    CHECK(JS_DefineProperty(modern, newGlobal, "host", OBJECT_TO_JSVAL(host), NULL, NULL, 0));
    CHECK(Evaluate(modern, newGlobal, "+host===42"));
    CHECK(conversions == 1);
    JS_GC(modern);
    CHECK(Evaluate(legacy, oldGlobal, "ModernString({toString:function(){return null;}})==='null'"));
    printf("CONVERSION-REALMS checks=%u failures=0\n", checks);
    status = 0;
  out:
    if (modern) { JS_EndRequest(modern); JS_DestroyContextNoGC(modern); }
    if (legacy) { JS_EndRequest(legacy); JS_DestroyContext(legacy); }
    JS_DestroyRuntime(rt);
    JS_ShutDown();
    return status;
}
