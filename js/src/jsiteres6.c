/* ES2015 Array and String iterators. Classic iterators remain in jsiter.c.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
#include <string.h>
#include "jsapi.h"
#include "jsarray.h"
#include "jsbinarydata.h"
#include "jsatom.h"
#include "jsbool.h"
#include "jscntxt.h"
#include "jsfun.h"
#include "jsgc.h"
#include "jsiteres6.h"
#include "jsinterp.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsrealm.h"
#include "jsstr.h"
#include "jssymbol.h"

static JSClass arrayIteratorClass = {
    "Array Iterator", JSCLASS_HAS_RESERVED_SLOTS(3),
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};
static JSClass stringIteratorClass = {
    "String Iterator", JSCLASS_HAS_RESERVED_SLOTS(2),
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};
static JSBool ArrayNext(JSContext *, JSObject *, uintN, jsval *, jsval *);
static JSBool StringNext(JSContext *, JSObject *, uintN, jsval *, jsval *);

JSObject *
js_BuiltinGlobal(JSContext *cx, jsval *argv)
{
    JSObject *global = JSVAL_TO_OBJECT(argv[-2]), *parent;
    while ((parent = OBJ_GET_PARENT(cx, global)) != NULL)
        global = parent;
    return global;
}

/* Intrinsics belong to the executing built-in, including legacy callers. */
JSObject *
js_BuiltinPrototype(JSContext *cx, JSObject *global, JSProtoKey key)
{
    JSObject *ctor = js_GetCachedClassObject(cx, global, key);
    jsval value;
    if (!ctor && !js_GetClassObject(cx, global, key, &ctor))
        return NULL;
    if (!ctor || !OBJ_GET_PROPERTY(cx, ctor,
              ATOM_TO_JSID(cx->runtime->atomState.classPrototypeAtom), &value))
        return NULL;
    if (JSVAL_IS_PRIMITIVE(value)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_BAD_PROTOTYPE, "iterator intrinsic");
        return NULL;
    }
    return JSVAL_TO_OBJECT(value);
}

/* ToObject wrappers and default result arrays belong to the method's realm. */
JSObject *
js_BuiltinToObject(JSContext *cx, JSObject *global, jsval value)
{
    JSProtoKey key;
    JSClass *clasp;
    JSObject *proto, *obj;
    if (!JSVAL_IS_PRIMITIVE(value)) return JSVAL_TO_OBJECT(value);
    if (JSVAL_IS_NULL(value) || JSVAL_IS_VOID(value)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_OBJECT_REQUIRED);
        return NULL;
    }
    if (JSVAL_IS_SYMBOL(value)) { key = JSProto_Symbol; clasp = &js_SymbolClass; }
    else if (JSVAL_IS_STRING(value)) { key = JSProto_String; clasp = &js_StringClass; }
    else if (JSVAL_IS_BOOLEAN(value)) { key = JSProto_Boolean; clasp = &js_BooleanClass; }
    else { key = JSProto_Number; clasp = &js_NumberClass; }
    proto = js_BuiltinPrototype(cx, global, key);
    if (!proto) return NULL;
    obj = js_NewObject(cx, clasp, proto, global);
    if (!obj) return NULL;
    if (key == JSProto_Symbol) {
        if (!JS_SetReservedSlot(cx, obj, 0, value)) return NULL;
    } else OBJ_SET_SLOT(cx, obj, JSSLOT_PRIVATE, value);
    return obj;
}

static JSBool
IteratorSelf(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    *rval = argv[-1];
    return JS_TRUE;
}

static JSBool
DefineIteratorMethod(JSContext *cx, JSObject *global, JSObject *proto,
                      JSNative native)
{
    JSAtom *name = js_Atomize(cx, "[Symbol.iterator]", 17, 0);
    JSFunction *fun;
    JSTempValueRooter root;
    jsid id;
    JSBool ok;
    if (!name)
        return JS_FALSE;
    fun = js_NewFunction(cx, NULL, native, 0, JSFUN_STRICT | JSFUN_NO_CONSTRUCT,
                         global, name);
    if (!fun)
        return JS_FALSE;
    JS_PUSH_TEMP_ROOT_OBJECT(cx, fun->object, &root);
    ok = js_WellKnownSymbolId(cx, JS_WKS_ITERATOR, &id) &&
         OBJ_DEFINE_PROPERTY(cx, proto, id, OBJECT_TO_JSVAL(fun->object),
                             NULL, NULL, 0, NULL);
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}

