/* ES2015 Proxy internal operations. MPL 1.1/GPL 2.0/LGPL 2.1. */
#include <string.h>
#include "jsapi.h"
#include "jsarray.h"
#include "jsatom.h"
#include "jscntxt.h"
#include "jsfun.h"
#include "jsgc.h"
#include "jsinterp.h"
#include "jsiteres6.h"
#include "jslock.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsproxy.h"
#include "jsrealm.h"
#include "jsreflect.h"
#include "jsscope.h"
#include "jsscript.h"
#include "jsstr.h"
#include "jssymbol.h"

enum {
    P_PROXY, P_TARGET, P_HANDLER, P_TRAP, P_KEY, P_RESULT,
    P_DESC, P_EXTRA, P_RECORD, P_ARG0, P_ARG1, P_ARG2, P_ARG3, P_COUNT
};
typedef struct ProxyRoots {
    JSTempValueRooter root;
    jsval v[P_COUNT];
} ProxyRoots;


static JSBool
ProxyError(JSContext *cx, const char *operation)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_INCOMPATIBLE_PROTO,
                         "Proxy", operation, "target or handler");
    return JS_FALSE;
}

JSBool
js_IsProxy(JSContext *cx, JSObject *obj)
{
    return OBJ_GET_CLASS(cx, obj) == &js_ProxyClass;
}

static void
RootProxy(JSContext *cx, JSObject *obj, ProxyRoots *roots)
{
    uintN i;
    for (i = 0; i < P_COUNT; ++i) roots->v[i] = JSVAL_VOID;
    roots->v[P_PROXY] = OBJECT_TO_JSVAL(obj);
    JS_PUSH_TEMP_ROOT(cx, P_COUNT, roots->v, &roots->root);
}

/* Capture both slots before looking up a trap. Revocation from a trap getter
 * must not invalidate the target and handler of the operation already begun. */
static JSBool
GetTrap(JSContext *cx, JSObject *obj, const char *name, ProxyRoots *roots)
{
    int stackDummy;
    if (!JS_CHECK_STACK_SIZE(cx, stackDummy)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_OVER_RECURSED);
        return JS_FALSE;
    }
    JS_LOCK_GC(cx->runtime);
    roots->v[P_TARGET] = obj->slots[JSSLOT_START(&js_ProxyClass)];
    roots->v[P_HANDLER] = obj->slots[JSSLOT_START(&js_ProxyClass) + 1];
    JS_UNLOCK_GC(cx->runtime);
    if (JSVAL_IS_PRIMITIVE(roots->v[P_TARGET]) ||
        JSVAL_IS_PRIMITIVE(roots->v[P_HANDLER])) return ProxyError(cx, "revoked");
    if (!JS_GetProperty(cx, JSVAL_TO_OBJECT(roots->v[P_HANDLER]), name,
                        &roots->v[P_TRAP])) return JS_FALSE;
    if (JSVAL_IS_NULL(roots->v[P_TRAP])) roots->v[P_TRAP] = JSVAL_VOID;
    if (!JSVAL_IS_VOID(roots->v[P_TRAP]) && !js_IsCallable(cx, roots->v[P_TRAP]))
        return ProxyError(cx, name);
    return JS_TRUE;
}

static JSBool
PropertyKey(JSContext *cx, jsid id, ProxyRoots *roots)
{
    JSString *str;
    roots->v[P_KEY] = ID_TO_VALUE(id);
    if (JSVAL_IS_SYMBOL(roots->v[P_KEY]) || JSVAL_IS_STRING(roots->v[P_KEY]))
        return JS_TRUE;
    str = js_ValueToString(cx, roots->v[P_KEY]);
    if (!str) return JS_FALSE;
    roots->v[P_KEY] = STRING_TO_JSVAL(str);
    return JS_TRUE;
}

static JSBool
CallTrap(JSContext *cx, ProxyRoots *roots, uintN argc)
{
    roots->v[P_ARG0] = roots->v[P_TARGET];
    return js_InternalInvokeValue(cx, roots->v[P_HANDLER], roots->v[P_TRAP],
                                   0, argc, &roots->v[P_ARG0], &roots->v[P_RESULT]);
}

static JSObject *
ProxyGlobal(JSContext *cx, JSObject *obj)
{
    JSObject *parent;
    while ((parent = OBJ_GET_PARENT(cx, obj)) != NULL) obj = parent;
    return obj;
}

/* Internal ordinary operations use C entry points, never mutable public
 * Reflect methods. The prefix supplies a realm for result allocations. */
static JSBool
TargetOperation(JSContext *cx, ProxyRoots *roots, JSNative native, uintN argc,
                 jsval second, jsval third, jsval fourth, jsval *rval)
{
    jsval args[6];
    JSTempValueRooter root;
    JSBool ok;
    args[0] = OBJECT_TO_JSVAL(js_ProxyOperationGlobal(cx));
    args[1] = JSVAL_VOID;
    args[2] = roots->v[P_TARGET]; args[3] = second;
    args[4] = third; args[5] = fourth;
    JS_PUSH_TEMP_ROOT(cx, 6, args, &root);
    ok = native(cx, NULL, argc, args + 2, rval);
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}

static JSBool
TargetDescriptor(JSContext *cx, ProxyRoots *roots)
{
    return TargetOperation(cx, roots, js_ReflectGetOwnPropertyDescriptor, 2,
                            roots->v[P_KEY], JSVAL_VOID, JSVAL_VOID, &roots->v[P_DESC]);
}

/* Descriptor objects produced by the engine must not acquire descriptor
 * fields from a polluted Object.prototype. */
static JSBool
DescriptorField(JSContext *cx, jsval desc, const char *name, jsval *value)
{
    JSObject *obj = JSVAL_TO_OBJECT(desc);
    JSAtom *atom = js_Atomize(cx, name, strlen(name), 0);
    JSScope *scope;
    JSBool found;
    if (!atom) return JS_FALSE;
    JS_ASSERT(OBJ_IS_NATIVE(obj));
    JS_LOCK_OBJ(cx, obj);
    scope = OBJ_SCOPE(obj);
    found = scope->object == obj && SCOPE_GET_PROPERTY(scope, ATOM_TO_JSID(atom));
    JS_UNLOCK_OBJ(cx, obj);
    if (!found) { *value = JSVAL_VOID; return JS_TRUE; }
    return JS_GetProperty(cx, obj, name, value);
}

static JSBool
SameValue(jsval a, jsval b)
{
    jsdouble x, y;
    if (JSVAL_IS_NUMBER(a) && JSVAL_IS_NUMBER(b)) {
        x = JSVAL_IS_INT(a) ? JSVAL_TO_INT(a) : *JSVAL_TO_DOUBLE(a);
        y = JSVAL_IS_INT(b) ? JSVAL_TO_INT(b) : *JSVAL_TO_DOUBLE(b);
        return (JSDOUBLE_IS_NaN(x) && JSDOUBLE_IS_NaN(y)) ||
               (x == y && (x != 0 || JSDOUBLE_IS_NEGZERO(x) == JSDOUBLE_IS_NEGZERO(y)));
    }
    if (JSVAL_IS_STRING(a) && JSVAL_IS_STRING(b))
        return js_EqualStrings(JSVAL_TO_STRING(a), JSVAL_TO_STRING(b));
    return a == b;
}

JSBool
js_ProxyTarget(JSContext *cx, JSObject *obj, JSObject **target)
{
    jsval value, handler;
    JS_LOCK_GC(cx->runtime);
    value = obj->slots[JSSLOT_START(&js_ProxyClass)];
    handler = obj->slots[JSSLOT_START(&js_ProxyClass) + 1];
    JS_UNLOCK_GC(cx->runtime);
    if (JSVAL_IS_PRIMITIVE(value) || JSVAL_IS_PRIMITIVE(handler))
        return ProxyError(cx, "revoked");
    *target = JSVAL_TO_OBJECT(value);
    return JS_TRUE;
}

