/* Typed-array detachment, GC and embedding receivers. MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "jsapi.h"
#include "jsdbgapi.h"
#include "../../src/jsbinarydata.h"
#include <stdio.h>
#include <string.h>
static unsigned checks, allocations, inspections;
static JSBool inspectOK = JS_TRUE;
static JSBool inHook;
static JSClass globalClass = {
    "global", 0,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};
static JSBool Detach(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    if (!argc || JSVAL_IS_PRIMITIVE(argv[0])) return JS_FALSE;
    *rval = JSVAL_VOID;
    return js_DetachArrayBuffer(cx, JSVAL_TO_OBJECT(argv[0]));
}
static JSBool Collect(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{ JS_GC(cx); *rval = JSVAL_VOID; return JS_TRUE; }
static void Allocation(JSContext *cx, JSObject *obj, JSBool creating, void *data)
{
    if (!creating || inHook) return;
    inHook = JS_TRUE; ++allocations;
    if (js_IsTypedArray(cx, obj)) {
        jsval argument = OBJECT_TO_JSVAL(obj), result = JSVAL_VOID;
        ++inspections;
        if (!JS_CallFunctionName(cx, JS_GetGlobalObject(cx), "inspectTyped", 1,
                                 &argument, &result) || result != JSVAL_TRUE) {
            inspectOK = JS_FALSE;
            JS_ClearPendingException(cx);
        }
    }
    JS_GC(cx); inHook = JS_FALSE;
}
static void Report(JSContext *cx, const char *message, JSErrorReport *report)
{ fprintf(stderr, "typed arrays: %s\n", message); }
static JSBool Evaluate(JSContext *cx, JSObject *global, const char *source)
{
    jsval value;
    JSBool ok = JS_EvaluateScript(cx, global, source, strlen(source), "typed-arrays", 1, &value);
    if (!ok || value != JSVAL_TRUE) fprintf(stderr, "SOURCE: %s\n", source);
    return ok && value == JSVAL_TRUE;
}
#define CHECK(c) do { ++checks; if (!(c)) { fprintf(stderr,"FAIL typed-array embedding check %u\n",checks); goto out; } } while (0)
int main(void)
{
    JSRuntime *rt = JS_NewRuntime(16 * 1024 * 1024);
    JSContext *cx;
    JSObject *global = NULL;
    int status = 1;
    if (!rt) return 1;
    cx = JS_NewContext(rt,8192);
    if (!cx) { JS_DestroyRuntime(rt); return 1; }
    JS_BeginRequest(cx); JS_SetVersion(cx,JSVERSION_ECMA_2015);
    JS_SetErrorReporter(cx, Report);
    JS_AddNamedRoot(cx, &global, "typed-array global");
    global = JS_NewObject(cx,&globalClass,NULL,NULL); CHECK(global);
    JS_SetGlobalObject(cx,global); CHECK(JS_InitStandardClasses(cx,global));
    CHECK(JS_DefineFunction(cx,global,"detach",Detach,1,0));
    CHECK(JS_DefineFunction(cx,global,"gc",Collect,0,0));
    CHECK(Evaluate(cx,global,"function typeError(f){try{f()}catch(e){return e instanceof TypeError}return false}var a=new Uint8Array([1,2,3]);a[1]===2"));
    CHECK(Evaluate(cx,global,"function inspectTyped(o){if(!Object.isExtensible(o)||Reflect.ownKeys(o).length||Object.getOwnPropertyDescriptor(o,'hook')!==undefined)return false;Object.defineProperty(o,'hook',{value:7,configurable:true});gc();if(Object.getOwnPropertyNames(o).join()!=='hook'||Object.getOwnPropertyDescriptor(o,'hook').value!==7)return false;return delete o.hook};true"));
    JS_SetObjectHook(rt, Allocation, NULL);
    CHECK(Evaluate(cx,global,"a=Uint8Array.from([1,2,3],function(v,i){gc();return v+i});a.join()==='1,3,5'&&a.map(function(v){gc();return v*2}).join()==='2,6,10'"));
    CHECK(Evaluate(cx,global,"a=new Float64Array([NaN,-0,3,1,2]);a.sort(function(x,y){gc();return x-y});a.length===5&&ArrayBuffer.isView(a)"));
    CHECK(Evaluate(cx,global,"a=new Int16Array([4,5]);a.foo=1;a.foo=2;Object.defineProperty(a,'bar',{get:function(){gc();return this.foo},set:function(v){gc();this.foo=v}});var child=Object.create(a);child.bar=7;child.bar===7&&a.foo===2&&Object.getOwnPropertyDescriptor(a,'bar').get.call(child)===7"));
    CHECK(Evaluate(cx,global,"a=new Uint8Array([1,2]);Object.defineProperty(a,'length',{value:0});var it=a.values();it.next().value===1&&it.next().value===2&&it.next().done"));
    CHECK(allocations > 20);
    CHECK(inspectOK && inspections > 0);
    CHECK(Evaluate(cx,global,"inspectTyped=function(o){Object.preventExtensions(o);gc();return !Object.isExtensible(o)};a=new Uint8Array([7,8]);a[0]===7&&a[1]===8&&!Object.isExtensible(a)"));
    CHECK(inspectOK);
    JS_SetObjectHook(rt, NULL, NULL);
    CHECK(Evaluate(cx,global,"a=new Uint8Array([1,2]);it=a.values();detach(a.buffer);gc();a.length===0&&a.byteLength===0&&a.byteOffset===0&&ArrayBuffer.isView(a)&&typeError(function(){return a[0]})&&typeError(function(){return 0 in a})&&typeError(function(){it.next()})"));
    CHECK(Evaluate(cx,global,"typeError(function(){Object.getOwnPropertyDescriptor(a,'0')})&&typeError(function(){Reflect.deleteProperty(a,'0')})&&typeError(function(){a[0]=1})&&Reflect.ownKeys(a).join()==='0,1'"));
    CHECK(Evaluate(cx,global,"a=new Uint8Array(2);typeError(function(){a[0]={valueOf:function(){detach(a.buffer);gc();return 1}}})"));
    CHECK(Evaluate(cx,global,"a=new Uint8Array(2);typeError(function(){Reflect.set(a,'-1',{valueOf:function(){detach(a.buffer);gc();return 1}})})"));
    CHECK(Evaluate(cx,global,"a=new Uint8Array(2);typeError(function(){a.fill({valueOf:function(){detach(a.buffer);gc();return 1}})})"));
    CHECK(Evaluate(cx,global,"a=new Uint8Array(2);typeError(function(){a.copyWithin({valueOf:function(){detach(a.buffer);gc();return 0}},1)})"));
    CHECK(Evaluate(cx,global,"a=new Uint8Array([2,1]);typeError(function(){a.sort(function(){detach(a.buffer);gc();return 0})})"));
    CHECK(Evaluate(cx,global,"a=new Uint8Array([2,1]);typeError(function(){a.map(function(v){detach(a.buffer);gc();return v})})"));
    CHECK(Evaluate(cx,global,"a=new Uint8Array(2);var b=a.buffer;typeError(function(){new Uint8Array(b,{valueOf:function(){detach(b);gc();return 0}})})"));
    CHECK(Evaluate(cx,global,"b=new ArrayBuffer(2);typeError(function(){new Uint8Array(b,0,{valueOf:function(){detach(b);gc();return 1}})})"));
    CHECK(Evaluate(cx,global,"a=new Uint8Array(2);Object.defineProperty(a.buffer,'constructor',{get:function(){detach(a.buffer);gc();return ArrayBuffer}});typeError(function(){new Uint8Array(a)})"));
    CHECK(Evaluate(cx,global,"a=new Uint8Array(2);var alt=new Proxy(function(){},{get:function(o,k){if(k==='prototype'){detach(a.buffer);gc()}return o[k]}});a.buffer.constructor={};a.buffer.constructor[Symbol.species]=alt;typeError(function(){new Uint8Array(a)})"));
    CHECK(Evaluate(cx,global,"a=new Uint8Array(2);a.constructor={};a.constructor[Symbol.species]=function(n){detach(a.buffer);gc();return new Uint8Array(n)};typeError(function(){a.slice()})"));
    CHECK(Evaluate(cx,global,"a=new Uint8Array(2);a.constructor={};a.constructor[Symbol.species]=function(n){var r=new Uint8Array(n);detach(r.buffer);gc();return r};typeError(function(){a.map(function(v){return v})})"));
    CHECK(Evaluate(cx,global,"a=new Uint8Array([1]);it=a.values();it.next();it.next();detach(a.buffer);it.next().done"));
    printf("ES6-TYPED-ARRAY-EMBEDDING PASS checks=%u allocations=%u inspections=%u\n",checks,allocations,inspections); status = 0;
  out:
    JS_SetObjectHook(rt, NULL, NULL);
    JS_RemoveRoot(cx, &global);
    JS_EndRequest(cx); JS_DestroyContext(cx); JS_DestroyRuntime(rt); JS_ShutDown();
    return status;
}