static JSObject *
IteratorPrototype(JSContext *cx, JSObject *global, JSRealmIntrinsic key)
{
    JSObject *proto, *parent;
    JSTempValueRooter root;
    JSBool ok;
    proto = js_GetCachedIntrinsic(cx, global, key);
    if (proto)
        return proto;
    parent = key == JS_INTRINSIC_ITERATOR_PROTO
             ? js_BuiltinPrototype(cx, global, JSProto_Object)
             : IteratorPrototype(cx, global, JS_INTRINSIC_ITERATOR_PROTO);
    if (!parent)
        return NULL;
    proto = js_NewObject(cx, &js_ObjectClass, parent, global);
    if (!proto)
        return NULL;
    JS_PUSH_TEMP_ROOT_OBJECT(cx, proto, &root);
    if (key == JS_INTRINSIC_ITERATOR_PROTO) {
        ok = DefineIteratorMethod(cx, global, proto, IteratorSelf);
    } else {
        ok = JS_DefineFunction(cx, proto, "next",
                    key == JS_INTRINSIC_ARRAY_ITERATOR_PROTO ? ArrayNext : StringNext,
                    0, JSFUN_STRICT | JSFUN_NO_CONSTRUCT) != NULL &&
             js_DefineBuiltinTag(cx, proto,
                    key == JS_INTRINSIC_ARRAY_ITERATOR_PROTO ? "Array Iterator" : "String Iterator");
    }
    if (ok)
        ok = js_CacheIntrinsic(cx, global, key, proto);
    JS_POP_TEMP_ROOT(cx, &root);
    return ok ? js_GetCachedIntrinsic(cx, global, key) : NULL;
}

JSBool
js_IteratorResult(JSContext *cx, JSObject *global, jsval value, JSBool done, jsval *rval)
{
    jsval roots[2];
    JSTempValueRooter root;
    JSObject *proto, *result;
    JSBool ok = JS_FALSE;
    roots[0] = value;
    roots[1] = JSVAL_VOID;
    JS_PUSH_TEMP_ROOT(cx, 2, roots, &root);
    proto = js_BuiltinPrototype(cx, global, JSProto_Object);
    result = proto ? js_NewObject(cx, &js_ObjectClass, proto, global) : NULL;
    if (result) {
        roots[1] = OBJECT_TO_JSVAL(result);
        ok = JS_DefineProperty(cx, result, "value", value, NULL, NULL, JSPROP_ENUMERATE) &&
             JS_DefineProperty(cx, result, "done", BOOLEAN_TO_JSVAL(done), NULL, NULL, JSPROP_ENUMERATE);
        if (ok)
            *rval = roots[1];
    }
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}

static JSObject *
IteratorReceiver(JSContext *cx, jsval value, JSClass *clasp)
{
    if (!JSVAL_IS_PRIMITIVE(value) && OBJ_GET_CLASS(cx, JSVAL_TO_OBJECT(value)) == clasp)
        return JSVAL_TO_OBJECT(value);
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                         JSMSG_INCOMPATIBLE_PROTO, clasp->name, "next", "receiver");
    return NULL;
}

typedef struct IteratorIdRoot {
    JSTempValueRooter root;
    jsid id;
} IteratorIdRoot;
JS_STATIC_DLL_CALLBACK(void)
MarkIteratorId(JSContext *cx, JSTempValueRooter *root)
{
    jsid id = ((IteratorIdRoot *)root)->id;
    if (JSID_IS_ATOM(id))
        js_MarkAtom(cx, JSID_TO_ATOM(id));
}

