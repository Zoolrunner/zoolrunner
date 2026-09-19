/* Private intrinsic cache for classic embedding globals.
 * MPL 1.1/GPL 2.0/LGPL 2.1.
 *
 * Historical globals need not reserve JSProto slots. Keep their original
 * constructors available to modern code without changing the public JSClass
 * layout or exposing hidden properties. The global key is weak: constructors
 * are marked only while that global is reachable, so their parent links do
 * not keep a dead realm alive through this table.
 */
#include <string.h>
#include "jsapi.h"
#include "jscntxt.h"
#include "jsdhash.h"
#include "jsgc.h"
#include "jslock.h"
#include "jsrealm.h"
#include "jsobj.h"
#include "jsbool.h"
#include "jsscope.h"

#define REALM_CACHE_LIMIT (JSProto_LIMIT + JS_INTRINSIC_LIMIT)

typedef struct ClassCacheEntry {
    JSDHashEntryHdr hdr;
    JSObject *global; /* Same position as JSDHashEntryStub.key. */
    JSBool modernGlobal;
    JSObject *constructors[REALM_CACHE_LIMIT];
} ClassCacheEntry;

static JSObject *
GetCachedObject(JSContext *cx, JSObject *global, uintN key)
{
    JSRuntime *rt = cx->runtime;
    ClassCacheEntry *entry;
    JSObject *constructor = NULL;
    JS_ASSERT(key < REALM_CACHE_LIMIT);
    JS_LOCK_GC(rt);
    if (rt->classObjectCache) {
        entry = (ClassCacheEntry *)JS_DHashTableOperate(rt->classObjectCache,
                                                       global, JS_DHASH_LOOKUP);
        if (JS_DHASH_ENTRY_IS_BUSY(&entry->hdr))
            constructor = entry->constructors[key];
    }
    JS_UNLOCK_GC(rt);
    return constructor;
}

static JSBool
CacheObject(JSContext *cx, JSObject *global, uintN key,
                    JSObject *constructor)
{
    JSRuntime *rt = cx->runtime;
    ClassCacheEntry *entry = NULL;
    JS_ASSERT(key < REALM_CACHE_LIMIT);
    JS_LOCK_GC(rt);
    if (!rt->classObjectCache) {
        rt->classObjectCache = JS_NewDHashTable(JS_DHashGetStubOps(), NULL,
                                               sizeof(ClassCacheEntry), 8);
    }
    if (rt->classObjectCache) {
        entry = (ClassCacheEntry *)JS_DHashTableOperate(rt->classObjectCache,
                                                       global, JS_DHASH_ADD);
        if (entry) {
            if (!entry->global) {
                entry->global = global;
                entry->modernGlobal = JS_VERSION_IS_ES2015(cx);
                memset(entry->constructors, 0, sizeof(entry->constructors));
            }
            /* A deleted global name can cause a legacy resolve hook to
             * initialize a class again. A realm's intrinsic is the first
             * constructor; only JS_ClearScope starts a new cache lifetime. */
            if (!entry->constructors[key])
                entry->constructors[key] = constructor;
        }
    }
    JS_UNLOCK_GC(rt);
    if (!entry) {
        JS_ReportOutOfMemory(cx);
        return JS_FALSE;
    }
    return JS_TRUE;
}

/* This is a property of the initialized global, not its current caller. */
JSBool
js_IsModernGlobal(JSContext *cx, JSObject *global)
{
    ClassCacheEntry *entry;
    JSBool modern = JS_FALSE;
    JS_LOCK_GC(cx->runtime);
    if (cx->runtime->classObjectCache) {
        entry = (ClassCacheEntry *)JS_DHashTableOperate(cx->runtime->classObjectCache,
                                                       global, JS_DHASH_LOOKUP);
        if (JS_DHASH_ENTRY_IS_BUSY(&entry->hdr)) modern = entry->modernGlobal;
    }
    JS_UNLOCK_GC(cx->runtime);
    return modern;
}

