/* Native Date conversion realms, lazy bootstrap and legacy JSAPI callers.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "jsapi.h"
#include "jsdate.h"
#include <stdio.h>
#include <string.h>
static unsigned checks, finalized;
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
    return JS_EvaluateScript(cx,global,source,strlen(source),"date-primitive",1,&value) && value==JSVAL_TRUE;
}
static JSBool Resolve(JSContext *cx, JSObject *global, const char *name)
{
    JSString *str=JS_InternString(cx,name);
    JSBool resolved=JS_FALSE;
    return str && JS_ResolveStandardClass(cx,global,STRING_TO_JSVAL(str),&resolved) && resolved;
}
#define CHECK(c) do { ++checks; if (!(c)) { fprintf(stderr,"FAIL Date primitive embedding check %u\n",checks); goto out; } } while (0)
int main(void)
{
    JSRuntime *rt=JS_NewRuntime(16*1024*1024);
    JSContext *cx,*other=NULL;
    JSObject *global,*foreign,*clone,*nativeDate;
    jsval value,method;
    int status=1;
    if (!rt) return 1;
    cx=JS_NewContext(rt,8192);
    if (!cx) {JS_DestroyRuntime(rt);return 1;}
    JS_BeginRequest(cx);JS_SetVersion(cx,JSVERSION_ECMA_2015);
    global=JS_NewObject(cx,&globalClass,NULL,NULL);CHECK(global);
    JS_SetGlobalObject(cx,global);CHECK(JS_InitStandardClasses(cx,global));
    nativeDate=js_NewDateObjectMsec(cx,1234);CHECK(nativeDate);
    CHECK(JS_DefineProperty(cx,global,"nativeDate",OBJECT_TO_JSVAL(nativeDate),NULL,NULL,0));
    CHECK(js_DateIsValid(cx,nativeDate) && js_DateGetMsecSinceEpoch(cx,nativeDate)==1234);
    CHECK(Evaluate(cx,global,"nativeDate.getTime()===1234 && nativeDate[Symbol.toPrimitive]('number')===1234 && new Date(nativeDate).getTime()===1234"));
    other=JS_NewContext(rt,8192);CHECK(other);
    JS_BeginRequest(other);JS_SetVersion(other,JSVERSION_ECMA_2015);
    foreign=JS_NewObject(other,&globalClass,NULL,NULL);CHECK(foreign);
    JS_SetGlobalObject(other,foreign);CHECK(Resolve(other,foreign,"Date"));
    CHECK(JS_InitStandardClasses(other,foreign));
    CHECK(Evaluate(other,foreign,"var exports={hook:Date.prototype[Symbol.toPrimitive],value:Date.prototype.valueOf,date:new Date(1234)};true"));
    CHECK(JS_GetProperty(other,foreign,"exports",&value));
    CHECK(JS_DefineProperty(cx,global,"foreign",value,NULL,NULL,0));
    JS_EndRequest(other);JS_DestroyContextNoGC(other);other=NULL;
    JS_ClearNewbornRoots(cx);JS_GC(cx);CHECK(finalized==0);
    CHECK(Evaluate(cx,global,"foreign.hook.call(foreign.date,'number')===1234 && foreign.hook.call(new Date(42),'number')===42"));
    CHECK(Evaluate(cx,global,"foreign.hook.call({toString:function(){return arguments.length===0?'yes':'no';}},'default')==='yes'"));
    CHECK(Evaluate(cx,global,"foreign.value.call(new Date(42),'string')===42"));
    CHECK(Evaluate(cx,global,"Object.defineProperty(foreign.date,Symbol.toPrimitive,{value:null});foreign.date+1===1235"));
    CHECK(JS_GetProperty(cx,JSVAL_TO_OBJECT(value),"hook",&method));
    clone=JS_CloneFunctionObject(cx,JSVAL_TO_OBJECT(method),global);CHECK(clone);
    CHECK(JS_DefineProperty(cx,global,"clonedHook",OBJECT_TO_JSVAL(clone),NULL,NULL,0));
    CHECK(Evaluate(cx,global,"clonedHook.call({valueOf:function(){return 19;}},'number')===19"));
    JS_SetVersion(cx,JSVERSION_1_7);
    CHECK(Evaluate(cx,global,"foreign.hook.call(new Date(3),'number')===3 && foreign.value.call(new Date(4),'string')===4"));
    CHECK(Evaluate(cx,global,"delete this.foreign;delete this.clonedHook;true"));
    JS_ClearNewbornRoots(cx);JS_GC(cx);CHECK(finalized==1);
    JS_ClearScope(cx,global);JS_SetVersion(cx,JSVERSION_1_7);
    CHECK(Resolve(cx,global,"Object"));
    /* A modern caller must not modernize Date lazily in an established legacy global. */
    JS_SetVersion(cx,JSVERSION_ECMA_2015);CHECK(Resolve(cx,global,"Date"));
    CHECK(Resolve(cx,global,"Symbol"));
    CHECK(Evaluate(cx,global,"Date.prototype[Symbol.toPrimitive]===void 0 && new Date(0).valueOf('string')===new Date(0).toString()"));
    JS_ClearScope(cx,global);JS_SetVersion(cx,JSVERSION_ECMA_2015);
    CHECK(Resolve(cx,global,"Date"));CHECK(JS_InitStandardClasses(cx,global));
    CHECK(Evaluate(cx,global,"typeof Date.prototype[Symbol.toPrimitive]==='function' && new Date(0).valueOf('string')===0"));
    printf("ES6-DATE-PRIMITIVE-EMBEDDING checks=%u failures=0\n",checks);status=0;
  out:
    if (other) {JS_EndRequest(other);JS_DestroyContextNoGC(other);}
    JS_EndRequest(cx);JS_DestroyContext(cx);JS_DestroyRuntime(rt);JS_ShutDown();
    return status;
}
