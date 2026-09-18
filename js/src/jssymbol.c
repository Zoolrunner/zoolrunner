/* ES2015 Symbol primitives and runtime identities.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
#include <stdlib.h>
#include <string.h>
#include "jsapi.h"
#include "jsatom.h"
#include "jscntxt.h"
#include "jsdhash.h"
#include "jsfun.h"
#include "jsgc.h"
#include "jslock.h"
#include "jsobj.h"
#include "jsprf.h"
#include "jssymbol.h"

typedef struct SymbolRegistryKey {
    const jschar *chars;
    size_t length;
} SymbolRegistryKey;

typedef struct SymbolRegistryEntry {
    JSDHashEntryHdr hdr;
    SymbolRegistryKey key;
    JSSymbol *symbol;
} SymbolRegistryEntry;

struct JSSymbolState {
    JSSymbol *wellKnown[JS_WKS_LIMIT];
    JSDHashTable *registry;
};

static const char *wellKnownNames[JS_WKS_LIMIT] = {
    "hasInstance", "isConcatSpreadable", "iterator", "match", "replace",
    "search", "species", "split", "toPrimitive", "toStringTag", "unscopables"
};

JS_PUBLIC_API(JSBool)
JS_IsSymbolValue(jsval value)
{
    return JSVAL_TAG(value) == JSVAL_STRING && JSVAL_TO_STRING(value) &&
           JSSTRING_IS_SYMBOL(JSVAL_TO_STRING(value));
}

static struct JSSymbolState *
SymbolState(JSContext *cx)
{
    JSRuntime *rt = cx->runtime;
    struct JSSymbolState *state;
    JS_LOCK_GC(rt);
    state = rt->symbolState;
    if (!state) {
        state = (struct JSSymbolState *)calloc(1, sizeof(*state));
        rt->symbolState = state;
    }
    JS_UNLOCK_GC(rt);
    if (!state)
        JS_ReportOutOfMemory(cx);
    return state;
}

JSSymbol *
js_NewSymbol(JSContext *cx, JSString *description)
{
    JSSymbol *symbol;
    JSTempValueRooter root;
    jschar *chars;
    size_t i, length = description ? JSSTRING_LENGTH(description) : 0;
    static const char prefix[] = "Symbol(";
    if (length > JSSTRING_LENGTH_MASK - 8 ||
        length > ((size_t)-1) / sizeof(jschar) - 9) {
        JS_ReportOutOfMemory(cx);
        return NULL;
    }
    JS_PUSH_SINGLE_TEMP_ROOT(cx, description ? STRING_TO_JSVAL(description)
                                            : JSVAL_VOID, &root);
    chars = (jschar *)JS_malloc(cx, (length + 9) * sizeof(jschar));
    if (!chars) {
        JS_POP_TEMP_ROOT(cx, &root);
        return NULL;
    }
    for (i = 0; i < 7; ++i)
        chars[i] = prefix[i];
    if (length)
        memcpy(chars + 7, JSSTRING_CHARS(description), length * sizeof(jschar));
    chars[length + 7] = ')';
    chars[length + 8] = 0;
    symbol = (JSSymbol *)js_NewGCThing(cx, GCX_STRING, sizeof(JSSymbol));
    if (symbol) {
        symbol->string.length = JSSTRFLAG_PREFIX | (length + 8);
        symbol->string.chars = chars;
        symbol->registered = JS_FALSE;
        JS_RUNTIME_METER(cx->runtime, liveStrings);
        JS_RUNTIME_METER(cx->runtime, totalStrings);
    } else {
        JS_free(cx, chars);
    }
    JS_POP_TEMP_ROOT(cx, &root);
    return symbol;
}

JSSymbol *
js_GetWellKnownSymbol(JSContext *cx, JSWellKnownSymbol key)
{
    struct JSSymbolState *state = SymbolState(cx);
    JSSymbol *symbol, *created;
    JSString *description;
    JSTempValueRooter root;
    char name[64];
    if (!state)
        return NULL;
    JS_ASSERT((uintN)key < JS_WKS_LIMIT);
    JS_LOCK_GC(cx->runtime);
    symbol = state->wellKnown[key];
    JS_UNLOCK_GC(cx->runtime);
    if (symbol)
        return symbol;
    JS_snprintf(name, sizeof(name), "Symbol.%s", wellKnownNames[key]);
    description = JS_NewStringCopyZ(cx, name);
    if (!description)
        return NULL;
    created = js_NewSymbol(cx, description);
    if (!created)
        return NULL;
    JS_PUSH_SINGLE_TEMP_ROOT(cx, STRING_TO_JSVAL(created), &root);
    JS_LOCK_GC(cx->runtime);
    symbol = state->wellKnown[key];
    if (!symbol)
        symbol = state->wellKnown[key] = created;
    JS_UNLOCK_GC(cx->runtime);
    JS_POP_TEMP_ROOT(cx, &root);
    return symbol;
}

JSBool
js_WellKnownSymbolId(JSContext *cx, JSWellKnownSymbol key, jsid *idp)
{
    JSSymbol *symbol = js_GetWellKnownSymbol(cx, key);
    JSAtom *atom;
    if (!symbol)
        return JS_FALSE;
    atom = js_AtomizeValue(cx, STRING_TO_JSVAL(symbol), ATOM_PINNED);
    if (!atom)
        return JS_FALSE;
    *idp = ATOM_TO_JSID(atom);
    return JS_TRUE;
}

JSBool
js_DefineBuiltinTag(JSContext *cx, JSObject *obj, const char *name)
{
    jsval roots[2];
    JSTempValueRooter root;
    JSString *tag;
    jsid id;
    JSBool ok = JS_FALSE;
    roots[0] = OBJECT_TO_JSVAL(obj);
    roots[1] = JSVAL_VOID;
    JS_PUSH_TEMP_ROOT(cx, 2, roots, &root);
    tag = JS_NewStringCopyZ(cx, name);
    if (tag) {
        roots[1] = STRING_TO_JSVAL(tag);
        ok = js_WellKnownSymbolId(cx, JS_WKS_TO_STRING_TAG, &id) &&
             OBJ_DEFINE_PROPERTY(cx, obj, id, roots[1], NULL, NULL,
                                 JSPROP_READONLY, NULL);
    }
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}

JSString *
js_SymbolToString(JSContext *cx, JSSymbol *symbol)
{
    return js_NewStringCopyN(cx, symbol->string.chars,
                             JSSTRING_LENGTH(&symbol->string), 0);
}

JSObject *
js_SymbolToObject(JSContext *cx, JSSymbol *symbol)
{
    JSObject *obj;
    JSTempValueRooter root;
    JS_PUSH_SINGLE_TEMP_ROOT(cx, STRING_TO_JSVAL(symbol), &root);
    obj = js_NewObject(cx, &js_SymbolClass, NULL, NULL);
    if (obj && !JS_SetReservedSlot(cx, obj, 0, STRING_TO_JSVAL(symbol)))
        obj = NULL;
    JS_POP_TEMP_ROOT(cx, &root);
    return obj;
}

JSClass js_SymbolClass = {
    "Symbol", JSCLASS_HAS_RESERVED_SLOTS(1) | JSCLASS_HAS_CACHED_PROTO(JSProto_Symbol),
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSBool
SymbolReceiver(JSContext *cx, jsval value, jsval *result)
{
    if (!JSVAL_IS_PRIMITIVE(value) &&
        OBJ_GET_CLASS(cx, JSVAL_TO_OBJECT(value)) == &js_SymbolClass &&
        !JS_GetReservedSlot(cx, JSVAL_TO_OBJECT(value), 0, &value))
        return JS_FALSE;
    if (!JSVAL_IS_SYMBOL(value)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_SYMBOL_REQUIRED);
        return JS_FALSE;
    }
    *result = value;
    return JS_TRUE;
}

static JSBool
symbol_constructor(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSString *description = NULL;
    JSSymbol *symbol;
    if (argc && !JSVAL_IS_VOID(argv[0])) {
        description = js_ValueToString(cx, argv[0]);
        if (!description)
            return JS_FALSE;
        argv[0] = STRING_TO_JSVAL(description);
    }
    symbol = js_NewSymbol(cx, description);
    if (!symbol)
        return JS_FALSE;
    *rval = STRING_TO_JSVAL(symbol);
    return JS_TRUE;
}

static JSBool
symbol_valueOf(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    return SymbolReceiver(cx, argv[-1], rval);
}

static JSBool
symbol_toString(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSString *str;
    if (!SymbolReceiver(cx, argv[-1], rval))
        return JS_FALSE;
    str = js_SymbolToString(cx, (JSSymbol *)JSVAL_TO_STRING(*rval));
    if (!str)
        return JS_FALSE;
    *rval = STRING_TO_JSVAL(str);
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(const void *)
RegistryKey(JSDHashTable *table, JSDHashEntryHdr *header)
{
    return &((SymbolRegistryEntry *)header)->key;
}

JS_STATIC_DLL_CALLBACK(JSDHashNumber)
RegistryHash(JSDHashTable *table, const void *key)
{
    const SymbolRegistryKey *text = (const SymbolRegistryKey *)key;
    JSDHashNumber hash = 0;
    size_t i;
    for (i = 0; i < text->length; ++i)
        hash = hash * 33 ^ text->chars[i];
    return hash;
}

JS_STATIC_DLL_CALLBACK(JSBool)
RegistryMatch(JSDHashTable *table, const JSDHashEntryHdr *header, const void *key)
{
    const SymbolRegistryKey *left = &((const SymbolRegistryEntry *)header)->key;
    const SymbolRegistryKey *right = (const SymbolRegistryKey *)key;
    return left->length == right->length &&
           !memcmp(left->chars, right->chars, left->length * sizeof(jschar));
}

static const JSDHashTableOps registryOps = {
    JS_DHashAllocTable, JS_DHashFreeTable, RegistryKey,
    RegistryHash, RegistryMatch, JS_DHashMoveEntryStub,
    JS_DHashClearEntryStub, JS_DHashFinalizeStub, NULL
};

static JSBool
symbol_for(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    struct JSSymbolState *state;
    JSString *key;
    JSSymbol *symbol;
    SymbolRegistryEntry *entry;
    SymbolRegistryKey lookup;
    JSTempValueRooter root;
    JSBool ok = JS_FALSE;
    key = js_ValueToString(cx, argv[0]);
    if (!key)
        return JS_FALSE;
    argv[0] = STRING_TO_JSVAL(key);
    state = SymbolState(cx);
    if (!state)
        return JS_FALSE;
    lookup.chars = JSSTRING_CHARS(key);
    lookup.length = JSSTRING_LENGTH(key);
    JS_LOCK_GC(cx->runtime);
    if (!state->registry)
        state->registry = JS_NewDHashTable(&registryOps, NULL,
                                           sizeof(SymbolRegistryEntry), 8);
    entry = state->registry
            ? (SymbolRegistryEntry *)JS_DHashTableOperate(state->registry, &lookup,
                                                          JS_DHASH_LOOKUP)
            : NULL;
    symbol = entry && JS_DHASH_ENTRY_IS_BUSY(&entry->hdr) ? entry->symbol : NULL;
    JS_UNLOCK_GC(cx->runtime);
    if (!entry) {
        JS_ReportOutOfMemory(cx);
        return JS_FALSE;
    }
    if (symbol) {
        *rval = STRING_TO_JSVAL(symbol);
        return JS_TRUE;
    }
    symbol = js_NewSymbol(cx, key);
    if (!symbol)
        return JS_FALSE;
    JS_PUSH_SINGLE_TEMP_ROOT(cx, STRING_TO_JSVAL(symbol), &root);
    lookup.chars = symbol->string.chars + 7;
    lookup.length = JSSTRING_LENGTH(&symbol->string) - 8;
    JS_LOCK_GC(cx->runtime);
    entry = (SymbolRegistryEntry *)JS_DHashTableOperate(state->registry, &lookup,
                                                       JS_DHASH_ADD);
    if (entry) {
        if (!entry->symbol) {
            entry->key = lookup;
            entry->symbol = symbol;
            symbol->registered = JS_TRUE;
        }
        *rval = STRING_TO_JSVAL(entry->symbol);
        ok = JS_TRUE;
    }
    JS_UNLOCK_GC(cx->runtime);
    JS_POP_TEMP_ROOT(cx, &root);
    if (!ok)
        JS_ReportOutOfMemory(cx);
    return ok;
}

static JSBool
symbol_keyFor(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSSymbol *symbol;
    JSString *key;
    if (!JSVAL_IS_SYMBOL(argv[0])) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_SYMBOL_REQUIRED);
        return JS_FALSE;
    }
    symbol = (JSSymbol *)JSVAL_TO_STRING(argv[0]);
    *rval = JSVAL_VOID;
    if (symbol->registered) {
        key = js_NewStringCopyN(cx, symbol->string.chars + 7,
                                JSSTRING_LENGTH(&symbol->string) - 8, 0);
        if (!key)
            return JS_FALSE;
        *rval = STRING_TO_JSVAL(key);
    }
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSDHashOperator)
MarkRegistry(JSDHashTable *table, JSDHashEntryHdr *header, uint32 number, void *argument)
{
    SymbolRegistryEntry *entry = (SymbolRegistryEntry *)header;
    GC_MARK((JSContext *)argument, entry->symbol, "registered Symbol");
    return JS_DHASH_NEXT;
}

void
js_MarkSymbolState(JSContext *cx)
{
    struct JSSymbolState *state = cx->runtime->symbolState;
    uintN i;
    if (!state)
        return;
    for (i = 0; i < JS_WKS_LIMIT; ++i) {
        if (state->wellKnown[i])
            GC_MARK(cx, state->wellKnown[i], "well-known Symbol");
    }
    if (state->registry)
        JS_DHashTableEnumerate(state->registry, MarkRegistry, cx);
}

JS_STATIC_DLL_CALLBACK(JSDHashOperator)
FinishRegistry(JSDHashTable *table, JSDHashEntryHdr *header, uint32 number, void *argument)
{
    SymbolRegistryEntry *entry = (SymbolRegistryEntry *)header;
    js_FinalizeStringRT((JSRuntime *)argument, &entry->symbol->string);
    return JS_DHASH_NEXT;
}

/* Registry identities survive intervals with no live contexts. Their only
 * storage is the owned display buffer, so release it at runtime destruction,
 * after the final collection and before the GC arenas are freed. */
