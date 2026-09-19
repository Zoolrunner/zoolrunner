/* ES2015 Reflect namespace; MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "jsapi.h"
#include "jsarray.h"
#include "jsatom.h"
#include "jscntxt.h"
#include "jsfun.h"
#include "jsgc.h"
#include "jsinterp.h"
#include "jsobj.h"
#include "jsreflect.h"
#include "jsproxy.h"
#include "jssymbol.h"

JSClass js_ReflectClass = {
    "Reflect", JSCLASS_HAS_CACHED_PROTO(JSProto_Reflect),
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

typedef struct ReflectIdRoot {
    JSTempValueRooter root;
    jsid id;
} ReflectIdRoot;
JS_STATIC_DLL_CALLBACK(void)
MarkReflectId(JSContext *cx, JSTempValueRooter *root)
{
    jsid id = ((ReflectIdRoot *)root)->id;
    if (JSID_IS_ATOM(id)) js_MarkAtom(cx, JSID_TO_ATOM(id));
}
static JSObject *
RequireTarget(JSContext *cx, uintN argc, jsval *argv)
{
    if (!argc || JSVAL_IS_PRIMITIVE(argv[0])) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_OBJECT_REQUIRED);
        return NULL;
    }
    return JSVAL_TO_OBJECT(argv[0]);
}
JSBool
js_ReflectGet(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSObject *target = RequireTarget(cx, argc, argv);
    ReflectIdRoot id;
    JSBool ok;
    if (!target || !js_ValueToPropertyId(cx, argv[1], &id.id)) return JS_FALSE;
    JS_PUSH_TEMP_ROOT_MARKER(cx, MarkReflectId, &id.root);
    if (js_IsProxy(cx, target))
        ok = js_ProxyGet(cx, target, id.id, argc > 2 ? argv[2] : argv[0], rval);
    else if (target->map->ops->getProperty == js_GetProperty)
        ok = js_GetPropertyValue(cx, target, argc > 2 ? argv[2] : argv[0], id.id, rval);
    else
        ok = OBJ_GET_PROPERTY(cx, target, id.id, rval);
    JS_POP_TEMP_ROOT(cx, &id.root);
    return ok;
}
static JSBool
ReflectHas(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSObject *target = RequireTarget(cx, argc, argv), *owner;
    JSProperty *property;
    ReflectIdRoot id;
    JSBool ok;
    if (!target || !js_ValueToPropertyId(cx, argv[1], &id.id)) return JS_FALSE;
    JS_PUSH_TEMP_ROOT_MARKER(cx, MarkReflectId, &id.root);
    ok = OBJ_LOOKUP_PROPERTY(cx, target, id.id, &owner, &property);
    if (ok) {
        *rval = BOOLEAN_TO_JSVAL(property != NULL);
        if (property) OBJ_DROP_PROPERTY(cx, owner, property);
    }
    JS_POP_TEMP_ROOT(cx, &id.root);
    return ok;
}
static JSBool
ReflectInvoke(JSContext *cx, uintN argc, jsval *argv, jsval *rval, JSBool construct)
{
    JSObject *list, *newTarget = NULL;
    jsval *base, *oldsp, listValue;
    jsdouble length;
    JSStackFrame *frame = cx->fp;
    void *mark;
    uintN count, i;
    JSBool ok = JS_FALSE;
    if (!(construct ? js_IsConstructor(cx, argv[0]) : js_IsCallable(cx, argv[0]))) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_NOT_FUNCTION, "Reflect target");
        return JS_FALSE;
    }
    if (construct) {
        jsval value = argc > 2 ? argv[2] : argv[0];
        if (!js_IsConstructor(cx, value)) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_NOT_FUNCTION, "newTarget");
            return JS_FALSE;
        }
        newTarget = JSVAL_TO_OBJECT(value);
    }
    listValue = construct ? argv[1] : argv[2];
    if (JSVAL_IS_PRIMITIVE(listValue)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_OBJECT_REQUIRED);
        return JS_FALSE;
    }
    list = JSVAL_TO_OBJECT(listValue);
    if (!js_ArrayLikeLength(cx, list, &length)) return JS_FALSE;
    if (length >= ARRAY_INIT_LIMIT) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_TOO_MANY_FUN_ARGS);
        return JS_FALSE;
    }
    count = (uintN)length;
    base = js_AllocStack(cx, 2 + count, &mark);
    if (!base) return JS_FALSE;
    for (i = 0; i < count + 2; ++i) base[i] = JSVAL_VOID;
    base[0] = argv[0]; base[1] = construct ? JSVAL_NULL : argv[1];
    for (i = 0; i < count; ++i) {
        if (!JS_GetElement(cx, list, i, &base[2 + i])) goto out;
    }
    oldsp = frame->sp;
    frame->sp = base + count + 2;
    ok = construct ? js_InternalInvokeConstructorWithNewTarget(cx, base, count, newTarget)
                   : js_Invoke(cx, count, JSINVOKE_INTERNAL | JSINVOKE_SKIP_CALLER);
    if (ok) *rval = base[0];
    frame->sp = oldsp;
  out:
    js_FreeStack(cx, mark);
    return ok;
}
static JSBool ReflectApply(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{ return ReflectInvoke(cx, argc, argv, rval, JS_FALSE); }
static JSBool ReflectConstruct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{ return ReflectInvoke(cx, argc, argv, rval, JS_TRUE); }

typedef struct ReflectMethod { const char *name; JSNative native; uintN length; } ReflectMethod;
static const ReflectMethod methods[] = {
    {"apply", ReflectApply, 3}, {"construct", ReflectConstruct, 2},
    {"defineProperty", js_ReflectDefineProperty, 3},
    {"deleteProperty", js_ReflectDeleteProperty, 2},
    {"enumerate", js_ReflectEnumerate, 1},
    {"get", js_ReflectGet, 2}, {"getOwnPropertyDescriptor", js_ReflectGetOwnPropertyDescriptor, 2},
    {"getPrototypeOf", js_ReflectGetPrototypeOf, 1}, {"has", ReflectHas, 2},
    {"isExtensible", js_ReflectIsExtensible, 1}, {"ownKeys", js_ReflectOwnKeys, 1},
    {"preventExtensions", js_ReflectPreventExtensions, 1}, {"set", js_ReflectSet, 3},
    {"setPrototypeOf", js_ReflectSetPrototypeOf, 2}
};
JSObject *
js_InitReflectClass(JSContext *cx, JSObject *global)
{
    JSObject *obj = JS_InitClass(cx, global, NULL, &js_ReflectClass, NULL, 0,
                                 NULL, NULL, NULL, NULL);
    JSTempValueRooter root;
    uintN i;
    JSBool ok = JS_FALSE;
    if (!obj) return NULL;
    JS_PUSH_TEMP_ROOT_OBJECT(cx, obj, &root);
    for (i = 0; i < sizeof(methods) / sizeof(methods[0]); ++i) {
        if (!JS_DefineFunction(cx, obj, methods[i].name, methods[i].native, methods[i].length,
                               JSFUN_STRICT | JSFUN_NO_CONSTRUCT)) goto out;
    }
    ok = js_DefineBuiltinTag(cx, obj, "Reflect");
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok ? obj : NULL;
}
