/* ES2015 Promise state and reactions in the classic SpiderMonkey embedding.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "jsapi.h"
#include "jsarray.h"
#include "jsbool.h"
#include "jsnum.h"
#include "jscntxt.h"
#include "jsfun.h"
#include "jsgc.h"
#include "jsinterp.h"
#include "jsiteres6.h"
#include "jsobj.h"
#include "jspromise.h"
#include "jsrealm.h"
#include "jssymbol.h"
#include <string.h>

/* Prototype instances have an undefined state slot and fail the Promise brand
 * check. The initialized states are pending=0, fulfilled=1 and rejected=2. */
JSClass js_PromiseClass = {
    "Promise", JSCLASS_HAS_RESERVED_SLOTS(4) | JSCLASS_HAS_CACHED_PROTO(JSProto_Promise),
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};
/* Unobservable, GC-traced records hold capabilities, resolving state, reactions
 * and job arguments. No pointers into temporary C storage survive callbacks. */
static JSClass recordClass = {
    "PromiseRecord", JSCLASS_HAS_RESERVED_SLOTS(8),
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};
#define P_SLOT(obj, n) OBJ_GET_SLOT(cx, obj, JSSLOT_START(&js_PromiseClass) + (n))
#define R_SLOT(obj, n) OBJ_GET_SLOT(cx, obj, JSSLOT_START(&recordClass) + (n))
#define SET(obj, n, v) JS_SetReservedSlot(cx, obj, n, v)

static JSBool Resolve(JSContext *, JSObject *, uintN, jsval *, jsval *);
static JSBool Reject(JSContext *, JSObject *, uintN, jsval *, jsval *);
static JSBool ReactionJob(JSContext *, JSObject *, uintN, jsval *, jsval *);
static JSBool ThenableJob(JSContext *, JSObject *, uintN, jsval *, jsval *);

static JSBool
PromiseTypeError(JSContext *cx)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_PROMISE_OPERATION);
    return JS_FALSE;
}

static JSObject *
PromiseObject(JSContext *cx, jsval value)
{
    JSObject *obj;
    if (JSVAL_IS_PRIMITIVE(value)) return NULL;
    obj = JSVAL_TO_OBJECT(value);
    return OBJ_GET_CLASS(cx, obj) == &js_PromiseClass &&
           JSVAL_IS_INT(P_SLOT(obj, 0)) ? obj : NULL;
}

static JSObject *
Record(JSContext *cx, JSObject *global)
{
    JSObject *proto = js_BuiltinPrototype(cx, global, JSProto_Object);
    JSObject *obj = proto ? js_NewObject(cx, &recordClass, proto, global) : NULL;
    JSTempValueRooter root;
    uintN i;
    JSBool ok = JS_TRUE;
    if (!obj) return NULL;
    JS_PUSH_TEMP_ROOT_OBJECT(cx, obj, &root);
    /* js_NewObject only allocates its initial slot vector. Materialize all
     * reserved slots before any direct reads, including undefined captures. */
    for (i = 0; i < 8; i++) {
        if (!SET(obj, i, JSVAL_VOID)) { ok = JS_FALSE; break; }
    }
    JS_POP_TEMP_ROOT(cx, &root);
    return ok ? obj : NULL;
}

/* As with Proxy.revocable, reserve captures before installing length/name.
 * ES2015's anonymous built-ins have no own name property. */
static JSObject *
Closure(JSContext *cx, JSObject *global, JSNative native, uintN length,
        JSObject *state)
{
    JSFunction *fun;
    JSObject *result = NULL;
    jsval values[2] = {OBJECT_TO_JSVAL(state), JSVAL_VOID};
    JSTempValueRooter root;
    JS_PUSH_TEMP_ROOT(cx, 2, values, &root);
    fun = js_NewFunction(cx, NULL, NULL, length,
                         JSFUN_STRICT | JSFUN_NO_CONSTRUCT, global, NULL);
    if (!fun) goto out;
    values[1] = OBJECT_TO_JSVAL(fun->object);
    fun->u.n.spare = 1;
    fun->u.n.native = native;
    fun->edition = JSVERSION_ECMA_2015;
    if (SET(fun->object, 2, values[0]) &&
        js_InitFunctionProperties(cx, fun->object) &&
        JS_DeleteProperty(cx, fun->object, "name")) result = fun->object;
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return result;
}

