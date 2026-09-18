/* Native Array text realms, cloning, interrupts and legacy globals.
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
    return JS_EvaluateScript(cx, global, source, strlen(source), "array-text", 1, &value) && value == JSVAL_TRUE;
}
static JSBool Stop(JSContext *cx, JSScript *script)
{
    if (script) return JS_TRUE;
    ++interrupted; JS_GC(cx);
    if (interrupted < 3) return JS_TRUE;
    JS_ReportError(cx, "Array text interrupted");
    return JS_FALSE;
}
#define CHECK(c) do { ++checks; if (!(c)) { fprintf(stderr,"FAIL Array text embedding check %u\n",checks); goto out; } } while (0)
int main(void)
{
    JSRuntime *rt=JS_NewRuntime(16*1024*1024);
    JSContext *cx,*other=NULL;
    JSObject *global,*foreign,*clone,*receiver;
    jsval value,method,argument,result;
    JSBool rooted=JS_FALSE;
    uintN i;
    int status=1;
    const char *loops[]={"join","toLocaleString","sort"};
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
    CHECK(Evaluate(other,foreign,"var exports={objectLocale:Object.prototype.toLocaleString,join:Array.prototype.join,sort:Array.prototype.sort,text:Array.prototype.toString,locale:Array.prototype.toLocaleString,numberProto:Number.prototype,boolProto:Boolean.prototype};true"));
    CHECK(JS_GetProperty(other,foreign,"exports",&value));
    CHECK(JS_DefineProperty(cx,global,"foreign",value,NULL,NULL,0));
    JS_EndRequest(other);JS_DestroyContextNoGC(other);other=NULL;
    JS_ClearNewbornRoots(cx);JS_GC(cx);CHECK(finalized==0);
    CHECK(Evaluate(cx,global,"Object.getPrototypeOf(foreign.sort.call(1))===foreign.numberProto"));
    CHECK(Evaluate(cx,global,"Object.getPrototypeOf(foreign.sort.call(true))===foreign.boolProto"));
    CHECK(Evaluate(cx,global,"foreign.boolProto.join=function(){return Object.getPrototypeOf(this);};foreign.text.call(true)===foreign.boolProto"));
    CHECK(Evaluate(cx,global,"foreign.numberProto.toLocaleString=function(){'use strict';return typeof this+':'+this;};foreign.locale.call([3])==='number:3'"));
    CHECK(Evaluate(cx,global,"foreign.boolProto.toString=function(){'use strict';return typeof this;};foreign.objectLocale.call(true)==='boolean'"));
    CHECK(JS_GetProperty(cx,JSVAL_TO_OBJECT(value),"sort",&method));
    clone=JS_CloneFunctionObject(cx,JSVAL_TO_OBJECT(method),global);CHECK(clone);
    CHECK(JS_DefineProperty(cx,global,"clonedSort",OBJECT_TO_JSVAL(clone),NULL,NULL,0));
    CHECK(Evaluate(cx,global,"Object.getPrototypeOf(clonedSort.call(1))===Number.prototype"));
    CHECK(Evaluate(cx,global,"var loop={length:1e8};var methods=Array.prototype;var localJoin=Array.prototype.join;true"));
    CHECK(JS_GetProperty(cx,global,"loop",&value));receiver=JSVAL_TO_OBJECT(value);
    CHECK(JS_GetProperty(cx,global,"methods",&value));
    for(i=0;i<sizeof(loops)/sizeof(loops[0]);++i){
        CHECK(JS_GetProperty(cx,JSVAL_TO_OBJECT(value),loops[i],&method));
        argument=JSVAL_VOID;interrupted=0;JS_SetBranchCallback(cx,Stop);
        CHECK(!JS_CallFunctionValue(cx,receiver,method,1,&argument,&result)&&interrupted==3);
        JS_SetBranchCallback(cx,NULL);JS_ClearPendingException(cx);
    }
    CHECK(Evaluate(cx,global,"var a=[1];a.push(a);a.join()==='1,'&&[3,1,2].sort().join()==='1,2,3'"));
    CHECK(Evaluate(cx,global,"var cmpArray=[];for(var n=256;n>0;--n)cmpArray.push(n);true"));
    CHECK(JS_GetProperty(cx,global,"cmpArray",&value));receiver=JSVAL_TO_OBJECT(value);
    CHECK(JS_GetProperty(cx,receiver,"sort",&method));
    argument=JSVAL_VOID;interrupted=0;JS_SetBranchCallback(cx,Stop);
    CHECK(!JS_CallFunctionValue(cx,receiver,method,1,&argument,&result)&&interrupted==3);
    JS_SetBranchCallback(cx,NULL);JS_ClearPendingException(cx);
    CHECK(Evaluate(cx,global,"cmpArray[0]===256&&cmpArray[255]===1&&cmpArray.sort().length===256"));
    CHECK(JS_GetProperty(cx,global,"localJoin",&method));
    CHECK(JS_AddNamedRoot(cx,&method,"modern join method"));rooted=JS_TRUE;
    CHECK(Evaluate(cx,global,"delete this.foreign;delete this.clonedSort;true"));
    JS_ClearNewbornRoots(cx);JS_GC(cx);CHECK(finalized==1);
    JS_ClearScope(cx,global);JS_SetVersion(cx,JSVERSION_1_7);
    CHECK(JS_InitStandardClasses(cx,global));
    CHECK(Evaluate(cx,global,"Array.prototype.join.call({0:'legacy',length:4294967297})==='legacy'"));
    CHECK(Evaluate(cx,global,"var a=[1,2];a.join=function(){throw Error('legacy join');};a.toString()==='1,2'"));
    CHECK(Evaluate(cx,global,"Boolean.prototype.toString=function(){'use strict';return typeof this;};Object.prototype.toLocaleString.call(true)==='object'"));
    clone=JS_CloneFunctionObject(cx,JSVAL_TO_OBJECT(method),global);CHECK(clone);
    CHECK(JS_DefineProperty(cx,global,"clonedModernJoin",OBJECT_TO_JSVAL(clone),NULL,NULL,0));
    CHECK(Evaluate(cx,global,"clonedModernJoin.call({0:1,length:-1})===''"));
    JS_RemoveRoot(cx,&method);rooted=JS_FALSE;
    printf("ES6-ARRAY-TEXT-EMBEDDING checks=%u failures=0\n",checks);status=0;
  out:
    if(rooted)JS_RemoveRoot(cx,&method);
    JS_SetBranchCallback(cx,NULL);
    if(other){JS_EndRequest(other);JS_DestroyContextNoGC(other);}
    JS_EndRequest(cx);JS_DestroyContext(cx);JS_DestroyRuntime(rt);JS_ShutDown();
    return status;
}
