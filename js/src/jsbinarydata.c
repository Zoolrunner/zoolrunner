/* ES2015 ArrayBuffer and DataView for the classic embedding API.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
#include <stdlib.h>
#include <string.h>
#include "jsapi.h"
#include "jsarray.h"
#include "jsproxy.h"
#include "jsreflect.h"
#include "jsscript.h"
#include "jsatom.h"
#include "jsbinarydata.h"
#include "jsbool.h"
#include "jscntxt.h"
#include "jsfun.h"
#include "jsgc.h"
#include "jsinterp.h"
#include "jsiteres6.h"
#include "jsnum.h"
#include "jsmath.h"
#include "jsobj.h"
#include "jsprf.h"
#include "jsrealm.h"
#include "jsstr.h"
#include "jssymbol.h"

JS_STATIC_ASSERT(sizeof(float) == 4);
JS_STATIC_ASSERT(sizeof(jsdouble) == 8);

typedef struct BufferData {
    size_t length;
    unsigned char *bytes;
    JSBool detached;
} BufferData;
typedef struct ViewData {
    size_t offset, length;
} ViewData;

static void BufferFinalize(JSContext *, JSObject *);
static void ViewFinalize(JSContext *, JSObject *);
JSClass js_ArrayBufferClass = {
    "ArrayBuffer", JSCLASS_HAS_PRIVATE | JSCLASS_HAS_CACHED_PROTO(JSProto_ArrayBuffer),
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, BufferFinalize,
    JSCLASS_NO_OPTIONAL_MEMBERS
};
JSClass js_DataViewClass = {
    "DataView", JSCLASS_HAS_PRIVATE | JSCLASS_HAS_RESERVED_SLOTS(1) |
                JSCLASS_HAS_CACHED_PROTO(JSProto_DataView),
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, ViewFinalize,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

static BufferData *
GetBuffer(JSContext *cx, jsval value)
{
    JSObject *obj;
    jsval slot;
    if (JSVAL_IS_PRIMITIVE(value)) return NULL;
    obj = JSVAL_TO_OBJECT(value);
    if (OBJ_GET_CLASS(cx, obj) != &js_ArrayBufferClass) return NULL;
    slot = OBJ_GET_SLOT(cx, obj, JSSLOT_PRIVATE);
    return JSVAL_IS_VOID(slot) ? NULL : (BufferData *)JSVAL_TO_PRIVATE(slot);
}
static ViewData *
GetView(JSContext *cx, jsval value)
{
    JSObject *obj;
    jsval slot;
    if (JSVAL_IS_PRIMITIVE(value)) return NULL;
    obj = JSVAL_TO_OBJECT(value);
    if (OBJ_GET_CLASS(cx, obj) != &js_DataViewClass) return NULL;
    slot = OBJ_GET_SLOT(cx, obj, JSSLOT_PRIVATE);
    return JSVAL_IS_VOID(slot) ? NULL : (ViewData *)JSVAL_TO_PRIVATE(slot);
}
static JSBool
BinaryTypeError(JSContext *cx)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_BAD_DESCRIPTOR);
    return JS_FALSE;
}
static JSBool
BinaryRangeError(JSContext *cx)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_BAD_ARRAY_LENGTH);
    return JS_FALSE;
}
static void
BufferFinalize(JSContext *cx, JSObject *obj)
{
    BufferData *data = GetBuffer(cx, OBJECT_TO_JSVAL(obj));
    if (data) { free(data->bytes); JS_free(cx, data); }
}
static void
ViewFinalize(JSContext *cx, JSObject *obj)
{
    ViewData *data = GetView(cx, OBJECT_TO_JSVAL(obj));
    if (data) JS_free(cx, data);
}

/* Internal host hook; detachment never leaves a live view with a raw pointer. */
JSBool
js_DetachArrayBuffer(JSContext *cx, JSObject *obj)
{
    BufferData *data = GetBuffer(cx, OBJECT_TO_JSVAL(obj));
    if (!data) return BinaryTypeError(cx);
    free(data->bytes); data->bytes = NULL; data->length = 0; data->detached = JS_TRUE;
    return JS_TRUE;
}

static JSObject *
BinaryCreate(JSContext *cx, jsval *argv, JSClass *clasp, JSProtoKey key)
{
    JSObject *target = cx->fp->newTarget ? cx->fp->newTarget : JSVAL_TO_OBJECT(argv[-2]);
    JSObject *global = js_BuiltinGlobal(cx, argv), *proto, *obj = NULL;
    JSTempValueRooter root;
    JS_PUSH_SINGLE_TEMP_ROOT(cx, JSVAL_VOID, &root);
    if (!OBJ_GET_PROPERTY(cx, target, ATOM_TO_JSID(cx->runtime->atomState.classPrototypeAtom), &root.u.value)) goto out;
    if (JSVAL_IS_PRIMITIVE(root.u.value)) {
        global = js_ConstructorGlobal(cx, target);
        if (!global) goto out;
        proto = js_BuiltinPrototype(cx, global, key);
        if (!proto) goto out;
        root.u.value = OBJECT_TO_JSVAL(proto);
    } else proto = JSVAL_TO_OBJECT(root.u.value);
    obj = js_NewObject(cx, clasp, proto, global);
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return obj;
}

JSBool
js_ArrayBufferConstructor(JSContext *cx, JSObject *ignored, uintN argc, jsval *argv, jsval *rval)
{
    jsdouble number;
    JSObject *obj;
    BufferData *data;
    JSTempValueRooter root;
    JSBool ok = JS_FALSE;
    if (!(cx->fp->flags & JSFRAME_CONSTRUCTING)) return BinaryTypeError(cx);
    if (!js_ValueToNumber(cx, argv[0], &number)) return JS_FALSE;
    if (JSDOUBLE_IS_NaN(number) || number < 0 || number > 9007199254740991.0 ||
        js_DoubleToInteger(number) != number) return BinaryRangeError(cx);
    obj = BinaryCreate(cx, argv, &js_ArrayBufferClass, JSProto_ArrayBuffer);
    if (!obj) return JS_FALSE;
    JS_PUSH_TEMP_ROOT_OBJECT(cx, obj, &root);
    /* A uniform implementation resource limit avoids address/size truncation
     * on legacy 32-bit hosts. Failure to create a Data Block is a RangeError. */
    if (number > 2147483647.0) { BinaryRangeError(cx); goto out; }
    data = (BufferData *)JS_malloc(cx, sizeof(BufferData));
    if (!data) goto out;
    data->length = (size_t)number; data->detached = JS_FALSE;
    data->bytes = (unsigned char *)calloc(data->length ? data->length : 1, 1);
    if (!data->bytes) { JS_free(cx, data); BinaryRangeError(cx); goto out; }
    OBJ_SET_SLOT(cx, obj, JSSLOT_PRIVATE, PRIVATE_TO_JSVAL(data));
    *rval = OBJECT_TO_JSVAL(obj); ok = JS_TRUE;
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}

JSBool
js_DataViewConstructor(JSContext *cx, JSObject *ignored, uintN argc, jsval *argv, jsval *rval)
{
    BufferData *buffer;
    ViewData *view;
    JSObject *obj;
    jsdouble offset, length, total;
    JSTempValueRooter root;
    JSBool ok = JS_FALSE;
    if (!(cx->fp->flags & JSFRAME_CONSTRUCTING)) return BinaryTypeError(cx);
    buffer = GetBuffer(cx, argv[0]);
    if (!buffer) return BinaryTypeError(cx);
    /* Corrected ES2015: the optional offset uses ToInteger (TC39 #4516). */
    if (!js_ValueToNumber(cx, argc > 1 ? argv[1] : JSVAL_VOID, &offset))
        return JS_FALSE;
    offset = js_DoubleToInteger(offset);
    if (offset < 0) return BinaryRangeError(cx);
    if (buffer->detached) return BinaryTypeError(cx);
    total = buffer->length;
    if (offset > total) return BinaryRangeError(cx);
    length = total - offset;
    if (argc > 2 && !JSVAL_IS_VOID(argv[2])) {
        if (!js_ValueToNumber(cx, argv[2], &length)) return JS_FALSE;
        length = js_DoubleToInteger(length);
        if (length < 0) length = 0;
        if (length > 9007199254740991.0) length = 9007199254740991.0;
        if (length > total - offset) return BinaryRangeError(cx);
    }
    obj = BinaryCreate(cx, argv, &js_DataViewClass, JSProto_DataView);
    if (!obj) return JS_FALSE;
    JS_PUSH_TEMP_ROOT_OBJECT(cx, obj, &root);
    if (buffer->detached) { BinaryTypeError(cx); goto out; }
    view = (ViewData *)JS_malloc(cx, sizeof(ViewData));
    if (!view) goto out;
    view->offset = (size_t)offset; view->length = (size_t)length;
    OBJ_SET_SLOT(cx, obj, JSSLOT_PRIVATE, PRIVATE_TO_JSVAL(view));
    if (!JS_SetReservedSlot(cx, obj, 0, argv[0])) goto out;
    *rval = OBJECT_TO_JSVAL(obj); ok = JS_TRUE;
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}

static JSBool
BufferLength(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    BufferData *data = GetBuffer(cx, argv[-1]);
    if (!data || data->detached) return BinaryTypeError(cx);
    return js_NewNumberValue(cx, (jsdouble)data->length, rval);
}
static JSBool
ViewProperty(JSContext *cx, jsval receiver, jsval *rval, uintN which)
{
    ViewData *view = GetView(cx, receiver);
    BufferData *buffer;
    jsval value;
    if (!view) return BinaryTypeError(cx);
    if (!JS_GetReservedSlot(cx, JSVAL_TO_OBJECT(receiver), 0, &value)) return JS_FALSE;
    if (which == 0) { *rval = value; return JS_TRUE; }
    buffer = GetBuffer(cx, value);
    if (!buffer || buffer->detached) return BinaryTypeError(cx);
    return js_NewNumberValue(cx, (jsdouble)(which == 1 ? view->length : view->offset), rval);
}
#define VIEW_PROPERTY(name, which) \
static JSBool name(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) \
{ return ViewProperty(cx, argv[-1], rval, which); }
VIEW_PROPERTY(ViewBuffer, 0)
VIEW_PROPERTY(ViewLength, 1)
VIEW_PROPERTY(ViewOffset, 2)
#undef VIEW_PROPERTY
static JSBool
BufferIsView(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    *rval = BOOLEAN_TO_JSVAL((GetView(cx, argv[0]) != NULL || (!JSVAL_IS_PRIMITIVE(argv[0]) && js_IsTypedArray(cx, JSVAL_TO_OBJECT(argv[0]))))); return JS_TRUE;
}
static JSBool
BufferSpecies(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    *rval = argv[-1]; return JS_TRUE;
}