static JSBool
ArrayNext(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    jsval roots[5], kind;
    JSTempValueRooter root;
    IteratorIdRoot indexRoot;
    JSObject *iterator, *target, *global, *proto, *pair;
    jsdouble index, length;
    JSBool ok = JS_FALSE, done = JS_FALSE;
    uintN i;
    iterator = IteratorReceiver(cx, argv[-1], &arrayIteratorClass);
    if (!iterator)
        return JS_FALSE;
    for (i = 0; i < 5; ++i) roots[i] = JSVAL_VOID;
    roots[0] = argv[-1];
    JS_PUSH_TEMP_ROOT(cx, 5, roots, &root);
    indexRoot.id = INT_TO_JSID(0);
    JS_PUSH_TEMP_ROOT_MARKER(cx, MarkIteratorId, &indexRoot.root);
    global = js_BuiltinGlobal(cx, argv);
    if (!JS_GetReservedSlot(cx, iterator, 0, &roots[1])) goto out;
    if (JSVAL_IS_VOID(roots[1])) { done = JS_TRUE; goto result; }
    target = JSVAL_TO_OBJECT(roots[1]);
    if (!JS_GetReservedSlot(cx, iterator, 1, &roots[2]) ||
        !js_ValueToNumber(cx, roots[2], &index) ||
        !JS_GetReservedSlot(cx, iterator, 2, &kind) ||
        !(js_IsTypedArray(cx, target)
            ? js_TypedArrayLength(cx, target, &length)
            : js_ArrayLikeLength(cx, target, &length))) goto out;
    if (index >= length) {
        if (!JS_SetReservedSlot(cx, iterator, 0, JSVAL_VOID)) goto out;
        done = JS_TRUE;
        goto result;
    }
    if (!js_NewNumberValue(cx, index + 1, &roots[3]) ||
        !JS_SetReservedSlot(cx, iterator, 1, roots[3])) goto out;
    roots[3] = JSVAL_VOID;
    if (kind == INT_TO_JSVAL(0)) {
        roots[4] = roots[2];
    } else {
        if (!js_ArrayLikeIndex(cx, index, &indexRoot.id) ||
            !OBJ_GET_PROPERTY(cx, target, indexRoot.id, &roots[3])) goto out;
        if (kind == INT_TO_JSVAL(1)) {
            roots[4] = roots[3];
        } else {
            proto = js_BuiltinPrototype(cx, global, JSProto_Array);
            if (!proto) goto out;
            pair = js_NewArrayObjectWithProto(cx, 0, NULL, proto, global);
            if (!pair) goto out;
            roots[4] = OBJECT_TO_JSVAL(pair);
            if (!js_CreateDataPropertyOrThrow(cx, pair, INT_TO_JSID(0), roots[2]) ||
                !js_CreateDataPropertyOrThrow(cx, pair, INT_TO_JSID(1), roots[3])) goto out;
        }
    }
  result:
    ok = js_IteratorResult(cx, global, roots[4], done, rval);
  out:
    JS_POP_TEMP_ROOT(cx, &indexRoot.root);
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}

static JSBool
StringNext(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    jsval roots[4];
    JSTempValueRooter root;
    JSObject *iterator;
    JSString *str, *part;
    jsdouble number;
    size_t index, length, count = 1;
    const jschar *chars;
    JSBool ok = JS_FALSE, done = JS_FALSE;
    iterator = IteratorReceiver(cx, argv[-1], &stringIteratorClass);
    if (!iterator) return JS_FALSE;
    roots[0] = argv[-1]; roots[1] = roots[2] = roots[3] = JSVAL_VOID;
    JS_PUSH_TEMP_ROOT(cx, 4, roots, &root);
    if (!JS_GetReservedSlot(cx, iterator, 0, &roots[1])) goto out;
    if (JSVAL_IS_VOID(roots[1])) { done = JS_TRUE; goto result; }
    str = JSVAL_TO_STRING(roots[1]);
    if (!JS_GetReservedSlot(cx, iterator, 1, &roots[2]) ||
        !js_ValueToNumber(cx, roots[2], &number)) goto out;
    length = JSSTRING_LENGTH(str);
    index = (size_t)number;
    if (index >= length) {
        if (!JS_SetReservedSlot(cx, iterator, 0, JSVAL_VOID)) goto out;
        done = JS_TRUE;
        goto result;
    }
    chars = JSSTRING_CHARS(str);
    if (chars[index] >= 0xd800 && chars[index] <= 0xdbff && index + 1 < length &&
        chars[index + 1] >= 0xdc00 && chars[index + 1] <= 0xdfff)
        count = 2;
    part = js_NewDependentString(cx, str, index, count, 0);
    if (!part) goto out;
    roots[3] = STRING_TO_JSVAL(part);
    if (!js_NewNumberValue(cx, (jsdouble)(index + count), &roots[2]) ||
        !JS_SetReservedSlot(cx, iterator, 1, roots[2])) goto out;
  result:
    ok = js_IteratorResult(cx, js_BuiltinGlobal(cx, argv), roots[3], done, rval);
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}

