/* Weak realm cache lifetime, classic globals and scope clearing.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "jsapi.h"
#include <stdio.h>
#include <string.h>
static unsigned checks, finalized;
static void Finalize(JSContext *cx, JSObject *obj) { ++finalized; }
static JSClass globalClass = {
    "global", 0,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, Finalize,
    JSCLASS_NO_OPTIONAL_MEMBERS
};
static JSBool Evaluate(JSContext *cx, JSObject *global, const char *source)
{
    jsval result;
    return JS_EvaluateScript(cx, global, source, strlen(source),
                             "realm-intrinsics", 1, &result) && result == JSVAL_TRUE;
}
#define CHECK(c) do { ++checks; if (!(c)) { fprintf(stderr,"FAIL realm embedding %u\n",checks); goto out; } } while (0)
int main(void)
{
    JSRuntime *rt = JS_NewRuntime(8 * 1024 * 1024);
    JSContext *cx, *other = NULL;
    JSObject *global, *second = NULL;
    jsval firstArray, secondArray;
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
    CHECK(JS_GetProperty(cx, global, "Array", &firstArray));
    CHECK(Evaluate(cx, global, "var old=Array.prototype;Array=function(){};Object.getPrototypeOf([])===old"));
    JS_GC(cx);
    CHECK(Evaluate(cx, global, "Object.getPrototypeOf([])===old"));
    other = JS_NewContext(rt, 8192);
    CHECK(other);
    JS_BeginRequest(other);
    JS_SetVersion(other, JSVERSION_ECMA_2015);
    second = JS_NewObject(other, &globalClass, NULL, NULL);
    CHECK(second);
    JS_SetGlobalObject(other, second);
    CHECK(JS_InitStandardClasses(other, second));
    CHECK(JS_GetProperty(other, second, "Array", &secondArray));
    CHECK(firstArray != secondArray);
    CHECK(JS_AddNamedRoot(cx, &second, "test secondary realm"));
    CHECK(Evaluate(other, second, "Array.prototype.realmMarker=27;Array=null;[].realmMarker===27"));
    CHECK(Evaluate(other, second,
        "RegExp.prototype.realmMarker=31;RegExp=null;"
        "function realmRegExp(){return /realm/g;}"
        "realmRegExp().realmMarker===31 && realmRegExp()!==realmRegExp()"));
    JS_EndRequest(other);
    JS_DestroyContextNoGC(other); other = NULL;
    JS_ClearNewbornRoots(cx);
    JS_GC(cx);
    CHECK(finalized == 0);
    CHECK(Evaluate(cx, second, "[].realmMarker===27"));
    CHECK(Evaluate(cx, second,
        "realmRegExp().realmMarker===31 && realmRegExp()!==realmRegExp()"));
    CHECK(Evaluate(cx, global, "/realm/.realmMarker===undefined"));
    JS_RemoveRoot(cx, &second); second = NULL;
    JS_ClearNewbornRoots(cx);
    JS_GC(cx);
    CHECK(finalized == 1);
    JS_ClearScope(cx, global);
    CHECK(JS_InitStandardClasses(cx, global));
    CHECK(JS_GetProperty(cx, global, "Array", &secondArray));
    CHECK(firstArray != secondArray);
    CHECK(Evaluate(cx, global, "var old=Array.prototype;Array=null;Object.getPrototypeOf([])===old"));
    printf("ES6-REALM-EMBEDDING checks=%u failures=0\n", checks);
    status = 0;
  out:
    if (other) { JS_EndRequest(other); JS_DestroyContextNoGC(other); }
    if (second) JS_RemoveRoot(cx, &second);
    JS_EndRequest(cx);
    JS_DestroyContext(cx);
    JS_DestroyRuntime(rt);
    JS_ShutDown();
    return status;
}