JSObject *
js_GetCachedClassObject(JSContext *cx, JSObject *global, JSProtoKey key)
{
    JS_ASSERT((uintN)key < JSProto_LIMIT);
    return GetCachedObject(cx, global, (uintN)key);
}

JSBool
js_CacheClassObject(JSContext *cx, JSObject *global, JSProtoKey key,
                    JSObject *constructor)
{
    JS_ASSERT((uintN)key < JSProto_LIMIT);
    return CacheObject(cx, global, (uintN)key, constructor);
}

JSObject *
js_GetCachedIntrinsic(JSContext *cx, JSObject *global, JSRealmIntrinsic key)
{
    JS_ASSERT((uintN)key < JS_INTRINSIC_LIMIT);
    return GetCachedObject(cx, global, JSProto_LIMIT + (uintN)key);
}

JSBool
js_CacheIntrinsic(JSContext *cx, JSObject *global, JSRealmIntrinsic key,
                   JSObject *value)
{
    JS_ASSERT((uintN)key < JS_INTRINSIC_LIMIT);
    return CacheObject(cx, global, JSProto_LIMIT + (uintN)key, value);
}

/* The collector has suspended mutator requests. Copy before marking, so a
 * recursive class mark hook cannot invalidate an entry pointer by rehashing. */
void
js_MarkCachedClassObjects(JSContext *cx, JSObject *global)
{
    ClassCacheEntry *entry;
    JSObject *constructors[REALM_CACHE_LIMIT];
    uintN i;
    if (!cx->runtime->classObjectCache)
        return;
    entry = (ClassCacheEntry *)JS_DHashTableOperate(cx->runtime->classObjectCache,
                                                   global, JS_DHASH_LOOKUP);
    if (!JS_DHASH_ENTRY_IS_BUSY(&entry->hdr))
        return;
    memcpy(constructors, entry->constructors, sizeof(constructors));
    for (i = 0; i < REALM_CACHE_LIMIT; ++i) {
        if (constructors[i])
            GC_MARK(cx, constructors[i], "realm intrinsic");
    }
}

JS_STATIC_DLL_CALLBACK(JSDHashOperator)
SweepClassCache(JSDHashTable *table, JSDHashEntryHdr *header, uint32 number,
                void *argument)
{
    ClassCacheEntry *entry = (ClassCacheEntry *)header;
    return (*js_GetGCThingFlags(entry->global) & GCF_MARK)
           ? JS_DHASH_NEXT : JS_DHASH_REMOVE;
}

void
js_SweepCachedClassObjects(JSRuntime *rt)
{
    if (rt->classObjectCache)
        JS_DHashTableEnumerate(rt->classObjectCache, SweepClassCache, NULL);
}

void
js_ClearCachedClassObjects(JSContext *cx, JSObject *global)
{
    JSRuntime *rt = cx->runtime;
    JS_LOCK_GC(rt);
    if (rt->classObjectCache)
        JS_DHashTableOperate(rt->classObjectCache, global, JS_DHASH_REMOVE);
    rt->gcPoke = JS_TRUE;
    JS_UNLOCK_GC(rt);
}

void
js_FinishCachedClassObjects(JSRuntime *rt)
{
    if (rt->classObjectCache) {
        JS_DHashTableDestroy(rt->classObjectCache);
        rt->classObjectCache = NULL;
    }
}

/* Persistent declarative bindings are private to the owning realm. */
static JSBool
GlobalLexicalGet(JSContext *cx, JSObject *obj, jsval name, jsval *value)
{
    if (*value != JSVAL_UNINITIALIZED) return JS_TRUE;
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_UNINITIALIZED_LEXICAL);
    return JS_FALSE;
}