static JSBool
NewIterator(JSContext *cx, jsval *argv, jsval target, JSRealmIntrinsic key,
             uintN kind, jsval *rval)
{
    JSTempValueRooter root;
    jsval roots[2];
    JSObject *global = js_BuiltinGlobal(cx, argv), *proto, *iterator;
    JSBool ok = JS_FALSE;
    roots[0] = target;
    roots[1] = JSVAL_VOID;
    JS_PUSH_TEMP_ROOT(cx, 2, roots, &root);
    proto = IteratorPrototype(cx, global, key);
    if (proto) {
        iterator = js_NewObject(cx, key == JS_INTRINSIC_ARRAY_ITERATOR_PROTO
                                   ? &arrayIteratorClass : &stringIteratorClass,
                                proto, global);
        if (iterator) {
            roots[1] = *rval = OBJECT_TO_JSVAL(iterator);
            ok = JS_SetReservedSlot(cx, iterator, 0, target) &&
                 JS_SetReservedSlot(cx, iterator, 1, JSVAL_ZERO) &&
                 (key != JS_INTRINSIC_ARRAY_ITERATOR_PROTO ||
                  JS_SetReservedSlot(cx, iterator, 2, INT_TO_JSVAL(kind)));
        }
    }
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}

JSBool
js_CreateArrayIterator(JSContext *cx, jsval *argv, jsval *rval, uintN kind)
{
    JSObject *target = js_ValueToNonNullObject(cx, argv[-1]);
    if (!target) return JS_FALSE;
    return NewIterator(cx, argv, OBJECT_TO_JSVAL(target),
                       JS_INTRINSIC_ARRAY_ITERATOR_PROTO, kind, rval);
}
static JSBool ArrayKeys(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{ return js_CreateArrayIterator(cx, argv, rval, 0); }
static JSBool ArrayValues(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{ return js_CreateArrayIterator(cx, argv, rval, 1); }
static JSBool ArrayEntries(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{ return js_CreateArrayIterator(cx, argv, rval, 2); }
static JSBool
StringIterator(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSString *str;
    if (JSVAL_IS_NULL(argv[-1]) || JSVAL_IS_VOID(argv[-1])) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_CANT_CONVERT_TO, "iterator receiver", "object");
        return JS_FALSE;
    }
    str = js_ValueToString(cx, argv[-1]);
    return str && NewIterator(cx, argv, STRING_TO_JSVAL(str),
                              JS_INTRINSIC_STRING_ITERATOR_PROTO, 0, rval);
}

/* Arguments uses the intrinsic values function even after user replacement,
 * and can request it before the lazy Array class has been initialized. */
static JSObject *
ArrayValuesFunction(JSContext *cx, JSObject *global)
{
    JSObject *object = js_GetCachedIntrinsic(cx, global, JS_INTRINSIC_ARRAY_VALUES);
    JSAtom *name;
    JSFunction *fun;
    JSTempValueRooter root;
    JSBool ok;
    if (object)
        return object;
    name = js_Atomize(cx, "values", 6, 0);
    if (!name)
        return NULL;
    fun = js_NewFunction(cx, NULL, ArrayValues, 0, JSFUN_STRICT | JSFUN_NO_CONSTRUCT,
                         global, name);
    if (!fun)
        return NULL;
    JS_PUSH_TEMP_ROOT_OBJECT(cx, fun->object, &root);
    ok = js_CacheIntrinsic(cx, global, JS_INTRINSIC_ARRAY_VALUES, fun->object);
    JS_POP_TEMP_ROOT(cx, &root);
    return ok ? js_GetCachedIntrinsic(cx, global, JS_INTRINSIC_ARRAY_VALUES) : NULL;
}

JSBool
js_InitArgumentsIterator(JSContext *cx, JSObject *global, JSObject *args)
{
    JSObject *values = ArrayValuesFunction(cx, global);
    jsid id;
    return values && js_WellKnownSymbolId(cx, JS_WKS_ITERATOR, &id) &&
           OBJ_DEFINE_PROPERTY(cx, args, id, OBJECT_TO_JSVAL(values),
                               NULL, NULL, 0, NULL);
}

JSBool
js_InitArrayIteratorMethods(JSContext *cx, JSObject *global, JSObject *proto)
{
    JSObject *values;
    jsid id;
    uintN flags = JSFUN_STRICT | JSFUN_NO_CONSTRUCT;
    if (!JS_DefineFunction(cx, proto, "keys", ArrayKeys, 0, flags) ||
        !JS_DefineFunction(cx, proto, "entries", ArrayEntries, 0, flags))
        return JS_FALSE;
    values = ArrayValuesFunction(cx, global);
    return values && JS_DefineProperty(cx, proto, "values", OBJECT_TO_JSVAL(values),
                                      NULL, NULL, 0) &&
           js_WellKnownSymbolId(cx, JS_WKS_ITERATOR, &id) &&
           OBJ_DEFINE_PROPERTY(cx, proto, id, OBJECT_TO_JSVAL(values),
                               NULL, NULL, 0, NULL);
}

JSBool
js_InitStringIteratorMethod(JSContext *cx, JSObject *global, JSObject *proto)
{
    return DefineIteratorMethod(cx, global, proto, StringIterator);
}

JSObject *
js_GetIteratorPrototype(JSContext *cx, JSObject *global)
{
    return IteratorPrototype(cx, global, JS_INTRINSIC_ITERATOR_PROTO);
}

/* ES2015 IteratorClose on a throw completion. A return getter failure takes
 * precedence; after a successful GetMethod the original throw takes precedence
 * over the return call's result or exception (7.4.6). */
void
js_IteratorCloseThrow(JSContext *cx, JSObject *iterator)
{
    jsval roots[3];
    JSTempValueRooter root;
    JSBool ok;
    if (!JS_IsExceptionPending(cx))
        return; /* Do not call user code following an uncatchable failure. */
    roots[0] = roots[1] = roots[2] = JSVAL_VOID;
    JS_PUSH_TEMP_ROOT(cx, 3, roots, &root);
    if (!JS_GetPendingException(cx, &roots[0]))
        goto out;
    JS_ClearPendingException(cx);
    ok = JS_GetProperty(cx, iterator, "return", &roots[1]);
    if (!ok)
        goto out;
    if (!JSVAL_IS_VOID(roots[1]) && !JSVAL_IS_NULL(roots[1])) {
        if (!js_IsCallable(cx, roots[1])) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                 JSMSG_NOT_FUNCTION, "iterator return");
            goto out;
        }
        js_InternalCall(cx, iterator, roots[1], 0, NULL, &roots[2]);
    }
    JS_SetPendingException(cx, roots[0]);
  out:
    JS_POP_TEMP_ROOT(cx, &root);
}

