/* Native Array iteration realms, cloning, interrupts and legacy globals.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "jsapi.h"
#include <stdio.h>
#include <string.h>
static unsigned checks, finalized, interrupted;
static void Finalize(JSContext *cx, JSObject *obj) { ++finalized; }
static JSClass globalClass = {
    "global", 0,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, Finalize,
    JSCLASS_NO_OPTIONAL_MEMBERS
};
static JSBool Evaluate(JSContext *cx, JSObject *global, const char *source)
{
    jsval value;
    return JS_EvaluateScript(cx, global, source, strlen(source), "array-iteration", 1, &value) && value == JSVAL_TRUE;
}
static JSBool Stop(JSContext *cx, JSScript *script)
{
    if (script) return JS_TRUE;
    ++interrupted; JS_GC(cx);
    if (interrupted < 3) return JS_TRUE;
    JS_ReportError(cx, "Array iteration interrupted");
    return JS_FALSE;
}
#define CHECK(c) do { ++checks; if (!(c)) { fprintf(stderr,"FAIL Array iteration embedding check %u\n",checks); goto out; } } while (0)
int main(void)
{
    JSRuntime *rt = JS_NewRuntime(16 * 1024 * 1024);
    JSContext *cx, *other = NULL;
    JSObject *global, *foreign, *clone, *receiver;
    jsval value, method, argument, result;
    int status = 1;
    JSBool methodRooted=JS_FALSE;
    if (!rt) return 1;
    cx = JS_NewContext(rt,8192);
    if (!cx) { JS_DestroyRuntime(rt); return 1; }
    JS_BeginRequest(cx); JS_SetVersion(cx,JSVERSION_ECMA_2015);
    global = JS_NewObject(cx,&globalClass,NULL,NULL); CHECK(global);
    JS_SetGlobalObject(cx,global); CHECK(JS_InitStandardClasses(cx,global));
    other = JS_NewContext(rt,8192); CHECK(other);
    JS_BeginRequest(other); JS_SetVersion(other,JSVERSION_ECMA_2015);
    foreign = JS_NewObject(other,&globalClass,NULL,NULL); CHECK(foreign);
    JS_SetGlobalObject(other,foreign); CHECK(JS_InitStandardClasses(other,foreign));
    CHECK(Evaluate(other,foreign,"var exports={map:Array.prototype.map,filter:Array.prototype.filter,each:Array.prototype.forEach,arrayProto:Array.prototype,stringProto:String.prototype,array:[1]};true"));
    CHECK(JS_GetProperty(other,foreign,"exports",&value));
    CHECK(JS_DefineProperty(cx,global,"foreign",value,NULL,NULL,0));
    JS_EndRequest(other); JS_DestroyContextNoGC(other); other=NULL;
    JS_ClearNewbornRoots(cx); JS_GC(cx); CHECK(finalized==0);
    CHECK(Evaluate(cx,global,"Object.getPrototypeOf(foreign.map.call([1],function(x){return x;}))===foreign.arrayProto"));
    CHECK(Evaluate(cx,global,"Object.getPrototypeOf(Array.prototype.map.call(foreign.array,function(x){return x;}))===Array.prototype"));
    CHECK(Evaluate(cx,global,"Object.getPrototypeOf(foreign.filter.call([1],function(){return true;}))===foreign.arrayProto"));
    CHECK(Evaluate(cx,global,"var seenObject;foreign.each.call('a',function(v,k,o){seenObject=o;});Object.getPrototypeOf(seenObject)===foreign.stringProto"));
    CHECK(Evaluate(cx,global,"foreign.map.call([1],function(){'use strict';return this===undefined;})[0]"));
    CHECK(Evaluate(cx,global,"Object.defineProperty(foreign.array.constructor,Symbol.species,{get:function(){throw Error('foreign species');},configurable:true});Array.prototype.map.call(foreign.array,function(x){return x;})[0]===1"));
    CHECK(JS_GetProperty(cx,JSVAL_TO_OBJECT(value),"map",&method));
    clone=JS_CloneFunctionObject(cx,JSVAL_TO_OBJECT(method),global); CHECK(clone);
    CHECK(JS_DefineProperty(cx,global,"clonedMap",OBJECT_TO_JSVAL(clone),NULL,NULL,0));
    CHECK(Evaluate(cx,global,"Object.getPrototypeOf(clonedMap.call([1],function(x){return x;}))===Array.prototype"));
    CHECK(Evaluate(cx,global,"var loop={length:1e12};var callback=function(){};var eachMethod=Array.prototype.forEach;var localMap=Array.prototype.map;true"));
    CHECK(JS_GetProperty(cx,global,"loop",&value)); receiver=JSVAL_TO_OBJECT(value);
    CHECK(JS_GetProperty(cx,global,"eachMethod",&method));
    CHECK(JS_GetProperty(cx,global,"callback",&argument));
    JS_SetBranchCallback(cx,Stop);
    CHECK(!JS_CallFunctionValue(cx,receiver,method,1,&argument,&result) && interrupted==3);
    JS_SetBranchCallback(cx,NULL); JS_ClearPendingException(cx);
    CHECK(Evaluate(cx,global,"[1].map(function(x){return x+1;})[0]===2"));
    CHECK(JS_GetProperty(cx,global,"localMap",&method));
    CHECK(JS_AddNamedRoot(cx,&method,"modern map method"));methodRooted=JS_TRUE;
    CHECK(Evaluate(cx,global,"delete this.foreign;delete this.clonedMap;seenObject=null;true"));
    JS_ClearNewbornRoots(cx); JS_GC(cx); CHECK(finalized==1);
    JS_ClearScope(cx,global); JS_SetVersion(cx,JSVERSION_1_7);
    CHECK(JS_InitStandardClasses(cx,global));
    CHECK(Evaluate(cx,global,"var a=[1];a.constructor={};a.constructor[Symbol.species]=function(){throw Error('legacy species');};a.map(function(x){return x+1;})[0]===2"));
    CHECK(Evaluate(cx,global,"var visits=0;Array.prototype.forEach.call({0:1,length:4294967297},function(){++visits;});visits===1"));
    clone=JS_CloneFunctionObject(cx,JSVAL_TO_OBJECT(method),global);CHECK(clone);
    CHECK(JS_DefineProperty(cx,global,"clonedModernMap",OBJECT_TO_JSVAL(clone),NULL,NULL,0));
    CHECK(Evaluate(cx,global,"clonedModernMap.call({0:1,length:-1},function(){throw Error('legacy length');}).length===0"));
    JS_RemoveRoot(cx,&method);methodRooted=JS_FALSE;
    printf("ES6-ARRAY-ITERATION-EMBEDDING checks=%u failures=0\n",checks);status=0;
  out:
    if (methodRooted) JS_RemoveRoot(cx,&method);
    JS_SetBranchCallback(cx,NULL);
    if (other) { JS_EndRequest(other);JS_DestroyContextNoGC(other); }
    JS_EndRequest(cx);JS_DestroyContext(cx);JS_DestroyRuntime(rt);JS_ShutDown();
    return status;
}