static JSObject *
Capture(JSContext *cx, jsval *argv)
{
    jsval value;
    JS_GetReservedSlot(cx, JSVAL_TO_OBJECT(argv[-2]), 2, &value);
    return JSVAL_TO_OBJECT(value);
}

static JSBool
ResolvingFunctions(JSContext *cx, JSObject *global, JSObject *promise,
                   jsval *resolve, jsval *reject)
{
    JSObject *state, *fun;
    JSTempValueRooter root;
    JSBool ok = JS_FALSE;
    state = Record(cx, global);
    if (!state) return JS_FALSE;
    JS_PUSH_TEMP_ROOT_OBJECT(cx, state, &root);
    if (!SET(state, 0, OBJECT_TO_JSVAL(promise)) ||
        !SET(state, 1, JSVAL_FALSE)) goto out;
    fun = Closure(cx, global, Resolve, 1, state);
    if (!fun) goto out;
    *resolve = OBJECT_TO_JSVAL(fun); /* Caller provides rooted output slots. */
    fun = Closure(cx, global, Reject, 1, state);
    if (!fun) goto out;
    *reject = OBJECT_TO_JSVAL(fun);
    ok = JS_TRUE;
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}

static JSBool
JobGlobal(JSContext *cx, JSObject *fallback, jsval callable, JSObject **global)
{
    *global = fallback;
    if (!JSVAL_IS_PRIMITIVE(callable)) {
        /* The callback's realm owns the job, including before sloppy-this
         * conversion. Bound functions and proxies follow their targets. A
         * revoked proxy still fails asynchronously when the job calls it. */
        JSObject *realm = js_ConstructorGlobal(cx, JSVAL_TO_OBJECT(callable));
        if (realm) *global = realm;
        else {
            if (!JS_IsExceptionPending(cx)) return JS_FALSE;
            JS_ClearPendingException(cx);
        }
    }
    return JS_TRUE;
}

static JSBool
QueueReaction(JSContext *cx, JSObject *reaction, int state, jsval argument)
{
    JSObject *global = OBJ_GET_PARENT(cx, reaction), *record, *job;
    jsval values[3] = {OBJECT_TO_JSVAL(reaction), argument, JSVAL_VOID};
    JSTempValueRooter root;
    JSBool ok = JS_FALSE;
    JS_PUSH_TEMP_ROOT(cx, 3, values, &root);
    if (!JobGlobal(cx, global, R_SLOT(reaction, state == 2 ? 2 : 1), &global)) goto out;
    record = Record(cx, global);
    if (!record) goto out;
    values[2] = OBJECT_TO_JSVAL(record);
    if (!SET(record, 0, values[0]) || !SET(record, 1, INT_TO_JSVAL(state)) ||
        !SET(record, 2, values[1])) goto out;
    job = Closure(cx, global, ReactionJob, 0, record);
    if (job) ok = JS_EnqueueJob(cx, job);
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}

static JSBool
Settle(JSContext *cx, JSObject *promise, int state, jsval argument)
{
    jsval values[3] = {OBJECT_TO_JSVAL(promise), argument, P_SLOT(promise, 2)};
    JSTempValueRooter root;
    JSObject *reaction;
    JSBool ok = JS_FALSE;
    JS_ASSERT(P_SLOT(promise, 0) == INT_TO_JSVAL(0));
    JS_PUSH_TEMP_ROOT(cx, 3, values, &root);
    if (!SET(promise, 0, INT_TO_JSVAL(state)) || !SET(promise, 1, argument) ||
        !SET(promise, 2, JSVAL_NULL) || !SET(promise, 3, JSVAL_NULL)) goto out;
    while (!JSVAL_IS_NULL(values[2])) {
        reaction = JSVAL_TO_OBJECT(values[2]);
        if (!QueueReaction(cx, reaction, state, argument)) goto out;
        values[2] = R_SLOT(reaction, 3);
    }
    ok = JS_TRUE;
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}

/* Convert an actual pending JS exception to a rejection. OOM/interruption
 * without an exception must propagate as engine failure, not fake success. */
static JSBool
RejectException(JSContext *cx, JSObject *promise)
{
    JSTempValueRooter root;
    JSBool ok;
    JS_PUSH_SINGLE_TEMP_ROOT(cx, JSVAL_VOID, &root);
    ok = JS_GetPendingException(cx, &root.u.value);
    if (ok) {
        JS_ClearPendingException(cx);
        ok = Settle(cx, promise, 2, root.u.value);
    }
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}

