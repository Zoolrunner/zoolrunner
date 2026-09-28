/* Scripted setter assignment values, mixed editions and callback collection.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "jsapi.h"
#include <stdio.h>
#include <string.h>
static unsigned checks;
static JSClass globalClass={"global",JSCLASS_GLOBAL_FLAGS,JS_PropertyStub,JS_PropertyStub,JS_PropertyStub,JS_PropertyStub,JS_EnumerateStub,JS_ResolveStub,JS_ConvertStub,JS_FinalizeStub,JSCLASS_NO_OPTIONAL_MEMBERS};
static JSBool Collect(JSContext *cx,JSObject *obj,uintN argc,jsval *argv,jsval *rval){JS_GC(cx);*rval=JSVAL_VOID;return JS_TRUE;}
static JSBool Evaluate(JSContext *cx,JSObject *global,const char *code,jsval *result){return JS_EvaluateScript(cx,global,code,strlen(code),"setter-result",1,result);}
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"SETTER-RESULT-NATIVE FAIL %u\n",checks);goto out;}}while(0)
int main(void){
 JSRuntime *rt=JS_NewRuntime(8*1024*1024);JSContext *cx;JSObject *global=NULL;
 jsval result=JSVAL_VOID,value=JSVAL_VOID;int status=1;unsigned i,j;
 JSVersion versions[]={JSVERSION_DEFAULT,JSVERSION_ECMA_2015,JSVERSION_1_5};
 const char *setup="var seen;var target={set x(v){collect();seen=v;return 91}};";
 if(!rt)return 1;cx=JS_NewContext(rt,8192);if(!cx){JS_DestroyRuntime(rt);return 1;}
 JS_BeginRequest(cx);JS_AddNamedRoot(cx,&global,"global");JS_AddNamedRoot(cx,&result,"result");JS_AddNamedRoot(cx,&value,"value");
 JS_SetVersion(cx,JSVERSION_ECMA_2015);global=JS_NewObject(cx,&globalClass,NULL,NULL);CHECK(global);JS_SetGlobalObject(cx,global);CHECK(JS_InitStandardClasses(cx,global));CHECK(JS_DefineFunction(cx,global,"collect",Collect,0,0));
 for(i=0;i<3;i++){
  JS_SetVersion(cx,versions[i]);CHECK(Evaluate(cx,global,setup,&result));
  for(j=0;j<3;j++){
   JS_SetVersion(cx,versions[j]);CHECK(Evaluate(cx,global,"target.x={marker:41}",&result));JS_GC(cx);
   if(versions[j]==JSVERSION_1_5){CHECK(result==INT_TO_JSVAL(91));}
   else{CHECK(JSVAL_IS_OBJECT(result)&&!JSVAL_IS_NULL(result));CHECK(JS_GetProperty(cx,JSVAL_TO_OBJECT(result),"marker",&value)&&value==INT_TO_JSVAL(41));}
   CHECK(Evaluate(cx,global,"seen.marker===41",&value)&&value==JSVAL_TRUE);
   CHECK(Evaluate(cx,global,"'use strict';target.x={marker:42}",&result));CHECK(JSVAL_IS_OBJECT(result)&&!JSVAL_IS_NULL(result));CHECK(JS_GetProperty(cx,JSVAL_TO_OBJECT(result),"marker",&value)&&value==INT_TO_JSVAL(42));
  }
 }
 printf("SETTER-RESULT-NATIVE PASS checks=%u\n",checks);status=0;
 out:JS_RemoveRoot(cx,&value);JS_RemoveRoot(cx,&result);JS_RemoveRoot(cx,&global);JS_EndRequest(cx);JS_DestroyContext(cx);JS_DestroyRuntime(rt);JS_ShutDown();return status;
}
