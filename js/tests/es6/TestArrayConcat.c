/* Native concat realms, boxing, species, interrupts and legacy globals.
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
    return JS_EvaluateScript(cx, global, source, strlen(source), "array-concat", 1, &value) && value == JSVAL_TRUE;
}
static JSBool Stop(JSContext *cx, JSScript *script)
{
    if (script) return JS_TRUE;
    ++interrupted; JS_GC(cx);
    if (interrupted < 3) return JS_TRUE;
    JS_ReportError(cx, "Array concat interrupted");
    return JS_FALSE;
}
#define CHECK(c) do { ++checks; if (!(c)) { fprintf(stderr,"FAIL Array concat embedding check %u\n",checks); goto out; } } while (0)
int main(void)
{
    JSRuntime *rt = JS_NewRuntime(16 * 1024 * 1024);
    JSContext *cx, *other = NULL;
    JSObject *global, *foreign, *clone, *receiver;
    jsval value, method, argument, result;
    int status = 1;
    JSBool methodRooted = JS_FALSE;
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
    CHECK(Evaluate(other,foreign,"var exports={concat:Array.prototype.concat,arrayProto:Array.prototype,stringProto:String.prototype,numberProto:Number.prototype,booleanProto:Boolean.prototype,symbolProto:Symbol.prototype,array:[1]};true"));
    CHECK(JS_GetProperty(other,foreign,"exports",&value));
    CHECK(JS_DefineProperty(cx,global,"foreign",value,NULL,NULL,0));
    JS_EndRequest(other); JS_DestroyContextNoGC(other); other=NULL;
    JS_ClearNewbornRoots(cx); JS_GC(cx); CHECK(finalized==0);
    CHECK(Evaluate(cx,global,"Object.getPrototypeOf(foreign.concat.call([1],2))===foreign.arrayProto"));
    CHECK(Evaluate(cx,global,"Object.getPrototypeOf(Array.prototype.concat.call(foreign.array,2))===Array.prototype"));
    CHECK(Evaluate(cx,global,"Object.getPrototypeOf(foreign.concat.call('x')[0])===foreign.stringProto"));
    CHECK(Evaluate(cx,global,"Object.getPrototypeOf(foreign.concat.call(3)[0])===foreign.numberProto"));
    CHECK(Evaluate(cx,global,"Object.getPrototypeOf(foreign.concat.call(true)[0])===foreign.booleanProto"));
    CHECK(Evaluate(cx,global,"var symbol=Symbol('box');var boxed=foreign.concat.call(symbol)[0];Object.getPrototypeOf(boxed)===foreign.symbolProto && boxed.valueOf()===symbol"));
    CHECK(Evaluate(cx,global,"Object.defineProperty(foreign.array.constructor,Symbol.species,{get:function(){throw Error('foreign species must be skipped');},configurable:true});Array.prototype.concat.call(foreign.array).join()==='1'"));
    CHECK(Evaluate(cx,global,"var custom=function(n){return {initial:n};};foreign.array.constructor={};foreign.array.constructor[Symbol.species]=custom;var derived=Array.prototype.concat.call(foreign.array,2);derived.initial===0 && derived.length===2 && derived[1]===2"));
    CHECK(JS_GetProperty(cx,JSVAL_TO_OBJECT(value),"concat",&method));
    clone=JS_CloneFunctionObject(cx,JSVAL_TO_OBJECT(method),global); CHECK(clone);
    CHECK(JS_DefineProperty(cx,global,"clonedConcat",OBJECT_TO_JSVAL(clone),NULL,NULL,0));
    CHECK(Evaluate(cx,global,"Object.getPrototypeOf(clonedConcat.call([],1))===Array.prototype"));
    CHECK(Evaluate(cx,global,"var loop=[];var item={length:1e12};item[Symbol.isConcatSpreadable]=true;var concatMethod=Array.prototype.concat;true"));
    CHECK(JS_GetProperty(cx,global,"loop",&value)); receiver=JSVAL_TO_OBJECT(value);
    CHECK(JS_GetProperty(cx,global,"concatMethod",&method));
    CHECK(JS_GetProperty(cx,global,"item",&argument));
    JS_SetBranchCallback(cx,Stop);
    CHECK(!JS_CallFunctionValue(cx,receiver,method,1,&argument,&result) && interrupted==3);
    JS_SetBranchCallback(cx,NULL); JS_ClearPendingException(cx);
    CHECK(Evaluate(cx,global,"[1].concat(2).join()==='1,2'"));
    JS_SetVersion(cx,JSVERSION_1_7);
    CHECK(Evaluate(cx,global,"foreign.concat.call([1],2).join()==='1,2'"));
    CHECK(Evaluate(cx,global,"delete this.foreign;delete this.clonedConcat;boxed=null;true"));
    JS_ClearNewbornRoots(cx); JS_GC(cx); CHECK(finalized==1);
    CHECK(JS_AddNamedRoot(cx,&method,"modern concat method")); methodRooted=JS_TRUE;
    JS_ClearScope(cx,global); JS_SetVersion(cx,JSVERSION_1_7);
    CHECK(JS_InitStandardClasses(cx,global));
    CHECK(Evaluate(cx,global,"var a=[1];a.constructor={};a.constructor[Symbol.species]=function(){throw Error('legacy species');};a[Symbol.isConcatSpreadable]=false;a.concat(2).join()==='1,2'"));
    clone=JS_CloneFunctionObject(cx,JSVAL_TO_OBJECT(method),global);CHECK(clone);
    CHECK(JS_DefineProperty(cx,global,"clonedModern",OBJECT_TO_JSVAL(clone),NULL,NULL,0));
    CHECK(Evaluate(cx,global,"var item={0:'x',length:1};item[Symbol.isConcatSpreadable]=true;clonedModern.call([],item).join()==='x' && clonedModern.call(7)[0].valueOf()===7"));
    JS_RemoveRoot(cx,&method);methodRooted=JS_FALSE;
    printf("ES6-ARRAY-CONCAT-EMBEDDING checks=%u failures=0\n",checks);status=0;
  out:
    if (methodRooted) JS_RemoveRoot(cx,&method);
    JS_SetBranchCallback(cx,NULL);
    if (other) { JS_EndRequest(other);JS_DestroyContextNoGC(other); }
    JS_EndRequest(cx);JS_DestroyContext(cx);JS_DestroyRuntime(rt);JS_ShutDown();
    return status;
}