JSBool
js_ProxyGet(JSContext *cx, JSObject *obj, jsid id, jsval receiver, jsval *rval)
{
    ProxyRoots roots;
    JSObject *target;
    JSBool ok = JS_FALSE;
    RootProxy(cx, obj, &roots);
    roots.v[P_ARG2] = receiver;
    if (!PropertyKey(cx, id, &roots) || !GetTrap(cx, obj, "get", &roots)) goto out;
    target = JSVAL_TO_OBJECT(roots.v[P_TARGET]);
    if (JSVAL_IS_VOID(roots.v[P_TRAP])) {
        if (js_IsProxy(cx, target)) ok = js_ProxyGet(cx, target, id, receiver, rval);
        else if (target->map->ops->getProperty == js_GetProperty)
            ok = js_GetPropertyValue(cx, target, receiver, id, rval);
        else ok = OBJ_GET_PROPERTY(cx, target, id, rval);
        goto out;
    }
    roots.v[P_ARG1] = roots.v[P_KEY];
    if (!CallTrap(cx, &roots, 3) || !TargetDescriptor(cx, &roots)) goto out;
    if (!JSVAL_IS_VOID(roots.v[P_DESC])) {
        if (!DescriptorField(cx, roots.v[P_DESC], "configurable", &roots.v[P_EXTRA])) goto out;
        if (roots.v[P_EXTRA] == JSVAL_FALSE) {
            if (!DescriptorField(cx, roots.v[P_DESC], "writable", &roots.v[P_EXTRA])) goto out;
            if (roots.v[P_EXTRA] == JSVAL_FALSE) {
                if (!DescriptorField(cx, roots.v[P_DESC], "value", &roots.v[P_EXTRA])) goto out;
                if (!SameValue(roots.v[P_RESULT], roots.v[P_EXTRA])) goto invariant;
            } else if (JSVAL_IS_VOID(roots.v[P_EXTRA])) {
                if (!DescriptorField(cx, roots.v[P_DESC], "get", &roots.v[P_EXTRA])) goto out;
                if (JSVAL_IS_VOID(roots.v[P_EXTRA]) && !JSVAL_IS_VOID(roots.v[P_RESULT])) goto invariant;
            }
        }
    }
    *rval = roots.v[P_RESULT]; ok = JS_TRUE;
    goto out;
  invariant:
    ProxyError(cx, "get invariant");
  out:
    JS_POP_TEMP_ROOT(cx, &roots.root);
    return ok;
}

JSBool
js_ProxySet(JSContext *cx, JSObject *obj, jsid id, jsval value,
             jsval receiver, JSBool *accepted)
{
    ProxyRoots roots;
    JSBool ok = JS_FALSE, boolean;
    RootProxy(cx, obj, &roots);
    roots.v[P_ARG2] = value; roots.v[P_ARG3] = receiver;
    if (!PropertyKey(cx, id, &roots) || !GetTrap(cx, obj, "set", &roots)) goto out;
    if (JSVAL_IS_VOID(roots.v[P_TRAP])) {
        ok = TargetOperation(cx, &roots, js_ReflectSet, 4, roots.v[P_KEY], value,
                              receiver, &roots.v[P_RESULT]);
        if (ok) *accepted = roots.v[P_RESULT] == JSVAL_TRUE;
        goto out;
    }
    roots.v[P_ARG1] = roots.v[P_KEY];
    if (!CallTrap(cx, &roots, 4) || !JS_ValueToBoolean(cx, roots.v[P_RESULT], &boolean)) goto out;
    if (!boolean) { *accepted = JS_FALSE; ok = JS_TRUE; goto out; }
    if (!TargetDescriptor(cx, &roots)) goto out;
    if (!JSVAL_IS_VOID(roots.v[P_DESC])) {
        if (!DescriptorField(cx, roots.v[P_DESC], "configurable", &roots.v[P_EXTRA])) goto out;
        if (roots.v[P_EXTRA] == JSVAL_FALSE) {
            if (!DescriptorField(cx, roots.v[P_DESC], "writable", &roots.v[P_EXTRA])) goto out;
            if (roots.v[P_EXTRA] == JSVAL_FALSE) {
                if (!DescriptorField(cx, roots.v[P_DESC], "value", &roots.v[P_EXTRA])) goto out;
                if (!SameValue(value, roots.v[P_EXTRA])) goto invariant;
            } else if (JSVAL_IS_VOID(roots.v[P_EXTRA])) {
                if (!DescriptorField(cx, roots.v[P_DESC], "set", &roots.v[P_EXTRA])) goto out;
                if (JSVAL_IS_VOID(roots.v[P_EXTRA])) goto invariant;
            }
        }
    }
    *accepted = JS_TRUE; ok = JS_TRUE;
    goto out;
  invariant:
    ProxyError(cx, "set invariant");
  out:
    JS_POP_TEMP_ROOT(cx, &roots.root);
    return ok;
}

JSBool
js_ProxyHas(JSContext *cx, JSObject *obj, jsid id, JSBool *found)
{
    ProxyRoots roots;
    JSObject *owner, *target;
    JSProperty *property;
    JSBool ok = JS_FALSE, boolean;
    RootProxy(cx, obj, &roots);
    if (!PropertyKey(cx, id, &roots) || !GetTrap(cx, obj, "has", &roots)) goto out;
    target = JSVAL_TO_OBJECT(roots.v[P_TARGET]);
    if (JSVAL_IS_VOID(roots.v[P_TRAP])) {
        if (js_IsProxy(cx, target)) ok = js_ProxyHas(cx, target, id, found);
        else {
            ok = OBJ_LOOKUP_PROPERTY(cx, target, id, &owner, &property);
            if (ok) {
                *found = property != NULL;
                if (property) OBJ_DROP_PROPERTY(cx, owner, property);
            }
        }
        goto out;
    }
    roots.v[P_ARG1] = roots.v[P_KEY];
    if (!CallTrap(cx, &roots, 2) || !JS_ValueToBoolean(cx, roots.v[P_RESULT], &boolean)) goto out;
    if (!boolean) {
        if (!TargetDescriptor(cx, &roots)) goto out;
        if (!JSVAL_IS_VOID(roots.v[P_DESC])) {
            if (!DescriptorField(cx, roots.v[P_DESC], "configurable", &roots.v[P_EXTRA])) goto out;
            if (roots.v[P_EXTRA] == JSVAL_FALSE) goto invariant;
            if (!TargetOperation(cx, &roots, js_ReflectIsExtensible, 1, JSVAL_VOID,
                                  JSVAL_VOID, JSVAL_VOID, &roots.v[P_EXTRA])) goto out;
            if (roots.v[P_EXTRA] == JSVAL_FALSE) goto invariant;
        }
    }
    *found = boolean; ok = JS_TRUE;
    goto out;
  invariant:
    ProxyError(cx, "has invariant");
  out:
    JS_POP_TEMP_ROOT(cx, &roots.root);
    return ok;
}