static JSBool
ViewAccess(JSContext *cx, uintN argc, jsval *argv, jsval *rval,
            uintN width, JSBool floating, JSBool signedValue, JSBool setter)
{
    ViewData *view = GetView(cx, argv[-1]);
    BufferData *buffer;
    jsdouble index, number = 0;
    jsval bufferValue, endianValue;
    JSBool little;
    size_t position;
    uint32 bits = 0;
    uintN i;
    unsigned char raw[8];
    const uint16 one = 1;
    JSBool hostLittle = *(const unsigned char *)&one != 0;
    float single;
    if (!view) return BinaryTypeError(cx);
    if (!js_ValueToNumber(cx, argv[0], &index)) return JS_FALSE;
    if (JSDOUBLE_IS_NaN(index) || index < 0 || js_DoubleToInteger(index) != index)
        return BinaryRangeError(cx);
    /* Corrected SetViewValue converts value before the final bounds check. */
    if (setter && !js_ValueToNumber(cx, argv[1], &number)) return JS_FALSE;
    endianValue = argc > (setter ? 2 : 1) ? argv[setter ? 2 : 1] : JSVAL_VOID;
    /* Modern ToBoolean never invokes an object's legacy conversion hook. */
    if (!JSVAL_IS_PRIMITIVE(endianValue)) little = JS_TRUE;
    else if (!js_ValueToBoolean(cx, endianValue, &little)) return JS_FALSE;
    if (!JS_GetReservedSlot(cx, JSVAL_TO_OBJECT(argv[-1]), 0, &bufferValue)) return JS_FALSE;
    buffer = GetBuffer(cx, bufferValue);
    if (!buffer || buffer->detached) return BinaryTypeError(cx);
    if (width > view->length || index > (jsdouble)(view->length - width)) return BinaryRangeError(cx);
    position = view->offset + (size_t)index;
    if (setter) {
        if (floating) {
            if (width == 4) { single = (float)js_RoundToFloat32(cx, number); memcpy(raw, &single, 4); }
            else memcpy(raw, &number, 8);
            for (i = 0; i < width; ++i)
                buffer->bytes[position + i] = raw[little == hostLittle ? i : width - i - 1];
        } else {
            if (!js_DoubleToECMAUint32(cx, number, &bits)) return JS_FALSE;
            for (i = 0; i < width; ++i)
                buffer->bytes[position + (little ? i : width - i - 1)] = (unsigned char)(bits >> (8 * i));
        }
        *rval = JSVAL_VOID; return JS_TRUE;
    }
    if (floating) {
        for (i = 0; i < width; ++i)
            raw[little == hostLittle ? i : width - i - 1] = buffer->bytes[position + i];
        if (width == 4) { memcpy(&single, raw, 4); number = single; }
        else memcpy(&number, raw, 8);
    } else {
        for (i = 0; i < width; ++i)
            bits |= (uint32)buffer->bytes[position + (little ? i : width - i - 1)] << (8 * i);
        number = (jsdouble)bits;
        if (signedValue && (bits & ((uint32)1 << (width * 8 - 1))))
            number -= width == 4 ? 4294967296.0 : width == 2 ? 65536.0 : 256.0;
    }
    return js_NewNumberValue(cx, number, rval);
}
#define VIEW_ACCESS(name, width, floating, sign) \
static JSBool ViewGet##name(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) \
{ return ViewAccess(cx, argc, argv, rval, width, floating, sign, JS_FALSE); } \
static JSBool ViewSet##name(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) \
{ return ViewAccess(cx, argc, argv, rval, width, floating, sign, JS_TRUE); }
VIEW_ACCESS(Int8, 1, JS_FALSE, JS_TRUE)
VIEW_ACCESS(Uint8, 1, JS_FALSE, JS_FALSE)
VIEW_ACCESS(Int16, 2, JS_FALSE, JS_TRUE)
VIEW_ACCESS(Uint16, 2, JS_FALSE, JS_FALSE)
VIEW_ACCESS(Int32, 4, JS_FALSE, JS_TRUE)
VIEW_ACCESS(Uint32, 4, JS_FALSE, JS_FALSE)
VIEW_ACCESS(Float32, 4, JS_TRUE, JS_TRUE)
VIEW_ACCESS(Float64, 8, JS_TRUE, JS_TRUE)
#undef VIEW_ACCESS

static JSBool
BinaryConstruct(JSContext *cx, jsval constructor, jsdouble length, jsval *rval)
{
    jsval *base, *oldsp;
    JSStackFrame *frame = cx->fp;
    void *mark;
    JSBool ok;
    base = js_AllocStack(cx, 3, &mark);
    if (!base) return JS_FALSE;
    base[0] = constructor; base[1] = JSVAL_NULL; base[2] = JSVAL_VOID;
    oldsp = frame->sp; frame->sp = base + 3;
    ok = js_NewNumberValue(cx, length, &base[2]) &&
         js_InvokeConstructorWithNewTarget(cx, base, 1, NULL);
    if (ok) *rval = base[0];
    frame->sp = oldsp; js_FreeStack(cx, mark);
    return ok;
}
static JSBool
BinaryRelativeIndex(JSContext *cx, jsval value, jsdouble length, jsdouble *result)
{
    if (!js_ValueToNumber(cx, value, result)) return JS_FALSE;
    *result = js_DoubleToInteger(*result);
    /* These are algorithmic indices, whose property-key conversion would
     * stringify either zero as "0", not the canonical numeric string "-0". */
    if (*result == 0) *result = 0;
    *result = *result < 0 ? JS_MAX(length + *result, 0) : JS_MIN(*result, length);
    return JS_TRUE;
}
static JSBool
BufferSlice(JSContext *cx, JSObject *ignored, uintN argc, jsval *argv, jsval *rval)
{
    jsval values[3] = {argv[-1], JSVAL_VOID, JSVAL_VOID};
    JSTempValueRooter root;
    BufferData *source = GetBuffer(cx, argv[-1]), *target;
    JSObject *global = js_BuiltinGlobal(cx, argv), *ctor;
    jsdouble length, start, end, count;
    jsid id;
    size_t copied, amount;
    JSBool ok = JS_FALSE;
    if (!source || source->detached) return BinaryTypeError(cx);
    length = source->length;
    JS_PUSH_TEMP_ROOT(cx, 3, values, &root);
    if (!BinaryRelativeIndex(cx, argv[0], length, &start)) goto out;
    end = length;
    if (argc > 1 && !JSVAL_IS_VOID(argv[1]) && !BinaryRelativeIndex(cx, argv[1], length, &end)) goto out;
    count = JS_MAX(end - start, 0);
    if (!JS_GetProperty(cx, JSVAL_TO_OBJECT(values[0]), "constructor", &values[1])) goto out;
    if (!JSVAL_IS_VOID(values[1])) {
        if (JSVAL_IS_PRIMITIVE(values[1])) { BinaryTypeError(cx); goto out; }
        if (!js_WellKnownSymbolId(cx, JS_WKS_SPECIES, &id) ||
            !OBJ_GET_PROPERTY(cx, JSVAL_TO_OBJECT(values[1]), id, &values[1])) goto out;
    }
    if (JSVAL_IS_VOID(values[1]) || JSVAL_IS_NULL(values[1])) {
        ctor = js_GetCachedClassObject(cx, global, JSProto_ArrayBuffer);
        if (!ctor && !js_GetClassObject(cx, global, JSProto_ArrayBuffer, &ctor)) goto out;
        if (!ctor) { BinaryTypeError(cx); goto out; }
        values[1] = OBJECT_TO_JSVAL(ctor);
    }
    if (!js_IsConstructor(cx, values[1])) { BinaryTypeError(cx); goto out; }
    if (!BinaryConstruct(cx, values[1], count, &values[2])) goto out;
    target = GetBuffer(cx, values[2]);
    if (!target || target->detached || values[2] == values[0] ||
        (jsdouble)target->length < count || source->detached) { BinaryTypeError(cx); goto out; }
    for (copied = 0; copied < (size_t)count; copied += amount) {
        if (cx->branchCallback && !cx->branchCallback(cx, NULL)) goto out;
        if (source->detached || target->detached) { BinaryTypeError(cx); goto out; }
        amount = JS_MIN((size_t)count - copied, (size_t)65536);
        memcpy(target->bytes + copied, source->bytes + (size_t)start + copied, amount);
    }
    *rval = values[2]; ok = JS_TRUE;
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}

static JSBool
BinaryGetter(JSContext *cx, JSObject *global, JSObject *obj,
              const char *name, JSNative native, JSBool symbol)
{
    JSFunction *fun;
    JSTempValueRooter root;
    JSAtom *atom;
    jsid id;
    JSBool ok;
    char getterName[64];
    JS_snprintf(getterName, sizeof getterName, "get %s", name);
    fun = JS_NewFunction(cx, native, 0, JSFUN_STRICT | JSFUN_NO_CONSTRUCT, global, getterName);
    if (!fun) return JS_FALSE;
    JS_PUSH_TEMP_ROOT_OBJECT(cx, fun->object, &root);
    if (symbol) ok = js_WellKnownSymbolId(cx, JS_WKS_SPECIES, &id);
    else {
        atom = js_Atomize(cx, name, strlen(name), 0);
        ok = atom != NULL; if (ok) id = ATOM_TO_JSID(atom);
    }
    if (ok) ok = OBJ_DEFINE_PROPERTY(cx, obj, id, JSVAL_VOID,
                         (JSPropertyOp)fun->object, NULL, JSPROP_GETTER | JSPROP_SHARED, NULL);
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}

JSObject *
js_InitArrayBufferClass(JSContext *cx, JSObject *global)
{
    JSObject *proto, *ctor;
    JSFunction *fun;
    JSTempValueRooter root;
    JSBool ok;
    proto = JS_InitClass(cx, global, NULL, &js_ArrayBufferClass,
                         js_ArrayBufferConstructor, 1, NULL, NULL, NULL, NULL);
    if (!proto) return NULL;
    JS_PUSH_TEMP_ROOT_OBJECT(cx, proto, &root);
    ctor = JS_GetConstructor(cx, proto);
    fun = ctor ? (JSFunction *)JS_GetPrivate(cx, ctor) : NULL;
    if (fun) fun->flags |= JSFUN_STRICT;
    ok = ctor &&
         BinaryGetter(cx, global, proto, "byteLength", BufferLength, JS_FALSE) &&
         BinaryGetter(cx, global, ctor, "[Symbol.species]", BufferSpecies, JS_TRUE) &&
         JS_DefineFunction(cx, proto, "slice", BufferSlice, 2, JSFUN_STRICT | JSFUN_NO_CONSTRUCT) &&
         JS_DefineFunction(cx, ctor, "isView", BufferIsView, 1, JSFUN_STRICT | JSFUN_NO_CONSTRUCT) &&
         js_DefineBuiltinTag(cx, proto, "ArrayBuffer");
    JS_POP_TEMP_ROOT(cx, &root);
    return ok ? proto : NULL;
}
JSObject *
js_InitDataViewClass(JSContext *cx, JSObject *global)
{
    static struct { const char *name; JSNative get, set; } types[] = {
        {"Int8", ViewGetInt8, ViewSetInt8}, {"Uint8", ViewGetUint8, ViewSetUint8},
        {"Int16", ViewGetInt16, ViewSetInt16}, {"Uint16", ViewGetUint16, ViewSetUint16},
        {"Int32", ViewGetInt32, ViewSetInt32}, {"Uint32", ViewGetUint32, ViewSetUint32},
        {"Float32", ViewGetFloat32, ViewSetFloat32}, {"Float64", ViewGetFloat64, ViewSetFloat64}
    };
    JSObject *proto, *ctor;
    JSFunction *fun;
    JSTempValueRooter root;
    JSBool ok;
    uintN i;
    char name[24];
    proto = JS_InitClass(cx, global, NULL, &js_DataViewClass,
                         js_DataViewConstructor, 1, NULL, NULL, NULL, NULL);
    if (!proto) return NULL;
    JS_PUSH_TEMP_ROOT_OBJECT(cx, proto, &root);
    ctor = JS_GetConstructor(cx, proto);
    fun = ctor ? (JSFunction *)JS_GetPrivate(cx, ctor) : NULL;
    if (fun) fun->flags |= JSFUN_STRICT;
    ok = ctor && BinaryGetter(cx, global, proto, "buffer", ViewBuffer, JS_FALSE) &&
         BinaryGetter(cx, global, proto, "byteLength", ViewLength, JS_FALSE) &&
         BinaryGetter(cx, global, proto, "byteOffset", ViewOffset, JS_FALSE) &&
         js_DefineBuiltinTag(cx, proto, "DataView");
    for (i = 0; ok && i < sizeof types / sizeof types[0]; ++i) {
        JS_snprintf(name, sizeof name, "get%s", types[i].name);
        ok = JS_DefineFunction(cx, proto, name, types[i].get, 1, JSFUN_STRICT | JSFUN_NO_CONSTRUCT) != NULL;
        if (!ok) break;
        JS_snprintf(name, sizeof name, "set%s", types[i].name);
        ok = JS_DefineFunction(cx, proto, name, types[i].set, 2, JSFUN_STRICT | JSFUN_NO_CONSTRUCT) != NULL;
    }
    JS_POP_TEMP_ROOT(cx, &root);
    return ok ? proto : NULL;
}

/* Integer-indexed storage is kept separate from ordinary expando properties.
 * No pointer into a Data Block is retained across calls into JavaScript. */
typedef struct TypedData {
    size_t offset, length;
    uintN kind;
} TypedData;
static JSObjectOps typedOps;
static const uintN typedSizes[9] = {1, 1, 1, 2, 2, 4, 4, 4, 8};
static const char *typedNames[9] = {
    "Int8Array", "Uint8Array", "Uint8ClampedArray", "Int16Array",
    "Uint16Array", "Int32Array", "Uint32Array", "Float32Array", "Float64Array"
};
static JSObjectOps *TypedObjectOps(JSContext *cx, JSClass *clasp) { return &typedOps; }
static void TypedFinalize(JSContext *cx, JSObject *obj);
#define TYPED_CLASS(name) \
JSClass js_##name##Class = { \
    #name, JSCLASS_HAS_PRIVATE | JSCLASS_HAS_RESERVED_SLOTS(2) | \
           JSCLASS_HAS_CACHED_PROTO(JSProto_##name), \
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, \
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, TypedFinalize, \
    TypedObjectOps, NULL, NULL, NULL, NULL, NULL, NULL, NULL }