static JSBool
Resolve(JSContext *cx, JSObject *ignored, uintN argc, jsval *argv, jsval *rval)
{
    JSObject *record = Capture(cx, argv), *promise, *global, *job, *args;
    jsval values[3] = {argv[0], JSVAL_VOID, JSVAL_VOID};
    JSTempValueRooter root;
    JSBool ok = JS_FALSE;
    *rval = JSVAL_VOID;
    if (R_SLOT(record, 1) == JSVAL_TRUE) return JS_TRUE;
    if (!SET(record, 1, JSVAL_TRUE)) return JS_FALSE;
    promise = JSVAL_TO_OBJECT(R_SLOT(record, 0));
    if (values[0] == OBJECT_TO_JSVAL(promise)) {
        PromiseTypeError(cx);
        return RejectException(cx, promise);
    }
    if (JSVAL_IS_PRIMITIVE(values[0])) return Settle(cx, promise, 1, values[0]);
    JS_PUSH_TEMP_ROOT(cx, 3, values, &root);
    if (!JS_GetProperty(cx, JSVAL_TO_OBJECT(values[0]), "then", &values[1])) {
        ok = RejectException(cx, promise);
        goto out;
    }
    if (!js_IsCallable(cx, values[1])) {
        ok = Settle(cx, promise, 1, values[0]);
        goto out;
    }
    if (!JobGlobal(cx, js_BuiltinGlobal(cx, argv), values[1], &global)) goto out;
    args = Record(cx, global);
    if (!args) goto out;
    values[2] = OBJECT_TO_JSVAL(args);
    if (!SET(args, 0, OBJECT_TO_JSVAL(promise)) ||
        !SET(args, 1, values[0]) || !SET(args, 2, values[1])) goto out;
    job = Closure(cx, global, ThenableJob, 0, args);
    if (job) ok = JS_EnqueueJob(cx, job);
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}

static JSBool
Reject(JSContext *cx, JSObject *ignored, uintN argc, jsval *argv, jsval *rval)
{
    JSObject *state = Capture(cx, argv);
    *rval = JSVAL_VOID;
    if (R_SLOT(state, 1) == JSVAL_TRUE) return JS_TRUE;
    return SET(state, 1, JSVAL_TRUE) &&
           Settle(cx, JSVAL_TO_OBJECT(R_SLOT(state, 0)), 2, argv[0]);
}

static JSBool
ThenableJob(JSContext *cx, JSObject *ignored, uintN argc, jsval *argv, jsval *rval)
{
    JSObject *args = Capture(cx, argv);
    jsval values[4] = {JSVAL_VOID, JSVAL_VOID, JSVAL_VOID, JSVAL_VOID};
    JSTempValueRooter root;
    JSBool ok = JS_FALSE;
    JS_PUSH_TEMP_ROOT(cx, 4, values, &root);
    if (!ResolvingFunctions(cx, js_BuiltinGlobal(cx, argv),
                            JSVAL_TO_OBJECT(R_SLOT(args, 0)), &values[0], &values[1])) goto out;
    ok = js_InternalInvokeValue(cx, R_SLOT(args, 1), R_SLOT(args, 2), 0,
                                2, values, &values[2]);
    if (!ok && JS_GetPendingException(cx, &values[3])) {
        JS_ClearPendingException(cx);
        ok = js_InternalInvokeValue(cx, JSVAL_VOID, values[1], 0, 1, &values[3], &values[2]);
    }
    *rval = JSVAL_VOID;
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}

static JSBool
ReactionJob(JSContext *cx, JSObject *ignored, uintN argc, jsval *argv, jsval *rval)
{
    JSObject *args = Capture(cx, argv), *reaction, *capability;
    jsval values[3] = {R_SLOT(args, 2), JSVAL_VOID, JSVAL_VOID};
    JSTempValueRooter root;
    jsval handler;
    JSBool ok, rejected;
    reaction = JSVAL_TO_OBJECT(R_SLOT(args, 0));
    capability = JSVAL_TO_OBJECT(R_SLOT(reaction, 0));
    rejected = R_SLOT(args, 1) == INT_TO_JSVAL(2);
    handler = R_SLOT(reaction, rejected ? 2 : 1);
    JS_PUSH_TEMP_ROOT(cx, 3, values, &root);
    if (JSVAL_IS_VOID(handler)) values[1] = values[0];
    else {
        ok = js_InternalInvokeValue(cx, JSVAL_VOID, handler, 0, 1, values, &values[1]);
        rejected = !ok;
        if (!ok) {
            if (!JS_GetPendingException(cx, &values[1])) goto out;
            JS_ClearPendingException(cx);
        }
    }
    ok = js_InternalInvokeValue(cx, JSVAL_VOID, R_SLOT(capability, rejected ? 2 : 1),
                                0, 1, &values[1], &values[2]);
    *rval = JSVAL_VOID;
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}