void
js_FinishSymbolState(JSRuntime *rt)
{
    uintN i;
    if (rt->symbolState) {
        if (rt->symbolState->registry) {
            JS_DHashTableEnumerate(rt->symbolState->registry, FinishRegistry, rt);
            JS_DHashTableDestroy(rt->symbolState->registry);
        }
        for (i = 0; i < JS_WKS_LIMIT; ++i) {
            if (rt->symbolState->wellKnown[i])
                js_FinalizeStringRT(rt, &rt->symbolState->wellKnown[i]->string);
        }
        free(rt->symbolState);
        rt->symbolState = NULL;
    }
}

static JSBool
DefineSymbolMethod(JSContext *cx, JSObject *global, JSObject *proto)
{
    JSAtom *atom = js_Atomize(cx, "[Symbol.toPrimitive]", 20, 0);
    JSFunction *fun;
    JSTempValueRooter root;
    jsid id;
    JSBool ok;
    if (!atom)
        return JS_FALSE;
    fun = js_NewFunction(cx, NULL, symbol_valueOf, 1,
                         JSFUN_NO_CONSTRUCT | JSFUN_STRICT, global, atom);
    if (!fun)
        return JS_FALSE;
    JS_PUSH_TEMP_ROOT_OBJECT(cx, fun->object, &root);
    ok = js_WellKnownSymbolId(cx, JS_WKS_TO_PRIMITIVE, &id) &&
         OBJ_DEFINE_PROPERTY(cx, proto, id, OBJECT_TO_JSVAL(fun->object),
                              NULL, NULL, JSPROP_READONLY, NULL);
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}

