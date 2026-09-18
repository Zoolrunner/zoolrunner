/* Array.from defaults to its defining realm across classic JSAPI globals.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "jsapi.h"
#include <stdio.h>
#include <string.h>
static JSClass globalClass = {
    "global", 0,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};
static unsigned checks;
static JSBool Evaluate(JSContext *cx, JSObject *global, const char *source)
{
    jsval result;
    return JS_EvaluateScript(cx, global, source, strlen(source),
                             "array-from-embedding", 1, &result) && result == JSVAL_TRUE;
}
#define CHECK(c) do { ++checks; if (!(c)) { fprintf(stderr,"FAIL Array.from embedding %u\n",checks); goto out; } } while (0)
int main(void)
{
    JSRuntime *rt = JS_NewRuntime(8 * 1024 * 1024);
    JSContext *cx, *other = NULL;
    JSObject *global, *second;
    jsval array, method, proto;
    int status = 1;
    if (!rt) return 1;
    cx = JS_NewContext(rt, 8192);
    if (!cx) { JS_DestroyRuntime(rt); return 1; }
    JS_BeginRequest(cx);
    JS_SetVersion(cx, JSVERSION_ECMA_2015);
    global = JS_NewObject(cx, &globalClass, NULL, NULL);
    CHECK(global);
    JS_SetGlobalObject(cx, global);
    CHECK(JS_InitStandardClasses(cx, global));
    other = JS_NewContext(rt, 8192);
    CHECK(other);
    JS_BeginRequest(other);
    JS_SetVersion(other, JSVERSION_ECMA_2015);
    second = JS_NewObject(other, &globalClass, NULL, NULL);
    CHECK(second);
    JS_SetGlobalObject(other, second);
    CHECK(JS_InitStandardClasses(other, second));
    CHECK(JS_GetProperty(other, second, "Array", &array));
    CHECK(JS_GetProperty(other, JSVAL_TO_OBJECT(array), "from", &method));
    CHECK(JS_GetProperty(other, JSVAL_TO_OBJECT(array), "prototype", &proto));
    CHECK(JS_DefineProperty(cx, global, "foreignFrom", method, NULL, NULL, 0));
    CHECK(JS_DefineProperty(cx, global, "foreignPrototype", proto, NULL, NULL, 0));
    CHECK(Evaluate(other, second, "Array=null;true"));
    JS_EndRequest(other);
    JS_DestroyContextNoGC(other); other = NULL;
    JS_GC(cx);
    CHECK(Evaluate(cx, global, "Object.getPrototypeOf(foreignFrom.call(null,{0:1,length:1}))===foreignPrototype"));
    CHECK(Evaluate(cx, global, "Object.getPrototypeOf(foreignFrom.call(Array,[1]))===Array.prototype"));
    CHECK(Evaluate(cx, global, "Object.getPrototypeOf(foreignFrom.call(null,[1]))===foreignPrototype"));
    CHECK(Evaluate(cx, global, "foreignFrom.call(null,'a\\ud83d\\ude00').length===2"));
    JS_SetVersion(cx, JSVERSION_DEFAULT);
    CHECK(Evaluate(cx, global, "Object.getPrototypeOf(foreignFrom.call(null,{0:1,length:1}))===foreignPrototype"));
    JS_GC(cx);
    CHECK(Evaluate(cx, global, "foreignFrom.call(null,[1,2]).join()==='1,2'"));
    printf("ES6-ARRAY-FROM-EMBEDDING checks=%u failures=0\n", checks);
    status = 0;
  out:
    if (other) { JS_EndRequest(other); JS_DestroyContextNoGC(other); }
    JS_EndRequest(cx);
    JS_DestroyContext(cx);
    JS_DestroyRuntime(rt);
    JS_ShutDown();
    return status;
}
