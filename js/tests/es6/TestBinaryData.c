/* Native binary-data detachment, realms and callback lifetime checks.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "jsapi.h"
#include "../../src/jsbinarydata.h"
#include <stdio.h>
#include <string.h>
static unsigned checks, interrupted;
static JSBool stopOnBranch;
static JSObject *detachOnBranch;
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
    return JS_DetachArrayBuffer(cx, JSVAL_TO_OBJECT(argv[0]));
}
static JSBool Collect(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JS_GC(cx); *rval = JSVAL_VOID; return JS_TRUE;
}
static JSBool Branch(JSContext *cx, JSScript *script)
{
    if (!script) {
        ++interrupted;
        JS_GC(cx);
        if (detachOnBranch) return JS_DetachArrayBuffer(cx, detachOnBranch);
        if (stopOnBranch) return JS_FALSE;
    }
    return JS_TRUE;
}
static JSBool Evaluate(JSContext *cx, JSObject *global, const char *source)
{
    jsval value;
    return JS_EvaluateScript(cx, global, source, strlen(source), "binary-data", 1, &value) && value == JSVAL_TRUE;
}
#define CHECK(c) do { ++checks; if (!(c)) { fprintf(stderr,"FAIL binary-data embedding check %u\n",checks); goto out; } } while (0)
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
    CHECK(JS_DefineFunction(cx,global,"detach",Detach,1,0));
    CHECK(JS_DefineFunction(cx,global,"gc",Collect,0,0));
    CHECK(Evaluate(cx,global,"function typeError(f){try{f()}catch(e){return e instanceof TypeError}return false}var b=new ArrayBuffer(16),v=new DataView(b);v.setInt32(0,123);gc();v.getInt32(0)===123"));
    CHECK(Evaluate(cx,global,"detach(b);gc();typeError(function(){return v.getInt8(0)})&&typeError(function(){return b.byteLength})&&typeError(function(){return v.byteLength})&&typeError(function(){return v.byteOffset})&&v.buffer===b&&ArrayBuffer.isView(v)"));
    CHECK(Evaluate(cx,global,"detach(b);typeError(function(){return b.slice(0)})"));
    CHECK(Evaluate(cx,global,"b=new ArrayBuffer(8);typeError(function(){new DataView(b,{valueOf:function(){detach(b);gc();return 0}})})"));
    CHECK(Evaluate(cx,global,"b=new ArrayBuffer(8);typeError(function(){new DataView(b,0,{valueOf:function(){detach(b);gc();return 1}})})"));
    CHECK(Evaluate(cx,global,"b=new ArrayBuffer(8);var target=new Proxy(function(){},{get:function(o,p){if(p==='prototype'){detach(b);gc();return {}}return o[p]}});typeError(function(){Reflect.construct(DataView,[b,0,1],target)})"));
    CHECK(Evaluate(cx,global,"b=new ArrayBuffer(8);v=new DataView(b);typeError(function(){v.getInt8({valueOf:function(){detach(b);gc();return 0}})})"));
    CHECK(Evaluate(cx,global,"b=new ArrayBuffer(8);v=new DataView(b);typeError(function(){v.setFloat64(0,{valueOf:function(){detach(b);gc();return 3}})})"));
    CHECK(Evaluate(cx,global,"b=new ArrayBuffer(8);b.constructor={};b.constructor[Symbol.species]=function(n){detach(b);gc();return new ArrayBuffer(n)};typeError(function(){b.slice(0,1)})"));
    CHECK(Evaluate(cx,global,"b=new ArrayBuffer(8);b.constructor={};b.constructor[Symbol.species]=function(n){var r=new ArrayBuffer(n);detach(r);gc();return r};typeError(function(){b.slice(0,1)})"));
    CHECK(Evaluate(cx,global,"b=new ArrayBuffer(200000);v=new DataView(b);v.setInt8(0,42);true"));
    JS_SetBranchCallback(cx,Branch);
    CHECK(Evaluate(cx,global,"new DataView(b.slice(0)).getInt8(0)===42"));
    CHECK(interrupted >= 4);
    CHECK(JS_GetProperty(cx,global,"b",&value)); detachOnBranch = JSVAL_TO_OBJECT(value);
    CHECK(Evaluate(cx,global,"typeError(function(){b.slice(0)})"));
    detachOnBranch = NULL;
    CHECK(Evaluate(cx,global,"b=new ArrayBuffer(200000);var destination=new ArrayBuffer(200000);b.constructor={};b.constructor[Symbol.species]=function(){return destination};true"));
    CHECK(JS_GetProperty(cx,global,"destination",&value)); detachOnBranch = JSVAL_TO_OBJECT(value);
    CHECK(Evaluate(cx,global,"typeError(function(){b.slice(0)})"));
    detachOnBranch = NULL; stopOnBranch = JS_TRUE;
    CHECK(!Evaluate(cx,global,"delete b.constructor;b.slice(0);true"));
    stopOnBranch = JS_FALSE; JS_ClearPendingException(cx);
    CHECK(Evaluate(cx,global,"b.slice(0).byteLength===200000"));
    JS_SetBranchCallback(cx,NULL);
    other = JS_NewContext(rt,8192); CHECK(other);
    JS_BeginRequest(other); JS_SetVersion(other,JSVERSION_ECMA_2015);
    foreign = JS_NewObject(other,&globalClass,NULL,NULL); CHECK(foreign);
    JS_SetGlobalObject(other,foreign); CHECK(JS_InitStandardClasses(other,foreign));
    CHECK(Evaluate(other,foreign,"function Alt(){}Alt.prototype=null;var exports={buffer:ArrayBuffer,view:DataView,bufferProto:ArrayBuffer.prototype,viewProto:DataView.prototype,alt:Alt};true"));
    CHECK(JS_GetProperty(other,foreign,"exports",&value));
    CHECK(JS_DefineProperty(cx,global,"foreign",value,NULL,NULL,0));
    JS_EndRequest(other); JS_DestroyContextNoGC(other); other = NULL;
    JS_ClearNewbornRoots(cx); JS_GC(cx);
    CHECK(Evaluate(cx,global,"b=Reflect.construct(ArrayBuffer,[8],foreign.alt);Object.getPrototypeOf(b)===foreign.bufferProto&&b.byteLength===8"));
    CHECK(Evaluate(cx,global,"v=Reflect.construct(DataView,[b],foreign.alt);Object.getPrototypeOf(v)===foreign.viewProto&&v.buffer===b"));
    CHECK(Evaluate(cx,global,"b.constructor=void 0;Object.getPrototypeOf(b.slice(0))===foreign.bufferProto"));
    CHECK(JS_GetProperty(cx,JSVAL_TO_OBJECT(value),"buffer",&method));
    clone = JS_CloneFunctionObject(cx,JSVAL_TO_OBJECT(method),global); CHECK(clone);
    CHECK(JS_DefineProperty(cx,global,"clonedBuffer",OBJECT_TO_JSVAL(clone),NULL,NULL,0));
    CHECK(Evaluate(cx,global,"b=new clonedBuffer(4);Object.getPrototypeOf(b)===clonedBuffer.prototype&&Object.getOwnPropertyDescriptor(ArrayBuffer.prototype,'byteLength').get.call(b)===4"));
    CHECK(Evaluate(cx,global,"delete this.ArrayBuffer;typeof ArrayBuffer==='undefined'"));
    CHECK(Evaluate(cx,global,"delete this.DataView;typeof DataView==='undefined'"));
    JS_ClearScope(cx,global); JS_SetVersion(cx,JSVERSION_1_7);
    CHECK(JS_InitStandardClasses(cx,global));
    CHECK(Evaluate(cx,global,"var b=new ArrayBuffer(8),v=new DataView(b);v.setUint16(0,4660,{valueOf:function(){throw 'must not coerce'}});v.getUint8(0)===52&&v.getUint8(1)===18"));
    printf("ES6-BINARY-DATA-EMBEDDING checks=%u failures=0\n",checks); status = 0;
  out:
    JS_SetBranchCallback(cx,NULL); detachOnBranch = NULL;
    if (other) { JS_EndRequest(other); JS_DestroyContextNoGC(other); }
    JS_EndRequest(cx); JS_DestroyContext(cx); JS_DestroyRuntime(rt); JS_ShutDown();
    return status;
}