static JSBool
CapabilityExecutor(JSContext *cx, JSObject *ignored, uintN argc, jsval *argv, jsval *rval)
{
    JSObject *record = Capture(cx, argv);
    if (!JSVAL_IS_VOID(R_SLOT(record, 1)) || !JSVAL_IS_VOID(R_SLOT(record, 2)))
        return PromiseTypeError(cx);
    *rval = JSVAL_VOID;
    return SET(record, 1, argv[0]) && SET(record, 2, argv[1]);
}

static JSObject *
ConstructCapability(JSContext *cx, jsval constructor, jsval executor)
{
    JSStackFrame *frame = cx->fp;
    jsval *base, *oldsp;
    JSObject *result = NULL;
    void *mark;
    base = js_AllocStack(cx, 3, &mark);
    if (!base) return NULL;
    base[0] = constructor; base[1] = JSVAL_NULL; base[2] = executor;
    oldsp = frame->sp; frame->sp = base + 3;
    if (js_InvokeConstructorWithNewTarget(cx, base, 1, NULL))
        result = JSVAL_TO_OBJECT(base[0]);
    frame->sp = oldsp;
    js_FreeStack(cx, mark);
    return result;
}

static JSObject *
Capability(JSContext *cx, JSObject *global, jsval constructor)
{
    JSObject *record = NULL, *executor, *promise, *result = NULL;
    jsval values[3] = {constructor, JSVAL_VOID, JSVAL_VOID};
    JSTempValueRooter root;
    if (!js_IsConstructor(cx, constructor)) { PromiseTypeError(cx); return NULL; }
    JS_PUSH_TEMP_ROOT(cx, 3, values, &root);
    record = Record(cx, global);
    if (!record) goto out;
    values[1] = OBJECT_TO_JSVAL(record);
    executor = Closure(cx, global, CapabilityExecutor, 2, record);
    if (!executor) goto out;
    values[2] = OBJECT_TO_JSVAL(executor);
    promise = ConstructCapability(cx, constructor, values[2]);
    if (!promise) goto out;
    values[0] = OBJECT_TO_JSVAL(promise);
    if (!js_IsCallable(cx, R_SLOT(record, 1)) || !js_IsCallable(cx, R_SLOT(record, 2))) {
        PromiseTypeError(cx); goto out;
    }
    if (SET(record, 0, values[0])) result = record;
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return result;
}

JSBool
js_PromiseConstructor(JSContext *cx, JSObject *ignored, uintN argc,
                      jsval *argv, jsval *rval)
{
    JSObject *target, *global, *proto, *promise;
    jsval values[5] = {JSVAL_VOID, JSVAL_VOID, JSVAL_VOID, JSVAL_VOID, JSVAL_VOID};
    JSTempValueRooter root;
    JSBool ok = JS_FALSE;
    if (!(cx->fp->flags & JSFRAME_CONSTRUCTING) || !js_IsCallable(cx, argv[0]))
        return PromiseTypeError(cx);
    target = cx->fp->newTarget ? cx->fp->newTarget : JSVAL_TO_OBJECT(argv[-2]);
    global = js_BuiltinGlobal(cx, argv);
    JS_PUSH_TEMP_ROOT(cx, 5, values, &root);
    if (!OBJ_GET_PROPERTY(cx, target, ATOM_TO_JSID(cx->runtime->atomState.classPrototypeAtom), &values[0])) goto out;
    if (JSVAL_IS_PRIMITIVE(values[0])) {
        global = js_ConstructorGlobal(cx, target);
        if (!global) goto out;
        proto = js_BuiltinPrototype(cx, global, JSProto_Promise);
        if (!proto) goto out;
        values[0] = OBJECT_TO_JSVAL(proto);
    }
    promise = js_NewObject(cx, &js_PromiseClass, JSVAL_TO_OBJECT(values[0]), global);
    if (!promise) goto out;
    values[0] = OBJECT_TO_JSVAL(promise);
    if (!SET(promise, 0, INT_TO_JSVAL(0)) || !SET(promise, 1, JSVAL_VOID) ||
        !SET(promise, 2, JSVAL_NULL) || !SET(promise, 3, JSVAL_NULL) ||
        !ResolvingFunctions(cx, global, promise, &values[1], &values[2])) goto out;
    ok = js_InternalInvokeValue(cx, JSVAL_VOID, argv[0], 0, 2, &values[1], &values[3]);
    if (!ok && JS_GetPendingException(cx, &values[4])) {
        JS_ClearPendingException(cx);
        ok = js_InternalInvokeValue(cx, JSVAL_VOID, values[2], 0, 1, &values[4], &values[3]);
    }
    if (ok) *rval = values[0];
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}

