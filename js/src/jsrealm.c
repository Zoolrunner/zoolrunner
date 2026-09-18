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
