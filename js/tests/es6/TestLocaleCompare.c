/* Native collation hooks, collection, method policy and cloning.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "jsapi.h"
#include <stdio.h>
#include <string.h>
static unsigned checks,calls,leftLength;
static JSBool failHook;
static JSClass globalClass={"global",0,JS_PropertyStub,JS_PropertyStub,
 JS_PropertyStub,JS_PropertyStub,JS_EnumerateStub,JS_ResolveStub,JS_ConvertStub,
 JS_FinalizeStub,JSCLASS_NO_OPTIONAL_MEMBERS};
static JSBool Compare(JSContext *cx, JSString *a, JSString *b, jsval *rval)
{
    ++calls;JS_GC(cx);leftLength=JS_GetStringLength(a);
    if(failHook){JS_ReportError(cx,"collation callback failure");return JS_FALSE;}
    *rval=INT_TO_JSVAL(1);return JS_TRUE;
}
static JSBool Evaluate(JSContext *cx, JSObject *global, const char *source)
{
    jsval v;return JS_EvaluateScript(cx,global,source,strlen(source),"locale-compare",1,&v)&&v==JSVAL_TRUE;
}
#define CHECK(c) do{++checks;if(!(c)){fprintf(stderr,"FAIL locale compare check %u calls=%u length=%u\n",checks,calls,leftLength);goto out;}}while(0)
int main(void)
{
    JSRuntime *rt=JS_NewRuntime(16*1024*1024);
    JSContext *cx;
    JSObject *global,*legacy,*clone;
    JSLocaleCallbacks callbacks;
    jsval method;
    JSBool rooted=JS_FALSE;
    int status=1;
    if(!rt)return 1;
    cx=JS_NewContext(rt,8192);if(!cx){JS_DestroyRuntime(rt);return 1;}
    JS_BeginRequest(cx);JS_SetVersion(cx,JSVERSION_ECMA_2015);
    global=JS_NewObject(cx,&globalClass,NULL,NULL);CHECK(global);
    JS_SetGlobalObject(cx,global);CHECK(JS_InitStandardClasses(cx,global));
    CHECK(Evaluate(cx,global,"'o\\u0308'.localeCompare('\\u00f6')===0"));
    CHECK(Evaluate(cx,global,"var modern=String.prototype.localeCompare;true"));
    CHECK(JS_GetProperty(cx,global,"modern",&method));
    CHECK(JS_AddNamedRoot(cx,&method,"modern collation"));rooted=JS_TRUE;
    memset(&callbacks,0,sizeof(callbacks));callbacks.localeCompare=Compare;JS_SetLocaleCallbacks(cx,&callbacks);
    CHECK(Evaluate(cx,global,"'o\\u0308'.localeCompare('\\u00f6')===0")&&calls==0);
    CHECK(Evaluate(cx,global,"'o\\u0308'.localeCompare('z')===1")&&calls==1&&leftLength==1);
    failHook=JS_TRUE;
    CHECK(!Evaluate(cx,global,"'a'.localeCompare('b')===1")&&calls==2);
    JS_ClearPendingException(cx);failHook=JS_FALSE;
    CHECK(Evaluate(cx,global,"'a'.localeCompare('b')===1")&&calls==3);
    JS_SetVersion(cx,JSVERSION_1_7);
    legacy=JS_NewObject(cx,&globalClass,NULL,NULL);CHECK(legacy);
    CHECK(JS_SetParent(cx,legacy,NULL) && JS_DefineProperty(cx,global,"legacy",OBJECT_TO_JSVAL(legacy),NULL,NULL,0));
    JS_SetGlobalObject(cx,legacy);CHECK(JS_InitStandardClasses(cx,legacy));
    CHECK(Evaluate(cx,legacy,"'o\\u0308'.localeCompare('\\u00f6')===1")&&calls==4&&leftLength==2);
    CHECK(JS_DefineProperty(cx,legacy,"modern",method,NULL,NULL,0));
    CHECK(Evaluate(cx,legacy,"modern.call('o\\u0308','\\u00f6')===0")&&calls==4);
    clone=JS_CloneFunctionObject(cx,JSVAL_TO_OBJECT(method),legacy);CHECK(clone);
    CHECK(JS_DefineProperty(cx,legacy,"clone",OBJECT_TO_JSVAL(clone),NULL,NULL,0));
    JS_ClearNewbornRoots(cx);JS_GC(cx);
    CHECK(Evaluate(cx,legacy,"clone.call('o\\u0308','z')===1")&&calls==5&&leftLength==1);
    JS_SetLocaleCallbacks(cx,NULL);
    CHECK(Evaluate(cx,legacy,"'o\\u0308'.localeCompare('\\u00f6')!==0&&clone.call('o\\u0308','\\u00f6')===0"));
    printf("ES6-LOCALE-COMPARE checks=%u failures=0\n",checks);status=0;
out:
    JS_SetLocaleCallbacks(cx,NULL);if(rooted)JS_RemoveRoot(cx,&method);
    JS_EndRequest(cx);JS_DestroyContext(cx);JS_DestroyRuntime(rt);JS_ShutDown();return status;
}
