/* Native job FIFO, request/GC roots, contexts, errors and thread ownership.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "jsapi.h"
#include "prthread.h"
#include "prlock.h"
#include "prcvar.h"
#include <stdio.h>
#include <string.h>
static unsigned checks;
typedef struct State {
    JSRuntime *rt;
    PRThread *mainThread;
    unsigned mainRuns, workerRuns, failures, finalized;
    PRLock *lock;
    PRCondVar *condition;
    JSContext *transfer;
    unsigned handoff;
    PRUintn cleanupIndex;
    unsigned cleaned;
} State;
static void Finalize(JSContext *cx, JSObject *obj)
{ State *s=(State *)JS_GetRuntimePrivate(JS_GetRuntime(cx)); if(s)++s->finalized; }
static JSClass globalClass = {
    "global", 0,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, Finalize,
    JSCLASS_NO_OPTIONAL_MEMBERS
};
static JSBool Enqueue(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    *rval=JSVAL_VOID;
    return argc && !JSVAL_IS_PRIMITIVE(argv[0]) && JS_EnqueueJob(cx,JSVAL_TO_OBJECT(argv[0]));
}
static JSBool Drain(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{ *rval=JSVAL_VOID; return JS_RunJobs(cx); }
static JSBool Collect(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{ JS_GC(cx); *rval=JSVAL_VOID; return JS_TRUE; }
static JSBool Stop(JSContext *cx, JSScript *script)
{ if(!script){JS_GC(cx);return JS_FALSE;}return JS_TRUE; }
static JSBool MainJob(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    State *s=(State *)JS_GetRuntimePrivate(JS_GetRuntime(cx));
    ++s->mainRuns;if(PR_GetCurrentThread()!=s->mainThread)++s->failures;
    *rval=JSVAL_VOID;return JS_TRUE;
}
static JSBool WorkerJob(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    State *s=(State *)JS_GetRuntimePrivate(JS_GetRuntime(cx));
    ++s->workerRuns;if(PR_GetCurrentThread()==s->mainThread)++s->failures;
    *rval=JSVAL_VOID;return JS_TRUE;
}
static JSBool Evaluate(JSContext *cx, JSObject *global, const char *source)
{
    jsval value;
    return JS_EvaluateScript(cx,global,source,strlen(source),"job-queue",1,&value)&&value==JSVAL_TRUE;
}
static void PR_CALLBACK Worker(void *data)
{
    State *s=(State *)data;
    JSContext *cx;
    JSObject *global;
    JSFunction *job;
    unsigned pass;
    for(pass=0;pass<2;pass++) {
        cx=JS_NewContext(s->rt,8192);
        if(!cx){++s->failures;return;}
        JS_BeginRequest(cx);JS_SetVersion(cx,JSVERSION_ECMA_2015);
        global=JS_NewObject(cx,&globalClass,NULL,NULL);
        JS_SetGlobalObject(cx,global);
        if(!global||!JS_InitStandardClasses(cx,global)) {++s->failures;goto done;}
        if(JS_HasPendingJobs(cx))++s->failures;
        job=JS_NewFunction(cx,WorkerJob,0,0,global,"workerJob");
        if(!job||!JS_EnqueueJob(cx,JS_GetFunctionObject(job))) {++s->failures;goto done;}
        JS_ClearNewbornRoots(cx);JS_GC(cx);
        if(pass==0 && (!JS_RunJobs(cx)||JS_HasPendingJobs(cx)))++s->failures;
        /* The second context deliberately leaves a queued job at teardown. */
      done:
        JS_EndRequest(cx);JS_DestroyContext(cx);
    }
}
#define CHECK(c) do {++checks;if(!(c)){fprintf(stderr,"FAIL job queue check %u\n",checks);goto out;}}while(0)
/* Transfer the last context while its original OS thread remains alive. The
 * old thread's TLS destructor must not retain a root or read an empty list as
 * a context. Classic embedders reattach directly without an explicit clear. */
