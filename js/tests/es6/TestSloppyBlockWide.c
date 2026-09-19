/* Wide non-strict block functions, lexical capture, collection and cache/source round trips.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "jsapi.h"
#include "jsxdrapi.h"
#include "jsscript.h"
#include "jsfun.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void report(JSContext *cx,const char *message,JSErrorReport *error){fprintf(stderr,"%.200s line %u\n",message,error?error->lineno:0);}
static unsigned checks;
static JSBool verifyWide(JSContext *cx,JSObject *obj,uintN argc,jsval *argv,jsval *rval){JSFunction *fun=argc?JS_ValueToFunction(cx,argv[0]):NULL;*rval=BOOLEAN_TO_JSVAL(fun&&FUN_INTERPRETED(fun)&&fun->u.i.script->atomMap.length>65535);return JS_TRUE;}
static JSBool collect(JSContext *cx,JSObject *obj,uintN argc,jsval *argv,jsval *rval){JS_GC(cx);*rval=JSVAL_VOID;return JS_TRUE;}
static JSClass globalClass={"global",JSCLASS_GLOBAL_FLAGS,JS_PropertyStub,JS_PropertyStub,JS_PropertyStub,JS_PropertyStub,JS_EnumerateStub,JS_ResolveStub,JS_ConvertStub,JS_FinalizeStub,JSCLASS_NO_OPTIONAL_MEMBERS};
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"SLOPPY-BLOCK-WIDE FAIL %u\n",checks);goto out;}}while(0)
int main(void){
 JSRuntime *rt=JS_NewRuntime(64*1024*1024);JSContext *cx;
 JSObject *global=NULL,*other=NULL,*scriptObject=NULL,*decodedObject=NULL;
 JSScript *script=NULL,*decoded=NULL;JSString *source=NULL;jsval result=JSVAL_VOID;
 JSXDRState *encoder=NULL,*decoder=NULL;char *program=NULL;void *data;uint32 length;size_t pos=0;unsigned i;int status=1;
 if(!rt)return 1;cx=JS_NewContext(rt,8192);if(!cx){JS_DestroyRuntime(rt);return 1;}
 JS_SetErrorReporter(cx,report);JS_BeginRequest(cx);JS_SetVersion(cx,JSVERSION_ECMA_2015);
 JS_AddNamedRoot(cx,&global,"global");JS_AddNamedRoot(cx,&other,"other");JS_AddNamedRoot(cx,&result,"result");JS_AddNamedRoot(cx,&scriptObject,"script");JS_AddNamedRoot(cx,&decodedObject,"decoded");JS_AddNamedRoot(cx,&source,"source");
 global=JS_NewObject(cx,&globalClass,NULL,NULL);CHECK(global);JS_SetGlobalObject(cx,global);CHECK(JS_InitStandardClasses(cx,global));
 CHECK(JS_DefineFunction(cx,global,"collect",collect,0,0));CHECK(JS_DefineFunction(cx,global,"verifyWide",verifyWide,1,0));
 program=(char*)malloc(1600000);CHECK(program);pos+=(size_t)sprintf(program+pos,"(function wide(){var sink;");for(i=0;i<66000;i++)pos+=(size_t)sprintf(program+pos,"sink='unique%u';\n",i);
 strcpy(program+pos,"var captured;{let value=7; captured=f; if(f()!==7)return false;function f(){collect();return value}}if(typeof f!=='function'||f!==captured)return false;var switched;switch(1){case 1:switched=g();break;default:function g(){return 8}}if(switched!==8||typeof g!=='undefined')return false;collect();return verifyWide(wide)&&captured()===7})()");
 script=JS_CompileScript(cx,global,program,strlen(program),"wide-block-functions",1);CHECK(script&&(scriptObject=JS_NewScriptObject(cx,script))!=NULL);
 
 source=JS_DecompileScript(cx,script,"wide-block-functions",0);CHECK(source);
 encoder=JS_XDRNewMem(cx,JSXDR_ENCODE);decoder=JS_XDRNewMem(cx,JSXDR_DECODE);CHECK(encoder&&decoder&&JS_XDRScript(encoder,&script));data=JS_XDRMemGetData(encoder,&length);JS_XDRMemSetData(decoder,data,length);
 CHECK(JS_XDRScript(decoder,&decoded));CHECK((decodedObject=JS_NewScriptObject(cx,decoded))!=NULL);JS_GC(cx);
 CHECK(JS_ExecuteScript(cx,global,decoded,&result)&&result==JSVAL_TRUE);
 other=JS_NewObject(cx,&globalClass,NULL,NULL);CHECK(other);JS_SetParent(cx,other,NULL);JS_SetPrototype(cx,other,NULL);JS_SetGlobalObject(cx,other);CHECK(JS_InitStandardClasses(cx,other));CHECK(JS_DefineFunction(cx,other,"collect",collect,0,0));CHECK(JS_DefineFunction(cx,other,"verifyWide",verifyWide,1,0));
 CHECK(JS_ExecuteScript(cx,other,decoded,&result)&&result==JSVAL_TRUE);
 CHECK(JS_EvaluateScript(cx,other,JS_GetStringBytes(source),JS_GetStringLength(source),"wide-roundtrip",1,&result)&&result==JSVAL_TRUE);
 printf("SLOPPY-BLOCK-WIDE PASS checks=%u\n",checks);status=0;
 out:free(program);if(decoder){JS_XDRMemSetData(decoder,NULL,0);JS_XDRDestroy(decoder);}if(encoder)JS_XDRDestroy(encoder);
 JS_RemoveRoot(cx,&source);JS_RemoveRoot(cx,&decodedObject);JS_RemoveRoot(cx,&scriptObject);JS_RemoveRoot(cx,&result);JS_RemoveRoot(cx,&other);JS_RemoveRoot(cx,&global);JS_EndRequest(cx);JS_DestroyContext(cx);JS_DestroyRuntime(rt);JS_ShutDown();return status;
}