static JSBool
Then(JSContext *cx, JSObject *ignored, uintN argc, jsval *argv, jsval *rval)
{
    JSObject *promise = PromiseObject(cx, argv[-1]);
    JSObject *global = js_BuiltinGlobal(cx, argv), *capability, *reaction, *constructor;
    jsval values[3] = {JSVAL_VOID, JSVAL_VOID, JSVAL_VOID};
    JSTempValueRooter root;
    jsid species;
    int state;
    JSBool ok = JS_FALSE;
    if (!promise) return PromiseTypeError(cx);
    JS_PUSH_TEMP_ROOT(cx, 3, values, &root);
    if (!JS_GetProperty(cx, promise, "constructor", &values[0])) goto out;
    if (!JSVAL_IS_VOID(values[0])) {
        if (JSVAL_IS_PRIMITIVE(values[0])) { PromiseTypeError(cx); goto out; }
        if (!js_WellKnownSymbolId(cx, JS_WKS_SPECIES, &species) ||
            !OBJ_GET_PROPERTY(cx, JSVAL_TO_OBJECT(values[0]), species, &values[0])) goto out;
    }
    if (JSVAL_IS_VOID(values[0]) || JSVAL_IS_NULL(values[0])) {
        constructor = js_GetCachedClassObject(cx, global, JSProto_Promise);
        if (!constructor) goto out;
        values[0] = OBJECT_TO_JSVAL(constructor);
    }
    capability = Capability(cx, global, values[0]);
    if (!capability) goto out;
    values[1] = OBJECT_TO_JSVAL(capability);
    reaction = Record(cx, global);
    if (!reaction) goto out;
    values[2] = OBJECT_TO_JSVAL(reaction);
    if (!SET(reaction, 0, values[1]) ||
        !SET(reaction, 1, js_IsCallable(cx, argv[0]) ? argv[0] : JSVAL_VOID) ||
        !SET(reaction, 2, js_IsCallable(cx, argv[1]) ? argv[1] : JSVAL_VOID) ||
        !SET(reaction, 3, JSVAL_NULL)) goto out;
    /* Species/capability callbacks may have settled the source Promise. */
    state = JSVAL_TO_INT(P_SLOT(promise, 0));
    if (state == 0) {
        if (!JSVAL_IS_NULL(P_SLOT(promise, 3)) &&
            !SET(JSVAL_TO_OBJECT(P_SLOT(promise, 3)), 3, values[2])) goto out;
        if (JSVAL_IS_NULL(P_SLOT(promise, 2)) && !SET(promise, 2, values[2])) goto out;
        ok = SET(promise, 3, values[2]);
    } else ok = QueueReaction(cx, reaction, state, P_SLOT(promise, 1));
    if (ok) *rval = R_SLOT(capability, 0);
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}

static JSBool
GetRawProperty(JSContext *cx, JSObject *obj, jsval receiver,
               const char *name, jsval *value)
{
    JSAtom *atom = js_Atomize(cx, name, strlen(name), 0);
    JSTempValueRooter root;
    JSBool ok;
    if (!atom) return JS_FALSE;
    JS_PUSH_SINGLE_TEMP_ROOT(cx, ATOM_KEY(atom), &root);
    ok = js_GetPropertyValue(cx, obj, receiver, ATOM_TO_JSID(atom), value);
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}

static JSBool
Catch(JSContext *cx, JSObject *ignored, uintN argc, jsval *argv, jsval *rval)
{
    JSObject *receiver;
    jsval values[5] = {argv[-1], JSVAL_VOID, JSVAL_VOID, JSVAL_VOID, argv[0]};
    JSTempValueRooter root;
    JSBool ok = JS_FALSE;
    JS_PUSH_TEMP_ROOT(cx, 5, values, &root);
    receiver = js_BuiltinToObject(cx, js_BuiltinGlobal(cx, argv), values[0]);
    if (!receiver) goto out;
    values[1] = OBJECT_TO_JSVAL(receiver);
    if (!GetRawProperty(cx, receiver, values[0], "then", &values[2])) goto out;
    ok = js_InternalInvokeValue(cx, values[0], values[2], 0, 2, &values[3], rval);
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}