JSBool
js_ProxyDelete(JSContext *cx, JSObject *obj, jsid id, JSBool *accepted)
{
    ProxyRoots roots;
    JSBool ok = JS_FALSE, boolean;
    RootProxy(cx, obj, &roots);
    if (!PropertyKey(cx, id, &roots) || !GetTrap(cx, obj, "deleteProperty", &roots)) goto out;
    if (JSVAL_IS_VOID(roots.v[P_TRAP])) {
        ok = TargetOperation(cx, &roots, js_ReflectDeleteProperty, 2, roots.v[P_KEY],
                              JSVAL_VOID, JSVAL_VOID, &roots.v[P_RESULT]);
        if (ok) *accepted = roots.v[P_RESULT] == JSVAL_TRUE;
        goto out;
    }
    roots.v[P_ARG1] = roots.v[P_KEY];
    if (!CallTrap(cx, &roots, 2) || !JS_ValueToBoolean(cx, roots.v[P_RESULT], &boolean)) goto out;
    if (boolean) {
        if (!TargetDescriptor(cx, &roots)) goto out;
        if (!JSVAL_IS_VOID(roots.v[P_DESC])) {
            if (!DescriptorField(cx, roots.v[P_DESC], "configurable", &roots.v[P_EXTRA])) goto out;
            if (roots.v[P_EXTRA] == JSVAL_FALSE) { ProxyError(cx, "delete invariant"); goto out; }
        }
    }
    *accepted = boolean; ok = JS_TRUE;
  out:
    JS_POP_TEMP_ROOT(cx, &roots.root);
    return ok;
}

static JSBool
BooleanOperation(JSContext *cx, JSObject *obj, JSBool prevent, JSBool *result)
{
    ProxyRoots roots;
    JSBool ok = JS_FALSE, answer;
    RootProxy(cx, obj, &roots);
    if (!GetTrap(cx, obj, prevent ? "preventExtensions" : "isExtensible", &roots)) goto out;
    if (JSVAL_IS_VOID(roots.v[P_TRAP])) {
        ok = TargetOperation(cx, &roots, prevent ? js_ReflectPreventExtensions : js_ReflectIsExtensible,
                              1, JSVAL_VOID, JSVAL_VOID, JSVAL_VOID, &roots.v[P_RESULT]);
        if (ok) *result = roots.v[P_RESULT] == JSVAL_TRUE;
        goto out;
    }
    if (!CallTrap(cx, &roots, 1) || !JS_ValueToBoolean(cx, roots.v[P_RESULT], &answer)) goto out;
    if (!prevent || answer) {
        if (!TargetOperation(cx, &roots, js_ReflectIsExtensible, 1, JSVAL_VOID,
                              JSVAL_VOID, JSVAL_VOID, &roots.v[P_EXTRA])) goto out;
        if (prevent ? roots.v[P_EXTRA] == JSVAL_TRUE
                    : (roots.v[P_EXTRA] == JSVAL_TRUE) != answer) {
            ProxyError(cx, "extensibility invariant");
            goto out;
        }
    }
    *result = answer; ok = JS_TRUE;
  out:
    JS_POP_TEMP_ROOT(cx, &roots.root);
    return ok;
}

JSBool
js_ProxyIsExtensible(JSContext *cx, JSObject *obj, JSBool *result)
{
    return BooleanOperation(cx, obj, JS_FALSE, result);
}

JSBool
js_ProxyPreventExtensions(JSContext *cx, JSObject *obj, JSBool *result)
{
    return BooleanOperation(cx, obj, JS_TRUE, result);
}

JSBool
js_ProxyGetPrototype(JSContext *cx, JSObject *obj, JSObject **proto)
{
    ProxyRoots roots;
    JSBool ok = JS_FALSE;
    RootProxy(cx, obj, &roots);
    if (!GetTrap(cx, obj, "getPrototypeOf", &roots)) goto out;
    if (JSVAL_IS_VOID(roots.v[P_TRAP])) {
        if (!TargetOperation(cx, &roots, js_ReflectGetPrototypeOf, 1, JSVAL_VOID,
                              JSVAL_VOID, JSVAL_VOID, &roots.v[P_RESULT])) goto out;
    } else {
        if (!CallTrap(cx, &roots, 1)) goto out;
        if (!JSVAL_IS_OBJECT(roots.v[P_RESULT])) {
            ProxyError(cx, "getPrototypeOf result"); goto out;
        }
        if (!TargetOperation(cx, &roots, js_ReflectIsExtensible, 1, JSVAL_VOID,
                              JSVAL_VOID, JSVAL_VOID, &roots.v[P_EXTRA])) goto out;
        if (roots.v[P_EXTRA] == JSVAL_FALSE) {
            if (!TargetOperation(cx, &roots, js_ReflectGetPrototypeOf, 1, JSVAL_VOID,
                                  JSVAL_VOID, JSVAL_VOID, &roots.v[P_EXTRA])) goto out;
            if (roots.v[P_RESULT] != roots.v[P_EXTRA]) {
                ProxyError(cx, "getPrototypeOf invariant"); goto out;
            }
        }
    }
    *proto = JSVAL_TO_OBJECT(roots.v[P_RESULT]); ok = JS_TRUE;
  out:
    JS_POP_TEMP_ROOT(cx, &roots.root);
    return ok;
}

JSBool
js_ProxySetPrototype(JSContext *cx, JSObject *obj, JSObject *proto, JSBool *accepted)
{
    ProxyRoots roots;
    JSBool ok = JS_FALSE, answer;
    RootProxy(cx, obj, &roots);
    roots.v[P_ARG1] = OBJECT_TO_JSVAL(proto);
    if (!GetTrap(cx, obj, "setPrototypeOf", &roots)) goto out;
    if (JSVAL_IS_VOID(roots.v[P_TRAP])) {
        ok = TargetOperation(cx, &roots, js_ReflectSetPrototypeOf, 2, roots.v[P_ARG1],
                              JSVAL_VOID, JSVAL_VOID, &roots.v[P_RESULT]);
        if (ok) *accepted = roots.v[P_RESULT] == JSVAL_TRUE;
        goto out;
    }
    if (!CallTrap(cx, &roots, 2) || !JS_ValueToBoolean(cx, roots.v[P_RESULT], &answer)) goto out;
    /* ES2015 performs these target queries even when the trap returns false. */
    if (!TargetOperation(cx, &roots, js_ReflectIsExtensible, 1, JSVAL_VOID,
                          JSVAL_VOID, JSVAL_VOID, &roots.v[P_EXTRA])) goto out;
    if (roots.v[P_EXTRA] == JSVAL_FALSE) {
        if (!TargetOperation(cx, &roots, js_ReflectGetPrototypeOf, 1, JSVAL_VOID,
                              JSVAL_VOID, JSVAL_VOID, &roots.v[P_EXTRA])) goto out;
        if (answer && roots.v[P_ARG1] != roots.v[P_EXTRA]) {
            ProxyError(cx, "setPrototypeOf invariant");
            goto out;
        }
    }
    *accepted = answer; ok = JS_TRUE;
  out:
    JS_POP_TEMP_ROOT(cx, &roots.root);
    return ok;
}

