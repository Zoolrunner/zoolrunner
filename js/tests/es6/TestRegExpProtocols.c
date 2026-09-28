/* Native RegExp protocol realms, cloning and interruptibility.
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
    return JS_EvaluateScript(cx, global, source, strlen(source), "regexp-protocols", 1, &value) && value == JSVAL_TRUE;
}
static JSBool Stop(JSContext *cx, JSScript *script)
{
    ++interrupted; JS_GC(cx); JS_ReportError(cx, "RegExp protocol interrupted");
    return JS_FALSE;
}
#define CHECK(c) do { ++checks; if (!(c)) { fprintf(stderr,"FAIL RegExp protocols embedding check %u\n",checks); goto out; } } while (0)
int main(void)
{
    JSRuntime *rt = JS_NewRuntime(16 * 1024 * 1024);
    JSContext *cx, *other = NULL;
    JSObject *global, *foreign, *clone, *receiver;
    jsval value, method, argument, result;
    int status = 1;
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
    CHECK(Evaluate(other,foreign,"var exports={exec:RegExp.prototype.exec,match:RegExp.prototype[Symbol.match],"
          "search:RegExp.prototype[Symbol.search],test:RegExp.prototype.test,strmatch:String.prototype.match,"
          "strsearch:String.prototype.search,arrayProto:Array.prototype};"
          "Object.defineProperty(Number.prototype,Symbol.search,{get:function(){'use strict';if(this!==7)throw Error('receiver');"
          "return function(value){'use strict';return this===7 && value===19 ? 88 : 0;};}});true"));
    CHECK(JS_GetProperty(other,foreign,"exports",&value));
    CHECK(JS_DefineProperty(cx,global,"foreign",value,NULL,NULL,0));
    JS_EndRequest(other); JS_DestroyContextNoGC(other); other=NULL;
    JS_ClearNewbornRoots(cx); JS_GC(cx); CHECK(finalized==0);
    CHECK(Evaluate(cx,global,"Object.getPrototypeOf(foreign.exec.call(/a/,'a'))===foreign.arrayProto"));
    CHECK(Evaluate(cx,global,"Object.getPrototypeOf(foreign.match.call(/a/g,'aa'))===foreign.arrayProto"));
    CHECK(Evaluate(cx,global,"Object.getPrototypeOf(foreign.match.call(/a/,'a'))===Array.prototype"));
    CHECK(Evaluate(cx,global,"var r=/a/;r.exec=null;Object.getPrototypeOf(foreign.match.call(r,'a'))===foreign.arrayProto"));
    CHECK(Evaluate(cx,global,"foreign.strsearch.call(19,7)===88 && foreign.strsearch.call('ba','a')===1"));
    CHECK(Evaluate(cx,global,"Object.getPrototypeOf(foreign.strmatch.call('aa','a'))===foreign.arrayProto"));
    CHECK(JS_GetProperty(cx,JSVAL_TO_OBJECT(value),"match",&method));
    clone=JS_CloneFunctionObject(cx,JSVAL_TO_OBJECT(method),global); CHECK(clone);
    CHECK(JS_DefineProperty(cx,global,"clonedMatch",OBJECT_TO_JSVAL(clone),NULL,NULL,0));
    CHECK(Evaluate(cx,global,"Object.getPrototypeOf(clonedMatch.call(/a/g,'aa'))===Array.prototype"));
    CHECK(Evaluate(cx,global,"var loop={global:true,exec:function(){return ['x'];}};var matchMethod=RegExp.prototype[Symbol.match];true"));
    CHECK(JS_GetProperty(cx,global,"loop",&value)); receiver=JSVAL_TO_OBJECT(value);
    CHECK(JS_GetProperty(cx,global,"matchMethod",&method));
    argument=STRING_TO_JSVAL(JS_InternString(cx,"input"));
    JS_SetBranchCallback(cx,Stop);
    CHECK(!JS_CallFunctionValue(cx,receiver,method,1,&argument,&result) && interrupted>0);
    JS_SetBranchCallback(cx,NULL); JS_ClearPendingException(cx);
    CHECK(Evaluate(cx,global,"/a/.test('a') && 'ba'.search('a')===1"));
    CHECK(Evaluate(cx,global,"delete this.foreign;delete this.clonedMatch;true"));
    JS_ClearNewbornRoots(cx); JS_GC(cx); CHECK(finalized==1);
    JS_ClearScope(cx,global); JS_SetVersion(cx,JSVERSION_1_7);
    CHECK(JS_InitStandardClasses(cx,global));
    CHECK(Evaluate(cx,global,"var r=/a/;r.exec=function(){throw Error('legacy override');};r.test('a') && 'a'.match(r)[0]==='a' && 'ba'.search(r)===1"));
    printf("ES6-REGEXP-PROTOCOLS-EMBEDDING checks=%u failures=0\n",checks);status=0;
  out:
    JS_SetBranchCallback(cx,NULL);
    if (other) { JS_EndRequest(other);JS_DestroyContextNoGC(other); }
    JS_EndRequest(cx);JS_DestroyContext(cx);JS_DestroyRuntime(rt);JS_ShutDown();
    return status;
}
