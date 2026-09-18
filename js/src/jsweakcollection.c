/* ES2015 weak keyed collections; MPL 1.1/GPL 2.0/LGPL 2.1. */
#include <stdlib.h>
#include "jsapi.h"
#include "jscntxt.h"
#include "jscollection.h"
#include "jscollectiontable.h"
#include "jsfun.h"
#include "jsgc.h"
#include "jslock.h"
#include "jsobj.h"
#include "jssymbol.h"
#include "jsweakcollection.h"

typedef struct JSWeakCollection {
    struct JSWeakCollection *next, *previous;
    JSObject *owner; /* Non-owning: the runtime list does not mark this pointer. */
    JSCollectionData *table;
} JSWeakCollection;

JS_STATIC_DLL_CALLBACK(void)
WeakFinalize(JSContext *cx, JSObject *obj)
{
    JSWeakCollection *data = (JSWeakCollection *)JS_GetPrivate(cx, obj);
    JSRuntime *rt = cx->runtime;
    if (!data) return;
    JS_LOCK_GC(rt);
    if (data->previous) data->previous->next = data->next;
    else rt->weakCollections = data->next;
    if (data->next) data->next->previous = data->previous;
    JS_UNLOCK_GC(rt);
    js_ReleaseCollectionData(data->table);
    free(data);
}
#define WEAK_CLASS(name) { \
    #name, JSCLASS_HAS_PRIVATE | JSCLASS_HAS_CACHED_PROTO(JSProto_##name), \
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, \
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, WeakFinalize, \
    JSCLASS_NO_OPTIONAL_MEMBERS \
}
JSClass js_WeakMapClass = WEAK_CLASS(WeakMap);
JSClass js_WeakSetClass = WEAK_CLASS(WeakSet);

/* Mutator requests are stopped throughout these collector phases. */
JSBool
js_MarkWeakCollections(JSContext *cx)
{
    JSWeakCollection *data;
    JSBool changed = JS_FALSE;
    for (data = cx->runtime->weakCollections; data; data = data->next) {
        if ((*js_GetGCThingFlags(data->owner) & GCF_MARK) &&
            js_MarkWeakCollectionValues(cx, data->table))
            changed = JS_TRUE;
    }
    return changed;
}
void
js_SweepWeakCollections(JSContext *cx)
{
    JSWeakCollection *data;
    for (data = cx->runtime->weakCollections; data; data = data->next) {
        if (*js_GetGCThingFlags(data->owner) & GCF_MARK)
            js_SweepWeakCollectionKeys(cx, data->table);
    }
}
void
js_FinishWeakCollections(JSRuntime *rt)
{
    JSWeakCollection *data, *next;
    for (data = rt->weakCollections; data; data = next) {
        next = data->next;
        js_ReleaseCollectionData(data->table);
        free(data);
    }
    rt->weakCollections = NULL;
}

static JSBool
WeakOperation(JSContext *cx, uintN argc, jsval *argv, jsval *rval,
               JSBool set, unsigned operation)
{
    JSClass *clasp = set ? &js_WeakSetClass : &js_WeakMapClass;
    JSObject *obj;
    JSWeakCollection *data;
    jsval key = argc ? argv[0] : JSVAL_VOID;
    JSBool ok = JS_TRUE, found;
    if (JSVAL_IS_PRIMITIVE(argv[-1]) ||
        OBJ_GET_CLASS(cx, JSVAL_TO_OBJECT(argv[-1])) != clasp ||
        !JS_GetPrivate(cx, JSVAL_TO_OBJECT(argv[-1]))) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_INCOMPATIBLE_PROTO, clasp->name, "method", "receiver");
        return JS_FALSE;
    }
    if (JSVAL_IS_PRIMITIVE(key)) {
        if (operation == 0) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                 JSMSG_CANT_CONVERT_TO, "weak collection key", "object");
            return JS_FALSE;
        }
        *rval = operation == 1 ? JSVAL_VOID : JSVAL_FALSE;
        return JS_TRUE;
    }
    obj = JSVAL_TO_OBJECT(argv[-1]);
    JS_LOCK_OBJ(cx, obj);
    data = (JSWeakCollection *)JS_GetPrivate(cx, obj);
    switch (operation) {
      case 0:
        ok = js_CollectionPut(data->table, key, set ? JSVAL_TRUE : (argc > 1 ? argv[1] : JSVAL_VOID));
        *rval = argv[-1];
        break;
      case 1:
        if (!js_CollectionGet(data->table, key, rval)) *rval = JSVAL_VOID;
        break;
      case 2:
        found = js_CollectionGet(data->table, key, rval);
        *rval = BOOLEAN_TO_JSVAL(found);
        break;
      case 3:
        *rval = BOOLEAN_TO_JSVAL(js_CollectionDelete(data->table, key));
        break;
    }
    JS_UNLOCK_OBJ(cx, obj);
    if (!ok) JS_ReportOutOfMemory(cx);
    return ok;
}
#define WEAK_OP(name,set,op) \
static JSBool name(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) \
{ return WeakOperation(cx, argc, argv, rval, set, op); }
WEAK_OP(WeakMapSet, JS_FALSE, 0)
WEAK_OP(WeakMapGet, JS_FALSE, 1)
WEAK_OP(WeakMapHas, JS_FALSE, 2)
WEAK_OP(WeakMapDelete, JS_FALSE, 3)
WEAK_OP(WeakSetAdd, JS_TRUE, 0)
WEAK_OP(WeakSetHas, JS_TRUE, 2)
WEAK_OP(WeakSetDelete, JS_TRUE, 3)