JSBool
js_ProxyGetOwnDescriptor(JSContext *cx, JSObject *obj, jsid id,
                          JSObject *global, jsval *rval)
{
    ProxyRoots roots;
    JSBool ok = JS_FALSE, extensible, compatible;
    RootProxy(cx, obj, &roots);
    if (!PropertyKey(cx, id, &roots) || !GetTrap(cx, obj, "getOwnPropertyDescriptor", &roots)) goto out;
    if (JSVAL_IS_VOID(roots.v[P_TRAP])) {
        if (!TargetDescriptor(cx, &roots)) goto out;
        if (JSVAL_IS_VOID(roots.v[P_DESC])) { *rval = JSVAL_VOID; ok = JS_TRUE; }
        else ok = js_ConvertProxyDescriptor(cx, roots.v[P_DESC], JS_FALSE, JS_TRUE, global, rval);
        goto out;
    }
    roots.v[P_ARG1] = roots.v[P_KEY];
    if (!CallTrap(cx, &roots, 2)) goto out;
    if (JSVAL_IS_PRIMITIVE(roots.v[P_RESULT]) && !JSVAL_IS_VOID(roots.v[P_RESULT])) {
        ProxyError(cx, "getOwnPropertyDescriptor result"); goto out;
    }
    if (!TargetDescriptor(cx, &roots)) goto out;
    if (JSVAL_IS_VOID(roots.v[P_RESULT])) {
        if (JSVAL_IS_VOID(roots.v[P_DESC])) {
            *rval = JSVAL_VOID; ok = JS_TRUE; goto out;
        }
        if (!DescriptorField(cx, roots.v[P_DESC], "configurable", &roots.v[P_EXTRA])) goto out;
        if (roots.v[P_EXTRA] == JSVAL_FALSE) goto invariant;
        if (!TargetOperation(cx, &roots, js_ReflectIsExtensible, 1, JSVAL_VOID,
                              JSVAL_VOID, JSVAL_VOID, &roots.v[P_EXTRA])) goto out;
        if (roots.v[P_EXTRA] == JSVAL_FALSE) goto invariant;
        *rval = JSVAL_VOID; ok = JS_TRUE;
        goto out;
    }
    if (!TargetOperation(cx, &roots, js_ReflectIsExtensible, 1, JSVAL_VOID,
                          JSVAL_VOID, JSVAL_VOID, &roots.v[P_EXTRA])) goto out;
    extensible = roots.v[P_EXTRA] == JSVAL_TRUE;
    if (!js_ConvertProxyDescriptor(cx, roots.v[P_RESULT], JS_TRUE, JS_FALSE,
                                    global, &roots.v[P_EXTRA])) goto out;
    if (!js_CompatibleProxyDescriptor(cx, extensible, roots.v[P_EXTRA], roots.v[P_DESC],
                                       &compatible)) goto out;
    if (!compatible) goto invariant;
    if (!DescriptorField(cx, roots.v[P_EXTRA], "configurable", &roots.v[P_ARG2])) goto out;
    if (roots.v[P_ARG2] == JSVAL_FALSE) {
        if (JSVAL_IS_VOID(roots.v[P_DESC])) goto invariant;
        if (!DescriptorField(cx, roots.v[P_DESC], "configurable", &roots.v[P_ARG2])) goto out;
        if (roots.v[P_ARG2] == JSVAL_TRUE) goto invariant;
    }
    *rval = roots.v[P_EXTRA]; ok = JS_TRUE;
    goto out;
  invariant:
    ProxyError(cx, "getOwnPropertyDescriptor invariant");
  out:
    JS_POP_TEMP_ROOT(cx, &roots.root);
    return ok;
}

JSBool
js_ProxyDefineOwn(JSContext *cx, JSObject *obj, jsid id,
                   jsval descriptor, JSBool *accepted)
{
    ProxyRoots roots;
    JSBool ok = JS_FALSE, answer, extensible, compatible, settingPermanent;
    RootProxy(cx, obj, &roots);
    roots.v[P_RECORD] = descriptor;
    if (!PropertyKey(cx, id, &roots) || !GetTrap(cx, obj, "defineProperty", &roots)) goto out;
    if (!js_ConvertProxyDescriptor(cx, descriptor, JS_FALSE, JS_TRUE, NULL,
                                    &roots.v[P_RECORD])) goto out;
    if (JSVAL_IS_VOID(roots.v[P_TRAP])) {
        ok = TargetOperation(cx, &roots, js_ReflectDefineProperty, 3, roots.v[P_KEY],
                              roots.v[P_RECORD], JSVAL_VOID, &roots.v[P_RESULT]);
        if (ok) *accepted = roots.v[P_RESULT] == JSVAL_TRUE;
        goto out;
    }
    /* The handler may mutate its FromPropertyDescriptor object. Keep the
     * original internal record separate for post-trap invariant checks. */
    if (!js_ConvertProxyDescriptor(cx, roots.v[P_RECORD], JS_FALSE, JS_TRUE,
                                    js_ProxyOperationGlobal(cx), &roots.v[P_ARG2])) goto out;
    roots.v[P_ARG1] = roots.v[P_KEY];
    if (!CallTrap(cx, &roots, 3) || !JS_ValueToBoolean(cx, roots.v[P_RESULT], &answer)) goto out;
    if (!answer) { *accepted = JS_FALSE; ok = JS_TRUE; goto out; }
    if (!TargetDescriptor(cx, &roots) ||
        !TargetOperation(cx, &roots, js_ReflectIsExtensible, 1, JSVAL_VOID,
                          JSVAL_VOID, JSVAL_VOID, &roots.v[P_EXTRA])) goto out;
    extensible = roots.v[P_EXTRA] == JSVAL_TRUE;
    if (!DescriptorField(cx, roots.v[P_RECORD], "configurable", &roots.v[P_EXTRA])) goto out;
    settingPermanent = roots.v[P_EXTRA] == JSVAL_FALSE;
    if (JSVAL_IS_VOID(roots.v[P_DESC])) {
        if (!extensible || settingPermanent) goto invariant;
    } else {
        if (!js_CompatibleProxyDescriptor(cx, extensible, roots.v[P_RECORD], roots.v[P_DESC],
                                           &compatible)) goto out;
        if (!compatible) goto invariant;
        if (settingPermanent) {
            if (!DescriptorField(cx, roots.v[P_DESC], "configurable", &roots.v[P_EXTRA])) goto out;
            if (roots.v[P_EXTRA] == JSVAL_TRUE) goto invariant;
        }
    }
    *accepted = JS_TRUE; ok = JS_TRUE;
    goto out;
  invariant:
    ProxyError(cx, "defineProperty invariant");
  out:
    JS_POP_TEMP_ROOT(cx, &roots.root);
    return ok;
}

typedef struct ProxyIds {
    JSTempValueRooter root;
    JSIdArray *ids;
} ProxyIds;

JS_STATIC_DLL_CALLBACK(void)
MarkProxyIds(JSContext *cx, JSTempValueRooter *root)
{
    ProxyIds *ids = (ProxyIds *)root;
    jsint i;
    for (i = 0; ids->ids && i < ids->ids->length; ++i) {
        if (JSID_IS_ATOM(ids->ids->vector[i]))
            js_MarkAtom(cx, JSID_TO_ATOM(ids->ids->vector[i]));
    }
}

/* The caller installs the marker before conversion can invoke array getters. */
static JSBool
ArrayIds(JSContext *cx, jsval value, ProxyIds *ids)
{
    JSTempValueRooter root;
    jsval item = JSVAL_VOID;
    jsdouble length;
    jsuint index;
    jsint i, count;
    JSBool ok = JS_FALSE;
    if (JSVAL_IS_PRIMITIVE(value)) return ProxyError(cx, "ownKeys result");
    if (!js_ArrayLikeLength(cx, JSVAL_TO_OBJECT(value), &length)) return JS_FALSE;
    if (length > JSVAL_INT_MAX ||
        length > (((size_t)-1) - sizeof(JSIdArray)) / sizeof(jsid)) {
        JS_ReportOutOfMemory(cx); return JS_FALSE;
    }
    count = (jsint)length;
    ids->ids = js_NewIdArray(cx, count);
    if (!ids->ids) return JS_FALSE;
    for (i = 0; i < count; ++i) ids->ids->vector[i] = INT_TO_JSID(0);
    JS_PUSH_SINGLE_TEMP_ROOT(cx, item, &root);
    for (i = 0; i < count; ++i) {
        if (!JS_GetElement(cx, JSVAL_TO_OBJECT(value), i, &root.u.value)) goto out;
        item = root.u.value;
        if (!JSVAL_IS_STRING(item) && !JSVAL_IS_SYMBOL(item)) {
            ProxyError(cx, "ownKeys property key"); goto out;
        }
        if (!js_ValueToPropertyId(cx, item, &ids->ids->vector[i])) goto out;
        if (js_IdIsIndex(item, &index) && index <= JSVAL_INT_MAX)
            ids->ids->vector[i] = INT_TO_JSID(index);
    }
    ok = JS_TRUE;
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}