/* Private for-of state: underlying iterator, retained value, close eligibility.
 * Reserved slots trace values across callbacks without exposing mutable state. */
static JSClass forOfClass = {
    "For Of State", JSCLASS_HAS_RESERVED_SLOTS(4),
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

JSObject *
js_ForOfStart(JSContext *cx, jsval input)
{
    jsval roots[4];
    JSTempValueRooter root;
    JSObject *source, *state = NULL;
    jsid id;
    uintN i;
    for (i = 0; i < 4; ++i) roots[i] = JSVAL_VOID;
    roots[0] = input;
    JS_PUSH_TEMP_ROOT(cx, 4, roots, &root);
    source = js_ValueToNonNullObject(cx, input);
    if (!source) goto out;
    roots[1] = OBJECT_TO_JSVAL(source);
    if (!js_WellKnownSymbolId(cx, JS_WKS_ITERATOR, &id) ||
        !js_GetPropertyValue(cx, source, input, id, &roots[2])) goto out;
    if (!js_IsCallable(cx, roots[2])) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_NOT_FUNCTION, "Symbol.iterator");
        goto out;
    }
    if (!js_InternalInvokeValue(cx, input, roots[2], 0, 0, NULL, &roots[3])) goto out;
    if (JSVAL_IS_PRIMITIVE(roots[3])) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_BAD_ITERATOR_RETURN, "iterator", "Symbol.iterator");
        goto out;
    }
    state = js_NewObject(cx, &forOfClass, NULL, NULL);
    if (!state) goto out;
    JS_SetReservedSlot(cx, state, 0, roots[3]);
    JS_SetReservedSlot(cx, state, 1, JSVAL_VOID);
    JS_SetReservedSlot(cx, state, 2, JSVAL_FALSE);
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return state;
}

