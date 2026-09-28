/* Edition-aware tags preserve classic native class identities.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "jsapi.h"
#include <stdio.h>
#include <string.h>
static unsigned checks, calls;
static JSClass globalClass = {
    "global", 0,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};
static JSClass nativeClass = {
    "NativeTag", 0,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};
static JSClass callableClass = {
    "NativeCallable", 0,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};
static JSBool
Call(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    ++calls;
    *rval = JSVAL_VOID;
    return JS_TRUE;
}
static JSBool
Collect(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JS_GC(cx);
    *rval = JSVAL_VOID;
    return JS_TRUE;
}
static JSBool
Evaluate(JSContext *cx, JSObject *global, const char *source)
{
    jsval result;
    return JS_EvaluateScript(cx, global, source, strlen(source), "native-tags", 1, &result) &&
           result == JSVAL_TRUE;
}
#define CHECK(c) do { ++checks; if (!(c)) { \
    fprintf(stderr,"FAIL native tag check %u\n",checks); goto out; } } while (0)
int main(void)
{
    JSRuntime *rt = JS_NewRuntime(8 * 1024 * 1024);
    JSContext *cx;
    JSObject *global, *native, *callable;
    int status = 1;
    if (!rt) return 1;
    cx = JS_NewContext(rt, 8192);
    if (!cx) { JS_DestroyRuntime(rt); return 1; }
    JS_BeginRequest(cx);
    callableClass.call = Call;
    global = JS_NewObject(cx, &globalClass, NULL, NULL);
    CHECK(global);
    JS_SetGlobalObject(cx, global);
    CHECK(JS_InitStandardClasses(cx, global) && JS_DefineFunction(cx, global, "collect", Collect, 0, 0));
    native = JS_DefineObject(cx, global, "native", &nativeClass, NULL, 0);
    callable = JS_DefineObject(cx, global, "callable", &callableClass, NULL, 0);
    CHECK(native);
    CHECK(callable);
    CHECK(Evaluate(cx, global,
        "var tag=Object.prototype.toString;"
        "function legacyTags(){return tag.call(native)==='[object NativeTag]' && "
        "tag.call(callable)==='[object NativeCallable]';}legacyTags()"));
    JS_SetVersion(cx, JSVERSION_ECMA_2015);
    CHECK(Evaluate(cx, global,
        "tag.call(native)==='[object Object]' && tag.call(callable)==='[object Function]'"));
    CHECK(Evaluate(cx, global,
        "Object.defineProperty(native,Symbol.toStringTag,{configurable:true,get:function(){collect();return 'custom';}});"
        "tag.call(native)==='[object custom]'"));
    CHECK(Evaluate(cx, global,
        "delete native[Symbol.toStringTag];tag.call(native)==='[object Object]' && legacyTags()"));
    JS_SetVersion(cx, JSVERSION_1_7);
    CHECK(Evaluate(cx, global, "legacyTags()"));
    JS_SetVersion(cx, JSVERSION_DEFAULT);
    CHECK(Evaluate(cx, global, "legacyTags()"));
    CHECK(JS_GET_CLASS(cx, native) == &nativeClass);
    CHECK(JS_GET_CLASS(cx, callable) == &callableClass);
    CHECK(calls == 0);
    printf("ES6-BUILTIN-TAGS-EMBEDDING checks=%u failures=0\n", checks);
    status = 0;
  out:
    JS_EndRequest(cx);
    JS_DestroyContext(cx);
    JS_DestroyRuntime(rt);
    JS_ShutDown();
    return status;
}