JSIdArray *
js_ProxyOwnKeys(JSContext *cx, JSObject *obj)
{
    ProxyRoots roots;
    ProxyIds keys, targetKeys;
    JSIdArray *result = NULL;
    unsigned char *required = NULL, *used = NULL;
    JSBool extensible;
    jsint i, j;
    RootProxy(cx, obj, &roots);
    keys.ids = targetKeys.ids = NULL;
    JS_PUSH_TEMP_ROOT_MARKER(cx, MarkProxyIds, &keys.root);
    JS_PUSH_TEMP_ROOT_MARKER(cx, MarkProxyIds, &targetKeys.root);
    if (!GetTrap(cx, obj, "ownKeys", &roots)) goto out;
    if (JSVAL_IS_VOID(roots.v[P_TRAP])) {
        if (!TargetOperation(cx, &roots, js_ReflectOwnKeys, 1, JSVAL_VOID,
                              JSVAL_VOID, JSVAL_VOID, &roots.v[P_RESULT]) ||
            !ArrayIds(cx, roots.v[P_RESULT], &keys)) goto out;
        goto accept;
    }
    if (!CallTrap(cx, &roots, 1) || !ArrayIds(cx, roots.v[P_RESULT], &keys)) goto out;
    if (!TargetOperation(cx, &roots, js_ReflectIsExtensible, 1, JSVAL_VOID,
                          JSVAL_VOID, JSVAL_VOID, &roots.v[P_EXTRA])) goto out;
    extensible = roots.v[P_EXTRA] == JSVAL_TRUE;
    if (!TargetOperation(cx, &roots, js_ReflectOwnKeys, 1, JSVAL_VOID,
                          JSVAL_VOID, JSVAL_VOID, &roots.v[P_EXTRA]) ||
        !ArrayIds(cx, roots.v[P_EXTRA], &targetKeys)) goto out;
    required = (unsigned char *)JS_malloc(cx, targetKeys.ids->length ? targetKeys.ids->length : 1);
    used = (unsigned char *)JS_malloc(cx, keys.ids->length ? keys.ids->length : 1);
    if (!required || !used) goto out;
    memset(used, 0, keys.ids->length);
    /* Finish all descriptor queries before enforcing the key-set invariants. */
    for (i = 0; i < targetKeys.ids->length; ++i) {
        required[i] = !extensible;
        if (!PropertyKey(cx, targetKeys.ids->vector[i], &roots) ||
            !TargetDescriptor(cx, &roots)) goto out;
        if (!JSVAL_IS_VOID(roots.v[P_DESC])) {
            if (!DescriptorField(cx, roots.v[P_DESC], "configurable", &roots.v[P_RECORD])) goto out;
            if (roots.v[P_RECORD] == JSVAL_FALSE) required[i] = 1;
        }
    }
    for (i = 0; i < targetKeys.ids->length; ++i) {
        if (!required[i]) continue;
        for (j = 0; j < keys.ids->length; ++j) {
            if (!used[j] && keys.ids->vector[j] == targetKeys.ids->vector[i]) break;
        }
        if (j == keys.ids->length) goto invariant;
        used[j] = 1;
    }
    if (!extensible) {
        for (j = 0; j < keys.ids->length; ++j) {
            if (!used[j]) goto invariant;
        }
    }
  accept:
    result = keys.ids; keys.ids = NULL;
    goto out;
  invariant:
    ProxyError(cx, "ownKeys invariant");
  out:
    JS_free(cx, used); JS_free(cx, required);
    JS_POP_TEMP_ROOT(cx, &targetKeys.root);
    JS_POP_TEMP_ROOT(cx, &keys.root);
    if (targetKeys.ids) JS_DestroyIdArray(cx, targetKeys.ids);
    if (keys.ids) JS_DestroyIdArray(cx, keys.ids);
    JS_POP_TEMP_ROOT(cx, &roots.root);
    return result;
}

JSObject *
js_ProxyOperationGlobal(JSContext *cx)
{
    JSObject *obj = cx->fp && cx->fp->callee ? cx->fp->callee :
                    cx->fp && cx->fp->scopeChain ? cx->fp->scopeChain : cx->globalObject;
    return ProxyGlobal(cx, obj);
}

static JSBool
ArgumentArray(JSContext *cx, uintN argc, jsval *argv, jsval *rval)
{
    JSObject *global = js_ProxyOperationGlobal(cx), *proto, *array;
    proto = js_BuiltinPrototype(cx, global, JSProto_Array);
    array = proto ? js_NewArrayObjectWithProto(cx, argc, argv, proto, global) : NULL;
    if (!array) return JS_FALSE;
    *rval = OBJECT_TO_JSVAL(array);
    return JS_TRUE;
}

JSBool
js_ProxyCall(JSContext *cx, JSObject *obj, jsval receiver,
               uintN argc, jsval *argv, jsval *rval)
{
    ProxyRoots roots;
    JSBool ok = JS_FALSE;
    RootProxy(cx, obj, &roots);
    roots.v[P_ARG1] = receiver;
    if (!GetTrap(cx, obj, "apply", &roots)) goto out;
    if (JSVAL_IS_VOID(roots.v[P_TRAP])) {
        ok = js_InternalInvokeValue(cx, receiver, roots.v[P_TARGET], 0, argc, argv, rval);
        goto out;
    }
    if (!ArgumentArray(cx, argc, argv, &roots.v[P_ARG2]) || !CallTrap(cx, &roots, 3)) goto out;
    *rval = roots.v[P_RESULT]; ok = JS_TRUE;
  out:
    JS_POP_TEMP_ROOT(cx, &roots.root);
    return ok;
}

JSBool
js_ProxyConstruct(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
                    JSObject *newTarget, jsval *rval)
{
    ProxyRoots roots;
    jsval *base, *oldsp;
    JSStackFrame *frame = cx->fp;
    void *mark;
    uintN i;
    JSBool ok = JS_FALSE;
    RootProxy(cx, obj, &roots);
    roots.v[P_ARG2] = OBJECT_TO_JSVAL(newTarget);
    if (!GetTrap(cx, obj, "construct", &roots)) goto out;
    if (JSVAL_IS_VOID(roots.v[P_TRAP])) {
        if (argc >= ARRAY_INIT_LIMIT) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_TOO_MANY_FUN_ARGS);
            goto out;
        }
        base = js_AllocStack(cx, argc + 2, &mark);
        if (!base) goto out;
        base[0] = roots.v[P_TARGET]; base[1] = JSVAL_NULL;
        for (i = 0; i < argc; ++i) base[2 + i] = argv[i];
        oldsp = frame->sp; frame->sp = base + argc + 2;
        ok = js_InvokeConstructorWithNewTarget(cx, base, argc, newTarget);
        if (ok) *rval = base[0];
        frame->sp = oldsp;
        js_FreeStack(cx, mark);
        goto out;
    }
    if (!ArgumentArray(cx, argc, argv, &roots.v[P_ARG1]) || !CallTrap(cx, &roots, 3)) goto out;
    if (JSVAL_IS_PRIMITIVE(roots.v[P_RESULT])) { ProxyError(cx, "construct result"); goto out; }
    *rval = roots.v[P_RESULT]; ok = JS_TRUE;
  out:
    JS_POP_TEMP_ROOT(cx, &roots.root);
    return ok;
}

