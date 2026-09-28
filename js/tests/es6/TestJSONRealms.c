/* JSON native method editions, realms, collection and loop interruption.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "jsapi.h"
#include <stdio.h>
#include <string.h>
static unsigned checks, interrupted;
static JSClass globalClass = {
    "global", 0,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};
static JSBool Evaluate(JSContext *cx, JSObject *global, const char *source)
{
    jsval result;
    return JS_EvaluateScript(cx,global,source,strlen(source),"JSON-realms",1,&result) && result==JSVAL_TRUE;
}
static JSBool Stop(JSContext *cx, JSScript *script)
{
    if (script) return JS_TRUE;
    ++interrupted;JS_GC(cx);
    if (interrupted<3) return JS_TRUE;
    JS_ReportError(cx,"JSON loop interrupted");return JS_FALSE;
}
#define CHECK(c) do {++checks;if(!(c)){fprintf(stderr,"FAIL JSON native check %u\n",checks);goto out;}} while(0)
int main(void)
{
    JSRuntime *rt=JS_NewRuntime(16*1024*1024);
    JSContext *cx;
    JSObject *global,*other,*clone;
    jsval methods[2],json,value;
    JSBool rooted0=JS_FALSE,rooted1=JS_FALSE;
    unsigned i;
    int status=1;
    const char *loops[]={
        "JSON.stringify(longArray)",
        "JSON.stringify({},longArray)",
        "JSON.parse('{\"a\":0,\"b\":0}',function(k,v){if(k==='a')this.b=longArray;return v;})"
    };
    if(!rt)return 1;
    cx=JS_NewContext(rt,8192);if(!cx){JS_DestroyRuntime(rt);return 1;}
    JS_BeginRequest(cx);JS_SetVersion(cx,JSVERSION_ECMA_2015);
    global=JS_NewObject(cx,&globalClass,NULL,NULL);CHECK(global);
    JS_SetGlobalObject(cx,global);CHECK(JS_InitStandardClasses(cx,global));
    CHECK(JS_GetProperty(cx,global,"JSON",&json));
    CHECK(JS_GetProperty(cx,JSVAL_TO_OBJECT(json),"parse",&methods[0]));
    CHECK(JS_AddNamedRoot(cx,&methods[0],"modern JSON parse"));rooted0=JS_TRUE;
    CHECK(JS_GetProperty(cx,JSVAL_TO_OBJECT(json),"stringify",&methods[1]));
    CHECK(JS_AddNamedRoot(cx,&methods[1],"modern JSON stringify"));rooted1=JS_TRUE;
    CHECK(Evaluate(cx,global,"var longArray=new Proxy([],{get:function(t,k){return k==='length'?1e8:undefined;}});true"));
    for(i=0;i<3;i++) {
        interrupted=0;JS_SetBranchCallback(cx,Stop);
        CHECK(!Evaluate(cx,global,loops[i]) && interrupted==3);
        JS_SetBranchCallback(cx,NULL);JS_ClearPendingException(cx);
    }
    CHECK(Evaluate(cx,global,"JSON.stringify([1,2])==='[1,2]'&&JSON.parse('[1]')[0]===1"));
    /* Initialize an independent legacy global, retaining modern methods. */
    JS_SetVersion(cx,JSVERSION_1_7);
    other=JS_NewObject(cx,&globalClass,NULL,NULL);CHECK(other);
    CHECK(JS_SetParent(cx,other,NULL) && JS_DefineProperty(cx,global,"legacyGlobal",OBJECT_TO_JSVAL(other),NULL,NULL,0));
    JS_SetGlobalObject(cx,other);CHECK(JS_InitStandardClasses(cx,other));
    CHECK(JS_DefineProperty(cx,other,"modernParse",methods[0],NULL,NULL,0));
    CHECK(JS_DefineProperty(cx,other,"modernStringify",methods[1],NULL,NULL,0));
    CHECK(JS_GetProperty(cx,global,"Object",&value));
    CHECK(JS_GetProperty(cx,JSVAL_TO_OBJECT(value),"prototype",&value));
    CHECK(JS_DefineProperty(cx,other,"modernProto",value,NULL,NULL,0));
    CHECK(Evaluate(cx,other,"var reads=0;var array=new Proxy([],{get:function(t,k){if(k==='length')return 4294967296;if(k==='0'){reads++;throw 42;}return t[k];}});JSON.stringify(array)==='[]'&&JSON.stringify({},array)==='{}'&&reads===0"));
    CHECK(Evaluate(cx,other,"JSON.parse('{\"a\":0,\"b\":0}',function(k,v){if(k==='a')this.b=array;return v;});reads===0"));
    CHECK(Evaluate(cx,other,"var caught;try{modernStringify(array)}catch(e){caught=e}caught===42&&reads===1"));
    CHECK(Evaluate(cx,other,"Object.getPrototypeOf(modernParse('{}'))===modernProto"));
    clone=JS_CloneFunctionObject(cx,JSVAL_TO_OBJECT(methods[0]),other);CHECK(clone);
    CHECK(JS_DefineProperty(cx,other,"clonedParse",OBJECT_TO_JSVAL(clone),NULL,NULL,0));
    JS_ClearNewbornRoots(cx);JS_GC(cx);
    CHECK(Evaluate(cx,other,"Object.getPrototypeOf(clonedParse('{}'))===Object.prototype"));
    CHECK(Evaluate(cx,other,"var caught;try{clonedParse('{\"a\":0,\"b\":0}',function(k,v){if(k==='a')this.b=array;return v;})}catch(e){caught=e}caught===42"));
    printf("ES6-JSON-REALMS checks=%u failures=0\n",checks);status=0;
out:
    JS_SetBranchCallback(cx,NULL);
    if(rooted1)JS_RemoveRoot(cx,&methods[1]);
    if(rooted0)JS_RemoveRoot(cx,&methods[0]);
    JS_EndRequest(cx);JS_DestroyContext(cx);JS_DestroyRuntime(rt);JS_ShutDown();return status;
}
