/* ES2015 ArrayBuffer and DataView for the classic embedding API.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
#include <stdlib.h>
#include <string.h>
#include "jsapi.h"
#include "jsarray.h"
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
    if (!js_ValueToNumber(cx, argv[1], &offset)) return JS_FALSE;
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
    *rval = BOOLEAN_TO_JSVAL(GetView(cx, argv[0]) != NULL); return JS_TRUE;
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
                         js_DataViewConstructor, 3, NULL, NULL, NULL, NULL);
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
