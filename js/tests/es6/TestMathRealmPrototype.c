/* Eager Math initialization uses the target realm's Object prototype.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "jsapi.h"
#include <stdio.h>
#include <string.h>
static JSClass globalClass={"global",JSCLASS_GLOBAL_FLAGS,JS_PropertyStub,
 JS_PropertyStub,JS_PropertyStub,JS_PropertyStub,JS_EnumerateStub,JS_ResolveStub,
 JS_ConvertStub,JS_FinalizeStub,JSCLASS_NO_OPTIONAL_MEMBERS};
static unsigned checks;
#define CHECK(c) do{++checks;if(!(c)){fprintf(stderr,"FAIL Math realm check %u\n",checks);goto out;}}while(0)
int main(void)
{
 JSRuntime *rt=JS_NewRuntime(8*1024*1024);
 JSContext *cx;
 JSObject *global=NULL,*math=NULL,*proto=NULL,*other=NULL;
 jsval value=JSVAL_VOID;
 JSVersion versions[3]={JSVERSION_DEFAULT,JSVERSION_1_7,JSVERSION_ECMA_2015};
 unsigned i;
 int status=1;
 const char *program="Object.getPrototypeOf(Math)===Object.prototype";
 if(!rt)return 1;
 cx=JS_NewContext(rt,8192);if(!cx){JS_DestroyRuntime(rt);return 1;}
 JS_BeginRequest(cx);
 JS_AddNamedRoot(cx,&global,"Math global");JS_AddNamedRoot(cx,&math,"Math object");
 JS_AddNamedRoot(cx,&proto,"Math prototype");JS_AddNamedRoot(cx,&other,"other global");
 for(i=0;i<3;++i){
  JS_SetVersion(cx,versions[i]);
  global=JS_NewObject(cx,&globalClass,NULL,NULL);CHECK(global);
  CHECK(JS_SetParent(cx,global,NULL)&&JS_SetPrototype(cx,global,NULL));
  JS_SetGlobalObject(cx,global);CHECK(JS_InitStandardClasses(cx,global));
  CHECK(JS_EvaluateScript(cx,global,program,strlen(program),"Math prototype",1,&value)&&value==(i==1?JSVAL_FALSE:JSVAL_TRUE));
  CHECK(JS_GetProperty(cx,global,"Math",&value)&&JSVAL_IS_OBJECT(value));math=JSVAL_TO_OBJECT(value);
  proto=JS_GetPrototype(cx,math);CHECK(proto);
  JS_ClearNewbornRoots(cx);JS_GC(cx);CHECK(JS_GetPrototype(cx,math)==proto);
  CHECK(JS_EvaluateScript(cx,global,"Math.abs(-7)===7",strlen("Math.abs(-7)===7"),"Math call",1,&value)&&value==JSVAL_TRUE);
 }
 other=global;
 JS_SetGlobalObject(cx,NULL);global=JS_NewObject(cx,&globalClass,NULL,NULL);CHECK(global);
 JS_SetGlobalObject(cx,global);CHECK(JS_InitStandardClasses(cx,global));
 CHECK(JS_GetProperty(cx,global,"Math",&value)&&JSVAL_TO_OBJECT(value)!=math);
 CHECK(JS_GetPrototype(cx,JSVAL_TO_OBJECT(value))!=proto);
 CHECK(JS_EvaluateScript(cx,global,program,strlen(program),"independent realm",1,&value)&&value==JSVAL_TRUE);
 printf("ES6-MATH-REALM checks=%u failures=0\n",checks);status=0;
out:
 JS_RemoveRoot(cx,&other);JS_RemoveRoot(cx,&proto);JS_RemoveRoot(cx,&math);JS_RemoveRoot(cx,&global);
 JS_EndRequest(cx);JS_DestroyContext(cx);JS_DestroyRuntime(rt);JS_ShutDown();return status;
}