static JSBool
StaticResolution(JSContext *cx, uintN argc, jsval *argv, jsval *rval, JSBool reject)
{
    JSObject *promise, *capability;
    jsval values[2] = {JSVAL_VOID, JSVAL_VOID};
    JSTempValueRooter root;
    JSBool ok = JS_FALSE;
    if (JSVAL_IS_PRIMITIVE(argv[-1])) return PromiseTypeError(cx);
    JS_PUSH_TEMP_ROOT(cx, 2, values, &root);
    promise = reject ? NULL : PromiseObject(cx, argv[0]);
    if (promise) {
        if (!JS_GetProperty(cx, promise, "constructor", &values[0])) goto out;
        if (values[0] == argv[-1]) { *rval = argv[0]; ok = JS_TRUE; goto out; }
    }
    capability = Capability(cx, js_BuiltinGlobal(cx, argv), argv[-1]);
    if (!capability) goto out;
    values[0] = OBJECT_TO_JSVAL(capability);
    ok = js_InternalInvokeValue(cx, JSVAL_VOID, R_SLOT(capability, reject ? 2 : 1),
                                0, 1, argv, &values[1]);
    if (ok) *rval = R_SLOT(capability, 0);
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}
static JSBool
StaticResolve(JSContext *cx, JSObject *ignored, uintN argc, jsval *argv, jsval *rval)
{ return StaticResolution(cx, argc, argv, rval, JS_FALSE); }
static JSBool
StaticReject(JSContext *cx, JSObject *ignored, uintN argc, jsval *argv, jsval *rval)
{ return StaticResolution(cx, argc, argv, rval, JS_TRUE); }

/* Shared all-state: values array, capability, remaining count. Each element
 * closure separately captures shared-state, index, and AlreadyCalled. */
static JSBool
AllCompleted(JSContext *cx, JSObject *shared, jsval *rval)
{
    JSObject *capability;
    jsdouble remaining;
    jsval values[2] = {JSVAL_VOID, R_SLOT(shared, 0)};
    JSTempValueRooter root;
    JSBool ok = JS_FALSE;
    JS_PUSH_TEMP_ROOT(cx, 2, values, &root);
    if (!js_ValueToNumber(cx, R_SLOT(shared, 2), &remaining) ||
        !js_NewNumberValue(cx, remaining - 1, &values[0]) ||
        !SET(shared, 2, values[0])) goto out;
    if (remaining != 1) { *rval = JSVAL_VOID; ok = JS_TRUE; goto out; }
    capability = JSVAL_TO_OBJECT(R_SLOT(shared, 1));
    ok = js_InternalInvokeValue(cx, JSVAL_VOID, R_SLOT(capability, 1), 0, 1, &values[1], rval);
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}
static JSBool
AllElement(JSContext *cx, JSObject *ignored, uintN argc, jsval *argv, jsval *rval)
{
    JSObject *state = Capture(cx, argv), *shared;
    jsdouble index;
    jsid id;
    JSTempValueRooter root;
    JSBool ok;
    *rval = JSVAL_VOID;
    if (R_SLOT(state, 2) == JSVAL_TRUE) return JS_TRUE;
    if (!SET(state, 2, JSVAL_TRUE)) return JS_FALSE;
    shared = JSVAL_TO_OBJECT(R_SLOT(state, 0));
    if (!js_ValueToNumber(cx, R_SLOT(state, 1), &index) ||
        !js_ArrayLikeIndex(cx, index, &id)) return JS_FALSE;
    JS_PUSH_SINGLE_TEMP_ROOT(cx, ID_TO_VALUE(id), &root);
    ok = js_CreateDataPropertyOrThrow(cx, JSVAL_TO_OBJECT(R_SLOT(shared, 0)), id, argv[0]) &&
         AllCompleted(cx, shared, rval);
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}

