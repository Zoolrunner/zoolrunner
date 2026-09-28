/* Collection intrinsic realms, cache lifetime and classic globals.
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
    return JS_EvaluateScript(cx, global, source, strlen(source), "collection-realms", 1, &result) &&
           result == JSVAL_TRUE;
}
#define CHECK(c) do { ++checks; if (!(c)) { \
    fprintf(stderr,"FAIL collection embedding check %u\n",checks); goto out; } } while (0)
int main(void)
{
    JSRuntime *rt = JS_NewRuntime(16 * 1024 * 1024);
    JSContext *cx, *other = NULL;
    JSObject *global, *second;
    jsval exported, oldProto = JSVAL_VOID, result;
    JSBool rooted = JS_FALSE;
    int status = 1;
    const char *prototypeSource = "Object.getPrototypeOf(new Map().entries())";
    if (!rt) return 1;
    cx = JS_NewContext(rt, 8192);
    if (!cx) { JS_DestroyRuntime(rt); return 1; }
    JS_BeginRequest(cx);
    JS_SetVersion(cx, JSVERSION_ECMA_2015);
    global = JS_NewObject(cx, &globalClass, NULL, NULL);
    CHECK(global);
    JS_SetGlobalObject(cx, global);
    CHECK(JS_InitStandardClasses(cx, global));
    CHECK(Evaluate(cx, global, "Object.prototype.realmMarker=11;Array.prototype.realmMarker=12;true"));
    other = JS_NewContext(rt, 8192);
    CHECK(other);
    JS_BeginRequest(other);
    JS_SetVersion(other, JSVERSION_ECMA_2015);
    second = JS_NewObject(other, &globalClass, NULL, NULL);
    CHECK(second);
    JS_SetGlobalObject(other, second);
    CHECK(JS_InitStandardClasses(other, second));
    CHECK(Evaluate(other, second,
        "Object.prototype.realmMarker=21;Array.prototype.realmMarker=22;"
        "var exported={entries:Map.prototype.entries,values:Set.prototype.values,"
        "mapNext:new Map().entries().next,setNext:new Set().values().next,"
        "mapProto:Object.getPrototypeOf(new Map().entries()),"
        "target:new Map([[1,7]]),set:new Set([8])};"
        "Object=null;Array=null;Map=null;Set=null;true"));
    CHECK(JS_GetProperty(other, second, "exported", &exported));
    CHECK(JS_DefineProperty(cx, global, "foreign", exported, NULL, NULL, 0));
    JS_EndRequest(other);
    JS_DestroyContextNoGC(other); other = NULL;
    JS_ClearNewbornRoots(cx);
    JS_GC(cx);
    CHECK(finalized == 0);
    CHECK(Evaluate(cx, global,
        "(function(){var it=foreign.entries.call(new Map([[1,7]])),r=it.next();"
        "return Object.getPrototypeOf(it)===foreign.mapProto && r.realmMarker===21 && "
        "r.value.realmMarker===22 && r.value.join()==='1,7';})()"));
    CHECK(Evaluate(cx, global,
        "foreign.mapNext.call(new Map().entries()).realmMarker===21"));
    CHECK(Evaluate(cx, global,
        "foreign.setNext.call(new Set().values()).realmMarker===21"));
    CHECK(Evaluate(cx, global,
        "(function(){var r=Map.prototype.entries.call(foreign.target).next();"
        "return r.realmMarker===11 && r.value.realmMarker===12;})()"));
    JS_SetVersion(cx, JSVERSION_DEFAULT);
    CHECK(Evaluate(cx, global,
        "foreign.entries.call(foreign.target).next().value.realmMarker===22"));
    CHECK(Evaluate(cx, global,
        "foreign.values.call(foreign.set).next().realmMarker===21"));
    JS_SetVersion(cx, JSVERSION_ECMA_2015);
    CHECK(Evaluate(cx, global, "var savedMap=Map,savedSet=Set;delete this.Map;delete this.Set;true"));
    CHECK(JS_EnumerateStandardClasses(cx, global));
    CHECK(Evaluate(cx, global, "!Object.prototype.hasOwnProperty.call(this,'Map') && !Object.prototype.hasOwnProperty.call(this,'Set') && (this.Map=savedMap,this.Set=savedSet,true)"));
    CHECK(JS_DeleteProperty(cx, global, "foreign"));
    JS_ClearNewbornRoots(cx);
    JS_GC(cx);
    CHECK(finalized == 1);
    CHECK(JS_EvaluateScript(cx, global, prototypeSource, strlen(prototypeSource),
                            "iterator-prototype", 1, &oldProto));
    CHECK(JS_AddNamedRoot(cx, &oldProto, "old iterator prototype"));
    rooted = JS_TRUE;
    JS_ClearScope(cx, global);
    CHECK(JS_InitStandardClasses(cx, global));
    CHECK(JS_EvaluateScript(cx, global, prototypeSource, strlen(prototypeSource),
                            "new-iterator-prototype", 1, &result) && result != oldProto);
    CHECK(Evaluate(cx, global, "new Set([3]).values().next().value===3"));
    printf("ES6-COLLECTIONS-EMBEDDING checks=%u failures=0\n",checks);
    status = 0;
  out:
    if (rooted) JS_RemoveRoot(cx, &oldProto);
    if (other) { JS_EndRequest(other); JS_DestroyContextNoGC(other); }
    JS_EndRequest(cx);
    JS_DestroyContext(cx);
    JS_DestroyRuntime(rt);
    JS_ShutDown();
    return status;
}