JSBool
js_ForOfNext(JSContext *cx, JSObject *state, JSBool *more)
{
    jsval roots[4];
    JSTempValueRooter root;
    JSObject *iterator, *result;
    JSBool done, ok = JS_FALSE;
    uintN i;
    JS_ASSERT(OBJ_GET_CLASS(cx, state) == &forOfClass);
    for (i = 0; i < 4; ++i) roots[i] = JSVAL_VOID;
    JS_PUSH_TEMP_ROOT(cx, 4, roots, &root);
    JS_GetReservedSlot(cx, state, 0, &roots[0]);
    iterator = JSVAL_TO_OBJECT(roots[0]);
    JS_SetReservedSlot(cx, state, 2, JSVAL_FALSE);
    JS_SetReservedSlot(cx, state, 1, JSVAL_VOID);
    if (!JS_GetProperty(cx, iterator, "next", &roots[1])) goto out;
    if (!js_IsCallable(cx, roots[1])) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_NOT_FUNCTION, "iterator next");
        goto out;
    }
    if (!js_InternalCall(cx, iterator, roots[1], 0, NULL, &roots[2])) goto out;
    if (JSVAL_IS_PRIMITIVE(roots[2])) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_BAD_ITERATOR_RETURN, "iterator", "next");
        goto out;
    }
    result = JSVAL_TO_OBJECT(roots[2]);
    if (!JS_GetProperty(cx, result, "done", &roots[3]) ||
        !JS_ValueToBoolean(cx, roots[3], &done)) goto out;
    *more = !done;
    if (!done) {
        if (!JS_GetProperty(cx, result, "value", &roots[3])) goto out;
        JS_SetReservedSlot(cx, state, 1, roots[3]);
        JS_SetReservedSlot(cx, state, 2, JSVAL_TRUE);
    }
    ok = JS_TRUE;
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}

/* Array patterns retain exhaustion independently of whether closing is due.
 * Failures from next/done/value finish the record without calling return. */
JSObject *
js_PatternStart(JSContext *cx, jsval input)
{
    JSObject *state = js_ForOfStart(cx, input);
    if (state) {
        JS_SetReservedSlot(cx, state, 2, JSVAL_TRUE);
        JS_SetReservedSlot(cx, state, 3, JSVAL_FALSE);
    }
    return state;
}

JSBool
js_PatternStep(JSContext *cx, JSObject *state, JSBool readValue, jsval *value)
{
    jsval roots[4], exhausted;
    JSTempValueRooter root;
    JSObject *iterator, *result;
    JSBool done, ok = JS_FALSE;
    uintN i;
    *value = JSVAL_VOID;
    JS_GetReservedSlot(cx, state, 3, &exhausted);
    if (exhausted == JSVAL_TRUE) return JS_TRUE;
    for (i=0; i<4; i++) roots[i] = JSVAL_VOID;
    JS_PUSH_TEMP_ROOT(cx, 4, roots, &root);
    JS_GetReservedSlot(cx, state, 0, &roots[0]);
    iterator = JSVAL_TO_OBJECT(roots[0]);
    JS_SetReservedSlot(cx, state, 2, JSVAL_FALSE);
    JS_SetReservedSlot(cx, state, 3, JSVAL_TRUE);
    if (!JS_GetProperty(cx, iterator, "next", &roots[1])) goto out;
    if (!js_IsCallable(cx, roots[1])) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_NOT_FUNCTION, "iterator next");
        goto out;
    }
    if (!js_InternalCall(cx, iterator, roots[1], 0, NULL, &roots[2])) goto out;
    if (JSVAL_IS_PRIMITIVE(roots[2])) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_BAD_ITERATOR_RETURN, "iterator", "next");
        goto out;
    }
    result = JSVAL_TO_OBJECT(roots[2]);
    if (!JS_GetProperty(cx, result, "done", &roots[3]) ||
        !JS_ValueToBoolean(cx, roots[3], &done)) goto out;
    if (!done) {
        if (readValue && !JS_GetProperty(cx, result, "value", &roots[3])) goto out;
        if (readValue) *value = roots[3];
        JS_SetReservedSlot(cx, state, 2, JSVAL_TRUE);
        JS_SetReservedSlot(cx, state, 3, JSVAL_FALSE);
    }
    ok = JS_TRUE;
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}

