/* Per-instance method home objects, collection, cloning and cache templates.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "jsapi.h"
#include "jsfun.h"
#include "jsxdrapi.h"
#include <stdio.h>
#include <string.h>
static unsigned checks;
static JSClass globalClass={"global",JSCLASS_GLOBAL_FLAGS,JS_PropertyStub,JS_PropertyStub,JS_PropertyStub,JS_PropertyStub,JS_EnumerateStub,JS_ResolveStub,JS_ConvertStub,JS_FinalizeStub,JSCLASS_NO_OPTIONAL_MEMBERS};
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"METHOD-HOME FAIL %u\n",checks);goto out;}}while(0)
int main(void){
 JSRuntime *rt=JS_NewRuntime(8*1024*1024);JSContext *cx;
 JSObject *global=NULL,*secondHome=NULL,*home=NULL,*method=NULL,*clone=NULL,*scriptObject=NULL,*decodedObject=NULL;
 JSScript *script=NULL,*decoded=NULL;JSXDRState *encoder=NULL,*decoder=NULL;
 jsval result=JSVAL_VOID,objectValue=JSVAL_VOID,methodValue=JSVAL_VOID;void *data;uint32 length;unsigned i;int status=1;
 const char *cases[]={
 "(function(){var o={marker:37,m(){return /x/.test('x')}};return {o:o,m:o.m}})()",
 "(function(){var o={marker:37,['m'](){return /x/.test('x')}};return {o:o,m:o.m}})()",
 "(function(){var o={marker:37,get m(){return /x/.test('x')}};return {o:o,m:Object.getOwnPropertyDescriptor(o,'m').get}})()",
 "(function(){var o={marker:37,get ['m'](){return /x/.test('x')}};return {o:o,m:Object.getOwnPropertyDescriptor(o,'m').get}})()",
 "(function(){var o={marker:37,set m(v){return /x/.test('x')}};return {o:o,m:Object.getOwnPropertyDescriptor(o,'m').set}})()",
 "(function(){var o={marker:37,*m(){yield /x/.test('x')}};return {o:o,m:o.m}})()"};
 const char *program="(function(){var o={marker:37,m(){return /x/.test('x')}};return {o:o,m:o.m}})()";
 if(!rt)return 1;cx=JS_NewContext(rt,8192);if(!cx){JS_DestroyRuntime(rt);return 1;}JS_BeginRequest(cx);JS_SetVersion(cx,JSVERSION_ECMA_2015);
 JS_AddNamedRoot(cx,&global,"global");JS_AddNamedRoot(cx,&home,"home");JS_AddNamedRoot(cx,&secondHome,"second home");JS_AddNamedRoot(cx,&method,"method");JS_AddNamedRoot(cx,&clone,"clone");JS_AddNamedRoot(cx,&result,"result");JS_AddNamedRoot(cx,&objectValue,"object");JS_AddNamedRoot(cx,&methodValue,"methodValue");JS_AddNamedRoot(cx,&scriptObject,"script");JS_AddNamedRoot(cx,&decodedObject,"decoded");
 global=JS_NewObject(cx,&globalClass,NULL,NULL);CHECK(global);JS_SetGlobalObject(cx,global);CHECK(JS_InitStandardClasses(cx,global));
 for(i=0;i<sizeof(cases)/sizeof(cases[0]);i++){
  CHECK(JS_EvaluateScript(cx,global,cases[i],strlen(cases[i]),"method-home",1,&result));
  CHECK(JS_GetProperty(cx,JSVAL_TO_OBJECT(result),"o",&objectValue)&&JS_GetProperty(cx,JSVAL_TO_OBJECT(result),"m",&methodValue));method=JSVAL_TO_OBJECT(methodValue);
  CHECK(js_GetFunctionHomeObject(cx,method,&home)&&home==JSVAL_TO_OBJECT(objectValue));
  CHECK(JS_GetProperty(cx,home,"marker",&result)&&result==INT_TO_JSVAL(37));
  clone=JS_CloneFunctionObject(cx,method,global);CHECK(clone);CHECK(js_GetFunctionHomeObject(cx,clone,&home)&&home==JSVAL_TO_OBJECT(objectValue));
  result=objectValue=methodValue=JSVAL_VOID;home=NULL;method=NULL;JS_GC(cx);
  CHECK(js_GetFunctionHomeObject(cx,clone,&home)&&home);CHECK(JS_GetProperty(cx,home,"marker",&result)&&result==INT_TO_JSVAL(37));
  if(i<5)CHECK(JS_CallFunctionValue(cx,global,OBJECT_TO_JSVAL(clone),0,NULL,&result)&&result==JSVAL_TRUE);
  home=NULL;clone=NULL;JS_GC(cx);
 }
 script=JS_CompileScript(cx,global,program,strlen(program),"method-home-cache",1);CHECK(script&&(scriptObject=JS_NewScriptObject(cx,script))!=NULL);
 encoder=JS_XDRNewMem(cx,JSXDR_ENCODE);decoder=JS_XDRNewMem(cx,JSXDR_DECODE);CHECK(encoder&&decoder&&JS_XDRScript(encoder,&script));data=JS_XDRMemGetData(encoder,&length);JS_XDRMemSetData(decoder,data,length);CHECK(JS_XDRScript(decoder,&decoded));CHECK((decodedObject=JS_NewScriptObject(cx,decoded))!=NULL);
 CHECK(JS_ExecuteScript(cx,global,decoded,&result));CHECK(JS_GetProperty(cx,JSVAL_TO_OBJECT(result),"m",&methodValue));method=JSVAL_TO_OBJECT(methodValue);CHECK(js_GetFunctionHomeObject(cx,method,&home)&&home);
 CHECK(JS_ExecuteScript(cx,global,decoded,&result));CHECK(JS_GetProperty(cx,JSVAL_TO_OBJECT(result),"m",&methodValue));clone=JSVAL_TO_OBJECT(methodValue);CHECK(clone!=method);CHECK(js_GetFunctionHomeObject(cx,clone,&secondHome)&&secondHome!=home);
 CHECK(JS_EvaluateScript(cx,global,"({changed:1})",strlen("({changed:1})"),"super-base",1,&result));
 CHECK(JS_SetPrototype(cx,home,JSVAL_TO_OBJECT(result)));
 CHECK(js_GetFunctionSuperBase(cx,method,&secondHome)&&secondHome==JSVAL_TO_OBJECT(result));
 CHECK(JS_SetPrototype(cx,home,NULL));CHECK(js_GetFunctionSuperBase(cx,method,&secondHome)&&secondHome==NULL);
 CHECK(JS_EvaluateScript(cx,global,"(function(){var p={marker:43};return new Proxy({}, {getPrototypeOf:function(){return p}})})()",strlen("(function(){var p={marker:43};return new Proxy({}, {getPrototypeOf:function(){return p}})})()"),"proxy-home",1,&result));
 CHECK(js_SetFunctionHomeObject(cx,method,JSVAL_TO_OBJECT(result)));result=JSVAL_VOID;home=NULL;JS_GC(cx);
 CHECK(js_GetFunctionSuperBase(cx,method,&secondHome)&&secondHome);CHECK(JS_GetProperty(cx,secondHome,"marker",&result)&&result==INT_TO_JSVAL(43));
 printf("METHOD-HOME PASS checks=%u\n",checks);status=0;
 out:if(decoder){JS_XDRMemSetData(decoder,NULL,0);JS_XDRDestroy(decoder);}if(encoder)JS_XDRDestroy(encoder);
 JS_RemoveRoot(cx,&decodedObject);JS_RemoveRoot(cx,&scriptObject);JS_RemoveRoot(cx,&methodValue);JS_RemoveRoot(cx,&objectValue);JS_RemoveRoot(cx,&result);JS_RemoveRoot(cx,&clone);JS_RemoveRoot(cx,&method);JS_RemoveRoot(cx,&secondHome);JS_RemoveRoot(cx,&home);JS_RemoveRoot(cx,&global);JS_EndRequest(cx);JS_DestroyContext(cx);JS_DestroyRuntime(rt);JS_ShutDown();return status;
}