JSObject *
js_InitSymbolClass(JSContext *cx, JSObject *global)
{
    JSObject *proto, *ctor;
    JSFunction *fun;
    JSSymbol *symbol;
    JSString *tag;
    JSTempValueRooter root;
    jsid id;
    uintN i;
    JSBool ok = JS_FALSE;
    proto = JS_InitClass(cx, global, NULL, &js_SymbolClass, symbol_constructor,
                         0, NULL, NULL, NULL, NULL);
    if (!proto)
        return NULL;
    JS_PUSH_TEMP_ROOT_OBJECT(cx, proto, &root);
    ctor = JS_GetConstructor(cx, proto);
    if (!ctor)
        goto out;
    fun = (JSFunction *)JS_GetPrivate(cx, ctor);
    fun->flags |= JSFUN_NO_CONSTRUCT;
    if (!JS_DefineFunction(cx, ctor, "for", symbol_for, 1, JSFUN_NO_CONSTRUCT) ||
        !JS_DefineFunction(cx, ctor, "keyFor", symbol_keyFor, 1, JSFUN_NO_CONSTRUCT) ||
        !JS_DefineFunction(cx, proto, "toString", symbol_toString, 0,
                           JSFUN_NO_CONSTRUCT | JSFUN_STRICT) ||
        !JS_DefineFunction(cx, proto, "valueOf", symbol_valueOf, 0,
                           JSFUN_NO_CONSTRUCT | JSFUN_STRICT))
        goto out;
    for (i = 0; i < JS_WKS_LIMIT; ++i) {
        symbol = js_GetWellKnownSymbol(cx, (JSWellKnownSymbol)i);
        if (!symbol ||
            !JS_DefineProperty(cx, ctor, wellKnownNames[i], STRING_TO_JSVAL(symbol),
                               NULL, NULL, JSPROP_READONLY | JSPROP_PERMANENT))
            goto out;
    }
    if (!DefineSymbolMethod(cx, global, proto))
        goto out;
    if (!js_WellKnownSymbolId(cx, JS_WKS_TO_STRING_TAG, &id))
        goto out;
    tag = JS_NewStringCopyZ(cx, "Symbol");
    if (!tag || !OBJ_DEFINE_PROPERTY(cx, proto, id, STRING_TO_JSVAL(tag),
                                      NULL, NULL, JSPROP_READONLY, NULL))
        goto out;
    ok = JS_TRUE;
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok ? proto : NULL;
}