static void PR_CALLBACK TransferWorker(void *data)
{
    State *s = (State *)data;
    JSContext *cx = JS_NewContext(s->rt, 8192);
    JSObject *global;
    JSFunction *job;
    if (cx) {
        JS_BeginRequest(cx);
        global = JS_NewObject(cx, &globalClass, NULL, NULL);
        JS_SetGlobalObject(cx, global);
        if (!global || !JS_InitStandardClasses(cx, global)) ++s->failures;
        else {
            job = JS_NewFunction(cx, WorkerJob, 0, 0, global, "cancelOnTransfer");
            if (!job || !JS_EnqueueJob(cx, JS_GetFunctionObject(job))) ++s->failures;
        }
        JS_EndRequest(cx);
    }
    PR_Lock(s->lock);
    s->transfer = cx;
    s->handoff = 1;
    PR_NotifyCondVar(s->condition);
    while (s->handoff != 2)
        PR_WaitCondVar(s->condition, PR_INTERVAL_NO_TIMEOUT);
    PR_Unlock(s->lock);
}
static void PR_CALLBACK ContextCleanup(void *data)
{
    JSContext *cx = (JSContext *)data;
    State *s = (State *)JS_GetRuntimePrivate(JS_GetRuntime(cx));
    /* Registered after the engine's TLS key: its record is already gone. */
    if (JS_GetContextThread(cx) != 0) ++s->failures;
    if (JS_SetContextThread(cx) == -1) { ++s->failures; return; }
    JS_BeginRequest(cx);
    if (JS_HasPendingJobs(cx)) ++s->failures;
    JS_EndRequest(cx);
    JS_DestroyContext(cx);
    ++s->cleaned;
}
static void PR_CALLBACK ExitWithContext(void *data)
{
    State *s = (State *)data;
    JSContext *cx = JS_NewContext(s->rt, 8192);
    JSObject *global;
    JSFunction *job;
    if (!cx) { ++s->failures; return; }
    JS_BeginRequest(cx);
    global = JS_NewObject(cx, &globalClass, NULL, NULL);
    JS_SetGlobalObject(cx, global);
    if (!global || !JS_InitStandardClasses(cx, global)) ++s->failures;
    else {
        job = JS_NewFunction(cx, WorkerJob, 0, 0, global, "cancelAtThreadExit");
        if (!job || !JS_EnqueueJob(cx, JS_GetFunctionObject(job))) ++s->failures;
    }
    JS_EndRequest(cx);
    if (PR_SetThreadPrivate(s->cleanupIndex, cx) != PR_SUCCESS) {
        ++s->failures;
        JS_DestroyContext(cx);
    }
}
int main(void)
{
    State state;
    JSRuntime *rt;
    JSContext *cx,*other=NULL;
    JSObject *global,*foreign;
    JSFunction *job;
    PRThread *worker=NULL;
    jsval value;
    int status=1;
    memset(&state,0,sizeof state);
    rt=JS_NewRuntime(16*1024*1024);if(!rt)return 1;
    state.rt=rt;state.mainThread=PR_GetCurrentThread();JS_SetRuntimePrivate(rt,&state);
    cx=JS_NewContext(rt,8192);if(!cx){JS_DestroyRuntime(rt);return 1;}
    JS_BeginRequest(cx);JS_SetVersion(cx,JSVERSION_ECMA_2015);
    JS_SetOptions(cx,JS_GetOptions(cx)|JSOPTION_DONT_REPORT_UNCAUGHT);
    global=JS_NewObject(cx,&globalClass,NULL,NULL);CHECK(global);
    JS_SetGlobalObject(cx,global);CHECK(JS_InitStandardClasses(cx,global));
    CHECK(JS_DefineFunction(cx,global,"enqueue",Enqueue,1,0));
    CHECK(JS_DefineFunction(cx,global,"drain",Drain,0,0));
    CHECK(JS_DefineFunction(cx,global,"gc",Collect,0,0));
    CHECK(!JS_HasPendingJobs(cx));CHECK(JS_RunJobs(cx));
    CHECK(Evaluate(cx,global,"var events=[];enqueue(function(){'use strict';if(this!==void 0)throw 'receiver';events.push(1);enqueue(function(){events.push(3)});drain();gc();events.push(2)});enqueue(function(){events.push(4)});events.length===0"));
    JS_ClearNewbornRoots(cx);JS_GC(cx);CHECK(JS_HasPendingJobs(cx));
    CHECK(JS_RunJobs(cx));CHECK(Evaluate(cx,global,"events.join(',')==='1,2,4,3'"));
    CHECK(Evaluate(cx,global,"enqueue(function(){throw new Error('job error')});enqueue(function(){events.push('after')});true"));
    CHECK(!JS_RunJobs(cx));CHECK(JS_IsExceptionPending(cx));CHECK(JS_HasPendingJobs(cx));
    CHECK(!JS_RunJobs(cx));JS_ClearPendingException(cx);
    CHECK(JS_RunJobs(cx));CHECK(Evaluate(cx,global,"events[events.length-1]==='after'"));
    CHECK(Evaluate(cx,global,"var ran=false;enqueue(function(){ran=true});true"));
    JS_SetBranchCallback(cx,Stop);CHECK(!JS_RunJobs(cx));CHECK(JS_HasPendingJobs(cx));
    JS_SetBranchCallback(cx,NULL);CHECK(Evaluate(cx,global,"!ran"));
    CHECK(JS_RunJobs(cx));CHECK(Evaluate(cx,global,"ran"));
    CHECK(Evaluate(cx,global,"var correct=false;try{enqueue({})}catch(e){correct=e instanceof TypeError}correct"));
    other=JS_NewContext(rt,8192);CHECK(other);JS_BeginRequest(other);
    JS_SetVersion(other,JSVERSION_ECMA_2015);
    foreign=JS_NewObject(other,&globalClass,NULL,NULL);CHECK(foreign);
    JS_SetGlobalObject(other,foreign);CHECK(JS_InitStandardClasses(other,foreign));
    job=JS_NewFunction(other,MainJob,0,0,foreign,"foreignJob");CHECK(job);
    CHECK(JS_EnqueueJob(other,JS_GetFunctionObject(job)));
    JS_EndRequest(other);JS_DestroyContextNoGC(other);other=NULL;
    JS_ClearNewbornRoots(cx);JS_GC(cx);CHECK(JS_HasPendingJobs(cx));
    CHECK(JS_RunJobs(cx));CHECK(state.mainRuns==1);
    job=JS_NewFunction(cx,MainJob,0,0,global,"mainJob");CHECK(job);
    CHECK(JS_EnqueueJob(cx,JS_GetFunctionObject(job)));
    JS_EndRequest(cx);
    worker=PR_CreateThread(PR_USER_THREAD,Worker,&state,PR_PRIORITY_NORMAL,PR_GLOBAL_THREAD,PR_JOINABLE_THREAD,0);
    if(worker){PR_JoinThread(worker);worker=NULL;}else ++state.failures;
    JS_BeginRequest(cx);
    CHECK(state.failures==0&&state.workerRuns==1&&state.mainRuns==1);
    CHECK(JS_HasPendingJobs(cx));CHECK(JS_RunJobs(cx));CHECK(state.mainRuns==2);
    JS_ClearNewbornRoots(cx);JS_GC(cx);CHECK(state.finalized>=3);
    CHECK(!JS_HasPendingJobs(cx));
    job=JS_NewFunction(cx,MainJob,0,0,global,"cancelOnDetach");CHECK(job);
    CHECK(JS_EnqueueJob(cx,JS_GetFunctionObject(job)));
    JS_EndRequest(cx);JS_ClearContextThread(cx);JS_SetContextThread(cx);JS_BeginRequest(cx);
    CHECK(!JS_HasPendingJobs(cx));CHECK(JS_RunJobs(cx));CHECK(state.mainRuns==2);
    state.lock=PR_NewLock();CHECK(state.lock);
    state.condition=PR_NewCondVar(state.lock);CHECK(state.condition);
    JS_EndRequest(cx);
    worker=PR_CreateThread(PR_USER_THREAD,TransferWorker,&state,PR_PRIORITY_NORMAL,PR_GLOBAL_THREAD,PR_JOINABLE_THREAD,0);
    if(worker) {
        PR_Lock(state.lock);
        while(state.handoff!=1)PR_WaitCondVar(state.condition,PR_INTERVAL_NO_TIMEOUT);
        other=state.transfer;
        PR_Unlock(state.lock);
        if(other) {
            if(JS_SetContextThread(other)==-1)++state.failures;
            else {
                JS_BeginRequest(other);
                if(JS_HasPendingJobs(other))++state.failures;
                JS_EndRequest(other);
                JS_DestroyContext(other);other=NULL;
            }
        } else ++state.failures;
        PR_Lock(state.lock);state.handoff=2;PR_NotifyCondVar(state.condition);PR_Unlock(state.lock);
        PR_JoinThread(worker);worker=NULL;
    } else ++state.failures;
    JS_BeginRequest(cx);
    CHECK(state.failures==0);
    CHECK(PR_NewThreadPrivateIndex(&state.cleanupIndex,ContextCleanup)==PR_SUCCESS);
    JS_EndRequest(cx);
    worker=PR_CreateThread(PR_USER_THREAD,ExitWithContext,&state,PR_PRIORITY_NORMAL,PR_GLOBAL_THREAD,PR_JOINABLE_THREAD,0);
    if(worker){PR_JoinThread(worker);worker=NULL;}else ++state.failures;
    JS_BeginRequest(cx);
    CHECK(state.failures==0&&state.cleaned==1&&state.workerRuns==1);
    printf("ES6-JOB-QUEUE-EMBEDDING checks=%u failures=0\n",checks);status=0;
  out:
    JS_SetBranchCallback(cx,NULL);
    if(other){JS_EndRequest(other);JS_DestroyContextNoGC(other);}
    if(state.condition)PR_DestroyCondVar(state.condition);
    if(state.lock)PR_DestroyLock(state.lock);
    JS_EndRequest(cx);JS_DestroyContext(cx);JS_DestroyRuntime(rt);JS_ShutDown();
    return status;
}