TYPED_CLASS(Int8Array);
TYPED_CLASS(Uint8Array);
TYPED_CLASS(Uint8ClampedArray);
TYPED_CLASS(Int16Array);
TYPED_CLASS(Uint16Array);
TYPED_CLASS(Int32Array);
TYPED_CLASS(Uint32Array);
TYPED_CLASS(Float32Array);
TYPED_CLASS(Float64Array);
#undef TYPED_CLASS
static JSClass *typedClasses[9] = {
    &js_Int8ArrayClass, &js_Uint8ArrayClass, &js_Uint8ClampedArrayClass,
    &js_Int16ArrayClass, &js_Uint16ArrayClass, &js_Int32ArrayClass,
    &js_Uint32ArrayClass, &js_Float32ArrayClass, &js_Float64ArrayClass
};

JSBool
js_IsTypedArray(JSContext *cx, JSObject *obj)
{
    return obj && obj->map->ops == &typedOps;
}
static TypedData *
GetTyped(JSContext *cx, JSObject *obj)
{
    jsval slot;
    if (!js_IsTypedArray(cx, obj)) return NULL;
    slot = OBJ_GET_SLOT(cx, obj, JSSLOT_PRIVATE);
    return JSVAL_IS_VOID(slot) ? NULL : (TypedData *)JSVAL_TO_PRIVATE(slot);
}
static void
TypedFinalize(JSContext *cx, JSObject *obj)
{
    TypedData *data = GetTyped(cx, obj);
    if (data) JS_free(cx, data);
}
static jsval
TypedSlot(JSContext *cx, JSObject *obj, uintN slot)
{
    return OBJ_GET_SLOT(cx, obj, JSSLOT_START(OBJ_GET_CLASS(cx, obj)) + slot);
}
JSObject *
js_TypedArrayExpando(JSContext *cx, JSObject *obj)
{
    jsval value = TypedSlot(cx, obj, 1);
    return JSVAL_IS_PRIMITIVE(value) ? NULL : JSVAL_TO_OBJECT(value);
}
/* js_NewObject roots the new view and calls this before the embedding's
 * allocation hook. Reflection must never observe a missing property store. */
JSBool
js_InitTypedArrayObject(JSContext *cx, JSObject *obj)
{
    JSObject *backing;
    JSTempValueRooter root;
    JSBool ok;
    backing = js_NewObject(cx, &js_ObjectClass, NULL, OBJ_GET_PARENT(cx, obj));
    if (!backing) return JS_FALSE;
    JS_PUSH_TEMP_ROOT_OBJECT(cx, backing, &root);
    ok = JS_SetPrototype(cx, backing, NULL) &&
         JS_SetReservedSlot(cx, obj, 1, OBJECT_TO_JSVAL(backing));
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}
static BufferData *
TypedBuffer(JSContext *cx, JSObject *obj)
{
    return GetBuffer(cx, TypedSlot(cx, obj, 0));
}
JSBool
js_TypedArrayLength(JSContext *cx, JSObject *obj, jsdouble *length)
{
    TypedData *data = GetTyped(cx, obj);
    BufferData *buffer = data ? TypedBuffer(cx, obj) : NULL;
    if (!buffer || buffer->detached) return BinaryTypeError(cx);
    *length = (jsdouble)data->length;
    return JS_TRUE;
}

/* CanonicalNumericIndexString must not classify "01" or "1e0" as indices. */
JSBool
js_TypedArrayIndex(JSContext *cx, jsid id, JSBool *numeric, jsdouble *index)
{
    JSString *str, *canonical;
    JSTempValueRooter root;
    *numeric = JS_FALSE;
    if (JSID_IS_INT(id)) {
        *index = (jsdouble)JSID_TO_INT(id); *numeric = JS_TRUE;
        return JS_TRUE;
    }
    if (!JSVAL_IS_STRING(ID_TO_VALUE(id))) return JS_TRUE;
    str = JSVAL_TO_STRING(ID_TO_VALUE(id));
    if (JSSTRING_LENGTH(str) == 2 && JSSTRING_CHARS(str)[0] == '-' &&
        JSSTRING_CHARS(str)[1] == '0') {
        *index = -0.0; *numeric = JS_TRUE; return JS_TRUE;
    }
    if (!js_ValueToNumber(cx, STRING_TO_JSVAL(str), index)) return JS_FALSE;
    JS_PUSH_TEMP_ROOT_STRING(cx, str, &root);
    canonical = js_NumberToString(cx, *index);
    if (canonical) *numeric = js_CompareStrings(str, canonical) == 0;
    JS_POP_TEMP_ROOT(cx, &root);
    return canonical != NULL;
}
static JSBool
TypedValidIndex(TypedData *data, jsdouble index)
{
    return !JSDOUBLE_IS_NaN(index) && !JSDOUBLE_IS_NEGZERO(index) &&
           index >= 0 && index < (jsdouble)data->length &&
           js_DoubleToInteger(index) == index;
}
JSBool
js_TypedArrayIndexValid(JSContext *cx, JSObject *obj, jsdouble index)
{
    TypedData *data = GetTyped(cx, obj);
    return data && TypedValidIndex(data, index);
}
static JSBool
TypedRead(JSContext *cx, JSObject *obj, jsdouble index, jsval *rval)
{
    TypedData *data = GetTyped(cx, obj);
    BufferData *buffer = data ? TypedBuffer(cx, obj) : NULL;
    unsigned char *bytes;
    uint8 u8; uint16 u16; uint32 u32;
    float f; jsdouble number;
    if (!buffer || buffer->detached) return BinaryTypeError(cx);
    *rval = JSVAL_VOID;
    if (!TypedValidIndex(data, index)) return JS_TRUE;
    bytes = buffer->bytes + data->offset + (size_t)index * typedSizes[data->kind];
    switch (data->kind) {
      case 0: u8 = *bytes; number = u8 < 128 ? u8 : (jsdouble)u8 - 256; break;
      case 1: case 2: number = *bytes; break;
      case 3: memcpy(&u16, bytes, 2); number = u16 < 32768 ? u16 : (jsdouble)u16 - 65536; break;
      case 4: memcpy(&u16, bytes, 2); number = u16; break;
      case 5: memcpy(&u32, bytes, 4); number = u32 <= 2147483647U ? u32 : (jsdouble)u32 - 4294967296.0; break;
      case 6: memcpy(&u32, bytes, 4); number = u32; break;
      case 7: memcpy(&f, bytes, 4); number = f; break;
      default: memcpy(&number, bytes, 8); break;
    }
    return js_NewNumberValue(cx, number, rval);
}
JSBool
js_TypedArrayWrite(JSContext *cx, JSObject *obj, jsdouble index, jsval value, JSBool *accepted)
{
    TypedData *data;
    BufferData *buffer;
    unsigned char *bytes;
    uint8 u8; uint16 u16; uint32 u32 = 0;
    float f; jsdouble number, integer;
    *accepted = JS_FALSE;
    /* Conversion can detach the buffer, collect, or reenter the same view. */
    if (!js_ValueToNumber(cx, value, &number)) return JS_FALSE;
    data = GetTyped(cx, obj);
    buffer = data ? TypedBuffer(cx, obj) : NULL;
    if (!buffer || buffer->detached) return BinaryTypeError(cx);
    if (!TypedValidIndex(data, index)) return JS_TRUE;
    if (data->kind < 7 && data->kind != 2 &&
        !js_DoubleToECMAUint32(cx, number, &u32)) return JS_FALSE;
    bytes = buffer->bytes + data->offset + (size_t)index * typedSizes[data->kind];
    switch (data->kind) {
      case 0: case 1: u8 = (uint8)u32; memcpy(bytes, &u8, 1); break;
      case 2:
        if (JSDOUBLE_IS_NaN(number) || number <= 0) u8 = 0;
        else if (number >= 255) u8 = 255;
        else {
            integer = js_DoubleToInteger(number);
            u8 = (uint8)integer;
            if (number - integer > 0.5 || (number - integer == 0.5 && (u8 & 1))) ++u8;
        }
        memcpy(bytes, &u8, 1); break;
      case 3: case 4: u16 = (uint16)u32; memcpy(bytes, &u16, 2); break;
      case 5: case 6: memcpy(bytes, &u32, 4); break;
      case 7: f = (float)js_RoundToFloat32(cx, number); memcpy(bytes, &f, 4); break;
      default: memcpy(bytes, &number, 8); break;
    }
    *accepted = JS_TRUE;
    return JS_TRUE;
}

static JSBool
TypedLookup(JSContext *cx, JSObject *obj, jsid id, JSObject **owner, JSProperty **prop)
{
    JSBool numeric;
    jsdouble index;
    jsval value;
    JSObject *backing = js_TypedArrayExpando(cx, obj), *proto;
    if (!js_TypedArrayIndex(cx, id, &numeric, &index)) return JS_FALSE;
    *owner = NULL; *prop = NULL;
    if (numeric) {
        if (!TypedRead(cx, obj, index, &value)) return JS_FALSE;
        if (!JSVAL_IS_VOID(value)) { *owner = obj; *prop = (JSProperty *)obj; }
        return JS_TRUE;
    }
    if (backing) {
        if (!js_LookupOwnProperty(cx, backing, id, owner, prop)) return JS_FALSE;
        if (*prop) {
            OBJ_DROP_PROPERTY(cx, *owner, *prop);
            *owner = obj; *prop = (JSProperty *)obj;
            return JS_TRUE;
        }
    }
    proto = OBJ_GET_PROTO(cx, obj);
    return !proto || OBJ_LOOKUP_PROPERTY(cx, proto, id, owner, prop);
}
static void TypedDrop(JSContext *cx, JSObject *obj, JSProperty *prop) {}

