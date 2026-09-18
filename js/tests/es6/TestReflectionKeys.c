/* Reflection arrays, internal JSON callers, GC and selected legacy editions.
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
    return JS_EvaluateScript(cx,global,source,strlen(source),"reflection-keys",1,&value) && value == JSVAL_TRUE;
}
#define CHECK(c) do { ++checks; if (!(c)) { fprintf(stderr,"FAIL reflection-keys embedding check %u\n",checks); goto out; } } while (0)
int main(void)
{
    JSRuntime *rt = JS_NewRuntime(16*1024*1024);
    JSContext *cx, *other = NULL;
    JSObject *global, *foreign, *clone;
    jsval value, method;
    int status = 1;
    if (!rt) return 1;
    cx = JS_NewContext(rt,8192);
    if (!cx) { JS_DestroyRuntime(rt); return 1; }
    JS_BeginRequest(cx); JS_SetVersion(cx,JSVERSION_ECMA_2015);
    global = JS_NewObject(cx,&globalClass,NULL,NULL); CHECK(global);
    JS_SetGlobalObject(cx,global); CHECK(JS_InitStandardClasses(cx,global));
    CHECK(Evaluate(cx,global,"var large={},symbols={},i;for(i=0;i<512;i++){large['key'+i]=i;symbols[Symbol('key'+i)]=i}true"));
    JS_SetBranchCallback(cx,Branch);
    CHECK(Evaluate(cx,global,"Object.keys(large).length===512&&Object.getOwnPropertyNames(large).length===512&&Object.getOwnPropertySymbols(symbols).length===512"));
    CHECK(interrupted >= 12);
    stopOnBranch = JS_TRUE;
    CHECK(!Evaluate(cx,global,"Object.keys(large);true"));
    JS_ClearPendingException(cx);
    CHECK(!Evaluate(cx,global,"Object.getOwnPropertyNames(large);true"));
    JS_ClearPendingException(cx);
    CHECK(!Evaluate(cx,global,"Object.getOwnPropertySymbols(symbols);true"));
    stopOnBranch = JS_FALSE; JS_ClearPendingException(cx);
    CHECK(Evaluate(cx,global,"Object.keys(large).length===512"));
    CHECK(Evaluate(cx,global,"JSON.stringify({b:1,2:2,a:3,1:4})==='{\"1\":4,\"2\":2,\"b\":1,\"a\":3}'"));
    JS_SetBranchCallback(cx,NULL);
    other = JS_NewContext(rt,8192); CHECK(other);
    JS_BeginRequest(other); JS_SetVersion(other,JSVERSION_ECMA_2015);
    foreign = JS_NewObject(other,&globalClass,NULL,NULL); CHECK(foreign);
    JS_SetGlobalObject(other,foreign); CHECK(JS_InitStandardClasses(other,foreign));
    CHECK(Evaluate(other,foreign,"var exports={keys:Object.keys,names:Object.getOwnPropertyNames,symbols:Object.getOwnPropertySymbols,arrayProto:Array.prototype};true"));
    CHECK(JS_GetProperty(other,foreign,"exports",&value));
    CHECK(JS_DefineProperty(cx,global,"foreign",value,NULL,NULL,0));
    JS_EndRequest(other); JS_DestroyContextNoGC(other); other = NULL;
    JS_ClearNewbornRoots(cx); JS_GC(cx);
    CHECK(Evaluate(cx,global,"Object.getPrototypeOf(foreign.keys({a:1}))===foreign.arrayProto"));
    CHECK(Evaluate(cx,global,"Object.getPrototypeOf(foreign.names({a:1}))===foreign.arrayProto"));
    CHECK(Evaluate(cx,global,"Object.getPrototypeOf(foreign.symbols(symbols))===foreign.arrayProto"));
    CHECK(Evaluate(cx,global,"Object.getPrototypeOf(foreign.keys(new Proxy({a:1},{})))===foreign.arrayProto"));
    CHECK(JS_GetProperty(cx,JSVAL_TO_OBJECT(value),"keys",&method));
    clone = JS_CloneFunctionObject(cx,JSVAL_TO_OBJECT(method),global); CHECK(clone);
    CHECK(JS_DefineProperty(cx,global,"clonedKeys",OBJECT_TO_JSVAL(clone),NULL,NULL,0));
    CHECK(Evaluate(cx,global,"Object.getPrototypeOf(clonedKeys({a:1}))===Array.prototype&&clonedKeys({b:1,2:2,a:3,1:4}).join(',')==='1,2,b,a'"));
    JS_SetVersion(cx,JSVERSION_1_7);
    CHECK(Evaluate(cx,global,"Object.keys({b:1,2:2,a:3,1:4}).join(',')==='b,2,a,1'"));
    CHECK(Evaluate(cx,global,"var correct=false;try{Object.keys('ab')}catch(e){correct=e instanceof TypeError}correct"));
    CHECK(Evaluate(cx,global,"Object.getPrototypeOf(foreign.keys({a:1}))===Array.prototype"));
    CHECK(Evaluate(cx,global,"JSON.stringify({b:1,2:2,a:3,1:4})==='{\"b\":1,\"2\":2,\"a\":3,\"1\":4}'"));
    printf("ES6-REFLECTION-KEYS-EMBEDDING checks=%u failures=0\n",checks); status = 0;
  out:
    JS_SetBranchCallback(cx,NULL);
    if (other) { JS_EndRequest(other); JS_DestroyContextNoGC(other); }
    JS_EndRequest(cx); JS_DestroyContext(cx); JS_DestroyRuntime(rt); JS_ShutDown();
    return status;
}
