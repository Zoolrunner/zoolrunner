/* Capture super base/receiver before callbacks and RHS changes.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "jsapi.h"
#include "jsfun.h"
#include <stdio.h>
#include <string.h>
static unsigned checks;
static JSClass globalClass={"global",JSCLASS_GLOBAL_FLAGS,JS_PropertyStub,JS_PropertyStub,JS_PropertyStub,JS_PropertyStub,JS_EnumerateStub,JS_ResolveStub,JS_ConvertStub,JS_FinalizeStub,JSCLASS_NO_OPTIONAL_MEMBERS};
static JSBool Collect(JSContext *cx,JSObject *obj,uintN argc,jsval *argv,jsval *rval){JS_GC(cx);*rval=JSVAL_VOID;return JS_TRUE;}
static JSBool Eval(JSContext *cx,JSObject *global,const char *code,jsval *result){return JS_EvaluateScript(cx,global,code,strlen(code),"super-reference",1,result);}
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"SUPER-REFERENCE FAIL %u\n",checks);goto out;}}while(0)
int main(void){
 JSRuntime *rt=JS_NewRuntime(8*1024*1024);JSContext *cx;JSObject *global=NULL,*method=NULL,*home=NULL,*reference=NULL;
 jsval result=JSVAL_VOID,receiver=JSVAL_VOID,key=JSVAL_VOID;int status=1;
 if(!rt)return 1;cx=JS_NewContext(rt,8192);if(!cx){JS_DestroyRuntime(rt);return 1;}JS_BeginRequest(cx);JS_SetVersion(cx,JSVERSION_ECMA_2015);
 JS_AddNamedRoot(cx,&global,"global");JS_AddNamedRoot(cx,&method,"method");JS_AddNamedRoot(cx,&home,"home");JS_AddNamedRoot(cx,&reference,"reference");JS_AddNamedRoot(cx,&result,"result");JS_AddNamedRoot(cx,&receiver,"receiver");JS_AddNamedRoot(cx,&key,"key");
 global=JS_NewObject(cx,&globalClass,NULL,NULL);CHECK(global);JS_SetGlobalObject(cx,global);CHECK(JS_InitStandardClasses(cx,global));CHECK(JS_DefineFunction(cx,global,"gcNow",Collect,0,0));
 CHECK(Eval(cx,global,"({m(){}}).m",&result));method=JSVAL_TO_OBJECT(result);CHECK(js_GetFunctionHomeObject(cx,method,&home)&&home);
 CHECK(Eval(cx,global,"({get value(){gcNow();return this.marker},set value(v){gcNow();this.marker=v}})",&result));CHECK(JS_SetPrototype(cx,home,JSVAL_TO_OBJECT(result)));
 CHECK(Eval(cx,global,"({marker:17})",&receiver));key=STRING_TO_JSVAL(JS_InternString(cx,"value"));CHECK(JSVAL_TO_STRING(key));
 reference=js_NewSuperReference(cx,method,receiver,key,JS_TRUE);CHECK(reference);
 CHECK(Eval(cx,global,"({value:99})",&result));CHECK(JS_SetPrototype(cx,home,JSVAL_TO_OBJECT(result)));JS_GC(cx);
 CHECK(js_GetSuperReference(cx,reference,&result)&&result==INT_TO_JSVAL(17));
 CHECK(js_SetSuperReference(cx,reference,INT_TO_JSVAL(23)));CHECK(JS_GetProperty(cx,JSVAL_TO_OBJECT(receiver),"marker",&result)&&result==INT_TO_JSVAL(23));
 CHECK(js_GetSuperReference(cx,reference,&result)&&result==INT_TO_JSVAL(23));
 CHECK(Eval(cx,global,"Object.defineProperty({},'value',{value:4})",&result));CHECK(JS_SetPrototype(cx,home,JSVAL_TO_OBJECT(result)));
 reference=js_NewSuperReference(cx,method,receiver,key,JS_TRUE);CHECK(reference);CHECK(!js_SetSuperReference(cx,reference,INT_TO_JSVAL(5)));JS_ClearPendingException(cx);
 reference=js_NewSuperReference(cx,method,receiver,key,JS_FALSE);CHECK(reference);CHECK(js_SetSuperReference(cx,reference,INT_TO_JSVAL(5)));
 CHECK(Eval(cx,global,"new Proxy({}, {get:function(t,k,r){gcNow();return r.marker},set:function(t,k,v,r){gcNow();r.marker=v;return true}})",&result));CHECK(JS_SetPrototype(cx,home,JSVAL_TO_OBJECT(result)));
 reference=js_NewSuperReference(cx,method,receiver,key,JS_TRUE);CHECK(reference);CHECK(js_GetSuperReference(cx,reference,&result)&&result==INT_TO_JSVAL(23));CHECK(js_SetSuperReference(cx,reference,INT_TO_JSVAL(29)));CHECK(JS_GetProperty(cx,JSVAL_TO_OBJECT(receiver),"marker",&result)&&result==INT_TO_JSVAL(29));
 CHECK(JS_SetPrototype(cx,home,NULL));reference=js_NewSuperReference(cx,method,receiver,key,JS_TRUE);CHECK(reference);CHECK(!js_GetSuperReference(cx,reference,&result));JS_ClearPendingException(cx);CHECK(!js_SetSuperReference(cx,reference,INT_TO_JSVAL(1)));JS_ClearPendingException(cx);
 CHECK(Eval(cx,global,"({get value(){'use strict';gcNow();return this}})",&result));CHECK(JS_SetPrototype(cx,home,JSVAL_TO_OBJECT(result)));
 reference=js_NewSuperReference(cx,method,INT_TO_JSVAL(7),key,JS_TRUE);CHECK(reference);CHECK(js_GetSuperReference(cx,reference,&result)&&result==INT_TO_JSVAL(7));
 printf("SUPER-REFERENCE PASS checks=%u\n",checks);status=0;
 out:JS_RemoveRoot(cx,&key);JS_RemoveRoot(cx,&receiver);JS_RemoveRoot(cx,&result);JS_RemoveRoot(cx,&reference);JS_RemoveRoot(cx,&home);JS_RemoveRoot(cx,&method);JS_RemoveRoot(cx,&global);JS_EndRequest(cx);JS_DestroyContext(cx);JS_DestroyRuntime(rt);JS_ShutDown();return status;
}
