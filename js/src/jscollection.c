/* ES2015 Map and Set; MPL 1.1/GPL 2.0/LGPL 2.1. */
#include <stdlib.h>
#include <string.h>
#include "jsapi.h"
#include "jsarray.h"
#include "jsatom.h"
#include "jsbool.h"
#include "jscntxt.h"
#include "jscollection.h"
#include "jscollectiontable.h"
#include "jsfun.h"
#include "jsgc.h"
#include "jsinterp.h"
#include "jsiteres6.h"
#include "jslock.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsrealm.h"
#include "jssymbol.h"

JS_STATIC_DLL_CALLBACK(void)
CollectionFinalize(JSContext *cx, JSObject *obj)
{
    JSCollectionData *data = (JSCollectionData *)JS_GetPrivate(cx, obj);
    if (data) js_ReleaseCollectionData(data);
}
JS_STATIC_DLL_CALLBACK(uint32)
CollectionMark(JSContext *cx, JSObject *obj, void *arg)
{
    js_MarkCollectionData(cx, (JSCollectionData *)JS_GetPrivate(cx, obj));
    return 0;
}
#define COLLECTION_CLASS(name) { \
    #name, JSCLASS_HAS_PRIVATE | JSCLASS_HAS_CACHED_PROTO(JSProto_##name), \
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, \
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, CollectionFinalize, \
    NULL, NULL, NULL, NULL, NULL, NULL, CollectionMark, NULL \
}
JSClass js_MapClass = COLLECTION_CLASS(Map);
JSClass js_SetClass = COLLECTION_CLASS(Set);

static JSObject *
Receiver(JSContext *cx, jsval value, JSBool set)
{
    JSClass *clasp = set ? &js_SetClass : &js_MapClass;
    JSObject *obj;
    if (!JSVAL_IS_PRIMITIVE(value)) {
        obj = JSVAL_TO_OBJECT(value);
        if (OBJ_GET_CLASS(cx, obj) == clasp && JS_GetPrivate(cx, obj))
            return obj;
    }
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                         JSMSG_INCOMPATIBLE_PROTO, clasp->name, "method", "receiver");
    return NULL;
}

static JSBool
CollectionOperation(JSContext *cx, uintN argc, jsval *argv, jsval *rval,
                     JSBool set, unsigned operation)
{
    JSObject *obj = Receiver(cx, argv[-1], set);
    JSCollectionData *data;
    jsval key = argc ? argv[0] : JSVAL_VOID;
    JSBool ok = JS_TRUE, found;
    uint32 size = 0;
    if (!obj) return JS_FALSE;
    if (set && JSVAL_IS_DOUBLE(key) && *JSVAL_TO_DOUBLE(key) == 0)
        key = JSVAL_ZERO;
    JS_LOCK_OBJ(cx, obj);
    data = (JSCollectionData *)JS_GetPrivate(cx, obj);
    switch (operation) {
      case 0: /* add/set */
        ok = js_CollectionPut(data, key, set ? key : (argc > 1 ? argv[1] : JSVAL_VOID));
        *rval = argv[-1];
        break;
      case 1: /* get */
        if (!js_CollectionGet(data, key, rval)) *rval = JSVAL_VOID;
        break;
      case 2: /* has */
        found = js_CollectionGet(data, key, rval);
        *rval = BOOLEAN_TO_JSVAL(found);
        break;
      case 3: *rval = BOOLEAN_TO_JSVAL(js_CollectionDelete(data, key)); break;
      case 4: js_CollectionClear(data); *rval = JSVAL_VOID; break;
      case 5: size = js_CollectionSize(data); break;
    }
    JS_UNLOCK_OBJ(cx, obj);
    if (!ok) JS_ReportOutOfMemory(cx);
    if (ok && operation == 5) ok = js_NewNumberValue(cx, size, rval);
    return ok;
}
#define OP(name,set,op) \
static JSBool name(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) \
{ return CollectionOperation(cx, argc, argv, rval, set, op); }
OP(MapSet, JS_FALSE, 0)
OP(MapGet, JS_FALSE, 1)
OP(MapHas, JS_FALSE, 2)
OP(MapDelete, JS_FALSE, 3)
OP(MapClear, JS_FALSE, 4)
OP(MapSize, JS_FALSE, 5)
OP(SetAdd, JS_TRUE, 0)
OP(SetHas, JS_TRUE, 2)
OP(SetDelete, JS_TRUE, 3)
OP(SetClear, JS_TRUE, 4)
OP(SetSize, JS_TRUE, 5)