static JSBool
GlobalLexicalSet(JSContext *cx, JSObject *obj, jsval name, jsval *value)
{
    jsid id;
    JSScopeProperty *property;
    JSBool pending, immutable;
    if (!js_ValueToPropertyId(cx, name, &id)) return JS_FALSE;
    JS_LOCK_OBJ(cx, obj);
    property = SCOPE_GET_PROPERTY(OBJ_SCOPE(obj), id);
    pending = property && OBJ_GET_SLOT(cx, obj, property->slot) == JSVAL_UNINITIALIZED;
    immutable = property && (property->flags & SPROP_IS_CONST);
    JS_UNLOCK_OBJ(cx, obj);
    if (!property || pending || immutable) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             pending ? JSMSG_UNINITIALIZED_LEXICAL : JSMSG_BAD_DESCRIPTOR);
        return JS_FALSE;
    }
    return JS_TRUE;
}

static JSClass globalLexicalClass = {
    "Global Lexical Environment", JSCLASS_HAS_RESERVED_SLOTS(2) |
    JSCLASS_IS_ANONYMOUS | JSCLASS_HAS_CACHED_PROTO(JSProto_Object),
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

JSBool
js_IsLexicalEnvironment(JSContext *cx, JSObject *obj)
{
    return obj && OBJ_GET_CLASS(cx, obj) == &globalLexicalClass;
}

JSBool
js_IsGlobalLexicalEnvironment(JSContext *cx, JSObject *obj)
{
    jsval persistent;
    if (!js_IsLexicalEnvironment(cx, obj)) return JS_FALSE;
    JS_GetReservedSlot(cx, obj, 1, &persistent);
    return persistent == JSVAL_TRUE;
}

JSObject *
js_NewLexicalEnvironment(JSContext *cx, JSObject *outer)
{
    JSObject *env = js_NewObject(cx, &globalLexicalClass, NULL, outer);
    JSTempValueRooter root;
    JSBool ok;
    if (!env) return NULL;
    JS_PUSH_TEMP_ROOT_OBJECT(cx, env, &root);
    ok = JS_SetPrototype(cx, env, NULL) && JS_SetReservedSlot(cx, env, 1, JSVAL_FALSE);
    JS_POP_TEMP_ROOT(cx, &root);
    return ok ? env : NULL;
}

static JSObject *GlobalVarNames(JSContext *cx, JSObject *env, JSBool create);

JSObject *
js_GlobalLexicalEnvironment(JSContext *cx, JSObject *global, JSBool create)
{
    JSObject *env = js_GetCachedIntrinsic(cx, global, JS_INTRINSIC_GLOBAL_LEXICAL);
    JSTempValueRooter root;
    JSBool ok;
    if (env || !create) return env;
    env = js_NewLexicalEnvironment(cx, global);
    if (!env) return NULL;
    JS_PUSH_TEMP_ROOT_OBJECT(cx, env, &root);
    ok = JS_SetReservedSlot(cx, env, 1, JSVAL_TRUE) &&
         GlobalVarNames(cx, env, JS_TRUE) != NULL &&
         js_CacheIntrinsic(cx, global, JS_INTRINSIC_GLOBAL_LEXICAL, env);
    JS_POP_TEMP_ROOT(cx, &root);
    return ok ? js_GetCachedIntrinsic(cx, global, JS_INTRINSIC_GLOBAL_LEXICAL) : NULL;
}

static JSObject *
GlobalVarNames(JSContext *cx, JSObject *env, JSBool create)
{
    jsval value;
    JSObject *names;
    JSTempValueRooter root;
    JSBool ok;
    JS_GetReservedSlot(cx, env, 0, &value);
    if (JSVAL_IS_OBJECT(value) && !JSVAL_IS_NULL(value))
        return JSVAL_TO_OBJECT(value);
    if (!create) return NULL;
    names = js_NewObject(cx, &js_ObjectClass, NULL, env);
    if (!names) return NULL;
    JS_PUSH_TEMP_ROOT_OBJECT(cx, names, &root);
    ok = JS_SetPrototype(cx, names, NULL) &&
         JS_SetReservedSlot(cx, env, 0, OBJECT_TO_JSVAL(names));
    JS_POP_TEMP_ROOT(cx, &root);
    return ok ? names : NULL;
}

JSBool
js_RecordGlobalVarBinding(JSContext *cx, JSObject *env, jsid id)
{
    JSObject *names = GlobalVarNames(cx, env, JS_TRUE);
    return names && js_DefineNativeProperty(cx, names, id, JSVAL_TRUE,
                                            NULL, NULL, 0, 0, 0, NULL);
}

JSBool
js_ForgetGlobalVarBinding(JSContext *cx, JSObject *env, jsid id)
{
    JSObject *names = GlobalVarNames(cx, env, JS_FALSE);
    jsval result;
    return !names || OBJ_DELETE_PROPERTY(cx, names, id, &result);
}

JSBool
js_CanDeclareGlobalLexicalBinding(JSContext *cx, JSObject *env, jsid id)
{
    JSObject *names, *global, *owner;
    JSProperty *property;
    JSBool conflict, ok;
    uintN attrs;
    JS_LOCK_OBJ(cx, env);
    conflict = SCOPE_GET_PROPERTY(OBJ_SCOPE(env), id) != NULL;
    JS_UNLOCK_OBJ(cx, env);
    names = GlobalVarNames(cx, env, JS_FALSE);
    if (!conflict && names) {
        JS_LOCK_OBJ(cx, names);
        conflict = SCOPE_GET_PROPERTY(OBJ_SCOPE(names), id) != NULL;
        JS_UNLOCK_OBJ(cx, names);
    }
    if (!conflict) {
        global = OBJ_GET_PARENT(cx, env);
        if (!js_LookupOwnProperty(cx, global, id, &owner, &property)) return JS_FALSE;
        if (property) {
            ok = JS_TRUE;
            if (owner == global) {
                ok = OBJ_GET_ATTRIBUTES(cx, global, id, property, &attrs);
                if (ok) conflict = (attrs & JSPROP_PERMANENT) != 0;
            }
            OBJ_DROP_PROPERTY(cx, owner, property);
            if (!ok) return JS_FALSE;
        }
    }
    if (conflict)
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_STRICT_SYNTAX);
    return !conflict;
}