JSBool
js_TypedArrayGet(JSContext *cx, JSObject *obj, jsid id, jsval receiver, jsval *rval)
{
    JSBool numeric;
    jsdouble index;
    JSObject *backing = js_TypedArrayExpando(cx, obj), *owner, *proto;
    JSProperty *prop;
    if (!js_TypedArrayIndex(cx, id, &numeric, &index)) return JS_FALSE;
    if (numeric) {
        if (!TypedRead(cx, obj, index, rval)) return JS_FALSE;
        if (receiver == OBJECT_TO_JSVAL(obj) || !JSVAL_IS_VOID(*rval)) return JS_TRUE;
    } else if (backing) {
        if (!js_LookupOwnProperty(cx, backing, id, &owner, &prop)) return JS_FALSE;
        if (prop) {
            OBJ_DROP_PROPERTY(cx, owner, prop);
            return js_GetPropertyValue(cx, backing, receiver, id, rval);
        }
    }
    proto = OBJ_GET_PROTO(cx, obj);
    *rval = JSVAL_VOID;
    return !proto || js_GetPropertyValue(cx, proto, receiver, id, rval);
}
static JSBool TypedGet(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{ return js_TypedArrayGet(cx, obj, id, OBJECT_TO_JSVAL(obj), vp); }
JSBool
js_TypedArraySet(JSContext *cx, JSObject *obj, jsid id, jsval value, jsval receiver, JSBool *accepted)
{
    jsval args[4], result = JSVAL_VOID;
    JSTempValueRooter root;
    JSBool ok;
    args[0] = OBJECT_TO_JSVAL(obj); args[1] = ID_TO_VALUE(id);
    args[2] = value; args[3] = receiver;
    JS_PUSH_TEMP_ROOT(cx, 4, args, &root);
    ok = js_ReflectSet(cx, NULL, 4, args, &result);
    *accepted = result == JSVAL_TRUE;
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}
static JSBool
TypedSet(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    JSBool accepted;
    if (!js_TypedArraySet(cx, obj, id, *vp, OBJECT_TO_JSVAL(obj), &accepted)) return JS_FALSE;
    if (!accepted && cx->fp && cx->fp->script && cx->fp->script->strictMode)
        return BinaryTypeError(cx);
    return JS_TRUE;
}
static JSBool
TypedAttributes(JSContext *cx, JSObject *obj, jsid id, JSProperty *prop, uintN *attrs)
{
    JSBool numeric;
    jsdouble index;
    JSObject *backing = js_TypedArrayExpando(cx, obj);
    if (!js_TypedArrayIndex(cx, id, &numeric, &index)) return JS_FALSE;
    if (numeric) { *attrs = JSPROP_ENUMERATE | JSPROP_PERMANENT; return JS_TRUE; }
    return backing && OBJ_GET_ATTRIBUTES(cx, backing, id, NULL, attrs);
}
static JSBool
TypedDelete(JSContext *cx, JSObject *obj, jsid id, jsval *rval)
{
    JSBool numeric;
    jsdouble index;
    jsval value;
    JSObject *backing = js_TypedArrayExpando(cx, obj);
    if (!js_TypedArrayIndex(cx, id, &numeric, &index)) return JS_FALSE;
    if (!numeric) return backing && OBJ_DELETE_PROPERTY(cx, backing, id, rval);
    if (!TypedRead(cx, obj, index, &value)) return JS_FALSE;
    *rval = BOOLEAN_TO_JSVAL(JSVAL_IS_VOID(value));
    if (*rval == JSVAL_FALSE && cx->fp && cx->fp->script && cx->fp->script->strictMode)
        return BinaryTypeError(cx);
    return JS_TRUE;
}
static JSBool
TypedDefine(JSContext *cx, JSObject *obj, jsid id, jsval value,
            JSPropertyOp getter, JSPropertyOp setter, uintN attrs, JSProperty **prop)
{
    JSBool numeric, accepted;
    jsdouble index;
    JSObject *backing = js_TypedArrayExpando(cx, obj), *owner;
    if (!js_TypedArrayIndex(cx, id, &numeric, &index)) return JS_FALSE;
    if (!numeric) {
        if (!backing || !OBJ_DEFINE_PROPERTY(cx, backing, id, value, getter, setter, attrs, NULL)) return JS_FALSE;
    } else {
        if (attrs != (JSPROP_ENUMERATE | JSPROP_PERMANENT) ||
            (getter && getter != JS_PropertyStub) || (setter && setter != JS_PropertyStub)) return BinaryTypeError(cx);
        if (!js_TypedArrayWrite(cx, obj, index, value, &accepted)) return JS_FALSE;
        if (!accepted) return BinaryTypeError(cx);
    }
    return !prop || TypedLookup(cx, obj, id, &owner, prop);
}
static JSBool
TypedSetAttributes(JSContext *cx, JSObject *obj, jsid id, JSProperty *prop, uintN *attrs)
{
    JSBool numeric;
    jsdouble index;
    JSObject *backing = js_TypedArrayExpando(cx, obj);
    if (!js_TypedArrayIndex(cx, id, &numeric, &index)) return JS_FALSE;
    if (numeric) return *attrs == (JSPROP_ENUMERATE | JSPROP_PERMANENT) || BinaryTypeError(cx);
    return backing && OBJ_SET_ATTRIBUTES(cx, backing, id, NULL, attrs);
}
static JSBool
TypedCheckAccess(JSContext *cx, JSObject *obj, jsid id, JSAccessMode mode, jsval *vp, uintN *attrs)
{
    *attrs = 0;
    if ((mode & JSACC_TYPEMASK) == JSACC_PROTO) {
        if (!(mode & JSACC_WRITE)) *vp = OBJECT_TO_JSVAL(OBJ_GET_PROTO(cx, obj));
    } else if ((mode & JSACC_TYPEMASK) == JSACC_PARENT) {
        if (!(mode & JSACC_WRITE)) *vp = OBJECT_TO_JSVAL(OBJ_GET_PARENT(cx, obj));
    } else {
        /* Access checks must not invoke an expando getter. */
        *vp = JSVAL_VOID;
    }
    return !cx->runtime->checkObjectAccess ||
           cx->runtime->checkObjectAccess(cx, obj, ID_TO_VALUE(id), mode, vp);
}
static jsval
TypedGetSlot(JSContext *cx, JSObject *obj, uint32 slot)
{
    jsval value;
#ifdef JS_THREADSAFE
    if (CX_THREAD_IS_RUNNING_GC(cx))
        return slot < (uint32)obj->slots[-1] ? obj->slots[slot] : JSVAL_VOID;
#endif
    JS_LOCK_GC(cx->runtime);
    value = slot < (uint32)obj->slots[-1] ? obj->slots[slot] : JSVAL_VOID;
    JS_UNLOCK_GC(cx->runtime);
    return value;
}
static JSBool
TypedSetSlot(JSContext *cx, JSObject *obj, uint32 slot, jsval value)
{
    JSBool ok;
    JS_LOCK_GC(cx->runtime);
    ok = slot < (uint32)obj->slots[-1];
    if (ok) obj->slots[slot] = value;
    JS_UNLOCK_GC(cx->runtime);
    return ok;
}
static uint32 TypedMark(JSContext *cx, JSObject *obj, void *arg)
{ return (uint32)obj->slots[-1]; }
static void TypedClear(JSContext *cx, JSObject *obj)
{
    JSObject *backing = js_TypedArrayExpando(cx, obj);
    if (backing) JS_ClearScope(cx, backing);
}
static JSObjectMap *
TypedNewMap(JSContext *cx, jsrefcount refs, JSObjectOps *ops, JSClass *clasp, JSObject *obj)
{
    JSObjectMap *map = js_NewObjectMap(cx, refs, ops, clasp, obj);
    if (map) map->nslots = JS_MAX(map->nslots, JSSLOT_START(clasp) + JSCLASS_RESERVED_SLOTS(clasp));
    return map;
}
static JSBool
TypedSetObjectSlot(JSContext *cx, JSObject *obj, uint32 slot, JSObject *value)
{
    JSObject *cursor;
    for (cursor = value; cursor; cursor = slot == JSSLOT_PROTO ? OBJ_GET_PROTO(cx, cursor) : OBJ_GET_PARENT(cx, cursor)) {
        if (cursor == obj) return BinaryTypeError(cx);
    }
    return TypedSetSlot(cx, obj, slot, OBJECT_TO_JSVAL(value));
}
/* The enumeration snapshot is an ordinary rooted array; it owns all key atoms. */
static JSBool
TypedEnumerate(JSContext *cx, JSObject *obj, JSIterateOp op, jsval *state, jsid *id)
{
    jsval values[5];
    JSTempValueRooter root;
    JSObject *array;
    jsuint length, index;
    JSBool ok = JS_FALSE;
    uintN attrs;
    if (op == JSENUMERATE_DESTROY) { *state = JSVAL_NULL; return JS_TRUE; }
    values[0] = OBJECT_TO_JSVAL(obj); values[1] = *state; values[2] = JSVAL_VOID;
    values[2] = OBJECT_TO_JSVAL(JS_GetGlobalForObject(cx, obj));
    values[3] = JSVAL_VOID; values[4] = OBJECT_TO_JSVAL(obj);
    JS_PUSH_TEMP_ROOT(cx, 5, values, &root);
    if (op == JSENUMERATE_INIT) {
        if (!js_ReflectOwnKeys(cx, NULL, 1, values + 4, &values[1])) goto out;
        array = JSVAL_TO_OBJECT(values[1]);
        if (!JS_DefineProperty(cx, array, "index", JSVAL_ZERO, NULL, NULL, 0)) goto out;
        *state = values[1];
        if (id) *id = JSVAL_ZERO;
        ok = JS_TRUE; goto out;
    }
    array = JSVAL_TO_OBJECT(values[1]);
    if (!JS_GetProperty(cx, array, "index", &values[2]) ||
        !JS_ValueToECMAUint32(cx, values[2], &index) || !JS_GetArrayLength(cx, array, &length)) goto out;
    while (index < length) {
        if (!JS_GetElement(cx, array, index++, &values[2])) goto out;
        if (JSVAL_IS_SYMBOL(values[2])) continue;
        if (!js_ValueToPropertyId(cx, values[2], id) ||
            !TypedAttributes(cx, obj, *id, NULL, &attrs)) goto out;
        if (!(attrs & JSPROP_ENUMERATE)) continue;
        if (!js_NewNumberValue(cx, index, &values[2]) || !JS_SetProperty(cx, array, "index", &values[2])) goto out;
        ok = JS_TRUE; goto out;
    }
    *state = JSVAL_NULL; ok = JS_TRUE;
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}
static JSObjectOps typedOps = {
    TypedNewMap, js_DestroyObjectMap, TypedLookup, TypedDefine,
    TypedGet, TypedSet, TypedAttributes, TypedSetAttributes,
    TypedDelete, js_DefaultValue, TypedEnumerate, TypedCheckAccess,
    NULL, TypedDrop, NULL, NULL, NULL, NULL,
    TypedSetObjectSlot, TypedSetObjectSlot, TypedMark, TypedClear,
    TypedGetSlot, TypedSetSlot
};

static JSBool TypedConstructor(JSContext *, JSObject *, uintN, jsval *, jsval *);
static JSBool TypedDerivedConstructor(JSContext *, JSObject *, uintN, jsval *, jsval *);
#define TYPED_CONSTRUCTOR(name) \
static JSBool Construct##name(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) \
{ return TypedDerivedConstructor(cx, obj, argc, argv, rval); }
TYPED_CONSTRUCTOR(Int8Array)
TYPED_CONSTRUCTOR(Uint8Array)
TYPED_CONSTRUCTOR(Uint8ClampedArray)
TYPED_CONSTRUCTOR(Int16Array)
TYPED_CONSTRUCTOR(Uint16Array)
TYPED_CONSTRUCTOR(Int32Array)
TYPED_CONSTRUCTOR(Uint32Array)
TYPED_CONSTRUCTOR(Float32Array)
TYPED_CONSTRUCTOR(Float64Array)
#undef TYPED_CONSTRUCTOR
static JSNative typedConstructors[9] = {
    ConstructInt8Array, ConstructUint8Array, ConstructUint8ClampedArray,
    ConstructInt16Array, ConstructUint16Array, ConstructInt32Array,
    ConstructUint32Array, ConstructFloat32Array, ConstructFloat64Array
};
JSBool
js_IsTypedArrayConstructor(JSNative native)
{
    uintN i;
    if (native == TypedConstructor) return JS_TRUE;
    for (i = 0; i < 9; ++i) if (native == typedConstructors[i]) return JS_TRUE;
    return JS_FALSE;
}
jsuint js_TypedArrayRawLength(JSContext *cx, JSObject *obj)
{ TypedData *data = GetTyped(cx, obj); return data ? (jsuint)data->length : 0; }

static JSBool
TypedConstruct(JSContext *cx, jsval constructor, uintN argc, jsval *argv,
               JSObject *newTarget, jsval *rval)
{
    jsval *base, *oldsp;
    JSStackFrame *frame = cx->fp;
    void *mark;
    JSBool ok;
    uintN i;
    if (!js_IsConstructor(cx, constructor)) return BinaryTypeError(cx);
    base = js_AllocStack(cx, (size_t)argc + 2, &mark);
    if (!base) return JS_FALSE;
    base[0] = constructor; base[1] = JSVAL_NULL;
    for (i = 0; i < argc; ++i) base[i + 2] = argv[i];
    oldsp = frame->sp; frame->sp = base + argc + 2;
    ok = js_InvokeConstructorWithNewTarget(cx, base, argc, newTarget);
    if (ok) *rval = base[0];
    frame->sp = oldsp; js_FreeStack(cx, mark);
    return ok;
}
static JSBool
TypedDerivedConstructor(JSContext *cx, JSObject *ignored, uintN argc, jsval *argv, jsval *rval)
{
    JSObject *parent;
    JSTempValueRooter root;
    JSBool ok;
    if (!(cx->fp->flags & JSFRAME_CONSTRUCTING)) return BinaryTypeError(cx);
    parent = OBJ_GET_PROTO(cx, JSVAL_TO_OBJECT(argv[-2]));
    if (!parent) return BinaryTypeError(cx);
    JS_PUSH_TEMP_ROOT_OBJECT(cx, parent, &root);
    ok = TypedConstruct(cx, OBJECT_TO_JSVAL(parent), argc, argv,
                         cx->fp->newTarget ? cx->fp->newTarget : JSVAL_TO_OBJECT(argv[-2]), rval);
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}
static JSBool
TypedKind(JSContext *cx, JSObject *target, uintN *kind)
{
    jsval value = OBJECT_TO_JSVAL(target);
    JSTempValueRooter root;
    JSFunction *fun;
    JSObject *next;
    uintN i;
    JSBool ok = JS_FALSE;
    JS_PUSH_SINGLE_TEMP_ROOT(cx, value, &root);
    while (!JSVAL_IS_NULL(root.u.value)) {
        target = JSVAL_TO_OBJECT(root.u.value);
        if (OBJ_GET_CLASS(cx, target) == &js_FunctionClass) {
            fun = (JSFunction *)JS_GetPrivate(cx, target);
            if (fun && !FUN_INTERPRETED(fun)) {
                for (i = 0; i < 9; ++i) {
                    if (FUN_NATIVE(fun) == typedConstructors[i]) { *kind = i; ok = JS_TRUE; goto out; }
                }
                if (FUN_NATIVE(fun) == TypedConstructor) break;
            }
        }
        if (js_IsProxy(cx, target)) {
            if (!js_ProxyGetPrototype(cx, target, &next)) goto out;
        } else next = OBJ_GET_PROTO(cx, target);
        root.u.value = OBJECT_TO_JSVAL(next);
    }
    BinaryTypeError(cx);
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}
static JSObject *
TypedAllocate(JSContext *cx, JSObject *target, JSObject *global)
{
    jsval values[3] = {JSVAL_VOID, JSVAL_VOID, JSVAL_VOID};
    JSTempValueRooter root;
    JSObject *proto, *obj = NULL;
    TypedData *data;
    uintN kind;
    JS_PUSH_TEMP_ROOT(cx, 3, values, &root);
    if (!TypedKind(cx, target, &kind) ||
        !JS_GetProperty(cx, target, "prototype", &values[0])) goto out;
    if (JSVAL_IS_PRIMITIVE(values[0])) {
        global = js_ConstructorGlobal(cx, target);
        if (!global) goto out;
        proto = js_GetCachedIntrinsic(cx, global, JS_INTRINSIC_TYPED_ARRAY_PROTO);
        if (!proto) { BinaryTypeError(cx); goto out; }
        values[0] = OBJECT_TO_JSVAL(proto);
    }
    obj = js_NewObject(cx, typedClasses[kind], JSVAL_TO_OBJECT(values[0]), global);
    if (!obj) goto out;
    values[1] = OBJECT_TO_JSVAL(obj);
    data = (TypedData *)JS_malloc(cx, sizeof(TypedData));
    if (!data) { obj = NULL; goto out; }
    data->kind = kind; data->offset = data->length = 0;
    OBJ_SET_SLOT(cx, obj, JSSLOT_PRIVATE, PRIVATE_TO_JSVAL(data));
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return obj;
}
static JSBool
TypedNewBuffer(JSContext *cx, JSObject *global, jsval constructor, size_t bytes, jsval *rval)
{
    JSObject *proto, *buffer;
    BufferData *storage;
    JSTempValueRooter root;
    jsval values[2] = {constructor, JSVAL_VOID};
    JSBool ok = JS_FALSE;
    if (bytes > 2147483647U) return BinaryRangeError(cx);
    JS_PUSH_TEMP_ROOT(cx, 2, values, &root);
    if (!JSVAL_IS_VOID(constructor)) {
        if (!js_IsConstructor(cx, constructor)) { BinaryTypeError(cx); goto out; }
        if (!JS_GetProperty(cx, JSVAL_TO_OBJECT(constructor), "prototype", &values[1])) goto out;
        if (JSVAL_IS_PRIMITIVE(values[1])) {
            global = js_ConstructorGlobal(cx, JSVAL_TO_OBJECT(constructor));
            if (!global) goto out;
        }
    }
    proto = JSVAL_IS_PRIMITIVE(values[1])
            ? js_BuiltinPrototype(cx, global, JSProto_ArrayBuffer) : JSVAL_TO_OBJECT(values[1]);
    if (!proto) goto out;
    values[1] = OBJECT_TO_JSVAL(proto);
    buffer = js_NewObject(cx, &js_ArrayBufferClass, proto, global);
    if (!buffer) goto out;
    values[1] = OBJECT_TO_JSVAL(buffer);
    storage = (BufferData *)JS_malloc(cx, sizeof(BufferData));
    if (!storage) goto out;
    storage->bytes = (unsigned char *)calloc(bytes ? bytes : 1, 1);
    if (!storage->bytes) { JS_free(cx, storage); BinaryRangeError(cx); goto out; }
    storage->length = bytes; storage->detached = JS_FALSE;
    OBJ_SET_SLOT(cx, buffer, JSSLOT_PRIVATE, PRIVATE_TO_JSVAL(storage));
    *rval = values[1]; ok = JS_TRUE;
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}
static JSBool
TypedAllocateBuffer(JSContext *cx, JSObject *obj, JSObject *global, size_t length)
{
    TypedData *data = GetTyped(cx, obj);
    jsval value = JSVAL_VOID;
    JSTempValueRooter root;
    JSBool ok;
    if (length > 2147483647U / typedSizes[data->kind]) return BinaryRangeError(cx);
    JS_PUSH_SINGLE_TEMP_ROOT(cx, value, &root);
    ok = TypedNewBuffer(cx, global, JSVAL_VOID, length * typedSizes[data->kind], &root.u.value) &&
         JS_SetReservedSlot(cx, obj, 0, root.u.value);
    if (ok) { data->offset = 0; data->length = length; }
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}
static JSBool TypedSpecies(JSContext *, JSObject *, JSObject *, JSProtoKey, jsval *);
static JSBool TypedFrom(JSContext *, jsval, jsval, jsval, jsval, jsval *);
static JSBool
TypedConstructor(JSContext *cx, JSObject *ignored, uintN argc, jsval *argv, jsval *rval)
{
    JSObject *target, *obj, *source = NULL, *global = js_BuiltinGlobal(cx, argv);
    jsval values[3] = {JSVAL_VOID, JSVAL_VOID, JSVAL_VOID};
    JSTempValueRooter root;
    TypedData *data, *other;
    BufferData *buffer;
    jsdouble length = 0, offset;
    size_t i, size, bytes;
    JSBool ok = JS_FALSE, accepted;
    if (!(cx->fp->flags & JSFRAME_CONSTRUCTING)) return BinaryTypeError(cx);
    target = cx->fp->newTarget ? cx->fp->newTarget : JSVAL_TO_OBJECT(argv[-2]);
    if (argc && !JSVAL_IS_PRIMITIVE(argv[0])) source = JSVAL_TO_OBJECT(argv[0]);
    if (source && !GetBuffer(cx, argv[0]) && !GetTyped(cx, source))
        return TypedFrom(cx, OBJECT_TO_JSVAL(target), argv[0], JSVAL_VOID, JSVAL_VOID, rval);
    if (argc && !source) {
        if (JSVAL_IS_VOID(argv[0])) return BinaryTypeError(cx);
        if (!js_ValueToNumber(cx, argv[0], &length)) return JS_FALSE;
        if (JSDOUBLE_IS_NaN(length) || length < 0 || length > 2147483647.0 ||
            js_DoubleToInteger(length) != length) return BinaryRangeError(cx);
    }
    JS_PUSH_TEMP_ROOT(cx, 3, values, &root);
    obj = TypedAllocate(cx, target, global);
    if (!obj) goto out;
    values[0] = OBJECT_TO_JSVAL(obj);
    data = GetTyped(cx, obj); size = typedSizes[data->kind];
    if (source && (buffer = GetBuffer(cx, argv[0])) != NULL) {
        if (!js_ValueToNumber(cx, argc > 1 ? argv[1] : JSVAL_VOID, &offset)) goto out;
        offset = js_DoubleToInteger(offset);
        if (offset < 0 || offset > 2147483647.0 || (size_t)offset % size) { BinaryRangeError(cx); goto out; }
        if (buffer->detached) { BinaryTypeError(cx); goto out; }
        bytes = buffer->length;
        if (argc < 3 || JSVAL_IS_VOID(argv[2])) {
            if (bytes % size || offset > (jsdouble)bytes) { BinaryRangeError(cx); goto out; }
            length = ((jsdouble)bytes - offset) / size;
        } else {
            if (!js_ValueToNumber(cx, argv[2], &length)) goto out;
            length = js_DoubleToInteger(length);
            if (length < 0) length = 0;
            if (length > 9007199254740991.0) length = 9007199254740991.0;
            if (offset + length * size > (jsdouble)bytes) { BinaryRangeError(cx); goto out; }
        }
        if (buffer->detached) { BinaryTypeError(cx); goto out; }
        data->offset = (size_t)offset; data->length = (size_t)length;
        if (!JS_SetReservedSlot(cx, obj, 0, argv[0])) goto out;
    } else if (source) {
        other = GetTyped(cx, source);
        if (!js_TypedArrayLength(cx, source, &length) ||
            !TypedSpecies(cx, JSVAL_TO_OBJECT(TypedSlot(cx, source, 0)), global,
                          JSProto_ArrayBuffer, &values[1])) goto out;
        buffer = TypedBuffer(cx, source);
        if (buffer->detached) { BinaryTypeError(cx); goto out; }
        if (length > 2147483647.0 / size) { BinaryRangeError(cx); goto out; }
        bytes = data->kind == other->kind ? buffer->length - other->offset : (size_t)length * size;
        if (!TypedNewBuffer(cx, global, values[1], bytes, &values[2])) goto out;
        if (buffer->detached) { BinaryTypeError(cx); goto out; }
        if (!JS_SetReservedSlot(cx, obj, 0, values[2])) goto out;
        data->length = (size_t)length;
        if (data->kind == other->kind) {
            /* ES2015 CloneArrayBuffer copies the remaining Data Block, even
             * when the source is a shorter view, and preserves NaN payloads. */
            if (bytes) memcpy(TypedBuffer(cx, obj)->bytes, buffer->bytes + other->offset, bytes);
        } else {
            for (i = 0; i < (size_t)length; ++i) {
                if (!TypedRead(cx, source, (jsdouble)i, &values[1]) ||
                    !js_TypedArrayWrite(cx, obj, (jsdouble)i, values[1], &accepted)) goto out;
            }
        }
    } else if (!TypedAllocateBuffer(cx, obj, global, (size_t)length)) goto out;
    *rval = values[0]; ok = JS_TRUE;
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}

/* Iterable collection finishes before mapping/converting its elements. */
static JSBool
TypedFrom(JSContext *cx, jsval constructor, jsval source, jsval mapper, jsval receiver, jsval *rval)
{
    jsval values[9], args[2];
    JSTempValueRooter root, argsRoot;
    JSObject *items, *iterator = NULL, *list, *target;
    jsid id;
    jsdouble length, index;
    JSBool ok = JS_FALSE, done, accepted, mapping = !JSVAL_IS_VOID(mapper);
    uintN i;
    for (i = 0; i < 9; ++i) values[i] = JSVAL_VOID;
    values[0] = constructor; values[1] = source; values[2] = mapper; values[3] = receiver;
    if (!js_IsConstructor(cx, constructor) || (mapping && !js_IsCallable(cx, mapper))) return BinaryTypeError(cx);
    JS_PUSH_TEMP_ROOT(cx, 9, values, &root);
    args[0] = args[1] = JSVAL_VOID;
    JS_PUSH_TEMP_ROOT(cx, 2, args, &argsRoot);
    items = js_ValueToNonNullObject(cx, source);
    if (!items) goto out;
    values[1] = OBJECT_TO_JSVAL(items);
    if (!js_WellKnownSymbolId(cx, JS_WKS_ITERATOR, &id) ||
        !OBJ_GET_PROPERTY(cx, items, id, &values[4])) goto out;
    if (!JSVAL_IS_VOID(values[4]) && !JSVAL_IS_NULL(values[4])) {
        if (!js_IsCallable(cx, values[4])) { BinaryTypeError(cx); goto out; }
        if (!js_InternalInvokeValue(cx, source, values[4], 0, 0, NULL, &values[5])) goto out;
        if (JSVAL_IS_PRIMITIVE(values[5])) { BinaryTypeError(cx); goto out; }
        iterator = JSVAL_TO_OBJECT(values[5]);
        list = js_NewArrayObject(cx, 0, NULL);
        if (!list) goto out;
        values[6] = OBJECT_TO_JSVAL(list);
        length = 0;
        for (;;) {
            if (!js_ProxyIteratorNext(cx, values[5], &done, &values[7])) goto out;
            if (done) break;
            if (length >= JSVAL_INT_MAX) { BinaryRangeError(cx); goto close; }
            if (!js_CreateDataPropertyOrThrow(cx, list, INT_TO_JSID((jsint)length), values[7])) goto close;
            ++length;
        }
        items = list;
    } else if (!js_ArrayLikeLength(cx, items, &length)) goto out;
    if (!BinaryConstruct(cx, constructor, length, &values[8])) goto out;
    if (JSVAL_IS_PRIMITIVE(values[8])) { BinaryTypeError(cx); goto out; }
    target = JSVAL_TO_OBJECT(values[8]);
    if (!js_TypedArrayLength(cx, target, &index)) goto out;
    if (index < length) { BinaryTypeError(cx); goto out; }
    for (index = 0; index < length; ++index) {
        if (!js_ArrayLikeIndex(cx, index, &id) || !OBJ_GET_PROPERTY(cx, items, id, &values[7])) goto out;
        if (mapping) {
            args[0] = values[7];
            if (!js_NewNumberValue(cx, index, &args[1]) ||
                !js_InternalInvokeValue(cx, receiver, mapper, 0, 2, args, &values[7])) goto out;
        }
        if (!js_TypedArrayWrite(cx, target, index, values[7], &accepted)) goto out;
        if (!accepted) { BinaryTypeError(cx); goto out; }
    }
    *rval = values[8]; ok = JS_TRUE; goto out;
  close:
    if (iterator) js_IteratorCloseThrow(cx, iterator);
  out:
    JS_POP_TEMP_ROOT(cx, &argsRoot);
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}

static JSBool
TypedField(JSContext *cx, jsval value, uintN field, jsval *rval)
{
    JSObject *obj;
    TypedData *data;
    BufferData *buffer;
    size_t number;
    if (JSVAL_IS_PRIMITIVE(value)) return BinaryTypeError(cx);
    obj = JSVAL_TO_OBJECT(value); data = GetTyped(cx, obj);
    if (!data) return BinaryTypeError(cx);
    if (field == 0) { *rval = TypedSlot(cx, obj, 0); return JS_TRUE; }
    buffer = TypedBuffer(cx, obj);
    if (!buffer) return BinaryTypeError(cx);
    number = buffer->detached ? 0 : field == 1 ? data->length :
             field == 2 ? data->length * typedSizes[data->kind] : data->offset;
    return js_NewNumberValue(cx, (jsdouble)number, rval);
}
#define TYPED_FIELD(name, field) \
static JSBool name(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) \
{ return TypedField(cx, argv[-1], field, rval); }
TYPED_FIELD(TypedBufferGetter, 0)
TYPED_FIELD(TypedLengthGetter, 1)
TYPED_FIELD(TypedByteLengthGetter, 2)
TYPED_FIELD(TypedOffsetGetter, 3)
#undef TYPED_FIELD
static JSBool
TypedTagGetter(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    TypedData *data = JSVAL_IS_PRIMITIVE(argv[-1]) ? NULL : GetTyped(cx, JSVAL_TO_OBJECT(argv[-1]));
    JSString *str;
    *rval = JSVAL_VOID;
    if (!data) return JS_TRUE;
    str = JS_NewStringCopyZ(cx, typedNames[data->kind]);
    if (!str) return JS_FALSE;
    *rval = STRING_TO_JSVAL(str); return JS_TRUE;
}
static JSBool
TypedIterator(JSContext *cx, jsval *argv, uintN kind, jsval *rval)
{
    jsdouble length;
    if (JSVAL_IS_PRIMITIVE(argv[-1])) return BinaryTypeError(cx);
    if (!js_TypedArrayLength(cx, JSVAL_TO_OBJECT(argv[-1]), &length)) return JS_FALSE;
    return js_CreateArrayIterator(cx, argv, rval, kind);
}
static JSBool TypedKeys(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{ return TypedIterator(cx, argv, 0, rval); }
static JSBool TypedValues(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{ return TypedIterator(cx, argv, 1, rval); }
static JSBool TypedEntries(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{ return TypedIterator(cx, argv, 2, rval); }
static JSBool TypedStaticFrom(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{ return TypedFrom(cx, argv[-1], argc ? argv[0] : JSVAL_VOID,
                   argc > 1 ? argv[1] : JSVAL_VOID, argc > 2 ? argv[2] : JSVAL_VOID, rval); }
static JSBool
TypedStaticOf(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    jsval value = JSVAL_VOID;
    JSTempValueRooter root;
    jsdouble length;
    uintN i;
    JSBool ok = JS_FALSE, accepted;
    if (!js_IsConstructor(cx, argv[-1])) return BinaryTypeError(cx);
    JS_PUSH_SINGLE_TEMP_ROOT(cx, value, &root);
    if (!BinaryConstruct(cx, argv[-1], argc, &root.u.value)) goto out;
    if (JSVAL_IS_PRIMITIVE(root.u.value)) { BinaryTypeError(cx); goto out; }
    obj = JSVAL_TO_OBJECT(root.u.value);
    if (!js_TypedArrayLength(cx, obj, &length)) goto out;
    if (length < argc) { BinaryTypeError(cx); goto out; }
    for (i = 0; i < argc; ++i) {
        if (!js_TypedArrayWrite(cx, obj, i, argv[i], &accepted)) goto out;
        if (!accepted) { BinaryTypeError(cx); goto out; }
    }
    *rval = root.u.value; ok = JS_TRUE;
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}

static JSObject *
TypedReceiver(JSContext *cx, jsval value, jsdouble *length)
{
    JSObject *obj;
    if (JSVAL_IS_PRIMITIVE(value)) { BinaryTypeError(cx); return NULL; }
    obj = JSVAL_TO_OBJECT(value);
    return js_TypedArrayLength(cx, obj, length) ? obj : NULL;
}
static JSBool
TypedSpecies(JSContext *cx, JSObject *source, JSObject *global, JSProtoKey key, jsval *rval)
{
    jsval value = JSVAL_VOID;
    JSTempValueRooter root;
    JSObject *ctor;
    jsid id;
    JSBool ok = JS_FALSE;
    JS_PUSH_SINGLE_TEMP_ROOT(cx, value, &root);
    if (!JS_GetProperty(cx, source, "constructor", &root.u.value)) goto out;
    if (!JSVAL_IS_VOID(root.u.value)) {
        if (JSVAL_IS_PRIMITIVE(root.u.value)) { BinaryTypeError(cx); goto out; }
        if (!js_WellKnownSymbolId(cx, JS_WKS_SPECIES, &id) ||
            !OBJ_GET_PROPERTY(cx, JSVAL_TO_OBJECT(root.u.value), id, &root.u.value)) goto out;
        if (!JSVAL_IS_VOID(root.u.value) && !JSVAL_IS_NULL(root.u.value)) {
            if (!js_IsConstructor(cx, root.u.value)) { BinaryTypeError(cx); goto out; }
            *rval = root.u.value; ok = JS_TRUE; goto out;
        }
    }
    ctor = js_GetCachedClassObject(cx, global, key);
    if (!ctor && !js_GetClassObject(cx, global, key, &ctor)) goto out;
    if (!ctor) { BinaryTypeError(cx); goto out; }
    *rval = OBJECT_TO_JSVAL(ctor); ok = JS_TRUE;
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}
static JSBool
TypedSpeciesCreate(JSContext *cx, JSObject *source, JSObject *global, jsdouble length, jsval *rval)
{
    TypedData *data = GetTyped(cx, source);
    JSProtoKey key = (JSProtoKey)JSCLASS_CACHED_PROTO_KEY(typedClasses[data->kind]);
    jsval value = JSVAL_VOID;
    JSTempValueRooter root;
    jsdouble actual;
    JSBool ok = JS_FALSE;
    JS_PUSH_SINGLE_TEMP_ROOT(cx, value, &root);
    if (!TypedSpecies(cx, source, global, key, &root.u.value) ||
        !BinaryConstruct(cx, root.u.value, length, &root.u.value)) goto out;
    if (!TypedReceiver(cx, root.u.value, &actual)) goto out;
    if (actual < length) { BinaryTypeError(cx); goto out; }
    *rval = root.u.value; ok = JS_TRUE;
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}
static JSBool
TypedSetMethod(JSContext *cx, JSObject *ignored, uintN argc, jsval *argv, jsval *rval)
{
    JSObject *target, *source, *list;
    jsval values[3] = {JSVAL_VOID, JSVAL_VOID, JSVAL_VOID};
    JSTempValueRooter root;
    jsdouble length, sourceLength, offset, i;
    JSBool ok = JS_FALSE, accepted;
    jsid id;
    if (JSVAL_IS_PRIMITIVE(argv[-1])) return BinaryTypeError(cx);
    target = JSVAL_TO_OBJECT(argv[-1]);
    if (!GetTyped(cx, target)) return BinaryTypeError(cx);
    if (!js_ValueToNumber(cx, argc > 1 ? argv[1] : JSVAL_VOID, &offset)) return JS_FALSE;
    offset = js_DoubleToInteger(offset);
    if (offset < 0) return BinaryRangeError(cx);
    if (!js_TypedArrayLength(cx, target, &length)) return JS_FALSE;
    JS_PUSH_TEMP_ROOT(cx, 3, values, &root);
    source = js_ValueToNonNullObject(cx, argc ? argv[0] : JSVAL_VOID);
    if (!source) goto out;
    values[0] = OBJECT_TO_JSVAL(source);
    if (js_IsTypedArray(cx, source)) {
        if (!js_TypedArrayLength(cx, source, &sourceLength)) goto out;
    } else if (!js_ArrayLikeLength(cx, source, &sourceLength)) goto out;
    if (sourceLength + offset > length) { BinaryRangeError(cx); goto out; }
    if (js_IsTypedArray(cx, source) && GetTyped(cx, source)->kind == GetTyped(cx, target)->kind) {
        TypedData *srcData = GetTyped(cx, source), *dstData = GetTyped(cx, target);
        size_t width = typedSizes[srcData->kind];
        if (!js_TypedArrayLength(cx, target, &length)) goto out;
        if (sourceLength) memmove(TypedBuffer(cx, target)->bytes + dstData->offset + (size_t)offset * width,
                                 TypedBuffer(cx, source)->bytes + srcData->offset, (size_t)sourceLength * width);
        *rval = JSVAL_VOID; ok = JS_TRUE; goto out;
    }
    if (js_IsTypedArray(cx, source)) {
        /* Snapshot before any writes when views share a buffer. */
        if (TypedSlot(cx, source, 0) == TypedSlot(cx, target, 0)) {
            list = js_NewArrayObject(cx, 0, NULL);
            if (!list) goto out;
            values[2] = OBJECT_TO_JSVAL(list);
            for (i = 0; i < sourceLength; ++i) {
                if (!TypedRead(cx, source, i, &values[1]) ||
                    !JS_DefineElement(cx, list, (jsuint)i, values[1], NULL, NULL, JSPROP_ENUMERATE)) goto out;
            }
            source = list;
        }
    }
    for (i = 0; i < sourceLength; ++i) {
        if (!js_ArrayLikeIndex(cx, i, &id) || !OBJ_GET_PROPERTY(cx, source, id, &values[1]) ||
            !js_TypedArrayWrite(cx, target, offset + i, values[1], &accepted)) goto out;
        if (!accepted) { BinaryTypeError(cx); goto out; }
    }
    *rval = JSVAL_VOID; ok = JS_TRUE;
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}
static JSBool
TypedSubarray(JSContext *cx, JSObject *ignored, uintN argc, jsval *argv, jsval *rval)
{
    JSObject *source;
    TypedData *data;
    jsdouble length, begin, end;
    jsval values[4] = {JSVAL_VOID, JSVAL_VOID, JSVAL_VOID, JSVAL_VOID};
    JSTempValueRooter root;
    JSBool ok = JS_FALSE;
    if (JSVAL_IS_PRIMITIVE(argv[-1])) return BinaryTypeError(cx);
    source = JSVAL_TO_OBJECT(argv[-1]); data = GetTyped(cx, source);
    if (!data) return BinaryTypeError(cx);
    length = (jsdouble)data->length;
    JS_PUSH_TEMP_ROOT(cx, 4, values, &root);
    values[1] = TypedSlot(cx, source, 0);
    if (!BinaryRelativeIndex(cx, argc ? argv[0] : JSVAL_VOID, length, &begin)) goto out;
    end = length;
    if (argc > 1 && !JSVAL_IS_VOID(argv[1]) && !BinaryRelativeIndex(cx, argv[1], length, &end)) goto out;
    if (!js_NewNumberValue(cx, data->offset + begin * typedSizes[data->kind], &values[2]) ||
        !js_NewNumberValue(cx, JS_MAX(end - begin, 0), &values[3]) ||
        !TypedSpecies(cx, source, js_BuiltinGlobal(cx, argv),
                     (JSProtoKey)JSCLASS_CACHED_PROTO_KEY(typedClasses[data->kind]), &values[0])) goto out;
    ok = TypedConstruct(cx, values[0], 3, values + 1, NULL, rval);
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}
static JSBool
TypedSlice(JSContext *cx, JSObject *ignored, uintN argc, jsval *argv, jsval *rval)
{
    jsdouble length, start, end, count, i;
    JSObject *source = TypedReceiver(cx, argv[-1], &length), *target;
    jsval values[2] = {JSVAL_VOID, JSVAL_VOID};
    JSTempValueRooter root;
    JSBool ok = JS_FALSE, accepted;
    if (!source) return JS_FALSE;
    if (!BinaryRelativeIndex(cx, argc ? argv[0] : JSVAL_VOID, length, &start)) return JS_FALSE;
    end = length;
    if (argc > 1 && !JSVAL_IS_VOID(argv[1]) && !BinaryRelativeIndex(cx, argv[1], length, &end)) return JS_FALSE;
    count = JS_MAX(end - start, 0);
    JS_PUSH_TEMP_ROOT(cx, 2, values, &root);
    if (!TypedSpeciesCreate(cx, source, js_BuiltinGlobal(cx, argv), count, &values[0])) goto out;
    target = JSVAL_TO_OBJECT(values[0]);
    if (count && GetTyped(cx, source)->kind == GetTyped(cx, target)->kind) {
        TypedData *srcData = GetTyped(cx, source), *dstData = GetTyped(cx, target);
        size_t j, width = typedSizes[srcData->kind], total = (size_t)count * width;
        unsigned char *srcBytes, *dstBytes;
        if (!js_TypedArrayLength(cx, source, &length)) goto out;
        srcBytes = TypedBuffer(cx, source)->bytes + srcData->offset + (size_t)start * width;
        dstBytes = TypedBuffer(cx, target)->bytes + dstData->offset;
        for (j = 0; j < total; ++j) dstBytes[j] = srcBytes[j];
        *rval = values[0]; ok = JS_TRUE; goto out;
    }
    for (i = 0; i < count; ++i) {
        if (!TypedRead(cx, source, start + i, &values[1]) ||
            !js_TypedArrayWrite(cx, target, i, values[1], &accepted)) goto out;
        if (!accepted) { BinaryTypeError(cx); goto out; }
    }
    *rval = values[0]; ok = JS_TRUE;
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}
static JSBool
TypedFill(JSContext *cx, JSObject *ignored, uintN argc, jsval *argv, jsval *rval)
{
    jsdouble length, start, end, i;
    JSObject *source = TypedReceiver(cx, argv[-1], &length);
    JSBool accepted;
    if (!source || !BinaryRelativeIndex(cx, argc > 1 ? argv[1] : JSVAL_VOID, length, &start)) return JS_FALSE;
    end = length;
    if (argc > 2 && !JSVAL_IS_VOID(argv[2]) && !BinaryRelativeIndex(cx, argv[2], length, &end)) return JS_FALSE;
    /* The original 2015 edition uses Array.fill's Set loop: element conversion
     * occurs for each write, after the start/end conversions. */
    for (i = start; i < end; ++i) {
        if (!js_TypedArrayWrite(cx, source, i, argc ? argv[0] : JSVAL_VOID, &accepted)) return JS_FALSE;
        if (!accepted) return BinaryTypeError(cx);
    }
    *rval = argv[-1]; return JS_TRUE;
}
static JSBool
TypedCopyWithin(JSContext *cx, JSObject *ignored, uintN argc, jsval *argv, jsval *rval)
{
    jsdouble length, to, from, end, count, checked;
    JSObject *source = TypedReceiver(cx, argv[-1], &length);
    TypedData *data;
    BufferData *buffer;
    if (!source || !BinaryRelativeIndex(cx, argc ? argv[0] : JSVAL_VOID, length, &to) ||
        !BinaryRelativeIndex(cx, argc > 1 ? argv[1] : JSVAL_VOID, length, &from)) return JS_FALSE;
    end = length;
    if (argc > 2 && !JSVAL_IS_VOID(argv[2]) && !BinaryRelativeIndex(cx, argv[2], length, &end)) return JS_FALSE;
    count = JS_MIN(end - from, length - to);
    if (count > 0) {
        if (!js_TypedArrayLength(cx, source, &checked)) return JS_FALSE;
        data = GetTyped(cx, source); buffer = TypedBuffer(cx, source);
        memmove(buffer->bytes + data->offset + (size_t)to * typedSizes[data->kind],
                buffer->bytes + data->offset + (size_t)from * typedSizes[data->kind],
                (size_t)count * typedSizes[data->kind]);
    }
    *rval = argv[-1]; return JS_TRUE;
}
static JSBool
TypedReverse(JSContext *cx, JSObject *ignored, uintN argc, jsval *argv, jsval *rval)
{
    jsdouble length;
    JSObject *source = TypedReceiver(cx, argv[-1], &length);
    TypedData *data;
    BufferData *buffer;
    size_t i, j, size;
    unsigned char bytes[8];
    if (!source) return JS_FALSE;
    data = GetTyped(cx, source); buffer = TypedBuffer(cx, source); size = typedSizes[data->kind];
    for (i = 0; i < data->length / 2; ++i) {
        j = data->length - i - 1;
        memcpy(bytes, buffer->bytes + data->offset + i * size, size);
        memcpy(buffer->bytes + data->offset + i * size, buffer->bytes + data->offset + j * size, size);
        memcpy(buffer->bytes + data->offset + j * size, bytes, size);
    }
    *rval = argv[-1]; return JS_TRUE;
}

/* All callback methods read the fixed internal length, never a shadowing
 * ordinary length property. Each element read revalidates the Data Block. */
static JSBool
TypedEach(JSContext *cx, uintN argc, jsval *argv, jsval *rval, uintN kind)
{
    jsdouble length, index, count = 0;
    JSObject *source = TypedReceiver(cx, argv[-1], &length), *target = NULL, *list = NULL;
    jsval values[5], args[4];
    JSTempValueRooter root, argsRoot;
    JSBool ok = JS_FALSE, answer, accepted;
    uintN i, callargc;
    intN step = kind == 8 ? -1 : 1;
    if (!source) return JS_FALSE;
    if (!argc || !js_IsCallable(cx, argv[0])) return BinaryTypeError(cx);
    for (i = 0; i < 5; ++i) values[i] = JSVAL_VOID;
    for (i = 0; i < 4; ++i) args[i] = JSVAL_VOID;
    JS_PUSH_TEMP_ROOT(cx, 5, values, &root);
    JS_PUSH_TEMP_ROOT(cx, 4, args, &argsRoot);
    if (kind == 1) {
        if (!TypedSpeciesCreate(cx, source, js_BuiltinGlobal(cx, argv), length, &values[0])) goto out;
        target = JSVAL_TO_OBJECT(values[0]);
    } else if (kind == 2) {
        list = js_NewArrayObject(cx, 0, NULL);
        if (!list) goto out;
        values[0] = OBJECT_TO_JSVAL(list);
    }
    index = step > 0 ? 0 : length - 1;
    if (kind >= 7) {
        if (argc > 1) values[1] = argv[1];
        else {
            if (!length) { BinaryTypeError(cx); goto out; }
            if (!TypedRead(cx, source, index, &values[1])) goto out;
            index += step;
        }
    }
    while (index >= 0 && index < length) {
        if (!TypedRead(cx, source, index, &values[4])) goto out;
        if (kind >= 7) {
            args[0] = values[1]; args[1] = values[4];
            if (!js_NewNumberValue(cx, index, &args[2])) goto out;
            args[3] = argv[-1]; callargc = 4;
        } else {
            args[0] = values[4];
            if (!js_NewNumberValue(cx, index, &args[1])) goto out;
            args[2] = argv[-1]; callargc = 3;
        }
        if (!js_InternalInvokeValue(cx, kind >= 7 || argc < 2 ? JSVAL_VOID : argv[1],
                                    argv[0], 0, callargc, args, &values[2])) goto out;
        if (kind >= 7) values[1] = values[2];
        else if (kind == 1) {
            if (!js_TypedArrayWrite(cx, target, index, values[2], &accepted)) goto out;
            if (!accepted) { BinaryTypeError(cx); goto out; }
        } else if (kind >= 2) {
            if (!JS_ValueToBoolean(cx, values[2], &answer)) goto out;
            if (kind == 2 && answer) {
                if (!JS_DefineElement(cx, list, (jsuint)count++, values[4], NULL, NULL, JSPROP_ENUMERATE)) goto out;
            } else if (kind == 3 && !answer) { *rval = JSVAL_FALSE; ok = JS_TRUE; goto out; }
            else if (kind == 4 && answer) { *rval = JSVAL_TRUE; ok = JS_TRUE; goto out; }
            else if (kind == 5 && answer) { *rval = values[4]; ok = JS_TRUE; goto out; }
            else if (kind == 6 && answer) { ok = js_NewNumberValue(cx, index, rval); goto out; }
        }
        index += step;
    }
    if (kind == 2) {
        if (!TypedSpeciesCreate(cx, source, js_BuiltinGlobal(cx, argv), count, &values[3])) goto out;
        target = JSVAL_TO_OBJECT(values[3]);
        for (index = 0; index < count; ++index) {
            if (!JS_GetElement(cx, list, (jsuint)index, &values[4]) ||
                !js_TypedArrayWrite(cx, target, index, values[4], &accepted)) goto out;
            if (!accepted) { BinaryTypeError(cx); goto out; }
        }
        *rval = values[3];
    } else *rval = kind >= 7 ? values[1] : kind == 1 ? values[0] :
                    kind == 3 ? JSVAL_TRUE : kind == 4 ? JSVAL_FALSE :
                    kind == 6 ? INT_TO_JSVAL(-1) : JSVAL_VOID;
    ok = JS_TRUE;
  out:
    JS_POP_TEMP_ROOT(cx, &argsRoot);
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}
#define TYPED_EACH(name, kind) \
static JSBool name(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) \
{ return TypedEach(cx, argc, argv, rval, kind); }
TYPED_EACH(TypedForEach, 0)
TYPED_EACH(TypedMap, 1)
TYPED_EACH(TypedFilter, 2)
TYPED_EACH(TypedEvery, 3)
TYPED_EACH(TypedSome, 4)
TYPED_EACH(TypedFind, 5)
TYPED_EACH(TypedFindIndex, 6)
TYPED_EACH(TypedReduce, 7)
TYPED_EACH(TypedReduceRight, 8)
#undef TYPED_EACH
static JSBool
TypedSearch(JSContext *cx, uintN argc, jsval *argv, jsval *rval, JSBool reverse)
{
    jsdouble length, index;
    JSObject *source = TypedReceiver(cx, argv[-1], &length);
    jsval value = JSVAL_VOID;
    JSTempValueRooter root;
    JSBool ok = JS_FALSE, equal;
    if (!source) return JS_FALSE;
    *rval = INT_TO_JSVAL(-1);
    if (!length) return JS_TRUE;
    index = reverse ? length - 1 : 0;
    if (argc > 1) {
        if (!js_ValueToNumber(cx, argv[1], &index)) return JS_FALSE;
        index = js_DoubleToInteger(index);
        if (index == 0) index = 0;
        if (reverse) index = index >= 0 ? JS_MIN(index, length - 1) : length + index;
        else index = index < 0 ? JS_MAX(length + index, 0) : index;
    }
    JS_PUSH_SINGLE_TEMP_ROOT(cx, value, &root);
    while (index >= 0 && index < length) {
        if (!TypedRead(cx, source, index, &root.u.value)) goto out;
        equal = js_StrictlyEqual(root.u.value, argc ? argv[0] : JSVAL_VOID);
        if (equal) { ok = js_NewNumberValue(cx, index, rval); goto out; }
        index += reverse ? -1 : 1;
    }
    ok = JS_TRUE;
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}
static JSBool TypedIndexOf(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{ return TypedSearch(cx, argc, argv, rval, JS_FALSE); }
static JSBool TypedLastIndexOf(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{ return TypedSearch(cx, argc, argv, rval, JS_TRUE); }
static JSBool
TypedJoinInternal(JSContext *cx, uintN argc, jsval *argv, jsval *rval, JSBool locale)
{
    jsdouble length, i;
    JSObject *source = TypedReceiver(cx, argv[-1], &length), *boxed;
    jsval values[4] = {JSVAL_VOID, JSVAL_VOID, JSVAL_VOID, JSVAL_VOID};
    JSTempValueRooter root;
    JSString *str;
    JSBool ok = JS_FALSE;
    if (!source) return JS_FALSE;
    JS_PUSH_TEMP_ROOT(cx, 4, values, &root);
    str = !locale && argc && !JSVAL_IS_VOID(argv[0]) ? js_ValueToString(cx, argv[0]) : JS_NewStringCopyZ(cx, ",");
    if (!str) goto out;
    values[0] = STRING_TO_JSVAL(str);
    str = JS_NewStringCopyZ(cx, "");
    if (!str) goto out;
    values[1] = STRING_TO_JSVAL(str);
    for (i = 0; i < length; ++i) {
        if (i) {
            str = js_ConcatStrings(cx, JSVAL_TO_STRING(values[1]), JSVAL_TO_STRING(values[0]));
            if (!str) goto out;
            values[1] = STRING_TO_JSVAL(str);
        }
        if (!TypedRead(cx, source, i, &values[2])) goto out;
        if (locale) {
            boxed = js_BuiltinToObject(cx, js_BuiltinGlobal(cx, argv), values[2]);
            if (!boxed) goto out;
            values[3] = OBJECT_TO_JSVAL(boxed);
            if (!JS_GetProperty(cx, boxed, "toLocaleString", &values[3]) ||
                !js_InternalInvokeValue(cx, values[2], values[3], 0, 0, NULL, &values[2])) goto out;
        }
        str = js_ValueToString(cx, values[2]);
        if (!str) goto out;
        values[2] = STRING_TO_JSVAL(str);
        str = js_ConcatStrings(cx, JSVAL_TO_STRING(values[1]), str);
        if (!str) goto out;
        values[1] = STRING_TO_JSVAL(str);
    }
    *rval = values[1]; ok = JS_TRUE;
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}
static JSBool TypedJoin(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{ return TypedJoinInternal(cx, argc, argv, rval, JS_FALSE); }
static JSBool TypedLocale(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{ return TypedJoinInternal(cx, argc, argv, rval, JS_TRUE); }

typedef struct TypedSortData {
    JSContext *cx;
    JSObject *obj;
    jsval callback;
} TypedSortData;
static JSBool
TypedCompare(void *context, const void *left, const void *right, int *order)
{
    TypedSortData *sort = (TypedSortData *)context;
    JSContext *cx = sort->cx;
    jsval values[3] = {*(const jsval *)left, *(const jsval *)right, JSVAL_VOID};
    JSTempValueRooter root;
    jsdouble x, y, number, length;
    JSBool ok = JS_FALSE;
    JS_PUSH_TEMP_ROOT(cx, 3, values, &root);
    if (!JSVAL_IS_VOID(sort->callback)) {
        if (!js_InternalInvokeValue(cx, JSVAL_VOID, sort->callback, 0, 2, values, &values[2]) ||
            !js_ValueToNumber(cx, values[2], &number) ||
            !js_TypedArrayLength(cx, sort->obj, &length)) goto out;
        *order = JSDOUBLE_IS_NaN(number) || number == 0 ? 0 : number < 0 ? -1 : 1;
    } else {
        if (!js_ValueToNumber(cx, values[0], &x) || !js_ValueToNumber(cx, values[1], &y)) goto out;
        *order = JSDOUBLE_IS_NaN(x) ? (JSDOUBLE_IS_NaN(y) ? 0 : 1) :
                 JSDOUBLE_IS_NaN(y) ? -1 : x < y ? -1 : x > y ? 1 :
                 JSDOUBLE_IS_NEGZERO(x) && !JSDOUBLE_IS_NEGZERO(y) ? -1 :
                 !JSDOUBLE_IS_NEGZERO(x) && JSDOUBLE_IS_NEGZERO(y) ? 1 : 0;
    }
    ok = JS_TRUE;
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}
static JSBool
TypedSort(JSContext *cx, JSObject *ignored, uintN argc, jsval *argv, jsval *rval)
{
    jsdouble length;
    JSObject *source = TypedReceiver(cx, argv[-1], &length);
    TypedSortData sort;
    jsval *values;
    JSTempValueRooter root;
    size_t i, count;
    JSBool ok = JS_FALSE, accepted;
    if (!source) return JS_FALSE;
    if (argc && !JSVAL_IS_VOID(argv[0]) && !js_IsCallable(cx, argv[0])) return BinaryTypeError(cx);
    if (length > JSVAL_INT_MAX - 1 || length + 1 > (jsdouble)((size_t)-1 / sizeof(jsval))) return BinaryRangeError(cx);
    count = (size_t)length;
    values = (jsval *)JS_malloc(cx, (count + 1) * sizeof(jsval));
    if (!values) return JS_FALSE;
    for (i = 0; i <= count; ++i) values[i] = JSVAL_VOID;
    JS_PUSH_TEMP_ROOT(cx, (jsint)count + 1, values, &root);
    for (i = 0; i < count; ++i) if (!TypedRead(cx, source, (jsdouble)i, &values[i])) goto out;
    sort.cx = cx; sort.obj = source; sort.callback = argc ? argv[0] : JSVAL_VOID;
    if (!js_HeapSort(values, count, values + count, sizeof(jsval), TypedCompare, &sort)) goto out;
    for (i = 0; i < count; ++i) {
        if (!js_TypedArrayWrite(cx, source, (jsdouble)i, values[i], &accepted)) goto out;
    }
    *rval = argv[-1]; ok = JS_TRUE;
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    JS_free(cx, values);
    return ok;
}
static JSBool
TypedMethods(JSContext *cx, JSObject *proto)
{
    static struct { const char *name; JSNative native; uintN length; } methods[] = {
        {"set", TypedSetMethod, 1},
        {"subarray", TypedSubarray, 2},
        {"slice", TypedSlice, 2},
        {"copyWithin", TypedCopyWithin, 2},
        {"fill", TypedFill, 1},
        {"reverse", TypedReverse, 0},
        {"sort", TypedSort, 1},
        {"forEach", TypedForEach, 1},
        {"map", TypedMap, 1},
        {"filter", TypedFilter, 1},
        {"every", TypedEvery, 1},
        {"some", TypedSome, 1},
        {"find", TypedFind, 1},
        {"findIndex", TypedFindIndex, 1},
        {"reduce", TypedReduce, 1},
        {"reduceRight", TypedReduceRight, 1},
        {"indexOf", TypedIndexOf, 1},
        {"lastIndexOf", TypedLastIndexOf, 1},
        {"join", TypedJoin, 1},
        {"toLocaleString", TypedLocale, 0},
        {NULL, NULL, 0}
    };
    uintN i;
    for (i = 0; methods[i].name; ++i) {
        if (!JS_DefineFunction(cx, proto, methods[i].name, methods[i].native,
                               methods[i].length, JSFUN_STRICT | JSFUN_NO_CONSTRUCT)) return JS_FALSE;
    }
    return JS_TRUE;
}

static JSObject *
TypedBase(JSContext *cx, JSObject *global)
{
    JSObject *ctor = js_GetCachedIntrinsic(cx, global, JS_INTRINSIC_TYPED_ARRAY_CONSTRUCTOR), *proto;
    JSFunction *fun;
    jsval values[4] = {JSVAL_VOID, JSVAL_VOID, JSVAL_VOID, JSVAL_VOID};
    JSTempValueRooter root;
    jsid id;
    JSBool ok = JS_FALSE;
    if (ctor) return ctor;
    JS_PUSH_TEMP_ROOT(cx, 4, values, &root);
    proto = js_BuiltinPrototype(cx, global, JSProto_Object);
    if (!proto) goto out;
    proto = js_NewObject(cx, &js_ObjectClass, proto, global);
    if (!proto) goto out;
    values[0] = OBJECT_TO_JSVAL(proto);
    fun = JS_NewFunction(cx, TypedConstructor, 0, JSFUN_STRICT, global, "TypedArray");
    if (!fun) goto out;
    ctor = fun->object; values[1] = OBJECT_TO_JSVAL(ctor);
    if (!JS_DefineProperty(cx, ctor, "prototype", values[0], NULL, NULL, JSPROP_PERMANENT | JSPROP_READONLY) ||
        !JS_DefineProperty(cx, proto, "constructor", values[1], NULL, NULL, 0) ||
        !BinaryGetter(cx, global, proto, "buffer", TypedBufferGetter, JS_FALSE) ||
        !BinaryGetter(cx, global, proto, "length", TypedLengthGetter, JS_FALSE) ||
        !BinaryGetter(cx, global, proto, "byteLength", TypedByteLengthGetter, JS_FALSE) ||
        !BinaryGetter(cx, global, proto, "byteOffset", TypedOffsetGetter, JS_FALSE) ||
        !BinaryGetter(cx, global, ctor, "[Symbol.species]", BufferSpecies, JS_TRUE) ||
        !JS_DefineFunction(cx, ctor, "from", TypedStaticFrom, 1, JSFUN_STRICT | JSFUN_NO_CONSTRUCT) ||
        !JS_DefineFunction(cx, ctor, "of", TypedStaticOf, 0, JSFUN_STRICT | JSFUN_NO_CONSTRUCT) ||
        !JS_DefineFunction(cx, proto, "keys", TypedKeys, 0, JSFUN_STRICT | JSFUN_NO_CONSTRUCT) ||
        !JS_DefineFunction(cx, proto, "entries", TypedEntries, 0, JSFUN_STRICT | JSFUN_NO_CONSTRUCT)) goto out;
    fun = JS_DefineFunction(cx, proto, "values", TypedValues, 0, JSFUN_STRICT | JSFUN_NO_CONSTRUCT);
    if (!fun) goto out;
    values[2] = OBJECT_TO_JSVAL(fun->object);
    if (!js_WellKnownSymbolId(cx, JS_WKS_ITERATOR, &id) ||
        !OBJ_DEFINE_PROPERTY(cx, proto, id, values[2], NULL, NULL, 0, NULL)) goto out;
    fun = JS_NewFunction(cx, TypedTagGetter, 0, JSFUN_STRICT | JSFUN_NO_CONSTRUCT, global, "get [Symbol.toStringTag]");
    if (!fun) goto out;
    values[2] = OBJECT_TO_JSVAL(fun->object);
    if (!js_WellKnownSymbolId(cx, JS_WKS_TO_STRING_TAG, &id) ||
        !OBJ_DEFINE_PROPERTY(cx, proto, id, JSVAL_VOID, (JSPropertyOp)fun->object,
                             NULL, JSPROP_GETTER | JSPROP_SHARED, NULL)) goto out;
    {
        JSObject *arrayProto = js_BuiltinPrototype(cx, global, JSProto_Array);
        if (!arrayProto || !JS_GetProperty(cx, arrayProto, "toString", &values[3]) ||
            !JS_DefineProperty(cx, proto, "toString", values[3], NULL, NULL, 0)) goto out;
    }
    ok = TypedMethods(cx, proto) &&
         js_CacheIntrinsic(cx, global, JS_INTRINSIC_TYPED_ARRAY_PROTO, proto) &&
         js_CacheIntrinsic(cx, global, JS_INTRINSIC_TYPED_ARRAY_CONSTRUCTOR, ctor);
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok ? ctor : NULL;
}
static JSObject *
InitTypedClass(JSContext *cx, JSObject *global, uintN kind)
{
    JSObject *base, *proto, *ctor;
    JSFunction *fun;
    jsval values[3] = {JSVAL_VOID, JSVAL_VOID, JSVAL_VOID};
    JSTempValueRooter root;
    JSBool ok = JS_FALSE;
    JSVersion saved;
    JSProtoKey key = (JSProtoKey)JSCLASS_CACHED_PROTO_KEY(typedClasses[kind]);
    ctor = js_GetCachedClassObject(cx, global, key);
    if (ctor) return js_BuiltinPrototype(cx, global, key);
    saved = JS_SetVersion(cx, JSVERSION_ECMA_2015);
    JS_PUSH_TEMP_ROOT(cx, 3, values, &root);
    base = TypedBase(cx, global);
    if (!base) goto out;
    values[0] = OBJECT_TO_JSVAL(base);
    proto = js_GetCachedIntrinsic(cx, global, JS_INTRINSIC_TYPED_ARRAY_PROTO);
    proto = js_NewObject(cx, &js_ObjectClass, proto, global);
    if (!proto) goto out;
    values[1] = OBJECT_TO_JSVAL(proto);
    fun = JS_NewFunction(cx, typedConstructors[kind], 3, JSFUN_STRICT, global, typedNames[kind]);
    if (!fun) goto out;
    ctor = fun->object; values[2] = OBJECT_TO_JSVAL(ctor);
    ok = JS_SetPrototype(cx, ctor, base) &&
         JS_DefineProperty(cx, ctor, "prototype", values[1], NULL, NULL, JSPROP_PERMANENT | JSPROP_READONLY) &&
         JS_DefineProperty(cx, proto, "constructor", values[2], NULL, NULL, 0) &&
         JS_DefineProperty(cx, ctor, "BYTES_PER_ELEMENT", INT_TO_JSVAL(typedSizes[kind]), NULL, NULL, JSPROP_PERMANENT | JSPROP_READONLY) &&
         JS_DefineProperty(cx, proto, "BYTES_PER_ELEMENT", INT_TO_JSVAL(typedSizes[kind]), NULL, NULL, JSPROP_PERMANENT | JSPROP_READONLY) &&
         js_CacheClassObject(cx, global, key, ctor) &&
         JS_DefineProperty(cx, global, typedNames[kind], values[2], NULL, NULL, 0);
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    JS_SetVersion(cx, saved);
    return ok ? proto : NULL;
}
#define TYPED_INIT(name, kind) \
JSObject *js_Init##name##Class(JSContext *cx, JSObject *global) \
{ return InitTypedClass(cx, global, kind); }
TYPED_INIT(Int8Array, 0)
TYPED_INIT(Uint8Array, 1)
TYPED_INIT(Uint8ClampedArray, 2)
TYPED_INIT(Int16Array, 3)
TYPED_INIT(Uint16Array, 4)
TYPED_INIT(Int32Array, 5)
TYPED_INIT(Uint32Array, 6)
TYPED_INIT(Float32Array, 7)
TYPED_INIT(Float64Array, 8)
#undef TYPED_INIT
