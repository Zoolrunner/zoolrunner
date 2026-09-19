/* Wide atom operands and global lexical metadata/source round trips.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "jsapi.h"
#include "jsxdrapi.h"
#include "jsscript.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks;
static JSClass globalClass={"global",JSCLASS_GLOBAL_FLAGS,JS_PropertyStub,JS_PropertyStub,JS_PropertyStub,JS_PropertyStub,JS_EnumerateStub,JS_ResolveStub,JS_ConvertStub,JS_FinalizeStub,JSCLASS_NO_OPTIONAL_MEMBERS};
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"LEXICAL-WIDE FAIL %u\n",checks);goto out;}}while(0)
int main(void){
 JSRuntime *rt=JS_NewRuntime(64*1024*1024);JSContext *cx;
 JSObject *global=NULL,*other=NULL,*scriptObject=NULL,*decodedObject=NULL;
 JSScript *script=NULL,*decoded=NULL;JSString *source=NULL;jsval result=JSVAL_VOID;
 JSXDRState *encoder=NULL,*decoder=NULL;char *program=NULL;void *data;uint32 length;size_t pos=0;unsigned i;int status=1;
 if(!rt)return 1;cx=JS_NewContext(rt,8192);if(!cx){JS_DestroyRuntime(rt);return 1;}
 JS_BeginRequest(cx);JS_SetVersion(cx,JSVERSION_ECMA_2015);
 JS_AddNamedRoot(cx,&global,"global");JS_AddNamedRoot(cx,&other,"other");JS_AddNamedRoot(cx,&result,"result");JS_AddNamedRoot(cx,&scriptObject,"script");JS_AddNamedRoot(cx,&decodedObject,"decoded");JS_AddNamedRoot(cx,&source,"source");
 global=JS_NewObject(cx,&globalClass,NULL,NULL);CHECK(global);JS_SetGlobalObject(cx,global);CHECK(JS_InitStandardClasses(cx,global));
 program=(char*)malloc(1600000);CHECK(program);for(i=0;i<66000;i++)pos+=(size_t)sprintf(program+pos,"'unique%u';\n",i);
 strcpy(program+pos,"let wideLexical=13;const [widePattern=14]=[];wideLexical+widePattern===27");
 script=JS_CompileScript(cx,global,program,strlen(program),"wide-lexical",1);CHECK(script&&(scriptObject=JS_NewScriptObject(cx,script))!=NULL);
 CHECK(script->atomMap.length>65535 && script->globalLexicalIndex>65535);
 source=JS_DecompileScript(cx,script,"wide-lexical",0);CHECK(source);
 encoder=JS_XDRNewMem(cx,JSXDR_ENCODE);decoder=JS_XDRNewMem(cx,JSXDR_DECODE);CHECK(encoder&&decoder&&JS_XDRScript(encoder,&script));data=JS_XDRMemGetData(encoder,&length);JS_XDRMemSetData(decoder,data,length);
 CHECK(JS_XDRScript(decoder,&decoded));CHECK((decodedObject=JS_NewScriptObject(cx,decoded))!=NULL);JS_GC(cx);
 CHECK(JS_ExecuteScript(cx,global,decoded,&result)&&result==JSVAL_TRUE);
 other=JS_NewObject(cx,&globalClass,NULL,NULL);CHECK(other);JS_SetParent(cx,other,NULL);JS_SetPrototype(cx,other,NULL);JS_SetGlobalObject(cx,other);CHECK(JS_InitStandardClasses(cx,other));
 CHECK(JS_EvaluateScript(cx,other,JS_GetStringBytes(source),JS_GetStringLength(source),"wide-roundtrip",1,&result)&&result==JSVAL_TRUE);
 printf("GLOBAL-LEXICAL-WIDE PASS checks=%u\n",checks);status=0;
 out:free(program);if(decoder){JS_XDRMemSetData(decoder,NULL,0);JS_XDRDestroy(decoder);}if(encoder)JS_XDRDestroy(encoder);
 JS_RemoveRoot(cx,&source);JS_RemoveRoot(cx,&decodedObject);JS_RemoveRoot(cx,&scriptObject);JS_RemoveRoot(cx,&result);JS_RemoveRoot(cx,&other);JS_RemoveRoot(cx,&global);JS_EndRequest(cx);JS_DestroyContext(cx);JS_DestroyRuntime(rt);JS_ShutDown();return status;
}
