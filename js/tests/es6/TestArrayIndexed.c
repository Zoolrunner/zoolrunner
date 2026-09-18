/* Native Array indexed realms, cloning, interrupts and legacy globals.
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
    return JS_EvaluateScript(cx, global, source, strlen(source), "array-indexed", 1, &value) && value == JSVAL_TRUE;
}
static JSBool Stop(JSContext *cx, JSScript *script)
{
    if (script) return JS_TRUE;
    ++interrupted; JS_GC(cx);
    if (interrupted < 3) return JS_TRUE;
    JS_ReportError(cx, "Array indexed interrupted");
    return JS_FALSE;
}
#define CHECK(c) do { ++checks; if (!(c)) { fprintf(stderr,"FAIL Array indexed embedding check %u\n",checks); goto out; } } while (0)
int main(void)
{
    JSRuntime *rt = JS_NewRuntime(16 * 1024 * 1024);
    JSContext *cx, *other = NULL;
    JSObject *global, *foreign, *clone, *receiver;
    jsval value, method, argument, result;
    int status = 1;
    JSBool methodRooted = JS_FALSE;
    uintN i;
    const char *loops[] = {"indexOf", "lastIndexOf", "reverse", "shift", "unshift", "slice", "splice"};
    if (!rt) return 1;
    cx=JS_NewContext(rt,8192);
    if (!cx) { JS_DestroyRuntime(rt);return 1; }
    JS_BeginRequest(cx);JS_SetVersion(cx,JSVERSION_ECMA_2015);
    global=JS_NewObject(cx,&globalClass,NULL,NULL);CHECK(global);
    JS_SetGlobalObject(cx,global);CHECK(JS_InitStandardClasses(cx,global));
    other=JS_NewContext(rt,8192);CHECK(other);
    JS_BeginRequest(other);JS_SetVersion(other,JSVERSION_ECMA_2015);
    foreign=JS_NewObject(other,&globalClass,NULL,NULL);CHECK(foreign);
    JS_SetGlobalObject(other,foreign);CHECK(JS_InitStandardClasses(other,foreign));
    CHECK(Evaluate(other,foreign,"var exports={splice:Array.prototype.splice,slice:Array.prototype.slice,reverse:Array.prototype.reverse,push:Array.prototype.push,arrayProto:Array.prototype,numberProto:Number.prototype,boolProto:Boolean.prototype,symbolProto:Symbol.prototype,array:[1]};true"));
    CHECK(JS_GetProperty(other,foreign,"exports",&value));
    CHECK(JS_DefineProperty(cx,global,"foreign",value,NULL,NULL,0));
    JS_EndRequest(other);JS_DestroyContextNoGC(other);other=NULL;
    JS_ClearNewbornRoots(cx);JS_GC(cx);CHECK(finalized==0);
    CHECK(Evaluate(cx,global,"Object.getPrototypeOf(foreign.slice.call([1]))===foreign.arrayProto"));
    CHECK(Evaluate(cx,global,"Object.getPrototypeOf(Array.prototype.slice.call(foreign.array))===Array.prototype"));
    CHECK(Evaluate(cx,global,"Object.getPrototypeOf(foreign.splice.call([1],0,1))===foreign.arrayProto"));
    CHECK(Evaluate(cx,global,"Object.getPrototypeOf(foreign.reverse.call(1))===foreign.numberProto"));
    CHECK(Evaluate(cx,global,"Object.getPrototypeOf(foreign.reverse.call(true))===foreign.boolProto"));
    CHECK(Evaluate(cx,global,"Object.getPrototypeOf(foreign.reverse.call(Symbol()))===foreign.symbolProto"));
    CHECK(Evaluate(cx,global,"Object.defineProperty(foreign.array.constructor,Symbol.species,{get:function(){throw Error('foreign species');},configurable:true});Array.prototype.slice.call(foreign.array)[0]===1"));
    CHECK(JS_GetProperty(cx,JSVAL_TO_OBJECT(value),"slice",&method));
    clone=JS_CloneFunctionObject(cx,JSVAL_TO_OBJECT(method),global);CHECK(clone);
    CHECK(JS_DefineProperty(cx,global,"clonedSlice",OBJECT_TO_JSVAL(clone),NULL,NULL,0));
    CHECK(Evaluate(cx,global,"Object.getPrototypeOf(clonedSlice.call([1]))===Array.prototype"));
    CHECK(Evaluate(cx,global,"var loop={length:1e8};var methods=Array.prototype;var localSlice=Array.prototype.slice;true"));
    CHECK(JS_GetProperty(cx,global,"loop",&value));receiver=JSVAL_TO_OBJECT(value);
    CHECK(JS_GetProperty(cx,global,"methods",&value));
    for (i=0;i<sizeof(loops)/sizeof(loops[0]);++i) {
        CHECK(JS_GetProperty(cx,JSVAL_TO_OBJECT(value),loops[i],&method));
        argument=INT_TO_JSVAL(0);interrupted=0;JS_SetBranchCallback(cx,Stop);
        CHECK(!JS_CallFunctionValue(cx,receiver,method,1,&argument,&result) && interrupted==3);
        JS_SetBranchCallback(cx,NULL);JS_ClearPendingException(cx);
    }
    CHECK(Evaluate(cx,global,"[1,2].slice(1)[0]===2"));
    CHECK(JS_GetProperty(cx,global,"localSlice",&method));
    CHECK(JS_AddNamedRoot(cx,&method,"modern slice method"));methodRooted=JS_TRUE;
    CHECK(Evaluate(cx,global,"delete this.foreign;delete this.clonedSlice;true"));
    JS_ClearNewbornRoots(cx);JS_GC(cx);CHECK(finalized==1);
    JS_ClearScope(cx,global);JS_SetVersion(cx,JSVERSION_1_7);
    CHECK(JS_InitStandardClasses(cx,global));
    CHECK(Evaluate(cx,global,"var a=[1];a.constructor={};a.constructor[Symbol.species]=function(){throw Error('legacy species');};a.slice()[0]===1"));
    CHECK(Evaluate(cx,global,"var a={0:7,length:4294967297};Array.prototype.pop.call(a)===7&&a.length===0"));
    clone=JS_CloneFunctionObject(cx,JSVAL_TO_OBJECT(method),global);CHECK(clone);
    CHECK(JS_DefineProperty(cx,global,"clonedModernSlice",OBJECT_TO_JSVAL(clone),NULL,NULL,0));
    CHECK(Evaluate(cx,global,"clonedModernSlice.call({0:1,length:-1}).length===0"));
    JS_RemoveRoot(cx,&method);methodRooted=JS_FALSE;
    printf("ES6-ARRAY-INDEXED-EMBEDDING checks=%u failures=0\n",checks);status=0;
  out:
    if(methodRooted)JS_RemoveRoot(cx,&method);
    JS_SetBranchCallback(cx,NULL);
    if(other){JS_EndRequest(other);JS_DestroyContextNoGC(other);}
    JS_EndRequest(cx);JS_DestroyContext(cx);JS_DestroyRuntime(rt);JS_ShutDown();
    return status;
}