static JSBool
WeakConstruct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                jsval *rval, JSBool set)
{
    JSWeakCollection *data;
    JSRuntime *rt = cx->runtime;
    if (!JS_IsConstructing(cx)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_INCOMPATIBLE_PROTO, set ? "WeakSet" : "WeakMap",
                             "constructor", "receiver");
        return JS_FALSE;
    }
    data = (JSWeakCollection *)calloc(1, sizeof(*data));
    if (!data) { JS_ReportOutOfMemory(cx); return JS_FALSE; }
    data->table = js_NewCollectionData();
    if (!data->table) { free(data); JS_ReportOutOfMemory(cx); return JS_FALSE; }
    data->owner = obj;
    if (!JS_SetPrivate(cx, obj, data)) {
        js_ReleaseCollectionData(data->table);
        free(data);
        return JS_FALSE;
    }
    JS_LOCK_GC(rt);
    data->next = rt->weakCollections;
    if (data->next) data->next->previous = data;
    rt->weakCollections = data;
    JS_UNLOCK_GC(rt);
    *rval = OBJECT_TO_JSVAL(obj);
    return js_InitializeCollectionIterable(cx, obj, argc, argv, set);
}
static JSBool WeakMapConstructor(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{ return WeakConstruct(cx, obj, argc, argv, rval, JS_FALSE); }
static JSBool WeakSetConstructor(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{ return WeakConstruct(cx, obj, argc, argv, rval, JS_TRUE); }

static JSObject *
InitWeakCollection(JSContext *cx, JSObject *global, JSBool set)
{
    JSObject *proto;
    JSTempValueRooter root;
    JSBool ok;
    proto = JS_InitClass(cx, global, NULL, set ? &js_WeakSetClass : &js_WeakMapClass,
                         set ? WeakSetConstructor : WeakMapConstructor, 0,
                         NULL, NULL, NULL, NULL);
    if (!proto) return NULL;
    JS_PUSH_TEMP_ROOT_OBJECT(cx, proto, &root);
    ok = JS_DefineFunction(cx, proto, set ? "add" : "set", set ? WeakSetAdd : WeakMapSet,
                           set ? 1 : 2, JSFUN_STRICT | JSFUN_NO_CONSTRUCT) != NULL &&
         JS_DefineFunction(cx, proto, "has", set ? WeakSetHas : WeakMapHas, 1,
                           JSFUN_STRICT | JSFUN_NO_CONSTRUCT) != NULL &&
         JS_DefineFunction(cx, proto, "delete", set ? WeakSetDelete : WeakMapDelete, 1,
                           JSFUN_STRICT | JSFUN_NO_CONSTRUCT) != NULL &&
         (set || JS_DefineFunction(cx, proto, "get", WeakMapGet, 1,
                           JSFUN_STRICT | JSFUN_NO_CONSTRUCT) != NULL) &&
         js_DefineBuiltinTag(cx, proto, set ? "WeakSet" : "WeakMap");
    JS_POP_TEMP_ROOT(cx, &root);
    return ok ? proto : NULL;
}
JSObject *js_InitWeakMapClass(JSContext *cx, JSObject *global)
{ return InitWeakCollection(cx, global, JS_FALSE); }
JSObject *js_InitWeakSetClass(JSContext *cx, JSObject *global)
{ return InitWeakCollection(cx, global, JS_TRUE); }