JSBool
js_ProxyEnumerate(JSContext *cx, JSObject *obj, jsval *rval)
{
    ProxyRoots roots;
    JSBool ok = JS_FALSE;
    RootProxy(cx, obj, &roots);
    if (!GetTrap(cx, obj, "enumerate", &roots)) goto out;
    if (JSVAL_IS_VOID(roots.v[P_TRAP])) {
        ok = TargetOperation(cx, &roots, js_ReflectEnumerate, 1, JSVAL_VOID,
                              JSVAL_VOID, JSVAL_VOID, rval);
        goto out;
    }
    if (!CallTrap(cx, &roots, 1)) goto out;
    if (JSVAL_IS_PRIMITIVE(roots.v[P_RESULT])) { ProxyError(cx, "enumerate result"); goto out; }
    *rval = roots.v[P_RESULT]; ok = JS_TRUE;
  out:
    JS_POP_TEMP_ROOT(cx, &roots.root);
    return ok;
}

/* Opaque lookup handles root their owner and key until dropProperty. Unlike a
 * native JSScopeProperty, a handle is never cached or interpreted as a slot. */
typedef struct ProxyProperty {
    JSProperty property;
    jsval owner, key;
} ProxyProperty;

JSBool
js_ProxyLookupForAccess(JSContext *cx, JSObject *obj, jsid id,
                         JSObject **owner, JSProperty **property)
{
    ProxyProperty *handle;
    jsval values[2];
    JSTempValueRooter root;
    JSBool ok = JS_FALSE;
    values[0] = OBJECT_TO_JSVAL(obj); values[1] = ID_TO_VALUE(id);
    JS_PUSH_TEMP_ROOT(cx, 2, values, &root);
    handle = (ProxyProperty *)JS_malloc(cx, sizeof(*handle));
    if (!handle) goto out;
    handle->property.id = id; handle->owner = values[0]; handle->key = values[1];
    if (!JS_AddNamedRoot(cx, &handle->owner, "Proxy lookup owner")) {
        JS_free(cx, handle); goto out;
    }
    if (!JS_AddNamedRoot(cx, &handle->key, "Proxy lookup key")) {
        JS_RemoveRoot(cx, &handle->owner); JS_free(cx, handle); goto out;
    }
    *owner = obj; *property = &handle->property; ok = JS_TRUE;
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}

static JSBool
ProxyLookup(JSContext *cx, JSObject *obj, jsid id, JSObject **owner, JSProperty **property)
{
    JSBool found;
    *owner = NULL; *property = NULL;
    if (!js_ProxyHas(cx, obj, id, &found)) return JS_FALSE;
    return !found || js_ProxyLookupForAccess(cx, obj, id, owner, property);
}

static void
ProxyDrop(JSContext *cx, JSObject *obj, JSProperty *property)
{
    ProxyProperty *handle = (ProxyProperty *)property;
    JS_RemoveRoot(cx, &handle->key); JS_RemoveRoot(cx, &handle->owner);
    JS_free(cx, handle);
}

static JSBool
ProxyGet(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    return js_ProxyGet(cx, obj, id, OBJECT_TO_JSVAL(obj), vp);
}

static JSBool
ProxySet(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    JSBool accepted;
    if (!js_ProxySet(cx, obj, id, *vp, OBJECT_TO_JSVAL(obj), &accepted)) return JS_FALSE;
    if (!accepted && cx->fp && cx->fp->script && cx->fp->script->strictMode)
        return ProxyError(cx, "assignment rejected");
    return JS_TRUE;
}

static JSBool
ProxyDelete(JSContext *cx, JSObject *obj, jsid id, jsval *rval)
{
    JSBool accepted;
    if (!js_ProxyDelete(cx, obj, id, &accepted)) return JS_FALSE;
    if (!accepted && cx->fp && cx->fp->script && cx->fp->script->strictMode)
        return ProxyError(cx, "deletion rejected");
    *rval = BOOLEAN_TO_JSVAL(accepted);
    return JS_TRUE;
}

static JSBool
ProxyGetAttributes(JSContext *cx, JSObject *obj, jsid id, JSProperty *property, uintN *attrs)
{
    jsval values[2] = {JSVAL_VOID, JSVAL_VOID};
    JSTempValueRooter root;
    JSBool ok = JS_FALSE;
    JS_PUSH_TEMP_ROOT(cx, 2, values, &root);
    *attrs = 0;
    if (!js_ProxyGetOwnDescriptor(cx, obj, id, js_ProxyOperationGlobal(cx), &values[0])) goto out;
    if (!JSVAL_IS_VOID(values[0])) {
        if (!DescriptorField(cx, values[0], "enumerable", &values[1])) goto out;
        if (values[1] == JSVAL_TRUE) *attrs |= JSPROP_ENUMERATE;
        if (!DescriptorField(cx, values[0], "configurable", &values[1])) goto out;
        if (values[1] == JSVAL_FALSE) *attrs |= JSPROP_PERMANENT;
        if (!DescriptorField(cx, values[0], "writable", &values[1])) goto out;
        if (values[1] == JSVAL_FALSE) *attrs |= JSPROP_READONLY;
        if (JSVAL_IS_VOID(values[1])) *attrs |= JSPROP_GETTER | JSPROP_SETTER;
    }
    ok = JS_TRUE;
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}

static JSBool
DefineField(JSContext *cx, JSObject *obj, const char *name, jsval value)
{
    return JS_DefineProperty(cx, obj, name, value, NULL, NULL, JSPROP_ENUMERATE);
}

static JSBool
ProxyDefine(JSContext *cx, JSObject *obj, jsid id, jsval value,
              JSPropertyOp getter, JSPropertyOp setter, uintN attrs, JSProperty **property)
{
    JSObject *descriptor;
    jsval values[6];
    JSTempValueRooter root;
    JSBool ok = JS_FALSE, accepted;
    values[0] = OBJECT_TO_JSVAL(obj); values[1] = value;
    values[2] = values[3] = values[4] = JSVAL_VOID;
    values[5] = ID_TO_VALUE(id);
    if (attrs & JSPROP_GETTER) values[2] = getter ? OBJECT_TO_JSVAL((JSObject *)getter) : JSVAL_VOID;
    if (attrs & JSPROP_SETTER) values[3] = setter ? OBJECT_TO_JSVAL((JSObject *)setter) : JSVAL_VOID;
    JS_PUSH_TEMP_ROOT(cx, 6, values, &root);
    descriptor = js_NewObject(cx, &js_ObjectClass, NULL, js_ProxyOperationGlobal(cx));
    if (!descriptor) goto out;
    values[4] = OBJECT_TO_JSVAL(descriptor);
    if (!JS_SetPrototype(cx, descriptor, NULL) ||
        !DefineField(cx, descriptor, "enumerable", BOOLEAN_TO_JSVAL((attrs & JSPROP_ENUMERATE) != 0)) ||
        !DefineField(cx, descriptor, "configurable", BOOLEAN_TO_JSVAL((attrs & JSPROP_PERMANENT) == 0))) goto out;
    if (attrs & (JSPROP_GETTER | JSPROP_SETTER)) {
        if (!DefineField(cx, descriptor, "get", values[2]) ||
            !DefineField(cx, descriptor, "set", values[3])) goto out;
    } else {
        if ((getter && getter != JS_PropertyStub) || (setter && setter != JS_PropertyStub)) {
            ProxyError(cx, "native descriptor hooks"); goto out;
        }
        if (!DefineField(cx, descriptor, "value", value) ||
            !DefineField(cx, descriptor, "writable", BOOLEAN_TO_JSVAL((attrs & JSPROP_READONLY) == 0))) goto out;
    }
    if (!js_ProxyDefineOwn(cx, obj, id, values[4], &accepted)) goto out;
    if (!accepted) { ProxyError(cx, "definition rejected"); goto out; }
    ok = !property || js_ProxyLookupForAccess(cx, obj, id, &descriptor, property);
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}