static JSBool
Iterate(JSContext *cx, jsval *argv, jsval *rval, JSBool all)
{
    /* capability, source box, method/scratch, iterator, step, value,
     * next promise, shared state, then resolve argument, then reject argument,
     * element state/scratch, rooted index id */
    jsval values[12];
    JSTempValueRooter root;
    JSObject *global = js_BuiltinGlobal(cx, argv), *capability, *source, *iterator = NULL;
    JSObject *shared = NULL, *array, *proto, *state, *fun;
    jsid id;
    jsdouble index = 0, remaining;
    JSBool done = JS_TRUE, stepDone, ok = JS_FALSE;
    uintN i;
    if (JSVAL_IS_PRIMITIVE(argv[-1])) return PromiseTypeError(cx);
    for (i = 0; i < 12; i++) values[i] = JSVAL_VOID;
    JS_PUSH_TEMP_ROOT(cx, 12, values, &root);
    /* The pinned suite incorporates removal of static @@species lookup. */
    capability = Capability(cx, global, argv[-1]);
    if (!capability) goto out;
    values[0] = OBJECT_TO_JSVAL(capability);
    source = js_BuiltinToObject(cx, global, argv[0]);
    if (!source) goto reject;
    values[1] = OBJECT_TO_JSVAL(source);
    if (!js_WellKnownSymbolId(cx, JS_WKS_ITERATOR, &id) ||
        !js_GetPropertyValue(cx, source, argv[0], id, &values[2]) ||
        !js_InternalInvokeValue(cx, argv[0], values[2], 0, 0, NULL, &values[3])) goto reject;
    if (JSVAL_IS_PRIMITIVE(values[3])) { PromiseTypeError(cx); goto reject; }
    iterator = JSVAL_TO_OBJECT(values[3]);
    done = JS_FALSE;
    if (all) {
        shared = Record(cx, global);
        if (!shared) goto reject;
        values[7] = OBJECT_TO_JSVAL(shared);
        proto = js_BuiltinPrototype(cx, global, JSProto_Array);
        array = proto ? js_NewArrayObjectWithProto(cx, 0, NULL, proto, global) : NULL;
        if (!array) goto reject;
        values[10] = OBJECT_TO_JSVAL(array);
        if (!SET(shared, 0, values[10]) ||
            !SET(shared, 1, values[0]) || !SET(shared, 2, INT_TO_JSVAL(1))) goto reject;
    }
    for (;;) {
        if (cx->branchCallback && !cx->branchCallback(cx, NULL)) goto reject;
        /* Abrupt IteratorStep/IteratorValue sets [[Done]]: do not close. */
        done = JS_TRUE;
        if (!JS_GetProperty(cx, iterator, "next", &values[2]) ||
            !js_InternalInvokeValue(cx, values[3], values[2], 0, 0, NULL, &values[4])) goto reject;
        if (JSVAL_IS_PRIMITIVE(values[4])) { PromiseTypeError(cx); goto reject; }
        if (!JS_GetProperty(cx, JSVAL_TO_OBJECT(values[4]), "done", &values[2]) ||
            !js_ValueToBoolean(cx, values[2], &stepDone)) goto reject;
        if (stepDone) {
            if (all && !AllCompleted(cx, shared, &values[2])) goto reject;
            *rval = R_SLOT(capability, 0); ok = JS_TRUE; goto out;
        }
        if (!JS_GetProperty(cx, JSVAL_TO_OBJECT(values[4]), "value", &values[5])) goto reject;
        done = JS_FALSE;
        if (all) {
            if (index >= 4294967295.0) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_BAD_ARRAY_LENGTH);
                goto reject;
            }
            if (!js_ArrayLikeIndex(cx, index, &id)) goto reject;
            values[11] = ID_TO_VALUE(id);
            if (!js_CreateDataPropertyOrThrow(cx, JSVAL_TO_OBJECT(R_SLOT(shared, 0)), id, JSVAL_VOID)) goto reject;
        }
        if (!JS_GetProperty(cx, JSVAL_TO_OBJECT(argv[-1]), "resolve", &values[2]) ||
            !js_InternalInvokeValue(cx, argv[-1], values[2], 0, 1, &values[5], &values[6])) goto reject;
        if (all) {
            state = Record(cx, global);
            if (!state) goto reject;
            values[10] = OBJECT_TO_JSVAL(state);
            if (!js_NewNumberValue(cx, index, &values[2]) ||
                !SET(state, 0, values[7]) || !SET(state, 1, values[2]) ||
                !SET(state, 2, JSVAL_FALSE)) goto reject;
            fun = Closure(cx, global, AllElement, 1, state);
            if (!fun) goto reject;
            values[8] = OBJECT_TO_JSVAL(fun);
            if (!js_ValueToNumber(cx, R_SLOT(shared, 2), &remaining) ||
                !js_NewNumberValue(cx, remaining + 1, &values[2]) ||
                !SET(shared, 2, values[2])) goto reject;
        } else values[8] = R_SLOT(capability, 1);
        values[9] = R_SLOT(capability, 2);
        source = js_BuiltinToObject(cx, global, values[6]);
        if (!source) goto reject;
        values[1] = OBJECT_TO_JSVAL(source);
        if (!GetRawProperty(cx, source, values[6], "then", &values[2]) ||
            !js_InternalInvokeValue(cx, values[6], values[2], 0, 2, &values[8], &values[10])) goto reject;
        ++index;
    }
  reject:
    if (iterator && !done) js_IteratorCloseThrow(cx, iterator);
    if (JS_GetPendingException(cx, &values[10])) {
        JS_ClearPendingException(cx);
        ok = js_InternalInvokeValue(cx, JSVAL_VOID, R_SLOT(capability, 2),
                                    0, 1, &values[10], &values[2]);
        if (ok) *rval = R_SLOT(capability, 0);
    }
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}
static JSBool
All(JSContext *cx, JSObject *ignored, uintN argc, jsval *argv, jsval *rval)
{ return Iterate(cx, argv, rval, JS_TRUE); }
static JSBool
Race(JSContext *cx, JSObject *ignored, uintN argc, jsval *argv, jsval *rval)
{ return Iterate(cx, argv, rval, JS_FALSE); }
static JSBool
Species(JSContext *cx, JSObject *ignored, uintN argc, jsval *argv, jsval *rval)
{ *rval = argv[-1]; return JS_TRUE; }

