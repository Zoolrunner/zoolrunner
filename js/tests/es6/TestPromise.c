/* Promise jobs across GC, realms, context teardown, interruption and JSAPI clones.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "jsapi.h"
#include "jsdbgapi.h"
#include <stdio.h>
#include <string.h>
static unsigned checks, callbacks, finalized;
static JSBool stopOnBranch;
static JSBool CheckJobScope(JSContext *cx, JSObject *obj, uintN argc,
                            jsval *argv, jsval *rval)
{
    JSStackFrame *frame = NULL;
    *rval = JSVAL_FALSE;
    while ((frame = JS_FrameIterator(cx, &frame)) != NULL) {
        if (JS_IsJobFrame(cx, frame)) {
            *rval = BOOLEAN_TO_JSVAL(argc && !JSVAL_IS_PRIMITIVE(argv[0]) &&
                JS_GetFrameScopeChain(cx, frame) == JSVAL_TO_OBJECT(argv[0]));
            break;
        }
    }
    return JS_TRUE;
}
static void Finalize(JSContext *cx, JSObject *obj) { ++finalized; }
static JSClass globalClass = {
    "global", 0,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, Finalize,
    JSCLASS_NO_OPTIONAL_MEMBERS
};
static JSBool Branch(JSContext *cx, JSScript *script)
{
    if (!script) { ++callbacks; JS_GC(cx); if (stopOnBranch) return JS_FALSE; }
    return JS_TRUE;
}
static JSBool Evaluate(JSContext *cx, JSObject *global, const char *source)
{
    jsval result;
    return JS_EvaluateScript(cx,global,source,strlen(source),"promise-embedding",1,&result) && result==JSVAL_TRUE;
}
#define CHECK(c) do {++checks;if(!(c)){fprintf(stderr,"FAIL Promise embedding check %u\n",checks);JS_ReportPendingException(cx);goto out;}}while(0)
int main(void)
{
    JSRuntime *rt=JS_NewRuntime(16*1024*1024);
    JSContext *cx,*other=NULL;
    JSObject *global,*foreign,*clone;
    jsval value,method;
    JSBool rooted=JS_FALSE;
    unsigned before;
    int status=1;
    if(!rt)return 1;
    cx=JS_NewContext(rt,8192);if(!cx){JS_DestroyRuntime(rt);return 1;}
    JS_BeginRequest(cx);JS_SetVersion(cx,JSVERSION_ECMA_2015);
    JS_SetOptions(cx,JS_GetOptions(cx)|JSOPTION_DONT_REPORT_UNCAUGHT);
    global=JS_NewObject(cx,&globalClass,NULL,NULL);CHECK(global);
    JS_SetGlobalObject(cx,global);CHECK(JS_InitStandardClasses(cx,global));
    CHECK(JS_DefineFunction(cx,global,"checkJobScope",CheckJobScope,1,0));
    CHECK(Evaluate(cx,global,"var result,resolve,scopeOkay=false;var p=new Promise(function(r){resolve=r});p.then(function(v){result=v;scopeOkay=checkJobScope(this)});resolve(42);result===undefined"));
    JS_ClearNewbornRoots(cx);JS_GC(cx);JS_SetBranchCallback(cx,Branch);
    CHECK(JS_RunJobs(cx));CHECK(callbacks>0);CHECK(Evaluate(cx,global,"result===42"));
    CHECK(Evaluate(cx,global,"scopeOkay"));
    CHECK(Evaluate(cx,global,"var poison={then:function(r){r(poison)}};var settled;Promise.resolve(poison).then(function(v){settled=v});true"));
    stopOnBranch=JS_TRUE;CHECK(!JS_RunJobs(cx));CHECK(JS_HasPendingJobs(cx));
    CHECK(!JS_IsExceptionPending(cx));
    stopOnBranch=JS_FALSE;
    CHECK(Evaluate(cx,global,"poison.then=function(r){r(17)};true"));
    CHECK(JS_RunJobs(cx));CHECK(Evaluate(cx,global,"settled===17"));
    CHECK(Evaluate(cx,global,"var iterable={};iterable[Symbol.iterator]=function(){return{next:function(){return{done:false,value:1}}}};true"));
    stopOnBranch=JS_TRUE;CHECK(!Evaluate(cx,global,"Promise.all(iterable);true"));
    CHECK(!JS_IsExceptionPending(cx));stopOnBranch=JS_FALSE;
    CHECK(JS_RunJobs(cx));JS_SetBranchCallback(cx,NULL);
    other=JS_NewContext(rt,8192);CHECK(other);
    JS_BeginRequest(other);JS_SetVersion(other,JSVERSION_ECMA_2015);
    foreign=JS_NewObject(other,&globalClass,NULL,NULL);CHECK(foreign);
    JS_SetGlobalObject(other,foreign);CHECK(JS_InitStandardClasses(other,foreign));
    CHECK(Evaluate(other,foreign,"var exports={Promise:Promise,TypeError:TypeError,Array:Array};var foreignResult;Promise.resolve(3).then(function(v){foreignResult=v});true"));
    CHECK(JS_GetProperty(other,foreign,"exports",&value));
    CHECK(JS_DefineProperty(cx,global,"foreign",value,NULL,NULL,0));
    JS_EndRequest(other);JS_DestroyContextNoGC(other);other=NULL;
    JS_ClearNewbornRoots(cx);JS_GC(cx);CHECK(JS_RunJobs(cx));
    CHECK(Evaluate(cx,foreign,"foreignResult===3"));
    CHECK(Evaluate(cx,global,"(function(){var right=false;try{foreign.Promise(null)}catch(e){right=e instanceof foreign.TypeError&&!(e instanceof TypeError)}return right})()"));
    CHECK(Evaluate(cx,global,"(function(){var q=foreign.Promise.resolve(1);return Object.getPrototypeOf(q)===foreign.Promise.prototype&&foreign.Promise.resolve(q)===q})()"));
    CHECK(Evaluate(cx,global,"var realmCheck=false;foreign.Promise.all([1,2]).then(function(v){realmCheck=Object.getPrototypeOf(v)===foreign.Array.prototype});true"));
    JS_GC(cx);CHECK(JS_RunJobs(cx));CHECK(Evaluate(cx,global,"realmCheck"));
    CHECK(Evaluate(cx,global,"(function(){var q=foreign.Promise.resolve(1);var r=Promise.prototype.then.call(q,function(v){return v});return Object.getPrototypeOf(r)===foreign.Promise.prototype})()"));
    CHECK(JS_RunJobs(cx));
    CHECK(Evaluate(cx,global,"var captured,clonedValue;var clonePromise=new Promise(function(r){captured=r});clonePromise.then(function(v){clonedValue=v});true"));
    CHECK(JS_GetProperty(cx,global,"captured",&method));
    CHECK(JS_AddNamedRoot(cx,&method,"Promise resolving function"));rooted=JS_TRUE;
    clone=JS_CloneFunctionObject(cx,JSVAL_TO_OBJECT(method),global);CHECK(clone);
    CHECK(JS_DefineProperty(cx,global,"clonedResolve",OBJECT_TO_JSVAL(clone),NULL,NULL,0));
    CHECK(Evaluate(cx,global,"clonedResolve(8);captured(9);true"));
    JS_GC(cx);CHECK(JS_RunJobs(cx));CHECK(Evaluate(cx,global,"clonedValue===8"));
    JS_RemoveRoot(cx,&method);rooted=JS_FALSE;
    before=finalized;
    CHECK(JS_DeleteProperty(cx,global,"foreign"));
    JS_ClearNewbornRoots(cx);JS_GC(cx);CHECK(finalized>before);
    JS_SetVersion(cx,JSVERSION_1_7);
    CHECK(Evaluate(cx,global,"var legacyValue;Promise.resolve(5).then(function(v){legacyValue=v});var o={};o.x setter=function(v){this.y=v};o.x=3;o.y===3"));
    CHECK(JS_RunJobs(cx));CHECK(Evaluate(cx,global,"legacyValue===5"));
    other=JS_NewContext(rt,8192);CHECK(other);
    JS_BeginRequest(other);JS_SetVersion(other,JSVERSION_1_7);
    foreign=JS_NewObject(other,&globalClass,NULL,NULL);CHECK(foreign);
    JS_SetGlobalObject(other,foreign);CHECK(JS_InitStandardClasses(other,foreign));
    CHECK(JS_GetVersion(other)==JSVERSION_1_7);
    CHECK(Evaluate(other,foreign,"var exports={Promise:Promise,TypeError:TypeError};Object.getOwnPropertyDescriptor(Promise,'length').configurable&&Object.getOwnPropertyDescriptor(Promise.prototype.then,'length').configurable&&(function(){return arguments.length}).apply(null,{length:4294967296})===0"));
    CHECK(JS_GetProperty(other,foreign,"exports",&value));
    CHECK(JS_DefineProperty(cx,global,"legacyForeign",value,NULL,NULL,0));
    JS_EndRequest(other);JS_DestroyContextNoGC(other);other=NULL;
    CHECK(Evaluate(cx,global,"(function(){try{legacyForeign.Promise(null)}catch(e){return e instanceof legacyForeign.TypeError&&!(e instanceof TypeError)}return false})()"));
    printf("ES6-PROMISE-EMBEDDING checks=%u failures=0\n",checks);status=0;
  out:
    JS_SetBranchCallback(cx,NULL);
    if(rooted)JS_RemoveRoot(cx,&method);
    if(other){JS_EndRequest(other);JS_DestroyContextNoGC(other);}
    JS_EndRequest(cx);JS_DestroyContext(cx);JS_DestroyRuntime(rt);JS_ShutDown();
    return status;
}