static JSBool
ProxySetAttributes(JSContext *cx, JSObject *obj, jsid id, JSProperty *property, uintN *attrs)
{
    JSObject *descriptor;
    ProxyRoots roots;
    JSBool ok = JS_FALSE, accepted;
    RootProxy(cx, obj, &roots);
    roots.v[P_KEY] = ID_TO_VALUE(id);
    descriptor = js_NewObject(cx, &js_ObjectClass, NULL, js_ProxyOperationGlobal(cx));
    if (!descriptor) goto out;
    roots.v[P_DESC] = OBJECT_TO_JSVAL(descriptor);
    if (!JS_SetPrototype(cx, descriptor, NULL) ||
        !DefineField(cx, descriptor, "enumerable", BOOLEAN_TO_JSVAL((*attrs & JSPROP_ENUMERATE) != 0)) ||
        !DefineField(cx, descriptor, "configurable", BOOLEAN_TO_JSVAL((*attrs & JSPROP_PERMANENT) == 0))) goto out;
    if (!(*attrs & (JSPROP_GETTER | JSPROP_SETTER)) &&
        !DefineField(cx, descriptor, "writable", BOOLEAN_TO_JSVAL((*attrs & JSPROP_READONLY) == 0))) goto out;
    if (!js_ProxyDefineOwn(cx, obj, id, roots.v[P_DESC], &accepted)) goto out;
    ok = accepted || ProxyError(cx, "attributes rejected");
  out:
    JS_POP_TEMP_ROOT(cx, &roots.root);
    return ok;
}

static JSBool
ProxyCheckAccess(JSContext *cx, JSObject *obj, jsid id, JSAccessMode mode,
                   jsval *vp, uintN *attrs)
{
    JSObject *target;
    JSTempValueRooter root;
    JSBool ok;
    if (!js_ProxyTarget(cx, obj, &target)) return JS_FALSE;
    JS_PUSH_TEMP_ROOT_OBJECT(cx, target, &root);
    ok = OBJ_CHECK_ACCESS(cx, target, id, mode, vp, attrs);
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}

static JSBool
ProxyConvert(JSContext *cx, JSObject *obj, JSType type, jsval *vp)
{
    if (type == JSTYPE_FUNCTION) { *vp = OBJECT_TO_JSVAL(obj); return JS_TRUE; }
    return JS_ConvertStub(cx, obj, type, vp);
}

static JSBool
ProxyCallOp(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    return js_ProxyCall(cx, JSVAL_TO_OBJECT(argv[-2]), argv[-1], argc, argv, rval);
}

static JSBool
ProxyConstructOp(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSObject *callee = JSVAL_TO_OBJECT(argv[-2]);
    return js_ProxyConstruct(cx, callee, argc, argv,
                              (cx->fp->flags & JSFRAME_NEW_TARGET) ? cx->fp->newTarget : callee, rval);
}

static JSBool
ProxyHasInstance(JSContext *cx, JSObject *obj, jsval value, JSBool *result)
{
    jsval proto = JSVAL_VOID;
    JSTempValueRooter root;
    JSBool ok = JS_FALSE;
    *result = JS_FALSE;
    if (!js_IsCallable(cx, OBJECT_TO_JSVAL(obj))) return ProxyError(cx, "instanceof");
    if (JSVAL_IS_PRIMITIVE(value)) return JS_TRUE;
    JS_PUSH_SINGLE_TEMP_ROOT(cx, proto, &root);
    if (!ProxyGet(cx, obj, ATOM_TO_JSID(cx->runtime->atomState.classPrototypeAtom), &root.u.value)) goto out;
    if (JSVAL_IS_PRIMITIVE(root.u.value)) { ProxyError(cx, "instanceof prototype"); goto out; }
    ok = js_IsDelegate(cx, JSVAL_TO_OBJECT(root.u.value), value, result);
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}

static JSBool
ProxySetObjectSlot(JSContext *cx, JSObject *obj, uint32 slot, JSObject *value)
{
    JSBool accepted;
    JSObject *cursor;
    if (slot == JSSLOT_PROTO) {
        if (!js_ProxySetPrototype(cx, obj, value, &accepted)) return JS_FALSE;
        return accepted || ProxyError(cx, "prototype rejected");
    }
    for (cursor = value; cursor; cursor = OBJ_GET_PARENT(cx, cursor)) {
        if (cursor == obj) return ProxyError(cx, "parent cycle");
    }
    JS_LOCK_GC(cx->runtime);
    obj->slots[JSSLOT_PARENT] = OBJECT_TO_JSVAL(value);
    JS_UNLOCK_GC(cx->runtime);
    return JS_TRUE;
}

static jsval
ProxyGetSlot(JSContext *cx, JSObject *obj, uint32 slot)
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
ProxySetSlot(JSContext *cx, JSObject *obj, uint32 slot, jsval value)
{
    JSBool ok;
    JS_LOCK_GC(cx->runtime);
    ok = slot < (uint32)obj->slots[-1];
    if (ok) obj->slots[slot] = value;
    JS_UNLOCK_GC(cx->runtime);
    return ok;
}

static uint32
ProxyMark(JSContext *cx, JSObject *obj, void *arg)
{
    return (uint32)obj->slots[-1];
}

static void
ProxyClear(JSContext *cx, JSObject *obj)
{
    JS_LOCK_GC(cx->runtime);
    obj->slots[JSSLOT_START(&js_ProxyClass)] = JSVAL_NULL;
    obj->slots[JSSLOT_START(&js_ProxyClass) + 1] = JSVAL_NULL;
    JS_UNLOCK_GC(cx->runtime);
}

static JSObjectMap *
ProxyNewMap(JSContext *cx, jsrefcount nrefs, JSObjectOps *ops, JSClass *clasp, JSObject *obj)
{
    /* A distinct allocator identity disables native property-cache paths. */
    return js_NewObjectMap(cx, nrefs, ops, clasp, obj);
}

JSBool
js_ProxyIteratorNext(JSContext *cx, jsval iterator, JSBool *done, jsval *rval)
{
    jsval values[3];
    JSTempValueRooter root;
    JSBool ok = JS_FALSE;
    values[0] = iterator; values[1] = values[2] = JSVAL_VOID;
    JS_PUSH_TEMP_ROOT(cx, 3, values, &root);
    if (!JS_GetProperty(cx, JSVAL_TO_OBJECT(iterator), "next", &values[1]) ||
        !js_InternalInvokeValue(cx, iterator, values[1], 0, 0, NULL, &values[2])) goto out;
    if (JSVAL_IS_PRIMITIVE(values[2])) { ProxyError(cx, "iterator result"); goto out; }
    if (!JS_GetProperty(cx, JSVAL_TO_OBJECT(values[2]), "done", &values[1]) ||
        !JS_ValueToBoolean(cx, values[1], done)) goto out;
    if (*done) { *rval = JSVAL_VOID; ok = JS_TRUE; }
    else ok = JS_GetProperty(cx, JSVAL_TO_OBJECT(values[2]), "value", rval);
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}

static JSBool
ProxyEnumerateOp(JSContext *cx, JSObject *obj, JSIterateOp op, jsval *state, jsid *id)
{
    jsval values[3];
    JSTempValueRooter root;
    JSBool ok = JS_FALSE, done;
    if (op == JSENUMERATE_DESTROY) { *state = JSVAL_NULL; return JS_TRUE; }
    if (op == JSENUMERATE_INIT) {
        if (id) *id = JSVAL_ZERO;
        return js_ProxyEnumerate(cx, obj, state);
    }
    values[0] = *state; values[1] = values[2] = JSVAL_VOID;
    JS_PUSH_TEMP_ROOT(cx, 3, values, &root);
    if (!JS_GetProperty(cx, JSVAL_TO_OBJECT(values[0]), "next", &values[1]) ||
        !js_InternalInvokeValue(cx, values[0], values[1], 0, 0, NULL, &values[2])) goto out;
    if (JSVAL_IS_PRIMITIVE(values[2])) { ProxyError(cx, "iterator result"); goto out; }
    if (!JS_GetProperty(cx, JSVAL_TO_OBJECT(values[2]), "done", &values[1]) ||
        !JS_ValueToBoolean(cx, values[1], &done)) goto out;
    if (done) { *state = JSVAL_NULL; ok = JS_TRUE; goto out; }
    if (!JS_GetProperty(cx, JSVAL_TO_OBJECT(values[2]), "value", &values[1])) goto out;
    ok = js_ValueToPropertyId(cx, values[1], id);
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}

