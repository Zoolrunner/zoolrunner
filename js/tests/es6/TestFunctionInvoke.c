/* Native modern invocation, GC, interruption and legacy isolation.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "jsapi.h"
#include <stdio.h>
#include <string.h>
static unsigned checks, interrupted;
static JSBool stopOnBranch;
static JSClass globalClass = {
    "global", 0,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};
static JSBool Branch(JSContext *cx, JSScript *script)
{
    if (!script) { ++interrupted; JS_GC(cx); if (stopOnBranch) return JS_FALSE; }
    return JS_TRUE;
}
static JSBool Evaluate(JSContext *cx, JSObject *global, const char *source)
{
    jsval value;
    return JS_EvaluateScript(cx,global,source,strlen(source),"function-invoke",1,&value) && value == JSVAL_TRUE;
}
#define CHECK(c) do { ++checks; if (!(c)) { fprintf(stderr,"FAIL function-invoke embedding check %u\n",checks); goto out; } } while (0)
int main(void)
{
    JSRuntime *rt = JS_NewRuntime(16*1024*1024);
    JSContext *cx, *other = NULL;
    JSObject *global, *foreign, *clone;
    jsval value, method;
    JSBool rooted = JS_FALSE;
    int status = 1;
    if (!rt) return 1;
    cx = JS_NewContext(rt,8192);
    if (!cx) { JS_DestroyRuntime(rt); return 1; }
    JS_BeginRequest(cx); JS_SetVersion(cx,JSVERSION_ECMA_2015);
    global = JS_NewObject(cx,&globalClass,NULL,NULL); CHECK(global);
    JS_SetGlobalObject(cx,global); CHECK(JS_InitStandardClasses(cx,global));
    CHECK(Evaluate(cx,global,"function count(){return arguments.length}var list={length:512};true"));
    JS_SetBranchCallback(cx,Branch);
    CHECK(Evaluate(cx,global,"count.apply(null,list)===512"));
    CHECK(interrupted >= 4);
    stopOnBranch = JS_TRUE;
    CHECK(!Evaluate(cx,global,"count.apply(null,list);true"));
    stopOnBranch = JS_FALSE; JS_ClearPendingException(cx);
    CHECK(Evaluate(cx,global,"count.apply(null,list)===512"));
    JS_SetBranchCallback(cx,NULL);
    other = JS_NewContext(rt,8192); CHECK(other);
    JS_BeginRequest(other); JS_SetVersion(other,JSVERSION_ECMA_2015);
    foreign = JS_NewObject(other,&globalClass,NULL,NULL); CHECK(foreign);
    JS_SetGlobalObject(other,foreign); CHECK(JS_InitStandardClasses(other,foreign));
    CHECK(Evaluate(other,foreign,"var exports={error:TypeError,range:RangeError,buffer:ArrayBuffer,apply:Function.prototype.apply,call:Function.prototype.call,strict:function(){'use strict';return this}};true"));
    CHECK(JS_GetProperty(other,foreign,"exports",&value));
    CHECK(JS_DefineProperty(cx,global,"foreign",value,NULL,NULL,0));
    JS_EndRequest(other); JS_DestroyContextNoGC(other); other = NULL;
    JS_ClearNewbornRoots(cx); JS_GC(cx);
    CHECK(Evaluate(cx,global,"foreign.apply.call(count,null,{length:-1})===0&&foreign.call.call(foreign.strict,3)===3"));
    CHECK(Evaluate(cx,global,"var poison={toString:function(){throw 'conversion'}};var correct=false;try{foreign.call.call(poison)}catch(e){correct=e instanceof foreign.error}correct"));
    CHECK(Evaluate(cx,global,"var correct=false;try{foreign.apply.call(count,null,{length:4294967296})}catch(e){correct=e instanceof foreign.range}correct"));
    CHECK(Evaluate(cx,global,"var correct=false;try{foreign.buffer(1)}catch(e){correct=e instanceof foreign.error}correct"));
    CHECK(Evaluate(cx,global,"var correct=false;try{foreign.apply.call(count,null,{get length(){throw new TypeError('local')}})}catch(e){correct=e instanceof TypeError&&!(e instanceof foreign.error)}correct"));
    CHECK(JS_GetProperty(cx,JSVAL_TO_OBJECT(value),"apply",&method));
    CHECK(JS_AddNamedRoot(cx,&method,"modern apply")); rooted = JS_TRUE;
    clone = JS_CloneFunctionObject(cx,JSVAL_TO_OBJECT(method),global); CHECK(clone);
    CHECK(JS_DefineProperty(cx,global,"clonedApply",OBJECT_TO_JSVAL(clone),NULL,NULL,0));
    CHECK(Evaluate(cx,global,"clonedApply.call(count,null,{length:-1})===0"));
    JS_ClearScope(cx,global); JS_SetVersion(cx,JSVERSION_1_7);
    CHECK(JS_InitStandardClasses(cx,global));
    CHECK(Evaluate(cx,global,"function count(){return arguments.length}count.apply(null,{length:4294967296})===0"));
    clone = JS_CloneFunctionObject(cx,JSVAL_TO_OBJECT(method),global); CHECK(clone);
    CHECK(JS_DefineProperty(cx,global,"clonedModernApply",OBJECT_TO_JSVAL(clone),NULL,NULL,0));
    CHECK(Evaluate(cx,global,"clonedModernApply.call(count,null,{length:-1})===0"));
    CHECK(Evaluate(cx,global,"var correct=false;try{clonedModernApply.call(count,null,{length:4294967296})}catch(e){correct=e instanceof RangeError}correct"));
    JS_RemoveRoot(cx,&method); rooted = JS_FALSE;
    printf("ES6-FUNCTION-INVOKE-EMBEDDING checks=%u failures=0\n",checks); status = 0;
  out:
    JS_SetBranchCallback(cx,NULL);
    if (rooted) JS_RemoveRoot(cx,&method);
    if (other) { JS_EndRequest(other); JS_DestroyContextNoGC(other); }
    JS_EndRequest(cx); JS_DestroyContext(cx); JS_DestroyRuntime(rt); JS_ShutDown();
    return status;
}
