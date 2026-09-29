/* Native ephemeron storage: no mutable JS methods or unconditional roots.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "jsapi.h"
#include <stdio.h>
#include <string.h>
static unsigned checks, finalized[8];
static void Finalize(JSContext *cx, JSObject *obj)
{
    jsval id;
    if (JS_GetReservedSlot(cx, obj, 0, &id) && JSVAL_IS_INT(id) &&
        JSVAL_TO_INT(id) >= 0 && JSVAL_TO_INT(id) < 8)
        ++finalized[JSVAL_TO_INT(id)];
}
static JSClass probeClass = {
    "probe", JSCLASS_HAS_RESERVED_SLOTS(1),
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, Finalize,
    JSCLASS_NO_OPTIONAL_MEMBERS
};
static JSClass globalClass = {
    "global", 0,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};
static JSObject *Probe(JSContext *cx, unsigned id)
{
    JSObject *obj = JS_NewObject(cx, &probeClass, NULL, NULL);
    if (!obj || !JS_SetReservedSlot(cx, obj, 0, INT_TO_JSVAL(id))) return NULL;
    return obj;
}
static void Collect(JSContext *cx)
{
    JS_ClearNewbornRoots(cx); JS_GC(cx);
    JS_ClearNewbornRoots(cx); JS_GC(cx);
}
static JSBool Eval(JSContext *cx, JSObject *global, const char *source)
{
    jsval value;
    return JS_EvaluateScript(cx, global, source, strlen(source), "native-weak", 1, &value);
}
#define CHECK(expr) do { ++checks; if (!(expr)) { \
    fprintf(stderr,"FAIL native weak map line %d check %u\n",__LINE__,checks); goto out; } } while (0)
int main(void)
{
    JSRuntime *rt = JS_NewRuntime(16L * 1024 * 1024);
    JSContext *cx;
    JSObject *global, *map = NULL, *key = NULL, *valueObject = NULL, *other = NULL;
    jsval value = JSVAL_VOID;
    JSBool found;
    int status = 1, rooted = 0;
    if (!rt) return 1;
    cx = JS_NewContext(rt, 8192);
    if (!cx) { JS_DestroyRuntime(rt); return 1; }
    JS_BeginRequest(cx);
    JS_SetVersion(cx, JSVERSION_ECMA_2015);
    global = JS_NewObject(cx, &globalClass, NULL, NULL);
    CHECK(global);
    JS_SetGlobalObject(cx, global);
    CHECK(JS_InitStandardClasses(cx, global));
    CHECK(JS_AddNamedRoot(cx, &map, "native map")); rooted = 1;
    CHECK(JS_AddNamedRoot(cx, &key, "native key")); rooted = 2;
    CHECK(JS_AddNamedRoot(cx, &valueObject, "native value")); rooted = 3;
    CHECK(JS_AddNamedRoot(cx, &other, "native other realm")); rooted = 4;
    CHECK(JS_AddNamedRoot(cx, &value, "native result")); rooted = 5;
    CHECK(Eval(cx, global,
        "WeakMap.prototype.get=function(){throw 'poison get';};"
        "WeakMap.prototype.set=function(){throw 'poison set';};"
        "WeakMap=function(){throw 'poison constructor';};"));
    map = JS_NewWeakMapObject(cx, global); CHECK(map);
    key = Probe(cx, 1); CHECK(key);
    CHECK(JS_GetWeakMapEntry(cx, map, key, &value, &found) && !found && JSVAL_IS_VOID(value));
    CHECK(JS_SetWeakMapEntry(cx, map, key, JSVAL_VOID));
    CHECK(JS_GetWeakMapEntry(cx, map, key, &value, &found) && found && JSVAL_IS_VOID(value));
    CHECK(JS_SetWeakMapEntry(cx, map, key, INT_TO_JSVAL(27)));
    CHECK(JS_GetWeakMapEntry(cx, map, key, &value, &found) && found && value == INT_TO_JSVAL(27));
    valueObject = Probe(cx, 2); CHECK(valueObject);
    CHECK(JS_SetWeakMapEntry(cx, map, key, OBJECT_TO_JSVAL(valueObject)));
    valueObject = NULL; value = JSVAL_VOID;
    Collect(cx);
    CHECK(!finalized[1] && !finalized[2]);
    CHECK(JS_GetWeakMapEntry(cx, map, key, &value, &found) && found && JSVAL_IS_OBJECT(value));
    key = NULL; value = JSVAL_VOID;
    Collect(cx);
    CHECK(finalized[1] == 1 && finalized[2] == 1);
    key = Probe(cx, 3); CHECK(key);
    valueObject = Probe(cx, 4); CHECK(valueObject);
    CHECK(JS_DefineProperty(cx, valueObject, "back", OBJECT_TO_JSVAL(key), NULL, NULL, 0));
    CHECK(JS_SetWeakMapEntry(cx, map, key, OBJECT_TO_JSVAL(valueObject)));
    key = valueObject = NULL;
    Collect(cx);
    CHECK(finalized[3] == 1 && finalized[4] == 1);
    other = JS_NewObject(cx, &globalClass, NULL, NULL); CHECK(other);
    CHECK(JS_InitStandardClasses(cx, other));
    map = JS_NewWeakMapObject(cx, other); CHECK(map && JS_GetParent(cx, map) == other);
    key = Probe(cx, 5); CHECK(key);
    valueObject = Probe(cx, 6); CHECK(valueObject);
    CHECK(JS_SetWeakMapEntry(cx, map, key, OBJECT_TO_JSVAL(valueObject)));
    valueObject = NULL; other = NULL;
    Collect(cx);
    CHECK(!finalized[5] && !finalized[6]);
    CHECK(JS_GetWeakMapEntry(cx, map, key, &value, &found) && found);
    value = JSVAL_VOID; map = NULL;
    Collect(cx);
    CHECK(!finalized[5] && finalized[6] == 1);
    CHECK(!JS_SetWeakMapEntry(cx, global, key, JSVAL_NULL));
    JS_ClearPendingException(cx);
    map = JS_NewWeakMapObject(cx, global); CHECK(map);
    CHECK(!JS_GetWeakMapEntry(cx, map, NULL, &value, &found));
    JS_ClearPendingException(cx);
    key = NULL;
    Collect(cx);
    CHECK(finalized[5] == 1);
    printf("ES6-WEAK-MAP-EMBEDDING checks=%u failures=0\n", checks);
    status = 0;
  out:
    if (rooted >= 5) JS_RemoveRoot(cx, &value);
    if (rooted >= 4) JS_RemoveRoot(cx, &other);
    if (rooted >= 3) JS_RemoveRoot(cx, &valueObject);
    if (rooted >= 2) JS_RemoveRoot(cx, &key);
    if (rooted >= 1) JS_RemoveRoot(cx, &map);
    JS_EndRequest(cx); JS_DestroyContext(cx); JS_DestroyRuntime(rt); JS_ShutDown();
    return status;
}