static JSBool
ForEach(JSContext *cx, uintN argc, jsval *argv, jsval *rval, JSBool set)
{
    JSObject *obj = Receiver(cx, argv[-1], set);
    JSCollectionData *data;
    jsval roots[4], key;
    JSTempValueRooter root;
    uint32 cursor = 0;
    JSBool ok, found;
    if (!obj) return JS_FALSE;
    if (!argc || !js_IsCallable(cx, argv[0])) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_NOT_FUNCTION, "callback");
        return JS_FALSE;
    }
    roots[0] = roots[1] = roots[3] = JSVAL_VOID;
    roots[2] = argv[-1];
    JS_PUSH_TEMP_ROOT(cx, 4, roots, &root);
    JS_LOCK_OBJ(cx, obj);
    data = (JSCollectionData *)JS_GetPrivate(cx, obj);
    ok = js_BeginCollectionIteration(data);
    JS_UNLOCK_OBJ(cx, obj);
    if (!ok) {
        JS_ReportOutOfMemory(cx);
        goto out;
    }
    for (;;) {
        JS_LOCK_OBJ(cx, obj);
        found = js_CollectionNext(data, &cursor, &key, &roots[0]);
        if (found) roots[1] = set ? roots[0] : key;
        JS_UNLOCK_OBJ(cx, obj);
        if (!found) break;
        if (!js_InternalInvokeValue(cx, argc > 1 ? argv[1] : JSVAL_VOID,
                                     argv[0], 0, 3, roots, &roots[3])) {
            ok = JS_FALSE;
            break;
        }
    }
    JS_LOCK_OBJ(cx, obj);
    js_EndCollectionIteration(data);
    JS_UNLOCK_OBJ(cx, obj);
    *rval = JSVAL_VOID;
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}
static JSBool MapForEach(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{ return ForEach(cx, argc, argv, rval, JS_FALSE); }
static JSBool SetForEach(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{ return ForEach(cx, argc, argv, rval, JS_TRUE); }

/* C storage ownership survives either order of collection/iterator finalizers.
 * The reserved slot separately keeps the collection and its keys alive. */
typedef struct CollectionCursor {
    JSCollectionData *data;
    uint32 index;
    unsigned kind;
} CollectionCursor;
JS_STATIC_DLL_CALLBACK(void)
CursorFinalize(JSContext *cx, JSObject *obj)
{
    CollectionCursor *cursor = (CollectionCursor *)JS_GetPrivate(cx, obj);
    if (cursor) {
        if (cursor->data) js_EndCollectionIteration(cursor->data);
        free(cursor);
    }
}
#define CURSOR_CLASS(name) { \
    name " Iterator", JSCLASS_HAS_PRIVATE | JSCLASS_HAS_RESERVED_SLOTS(1), \
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, \
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, CursorFinalize, \
    JSCLASS_NO_OPTIONAL_MEMBERS \
}
static JSClass mapCursorClass = CURSOR_CLASS("Map");
static JSClass setCursorClass = CURSOR_CLASS("Set");
static JSBool
CursorNext(JSContext *cx, jsval *argv, jsval *rval, JSBool set)
{
    JSObject *obj, *target, *global, *proto, *pair;
    JSClass *clasp = set ? &setCursorClass : &mapCursorClass;
    CollectionCursor *cursor;
    jsval roots[4];
    JSTempValueRooter root;
    JSBool found = JS_FALSE, ok = JS_FALSE;
    unsigned kind = 0;
    if (JSVAL_IS_PRIMITIVE(argv[-1]) ||
        OBJ_GET_CLASS(cx, JSVAL_TO_OBJECT(argv[-1])) != clasp) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_INCOMPATIBLE_PROTO, clasp->name, "next", "receiver");
        return JS_FALSE;
    }
    obj = JSVAL_TO_OBJECT(argv[-1]);
    roots[0] = roots[1] = roots[2] = roots[3] = JSVAL_VOID;
    JS_PUSH_TEMP_ROOT(cx, 4, roots, &root);
    global = js_BuiltinGlobal(cx, argv);
    if (!JS_GetReservedSlot(cx, obj, 0, &roots[0])) goto out;
    cursor = (CollectionCursor *)JS_GetPrivate(cx, obj);
    if (cursor && !JSVAL_IS_VOID(roots[0])) {
        target = JSVAL_TO_OBJECT(roots[0]);
        JS_LOCK_OBJ(cx, target);
        if (cursor->data) {
            found = js_CollectionNext(cursor->data, &cursor->index, &roots[1], &roots[2]);
            kind = cursor->kind;
            if (!found) {
                js_EndCollectionIteration(cursor->data);
                cursor->data = NULL;
            }
        }
        JS_UNLOCK_OBJ(cx, target);
        if (!found && !JS_SetReservedSlot(cx, obj, 0, JSVAL_VOID)) goto out;
    }
    if (found) {
        if (kind < 2) roots[3] = roots[kind + 1];
        else {
            proto = js_BuiltinPrototype(cx, global, JSProto_Array);
            pair = proto ? js_NewArrayObjectWithProto(cx, 0, NULL, proto, global) : NULL;
            if (!pair) goto out;
            roots[3] = OBJECT_TO_JSVAL(pair);
            if (!js_CreateDataPropertyOrThrow(cx, pair, INT_TO_JSID(0), roots[1]) ||
                !js_CreateDataPropertyOrThrow(cx, pair, INT_TO_JSID(1), roots[2])) goto out;
        }
    }
    ok = js_IteratorResult(cx, global, roots[3], !found, rval);
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}
static JSBool MapNext(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{ return CursorNext(cx, argv, rval, JS_FALSE); }
static JSBool SetNext(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{ return CursorNext(cx, argv, rval, JS_TRUE); }

static JSObject *
CursorPrototype(JSContext *cx, JSObject *global, JSBool set)
{
    JSRealmIntrinsic key = set ? JS_INTRINSIC_SET_ITERATOR_PROTO : JS_INTRINSIC_MAP_ITERATOR_PROTO;
    JSObject *proto = js_GetCachedIntrinsic(cx, global, key), *parent;
    JSTempValueRooter root;
    JSBool ok;
    if (proto) return proto;
    parent = js_GetIteratorPrototype(cx, global);
    if (!parent) return NULL;
    proto = js_NewObject(cx, &js_ObjectClass, parent, global);
    if (!proto) return NULL;
    JS_PUSH_TEMP_ROOT_OBJECT(cx, proto, &root);
    ok = JS_DefineFunction(cx, proto, "next", set ? SetNext : MapNext, 0,
                           JSFUN_STRICT | JSFUN_NO_CONSTRUCT) != NULL &&
         js_DefineBuiltinTag(cx, proto, set ? "Set Iterator" : "Map Iterator") &&
         js_CacheIntrinsic(cx, global, key, proto);
    JS_POP_TEMP_ROOT(cx, &root);
    return ok ? js_GetCachedIntrinsic(cx, global, key) : NULL;
}
static JSBool
NewCursor(JSContext *cx, jsval *argv, jsval *rval, JSBool set, unsigned kind)
{
    JSObject *target = Receiver(cx, argv[-1], set), *global, *proto, *obj;
    CollectionCursor *cursor;
    JSTempValueRooter root;
    JSBool ok = JS_FALSE;
    if (!target) return JS_FALSE;
    global = js_BuiltinGlobal(cx, argv);
    proto = CursorPrototype(cx, global, set);
    if (!proto) return JS_FALSE;
    obj = js_NewObject(cx, set ? &setCursorClass : &mapCursorClass, proto, global);
    if (!obj) return JS_FALSE;
    JS_PUSH_TEMP_ROOT_OBJECT(cx, obj, &root);
    if (!JS_SetReservedSlot(cx, obj, 0, argv[-1])) goto out;
    cursor = (CollectionCursor *)calloc(1, sizeof(*cursor));
    if (!cursor) { JS_ReportOutOfMemory(cx); goto out; }
    JS_LOCK_OBJ(cx, target);
    cursor->data = (JSCollectionData *)JS_GetPrivate(cx, target);
    ok = js_BeginCollectionIteration(cursor->data);
    JS_UNLOCK_OBJ(cx, target);
    if (!ok) { free(cursor); JS_ReportOutOfMemory(cx); goto out; }
    cursor->kind = kind;
    ok = JS_SetPrivate(cx, obj, cursor);
    if (!ok) {
        JS_LOCK_OBJ(cx, target);
        js_EndCollectionIteration(cursor->data);
        JS_UNLOCK_OBJ(cx, target);
        free(cursor);
    } else *rval = OBJECT_TO_JSVAL(obj);
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}
#define ITER(name,set,kind) \
static JSBool name(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) \
{ return NewCursor(cx, argv, rval, set, kind); }
ITER(MapKeys, JS_FALSE, 0)
ITER(MapValues, JS_FALSE, 1)
ITER(MapEntries, JS_FALSE, 2)
ITER(SetValues, JS_TRUE, 1)
ITER(SetEntries, JS_TRUE, 2)

JSBool
js_InitializeCollectionIterable(JSContext *cx, JSObject *obj, uintN argc,
                                jsval *argv, JSBool set)
{
    jsval roots[8];
    JSTempValueRooter root;
    JSObject *source, *iterator;
    jsid id;
    JSBool done, ok = JS_FALSE;
    uintN i;
    if (!argc || JSVAL_IS_NULL(argv[0]) || JSVAL_IS_VOID(argv[0])) return JS_TRUE;
    for (i = 0; i < 8; ++i) roots[i] = JSVAL_VOID;
    JS_PUSH_TEMP_ROOT(cx, 8, roots, &root);
    if (!JS_GetProperty(cx, obj, set ? "add" : "set", &roots[0])) goto out;
    if (!js_IsCallable(cx, roots[0])) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_NOT_FUNCTION, "collection adder");
        goto out;
    }
    source = js_ValueToNonNullObject(cx, argv[0]);
    if (!source) goto out;
    roots[1] = OBJECT_TO_JSVAL(source);
    if (!js_WellKnownSymbolId(cx, JS_WKS_ITERATOR, &id) ||
        !(JSVAL_IS_PRIMITIVE(argv[0]) ? js_GetPropertyValue(cx, source, argv[0], id, &roots[2])
                                    : OBJ_GET_PROPERTY(cx, source, id, &roots[2]))) goto out;
    if (!js_IsCallable(cx, roots[2])) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_NOT_FUNCTION, "Symbol.iterator");
        goto out;
    }
    if (!js_InternalInvokeValue(cx, argv[0], roots[2], 0, 0, NULL, &roots[1])) goto out;
    if (JSVAL_IS_PRIMITIVE(roots[1])) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_BAD_ITERATOR_RETURN, "items", "Symbol.iterator");
        goto out;
    }
    iterator = JSVAL_TO_OBJECT(roots[1]);
    for (;;) {
        if (!JS_GetProperty(cx, iterator, "next", &roots[2])) goto out;
        if (!js_IsCallable(cx, roots[2])) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_NOT_FUNCTION, "iterator next");
            goto out;
        }
        if (!js_InternalCall(cx, iterator, roots[2], 0, NULL, &roots[3])) goto out;
        if (JSVAL_IS_PRIMITIVE(roots[3])) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_BAD_ITERATOR_RETURN, "iterator", "next");
            goto out;
        }
        if (!JS_GetProperty(cx, JSVAL_TO_OBJECT(roots[3]), "done", &roots[2]) ||
            !js_ValueToBoolean(cx, roots[2], &done)) goto out;
        if (done) break;
        if (!JS_GetProperty(cx, JSVAL_TO_OBJECT(roots[3]), "value", &roots[4])) goto out;
        if (set) roots[5] = roots[4];
        else {
            if (JSVAL_IS_PRIMITIVE(roots[4])) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                     JSMSG_BAD_ITERATOR_RETURN, "Map", "entry");
                goto close;
            }
            if (!JS_GetElement(cx, JSVAL_TO_OBJECT(roots[4]), 0, &roots[5]) ||
                !JS_GetElement(cx, JSVAL_TO_OBJECT(roots[4]), 1, &roots[6])) goto close;
        }
        if (!js_InternalCall(cx, obj, roots[0], set ? 1 : 2, &roots[5], &roots[7])) goto close;
    }
    ok = JS_TRUE;
    goto out;
  close:
    js_IteratorCloseThrow(cx, iterator);
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}
static JSBool
Construct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval, JSBool set)
{
    JSCollectionData *data;
    if (!JS_IsConstructing(cx)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_INCOMPATIBLE_PROTO,
                             set ? "Set" : "Map", "constructor", "receiver");
        return JS_FALSE;
    }
    data = js_NewCollectionData();
    if (!data) { JS_ReportOutOfMemory(cx); return JS_FALSE; }
    if (!JS_SetPrivate(cx, obj, data)) {
        js_ReleaseCollectionData(data);
        return JS_FALSE;
    }
    *rval = OBJECT_TO_JSVAL(obj);
    return js_InitializeCollectionIterable(cx, obj, argc, argv, set);
}
static JSBool MapConstructor(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{ return Construct(cx, obj, argc, argv, rval, JS_FALSE); }
static JSBool SetConstructor(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{ return Construct(cx, obj, argc, argv, rval, JS_TRUE); }
static JSBool Species(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{ *rval = argv[-1]; return JS_TRUE; }

typedef struct CollectionMethod {
    const char *name;
    JSNative native;
    uintN length;
} CollectionMethod;
static CollectionMethod mapMethods[] = {
    {"set", MapSet, 2},
    {"get", MapGet, 1},
    {"has", MapHas, 1},
    {"delete", MapDelete, 1},
    {"clear", MapClear, 0},
    {"forEach", MapForEach, 1},
    {"keys", MapKeys, 0},
    {"values", MapValues, 0},
    {"entries", MapEntries, 0},
    {0,0,0}
};
static CollectionMethod setMethods[] = {
    {"add", SetAdd, 1},
    {"has", SetHas, 1},
    {"delete", SetDelete, 1},
    {"clear", SetClear, 0},
    {"forEach", SetForEach, 1},
    {"values", SetValues, 0},
    {"entries", SetEntries, 0},
    {0,0,0}
};
typedef struct CollectionIdRoot {
    JSTempValueRooter root;
    jsid id;
} CollectionIdRoot;
JS_STATIC_DLL_CALLBACK(void)
MarkCollectionId(JSContext *cx, JSTempValueRooter *root)
{
    jsid id = ((CollectionIdRoot *)root)->id;
    if (JSID_IS_ATOM(id)) js_MarkAtom(cx, JSID_TO_ATOM(id));
}
static JSBool
DefineGetter(JSContext *cx, JSObject *global, JSObject *obj, jsid id,
              const char *name, JSNative native)
{
    JSAtom *atom;
    JSFunction *fun;
    JSTempValueRooter root;
    CollectionIdRoot idRoot;
    JSBool ok = JS_FALSE;
    idRoot.id = id;
    JS_PUSH_TEMP_ROOT_MARKER(cx, MarkCollectionId, &idRoot.root);
    atom = js_Atomize(cx, name, strlen(name), 0);
    if (!atom) goto out;
    fun = js_NewFunction(cx, NULL, native, 0, JSFUN_STRICT | JSFUN_NO_CONSTRUCT, global, atom);
    if (!fun) goto out;
    JS_PUSH_TEMP_ROOT_OBJECT(cx, fun->object, &root);
    ok = OBJ_DEFINE_PROPERTY(cx, obj, id, JSVAL_VOID, (JSPropertyOp)fun->object,
                             NULL, JSPROP_GETTER | JSPROP_SHARED, NULL);
    JS_POP_TEMP_ROOT(cx, &root);
  out:
    JS_POP_TEMP_ROOT(cx, &idRoot.root);
    return ok;
}

static JSObject *
InitCollection(JSContext *cx, JSObject *global, JSBool set)
{
    JSObject *proto, *ctor;
    jsval value;
    JSTempValueRooter root;
    JSAtom *atom;
    jsid id;
    JSBool ok = JS_FALSE;
    CollectionMethod *method;
    proto = JS_InitClass(cx, global, NULL, set ? &js_SetClass : &js_MapClass,
                         set ? SetConstructor : MapConstructor, 0, NULL, NULL, NULL, NULL);
    if (!proto) return NULL;
    JS_PUSH_TEMP_ROOT_OBJECT(cx, proto, &root);
    ctor = JS_GetConstructor(cx, proto);
    if (!ctor) goto out;
    for (method = set ? setMethods : mapMethods; method->name; ++method) {
        if (!JS_DefineFunction(cx, proto, method->name, method->native, method->length,
                               JSFUN_STRICT | JSFUN_NO_CONSTRUCT)) goto out;
    }
    atom = js_Atomize(cx, "size", 4, 0);
    if (!atom || !DefineGetter(cx, global, proto, ATOM_TO_JSID(atom), "get size", set ? SetSize : MapSize)) goto out;
    if (!js_WellKnownSymbolId(cx, JS_WKS_SPECIES, &id) ||
        !DefineGetter(cx, global, ctor, id, "get [Symbol.species]", Species)) goto out;
    if (!js_DefineBuiltinTag(cx, proto, set ? "Set" : "Map")) goto out;
    if (!JS_GetProperty(cx, proto, set ? "values" : "entries", &value)) goto out;
    if (set && !JS_DefineProperty(cx, proto, "keys", value, NULL, NULL, 0)) goto out;
    if (!js_WellKnownSymbolId(cx, JS_WKS_ITERATOR, &id) ||
        !OBJ_DEFINE_PROPERTY(cx, proto, id, value, NULL, NULL, 0, NULL)) goto out;
    ok = JS_TRUE;
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok ? proto : NULL;
}
JSObject *js_InitMapClass(JSContext *cx, JSObject *global)
{ return InitCollection(cx, global, JS_FALSE); }
JSObject *js_InitSetClass(JSContext *cx, JSObject *global)
{ return InitCollection(cx, global, JS_TRUE); }
