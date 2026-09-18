/* Native Modern Error realms, cloning, interrupts and legacy globals.
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
    return JS_EvaluateScript(cx, global, source, strlen(source), "error-modern", 1, &value) && value == JSVAL_TRUE;
}
#define CHECK(c) do { ++checks; if (!(c)) { fprintf(stderr,"FAIL Modern Error embedding check %u\n",checks); goto out; } } while (0)
int main(void)
{
    JSRuntime *rt=JS_NewRuntime(16*1024*1024);
    JSContext *cx,*other=NULL;
    JSObject *global,*foreign,*clone;
    jsval value,method,result;
    JSErrorReport *report;
    JSBool rooted=JS_FALSE;
    int status=1;
    const char *failure="null.property";
    if(!rt)return 1;
    cx=JS_NewContext(rt,8192);
    if(!cx){JS_DestroyRuntime(rt);return 1;}
    JS_BeginRequest(cx);JS_SetVersion(cx,JSVERSION_ECMA_2015);
    global=JS_NewObject(cx,&globalClass,NULL,NULL);CHECK(global);
    JS_SetGlobalObject(cx,global);CHECK(JS_InitStandardClasses(cx,global));
    other=JS_NewContext(rt,8192);CHECK(other);
    JS_BeginRequest(other);JS_SetVersion(other,JSVERSION_ECMA_2015);
    foreign=JS_NewObject(other,&globalClass,NULL,NULL);CHECK(foreign);
    JS_SetGlobalObject(other,foreign);CHECK(JS_InitStandardClasses(other,foreign));
    CHECK(Evaluate(other,foreign,"function Alt(){}Alt.prototype=null;var exports={error:Error,type:TypeError,errorProto:Error.prototype,typeProto:TypeError.prototype,alt:Alt,instance:new Error('foreign')};true"));
    CHECK(JS_GetProperty(other,foreign,"exports",&value));
    CHECK(JS_DefineProperty(cx,global,"foreign",value,NULL,NULL,0));
    JS_EndRequest(other);JS_DestroyContextNoGC(other);other=NULL;
    JS_ClearNewbornRoots(cx);JS_GC(cx);CHECK(finalized==0);
    CHECK(Evaluate(cx,global,"Object.getPrototypeOf(foreign.error('x'))===foreign.errorProto"));
    CHECK(Evaluate(cx,global,"Object.getPrototypeOf(Reflect.construct(TypeError,['x'],foreign.alt))===foreign.typeProto"));
    CHECK(Evaluate(cx,global,"foreign.instance.message==='foreign'&&typeof foreign.instance.stack==='string'"));
    CHECK(JS_GetProperty(cx,JSVAL_TO_OBJECT(value),"error",&method));
    clone=JS_CloneFunctionObject(cx,JSVAL_TO_OBJECT(method),global);CHECK(clone);
    CHECK(JS_DefineProperty(cx,global,"clonedError",OBJECT_TO_JSVAL(clone),NULL,NULL,0));
    CHECK(Evaluate(cx,global,"var cloneInstance=clonedError('cloned');Object.getPrototypeOf(cloneInstance)===clonedError.prototype&&cloneInstance.message==='cloned'"));
    JS_SetOptions(cx,JS_GetOptions(cx)|JSOPTION_DONT_REPORT_UNCAUGHT);
    CHECK(!JS_EvaluateScript(cx,global,failure,strlen(failure),"native-error",1,&result));
    CHECK(JS_GetPendingException(cx,&result));
    report=JS_ErrorFromException(cx,result);CHECK(report&&report->ucmessage);
    JS_GC(cx);CHECK(JS_ErrorFromException(cx,result)==report&&report->ucmessage);
    JS_ClearPendingException(cx);
    CHECK(Evaluate(cx,global,"new Error({toString:function(){return new Error('nested').message;}}).message==='nested'"));
    CHECK(JS_GetProperty(cx,global,"Error",&method));
    CHECK(JS_AddNamedRoot(cx,&method,"modern Error constructor"));rooted=JS_TRUE;
    CHECK(Evaluate(cx,global,"delete this.foreign;delete this.clonedError;cloneInstance=null;true"));
    JS_ClearNewbornRoots(cx);JS_GC(cx);CHECK(finalized==1);
    JS_ClearScope(cx,global);JS_SetVersion(cx,JSVERSION_1_7);
    CHECK(JS_InitStandardClasses(cx,global));
    CHECK(Evaluate(cx,global,"Object.getPrototypeOf(TypeError)===Function.prototype&&Object.prototype.toString.call(Error.prototype)==='[object Error]'"));
    CHECK(Evaluate(cx,global,"var e=new Error('legacy');delete e.message;e.message==='legacy'"));
    CHECK(Evaluate(cx,global,"typeof new Error.prototype==='object'"));
    clone=JS_CloneFunctionObject(cx,JSVAL_TO_OBJECT(method),global);CHECK(clone);
    CHECK(JS_DefineProperty(cx,global,"clonedModernError",OBJECT_TO_JSVAL(clone),NULL,NULL,0));
    CHECK(Evaluate(cx,global,"var e=clonedModernError('modern');delete e.message;!e.hasOwnProperty('message')"));
    JS_RemoveRoot(cx,&method);rooted=JS_FALSE;
    printf("ES6-ERROR-MODERN-EMBEDDING checks=%u failures=0\n",checks);status=0;
  out:
    if(rooted)JS_RemoveRoot(cx,&method);
    if(other){JS_EndRequest(other);JS_DestroyContextNoGC(other);}
    JS_EndRequest(cx);JS_DestroyContext(cx);JS_DestroyRuntime(rt);JS_ShutDown();
    return status;
}