JSBool
js_PatternRest(JSContext *cx, JSObject *state, jsval *value)
{
    jsval roots[2], exhausted;
    JSTempValueRooter root;
    IteratorIdRoot indexRoot;
    JSObject *array;
    jsdouble index = 0;
    JSBool ok = JS_FALSE;
    roots[0] = roots[1] = JSVAL_VOID;
    JS_PUSH_TEMP_ROOT(cx, 2, roots, &root);
    indexRoot.id = INT_TO_JSID(0);
    JS_PUSH_TEMP_ROOT_MARKER(cx, MarkIteratorId, &indexRoot.root);
    array = js_NewArrayObject(cx, 0, NULL);
    if (!array) goto out;
    roots[0] = OBJECT_TO_JSVAL(array);
    for (;;) {
        if (!js_PatternStep(cx, state, JS_TRUE, &roots[1])) goto out;
        JS_GetReservedSlot(cx, state, 3, &exhausted);
        if (exhausted == JSVAL_TRUE) break;
        if (!js_ArrayLikeIndex(cx, index, &indexRoot.id) ||
            !js_CreateDataPropertyOrThrow(cx, array, indexRoot.id, roots[1])) goto out;
        ++index;
    }
    *value = roots[0];
    ok = JS_TRUE;
  out:
    JS_POP_TEMP_ROOT(cx, &indexRoot.root);
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}

JSBool
js_ForOfClose(JSContext *cx, JSObject *state, JSBool throwing)
{
    jsval roots[3], active;
    JSTempValueRooter root;
    JSObject *iterator;
    JSBool ok = JS_TRUE, generatorReturn;
    JS_ASSERT(OBJ_GET_CLASS(cx, state) == &forOfClass);
    JS_GetReservedSlot(cx, state, 2, &active);
    if (active != JSVAL_TRUE) return JS_TRUE;
    roots[0] = roots[1] = roots[2] = JSVAL_VOID;
    JS_PUSH_TEMP_ROOT(cx, 3, roots, &root);
    JS_GetReservedSlot(cx, state, 0, &roots[0]);
    iterator = JSVAL_TO_OBJECT(roots[0]);
    /* Closing may reenter. Never invoke return a second time. */
    JS_SetReservedSlot(cx, state, 2, JSVAL_FALSE);
    JS_SetReservedSlot(cx, state, 1, JSVAL_VOID);
    generatorReturn = throwing && cx->exception == JSVAL_ARETURN;
    if (throwing && !generatorReturn) {
        js_IteratorCloseThrow(cx, iterator);
        goto out;
    }
    if (generatorReturn) JS_ClearPendingException(cx);
    ok = JS_GetProperty(cx, iterator, "return", &roots[1]);
    if (!ok) goto out;
    if (!JSVAL_IS_VOID(roots[1]) && !JSVAL_IS_NULL(roots[1])) {
        if (!js_IsCallable(cx, roots[1])) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                 JSMSG_NOT_FUNCTION, "iterator return");
            ok = JS_FALSE;
            goto out;
        }
        ok = js_InternalCall(cx, iterator, roots[1], 0, NULL, &roots[2]);
        if (!ok) goto out;
        if (JSVAL_IS_PRIMITIVE(roots[2])) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                 JSMSG_BAD_ITERATOR_RETURN, "iterator", "return");
            ok = JS_FALSE;
            goto out;
        }
    }
    if (generatorReturn) JS_SetPendingException(cx, JSVAL_ARETURN);
  out:
    JS_SetReservedSlot(cx, state, 0, JSVAL_VOID);
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}