#define PROXY_OPS(call, construct) { \
    ProxyNewMap, js_DestroyObjectMap, ProxyLookup, ProxyDefine, \
    ProxyGet, ProxySet, ProxyGetAttributes, ProxySetAttributes, \
    ProxyDelete, js_DefaultValue, ProxyEnumerateOp, ProxyCheckAccess, \
    NULL, ProxyDrop, call, construct, NULL, ProxyHasInstance, \
    ProxySetObjectSlot, ProxySetObjectSlot, ProxyMark, ProxyClear, \
    ProxyGetSlot, ProxySetSlot }
static JSObjectOps proxyOps[4] = {
    PROXY_OPS(NULL, NULL), PROXY_OPS(ProxyCallOp, NULL),
    PROXY_OPS(NULL, ProxyConstructOp), PROXY_OPS(ProxyCallOp, ProxyConstructOp)
};
#undef PROXY_OPS

static JSObjectOps *
ProxyObjectOps(JSContext *cx, JSClass *clasp)
{
    return &proxyOps[0];
}

JSClass js_ProxyClass = {
    "Proxy", JSCLASS_HAS_RESERVED_SLOTS(2) | JSCLASS_HAS_CACHED_PROTO(JSProto_Proxy),
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, ProxyConvert, JS_FinalizeStub,
    ProxyObjectOps, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

static JSObject *
NewProxy(JSContext *cx, jsval target, jsval handler, JSObject *global)
{
    JSObject *obj, *proto, *checked;
    JSTempValueRooter root;
    uintN capabilities;
    if (JSVAL_IS_PRIMITIVE(target) || JSVAL_IS_PRIMITIVE(handler)) {
        ProxyError(cx, "constructor arguments"); return NULL;
    }
    /* These two creation checks are required by the 2015 edition. */
    if (js_IsProxy(cx, JSVAL_TO_OBJECT(target)) &&
        !js_ProxyTarget(cx, JSVAL_TO_OBJECT(target), &checked)) return NULL;
    if (js_IsProxy(cx, JSVAL_TO_OBJECT(handler)) &&
        !js_ProxyTarget(cx, JSVAL_TO_OBJECT(handler), &checked)) return NULL;
    capabilities = (js_IsCallable(cx, target) ? 1 : 0) | (js_IsConstructor(cx, target) ? 2 : 0);
    proto = js_BuiltinPrototype(cx, global, JSProto_Object);
    obj = proto ? js_NewObject(cx, &js_ProxyClass, proto, global) : NULL;
    if (!obj) return NULL;
    JS_PUSH_TEMP_ROOT_OBJECT(cx, obj, &root);
    JS_ASSERT(obj->map->nrefs == 1);
    obj->map->ops = &proxyOps[capabilities];
    /* No ordinary prototype chain: [[GetPrototypeOf]] dispatches to the target. */
    obj->slots[JSSLOT_PROTO] = JSVAL_NULL;
    obj->slots[JSSLOT_START(&js_ProxyClass)] = target;
    obj->slots[JSSLOT_START(&js_ProxyClass) + 1] = handler;
    JS_POP_TEMP_ROOT(cx, &root);
    return obj;
}

JSBool
js_ProxyConstructor(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSObject *proxy;
    if (!(cx->fp->flags & JSFRAME_CONSTRUCTING)) return ProxyError(cx, "requires new");
    proxy = NewProxy(cx, argv[0], argv[1], js_BuiltinGlobal(cx, argv));
    if (!proxy) return JS_FALSE;
    *rval = OBJECT_TO_JSVAL(proxy);
    return JS_TRUE;
}

static JSClass revokerStateClass = {
    "Proxy Revocation State", JSCLASS_HAS_RESERVED_SLOTS(1),
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSBool
Revoke(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    jsval values[2] = {JSVAL_VOID, JSVAL_VOID};
    JSTempValueRooter root;
    JSObject *state;
    JSBool ok = JS_FALSE;
    JS_PUSH_TEMP_ROOT(cx, 2, values, &root);
    if (!JS_GetReservedSlot(cx, JSVAL_TO_OBJECT(argv[-2]), 2, &values[0])) goto out;
    state = JSVAL_TO_OBJECT(values[0]);
    JS_LOCK_OBJ(cx, state);
    values[1] = LOCKED_OBJ_GET_SLOT(state, JSSLOT_START(&revokerStateClass));
    LOCKED_OBJ_SET_SLOT(state, JSSLOT_START(&revokerStateClass), JSVAL_NULL);
    JS_UNLOCK_OBJ(cx, state);
    if (!JSVAL_IS_NULL(values[1])) ProxyClear(cx, JSVAL_TO_OBJECT(values[1]));
    *rval = JSVAL_VOID; ok = JS_TRUE;
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}

static JSBool
Revocable(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSObject *global = js_BuiltinGlobal(cx, argv), *proxy, *result, *proto, *state;
    JSFunction *revoke;
    jsval values[3] = {JSVAL_VOID, JSVAL_VOID, JSVAL_VOID};
    JSTempValueRooter root;
    JSBool ok = JS_FALSE;
    JS_PUSH_TEMP_ROOT(cx, 3, values, &root);
    proxy = NewProxy(cx, argv[0], argv[1], global);
    if (!proxy) goto out;
    values[0] = OBJECT_TO_JSVAL(proxy);
    /* Reserve the captured proxy before allocating own metadata properties:
     * otherwise reserved slot 2 would overlap the ordinary length slot. */
    revoke = js_NewFunction(cx, NULL, NULL, 0, JSFUN_STRICT | JSFUN_NO_CONSTRUCT, global, NULL);
    if (!revoke) goto out;
    values[1] = OBJECT_TO_JSVAL(revoke->object);
    revoke->u.n.spare = 1;
    revoke->u.n.native = Revoke;
    proto = js_BuiltinPrototype(cx, global, JSProto_Object);
    state = proto ? js_NewObject(cx, &revokerStateClass, proto, global) : NULL;
    if (!state) goto out;
    values[2] = OBJECT_TO_JSVAL(state);
    if (!JS_SetReservedSlot(cx, state, 0, values[0]) ||
        !JS_SetReservedSlot(cx, revoke->object, 2, values[2]) ||
        !js_InitFunctionProperties(cx, revoke->object) ||
        !JS_DeleteProperty(cx, revoke->object, "name")) goto out;
    proto = js_BuiltinPrototype(cx, global, JSProto_Object);
    result = proto ? js_NewObject(cx, &js_ObjectClass, proto, global) : NULL;
    if (!result) goto out;
    *rval = OBJECT_TO_JSVAL(result);
    ok = DefineField(cx, result, "proxy", values[0]) && DefineField(cx, result, "revoke", values[1]);
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}

JSObject *
js_InitProxyClass(JSContext *cx, JSObject *global)
{
    JSFunction *constructor = JS_DefineFunction(cx, global, "Proxy", js_ProxyConstructor, 2, JSFUN_STRICT);
    JSObject *obj;
    if (!constructor) return NULL;
    obj = constructor->object;
    if (!JS_DefineFunction(cx, obj, "revocable", Revocable, 2, JSFUN_STRICT | JSFUN_NO_CONSTRUCT) ||
        !js_CacheClassObject(cx, global, JSProto_Proxy, obj)) return NULL;
    return obj;
}