static JSBool
DefineMethod(JSContext *cx, JSObject *obj, const char *name, JSNative native, uintN length)
{
    JSFunction *fun = JS_DefineFunction(cx, obj, name, native, length,
                                       JSFUN_STRICT | JSFUN_NO_CONSTRUCT);
    if (!fun) return JS_FALSE;
    /* Promise is a modern intrinsic even when requested by a legacy global.
     * Keep its metadata and error realm independent of the calling script. */
    fun->edition = JSVERSION_ECMA_2015;
    return js_InitFunctionProperties(cx, fun->object);
}

JSObject *
js_InitPromiseClass(JSContext *cx, JSObject *global)
{
    JSObject *proto, *ctor;
    JSFunction *fun;
    JSTempValueRooter root;
    jsid id;
    JSBool ok = JS_FALSE;
    proto = JS_InitClass(cx, global, NULL, &js_PromiseClass,
                         js_PromiseConstructor, 1, NULL, NULL, NULL, NULL);
    if (!proto) return NULL;
    JS_PUSH_TEMP_ROOT_OBJECT(cx, proto, &root);
    ctor = JS_GetConstructor(cx, proto);
    if (!ctor) goto out;
    fun = (JSFunction *)JS_GetPrivate(cx, ctor);
    fun->flags |= JSFUN_STRICT;
    fun->edition = JSVERSION_ECMA_2015;
    if (!js_InitFunctionProperties(cx, ctor) ||
        !DefineMethod(cx, proto, "then", Then, 2) ||
        !DefineMethod(cx, proto, "catch", Catch, 1) ||
        !DefineMethod(cx, ctor, "resolve", StaticResolve, 1) ||
        !DefineMethod(cx, ctor, "reject", StaticReject, 1) ||
        !DefineMethod(cx, ctor, "all", All, 1) ||
        !DefineMethod(cx, ctor, "race", Race, 1) ||
        !js_DefineBuiltinTag(cx, proto, "Promise")) goto out;
    fun = JS_NewFunction(cx, Species, 0, JSFUN_STRICT | JSFUN_NO_CONSTRUCT,
                         global, "get [Symbol.species]");
    if (!fun) goto out;
    {
        JSTempValueRooter getterRoot;
        JS_PUSH_TEMP_ROOT_OBJECT(cx, fun->object, &getterRoot);
        fun->edition = JSVERSION_ECMA_2015;
        ok = js_InitFunctionProperties(cx, fun->object) &&
             js_WellKnownSymbolId(cx, JS_WKS_SPECIES, &id) &&
             OBJ_DEFINE_PROPERTY(cx, ctor, id, JSVAL_VOID, (JSPropertyOp)fun->object,
                                 NULL, JSPROP_GETTER | JSPROP_SHARED, NULL);
        JS_POP_TEMP_ROOT(cx, &getterRoot);
    }
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok ? proto : NULL;
}
