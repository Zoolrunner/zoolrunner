/* ES2015 per-realm template registry. MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "jsapi.h"
#include "jsarray.h"
#include "jsatom.h"
#include "jscntxt.h"
#include "jsgc.h"
#include "jsiteres6.h"
#include "jsobj.h"
#include "jsrealm.h"
#include "jsscan.h"
#include "jsstr.h"
#include "jstemplate.h"

typedef struct TemplateAtomRoot {
    JSTempValueRooter root;
    JSAtom *atom;
} TemplateAtomRoot;

JS_STATIC_DLL_CALLBACK(void)
MarkTemplateAtom(JSContext *cx, JSTempValueRooter *root)
{
    JSAtom *atom = ((TemplateAtomRoot *)root)->atom;
    if (atom) js_MarkAtom(cx, atom);
}

static JSBool
ReadWord(const jschar **cursor, const jschar *end, uint32 *value)
{
    const jschar *p = *cursor;
    if (end - p < 2) return JS_FALSE;
    *value = ((uint32)p[0] << 16) | p[1];
    *cursor = p + 2;
    return JS_TRUE;
}

static JSBool
ReadPart(const jschar **cursor, const jschar *end,
         const jschar **chars, uint32 *length)
{
    if (!ReadWord(cursor, end, length) || (size_t)(end - *cursor) < *length)
        return JS_FALSE;
    *chars = *cursor;
    *cursor += *length;
    return JS_TRUE;
}

static void
AppendWord(JSStringBuffer *buffer, uint32 value)
{
    js_AppendChar(buffer, (jschar)(value >> 16));
    js_AppendChar(buffer, (jschar)value);
}

JSBool
js_GetTemplateObject(JSContext *cx, JSString *record, jsval *rval)
{
    jsval roots[7];
    TemplateAtomRoot keyRoot;
    JSTempValueRooter root;
    JSStringBuffer keyBuffer;
    JSObject *global, *parent, *registry, *proto;
    JSAtom *key;
    JSString *str;
    const jschar *cursor, *end, *raw, *cooked;
    uint32 count, rawLength, cookedLength, i, j;
    JSBool ok = JS_FALSE;
    uintN attrs = JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT;

    for (i = 0; i < 7; ++i) roots[i] = JSVAL_VOID;
    roots[0] = STRING_TO_JSVAL(record);
    JS_PUSH_TEMP_ROOT(cx, 7, roots, &root);
    keyRoot.atom = NULL;
    JS_PUSH_TEMP_ROOT_MARKER(cx, MarkTemplateAtom, &keyRoot.root);
    js_InitStringBuffer(&keyBuffer);
    cursor = JSSTRING_CHARS(record);
    end = cursor + JSSTRING_LENGTH(record);
    if (!ReadWord(&cursor, end, &count) || !count || count > 65535U)
        goto malformed;
    AppendWord(&keyBuffer, count);
    for (i = 0; i < count; ++i) {
        if (!ReadPart(&cursor, end, &raw, &rawLength) ||
            !ReadPart(&cursor, end, &cooked, &cookedLength))
            goto malformed;
        AppendWord(&keyBuffer, rawLength);
        for (j = 0; j < rawLength; ++j)
            js_AppendChar(&keyBuffer, raw[j]);
    }
    if (cursor != end) goto malformed;
    if (!STRING_BUFFER_OK(&keyBuffer)) {
        JS_ReportOutOfMemory(cx);
        goto out;
    }
    str = JS_NewUCStringCopyN(cx, keyBuffer.base, STRING_BUFFER_OFFSET(&keyBuffer));
    if (!str) goto out;
    roots[3] = STRING_TO_JSVAL(str);
    key = keyRoot.atom = js_AtomizeString(cx, str, 0);
    if (!key) goto out;

    global = cx->fp->scopeChain;
    while ((parent = OBJ_GET_PARENT(cx, global)) != NULL)
        global = parent;
    roots[1] = OBJECT_TO_JSVAL(global);
    registry = js_GetCachedIntrinsic(cx, global, JS_INTRINSIC_TEMPLATE_REGISTRY);
    if (!registry) {
        proto = js_BuiltinPrototype(cx, global, JSProto_Object);
        if (!proto) goto out;
        registry = js_NewObject(cx, &js_ObjectClass, proto, global);
        if (!registry) goto out;
        roots[2] = OBJECT_TO_JSVAL(registry);
        OBJ_SET_PROTO(cx, registry, NULL);
        if (!js_CacheIntrinsic(cx, global, JS_INTRINSIC_TEMPLATE_REGISTRY, registry))
            goto out;
        /* A native allocation hook may have initialized the cache first. */
        registry = js_GetCachedIntrinsic(cx, global, JS_INTRINSIC_TEMPLATE_REGISTRY);
    }
    roots[2] = OBJECT_TO_JSVAL(registry);
    if (!OBJ_GET_PROPERTY(cx, registry, ATOM_TO_JSID(key), rval)) goto out;
    if (!JSVAL_IS_VOID(*rval)) {
        ok = JS_TRUE;
        goto out;
    }
    proto = js_BuiltinPrototype(cx, global, JSProto_Array);
    if (!proto) goto out;
    parent = js_NewArrayObjectWithProto(cx, count, NULL, proto, global);
    if (!parent) goto out;
    roots[4] = OBJECT_TO_JSVAL(parent);
    parent = js_NewArrayObjectWithProto(cx, count, NULL, proto, global);
    if (!parent) goto out;
    roots[5] = OBJECT_TO_JSVAL(parent);
    cursor = JSSTRING_CHARS(record) + 2;
    for (i = 0; i < count; ++i) {
        if (!ReadPart(&cursor, end, &raw, &rawLength) ||
            !ReadPart(&cursor, end, &cooked, &cookedLength))
            goto malformed;
        str = JS_NewUCStringCopyN(cx, raw, rawLength);
        if (!str) goto out;
        roots[6] = STRING_TO_JSVAL(str);
        if (!JS_DefineElement(cx, JSVAL_TO_OBJECT(roots[4]), i,
                              roots[6], NULL, NULL, attrs))
            goto out;
        str = JS_NewUCStringCopyN(cx, cooked, cookedLength);
        if (!str) goto out;
        roots[6] = STRING_TO_JSVAL(str);
        if (!JS_DefineElement(cx, JSVAL_TO_OBJECT(roots[5]), i,
                              roots[6], NULL, NULL, attrs))
            goto out;
    }
    if (!JS_DefineProperty(cx, JSVAL_TO_OBJECT(roots[5]), "raw", roots[4],
                           NULL, NULL, JSPROP_READONLY | JSPROP_PERMANENT) ||
        !js_FreezeObject(cx, JSVAL_TO_OBJECT(roots[4])) ||
        !js_FreezeObject(cx, JSVAL_TO_OBJECT(roots[5])))
        goto out;
    /* Reentrancy may have populated this raw-string entry during allocation. */
    if (!OBJ_GET_PROPERTY(cx, registry, ATOM_TO_JSID(key), rval)) goto out;
    if (JSVAL_IS_VOID(*rval)) {
        if (!OBJ_DEFINE_PROPERTY(cx, registry, ATOM_TO_JSID(key), roots[5],
                                  NULL, NULL, JSPROP_READONLY | JSPROP_PERMANENT, NULL))
            goto out;
        *rval = roots[5];
    }
    ok = JS_TRUE;
    goto out;
malformed:
    JS_ReportError(cx, "invalid template literal record");
out:
    js_FinishStringBuffer(&keyBuffer);
    JS_POP_TEMP_ROOT(cx, &keyRoot.root);
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}
