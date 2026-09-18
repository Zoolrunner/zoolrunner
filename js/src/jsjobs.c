/* Explicit ECMAScript job checkpoints for classic embedders.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "jsapi.h"
#include "jscntxt.h"
#include "jsfun.h"
#include "jsgc.h"
#include "jsinterp.h"
#include "jsjobs.h"
#include "jsiteres6.h"
#include "jsobj.h"
#include "jsrealm.h"
#include <string.h>

static JSClass jobClass = {
    "Job", JSCLASS_HAS_RESERVED_SLOTS(2),
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};
#ifdef JS_THREADSAFE
#define QUEUE(cx) (&(cx)->thread->jobs)
#else
#define QUEUE(cx) (&(cx)->runtime->jobs)
#endif
#if defined(JS_PARANOID_REQUEST) && defined(JS_THREADSAFE)
#define JOB_CHECK_REQUEST(cx) JS_ASSERT((cx)->requestDepth)
#else
#define JOB_CHECK_REQUEST(cx) ((void)0)
#endif
#define JOB_SLOT(obj, index) OBJ_GET_SLOT(cx, obj, JSSLOT_START(&jobClass) + (index))

void
js_MarkJobs(JSContext *cx)
{
#ifndef JS_THREADSAFE
    if (cx->runtime->jobs.head)
        GC_MARK(cx, cx->runtime->jobs.head, "pending ECMAScript jobs");
#endif
}

void
js_ClearJobs(JSContext *cx)
{
    JSJobQueue *queue = QUEUE(cx);
    queue->head = queue->tail = NULL;
}

JS_PUBLIC_API(JSBool)
JS_HasPendingJobs(JSContext *cx)
{
    JOB_CHECK_REQUEST(cx);
    return QUEUE(cx)->head != NULL;
}

JS_PUBLIC_API(JSBool)
JS_EnqueueJob(JSContext *cx, JSObject *callback)
{
    jsval values[2] = {OBJECT_TO_JSVAL(callback), JSVAL_VOID};
    JSTempValueRooter root;
    JSJobQueue *queue = QUEUE(cx);
    JSObject *job, *global = callback, *parent, *proto;
    JSBool ok = JS_FALSE;
    JOB_CHECK_REQUEST(cx);
    if (!js_IsCallable(cx, values[0])) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_NOT_FUNCTION, "job callback");
        return JS_FALSE;
    }
    JS_PUSH_TEMP_ROOT(cx, 2, values, &root);
    while ((parent = OBJ_GET_PARENT(cx, global)) != NULL) global = parent;
    proto = js_BuiltinPrototype(cx, global, JSProto_Object);
    if (!proto) goto out;
    job = js_NewObject(cx, &jobClass, proto, global);
    if (!job) goto out;
    values[1] = OBJECT_TO_JSVAL(job);
    if (!JS_SetReservedSlot(cx, job, 0, values[0]) ||
        !JS_SetReservedSlot(cx, job, 1, JSVAL_NULL)) goto out;
    /* Only this thread mutates its queue. A request prevents concurrent GC;
     * all contexts on this thread share one FIFO, including temporary ones. */
    if (queue->tail && !JS_SetReservedSlot(cx, queue->tail, 1, values[1])) goto out;
    if (!queue->head) queue->head = job;
    queue->tail = job;
    ok = JS_TRUE;
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}

JS_PUBLIC_API(JSBool)
JS_RunJobs(JSContext *cx)
{
    JSJobQueue *queue = QUEUE(cx);
    jsval values[2] = {JSVAL_VOID, JSVAL_VOID};
    JSTempValueRooter root;
    JSObject *job;
    JSStackFrame frame;
    JSBool ok = JS_TRUE;
    JOB_CHECK_REQUEST(cx);
    /* A callback requesting another checkpoint cannot run later jobs ahead
     * of the remainder of the currently executing job. */
    if (queue->draining) return JS_TRUE;
    if (JS_IsExceptionPending(cx)) return JS_FALSE;
    queue->draining = JS_TRUE;
    JS_PUSH_TEMP_ROOT(cx, 2, values, &root);
    while ((job = queue->head) != NULL) {
        if (cx->branchCallback && !cx->branchCallback(cx, NULL)) { ok = JS_FALSE; break; }
        values[0] = OBJECT_TO_JSVAL(job);
        queue->head = JSVAL_TO_OBJECT(JOB_SLOT(job, 1));
        if (!queue->head) queue->tail = NULL;
        /* Callback remains rooted by job through the entire invocation. */
        memset(&frame, 0, sizeof frame);
        frame.flags = JSFRAME_JOB | JSFRAME_INTERNAL;
        frame.scopeChain = OBJ_GET_PARENT(cx, job);
        frame.varobj = frame.scopeChain;
        frame.down = cx->fp;
        cx->fp = &frame;
        ok = js_InternalInvokeValue(cx, JSVAL_VOID, JOB_SLOT(job, 0), 0, 0, NULL, &values[1]);
        cx->fp = frame.down;
        if (!ok) break;
        values[0] = values[1] = JSVAL_VOID;
    }
    JS_POP_TEMP_ROOT(cx, &root);
    queue->draining = JS_FALSE;
    return ok;
}