JSBool
js_DefineLexicalBinding(JSContext *cx, JSObject *env, jsid id, JSBool immutable)
{
    JSBool duplicate;
    JS_ASSERT(js_IsLexicalEnvironment(cx, env));
    if (js_IsGlobalLexicalEnvironment(cx, env)) {
        if (!js_CanDeclareGlobalLexicalBinding(cx, env, id)) return JS_FALSE;
    } else {
        JS_LOCK_OBJ(cx, env);
        duplicate = SCOPE_GET_PROPERTY(OBJ_SCOPE(env), id) != NULL;
        JS_UNLOCK_OBJ(cx, env);
        if (duplicate) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_STRICT_SYNTAX);
            return JS_FALSE;
        }
    }
    return js_DefineNativeProperty(cx, env, id, JSVAL_UNINITIALIZED,
                                   GlobalLexicalGet, GlobalLexicalSet,
                                   JSPROP_ENUMERATE | JSPROP_PERMANENT,
                                   immutable ? SPROP_IS_CONST : 0, 0, NULL);
}

JSBool
js_InitializeLexicalBinding(JSContext *cx, JSObject *env, jsid id, jsval value)
{
    JSScopeProperty *property;
    JSBool pending;
    JS_ASSERT(js_IsLexicalEnvironment(cx, env));
    JS_LOCK_OBJ(cx, env);
    property = SCOPE_GET_PROPERTY(OBJ_SCOPE(env), id);
    pending = property && OBJ_GET_SLOT(cx, env, property->slot) == JSVAL_UNINITIALIZED;
    if (pending) OBJ_SET_SLOT(cx, env, property->slot, value);
    JS_UNLOCK_OBJ(cx, env);
    if (!pending)
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_BAD_DESCRIPTOR);
    return pending;
}
