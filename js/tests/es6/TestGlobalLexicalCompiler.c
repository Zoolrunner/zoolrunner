/* Persistent global and fresh eval lexical bindings, including XDR and GC.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "jsapi.h"
#include "jsxdrapi.h"
#include <stdio.h>
#include <string.h>
static unsigned checks;
static JSClass globalClass={"global",JSCLASS_GLOBAL_FLAGS,JS_PropertyStub,JS_PropertyStub,JS_PropertyStub,JS_PropertyStub,JS_EnumerateStub,JS_ResolveStub,JS_ConvertStub,JS_FinalizeStub,JSCLASS_NO_OPTIONAL_MEMBERS};
static JSBool Collect(JSContext *cx,JSObject *obj,uintN argc,jsval *argv,jsval *rval){JS_GC(cx);*rval=JSVAL_VOID;return JS_TRUE;}
static void Report(JSContext *cx,const char *message,JSErrorReport *report){fprintf(stderr,"lexical: %s\n",message);}
static JSBool Eval(JSContext *cx,JSObject *obj,const char *code,jsval *result){return JS_EvaluateScript(cx,obj,code,strlen(code),"lexical-compiler",1,result);}
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"LEXICAL-COMPILER FAIL %u\n",checks);goto out;}}while(0)
#define TRUE(code) CHECK(Eval(cx,global,code,&result)&&result==JSVAL_TRUE)
int main(void){
 JSRuntime *rt=JS_NewRuntime(4*1024*1024);JSContext *cx;
 JSObject *global=NULL,*other=NULL,*third=NULL,*scriptObject=NULL,*decodedObject=NULL;
 JSScript *script=NULL,*decoded=NULL;JSString *source=NULL;
 JSXDRState *encoder=NULL,*decoder=NULL;jsval result=JSVAL_VOID;void *data;uint32 length;int status=1;
 const char *program="let [cachedA,cachedB=4]=[3];const {x:cachedC}={x:7};gcNow();cachedA+cachedB===cachedC";
 if(!rt)return 1;cx=JS_NewContext(rt,8192);if(!cx){JS_DestroyRuntime(rt);return 1;}
 JS_BeginRequest(cx);JS_SetErrorReporter(cx,Report);JS_SetVersion(cx,JSVERSION_ECMA_2015);
 JS_AddNamedRoot(cx,&global,"global");JS_AddNamedRoot(cx,&other,"other");JS_AddNamedRoot(cx,&third,"third");JS_AddNamedRoot(cx,&result,"result");
 JS_AddNamedRoot(cx,&scriptObject,"script");JS_AddNamedRoot(cx,&decodedObject,"decoded");JS_AddNamedRoot(cx,&source,"source");
 global=JS_NewObject(cx,&globalClass,NULL,NULL);CHECK(global);JS_SetGlobalObject(cx,global);CHECK(JS_InitStandardClasses(cx,global));
 CHECK(JS_DefineFunction(cx,global,"gcNow",Collect,0,0));
 TRUE("function earlyReader(){return laterLexical};true");
 TRUE("let laterLexical=31;const stableLexical=9;laterLexical===31");
 TRUE("gcNow();earlyReader()===31&&!Object.prototype.hasOwnProperty.call(this,'laterLexical')");
 TRUE("try{stableLexical=3;false}catch(e){e instanceof TypeError}");
 CHECK(!Eval(cx,global,"var laterLexical",&result));JS_ClearPendingException(cx);
 CHECK(!Eval(cx,global,"let laterLexical",&result));JS_ClearPendingException(cx);
 CHECK(!Eval(cx,global,"var neverCreated;let laterLexical=2",&result));JS_ClearPendingException(cx);
 TRUE("!Object.prototype.hasOwnProperty.call(this,'neverCreated')");
 CHECK(!Eval(cx,global,"let undefined=1",&result));JS_ClearPendingException(cx);
 CHECK(!Eval(cx,global,"let pending=(function(){throw 1})();let alsoPending=2",&result));JS_ClearPendingException(cx);
 TRUE("try{typeof alsoPending;false}catch(e){e instanceof ReferenceError}");
 TRUE("try{pending=1;false}catch(e){e instanceof ReferenceError}");
 TRUE("let lexicalFn=function(){'use strict';return this===undefined};lexicalFn()");
 TRUE("earlyReader.__parent__===null && lexicalFn.__parent__===null");
 TRUE("var globalVar=5;true");
 CHECK(!Eval(cx,global,"let globalVar=2",&result));JS_ClearPendingException(cx);
 TRUE("this.configurableName=6;true");
 TRUE("let configurableName=7;configurableName===7&&this.configurableName===6");
 TRUE("eval('let evalOnly=4');typeof evalOnly==='undefined'");
 TRUE("eval('let undefined=4;undefined')===4");
 TRUE("var captured=eval('let saved=8;(function(){return saved})');gcNow();captured()===8");
 TRUE("try{eval('var laterLexical');false}catch(e){e instanceof SyntaxError}");
 TRUE("(function(){var local=1;eval('let local=2');return local===1})()");
 TRUE("(function(){let local=1;try{eval('var local');return false}catch(e){return e instanceof SyntaxError}})()");
 script=JS_CompileScript(cx,global,program,strlen(program),"cache-lexical",1);CHECK(script&&(scriptObject=JS_NewScriptObject(cx,script))!=NULL);
 source=JS_DecompileScript(cx,script,"cache-lexical",0);CHECK(source);fprintf(stderr,"source: %s\n",JS_GetStringBytes(source));
 encoder=JS_XDRNewMem(cx,JSXDR_ENCODE);decoder=JS_XDRNewMem(cx,JSXDR_DECODE);CHECK(encoder&&decoder&&JS_XDRScript(encoder,&script));
 data=JS_XDRMemGetData(encoder,&length);JS_XDRMemSetData(decoder,data,length);CHECK(JS_XDRScript(decoder,&decoded));
 CHECK((decodedObject=JS_NewScriptObject(cx,decoded))!=NULL);JS_GC(cx);
 JS_SetVersion(cx,JSVERSION_1_7);
 CHECK(JS_ExecuteScript(cx,global,decoded,&result)&&result==JSVAL_TRUE);
 JS_SetVersion(cx,JSVERSION_ECMA_2015);
 other=JS_NewObject(cx,&globalClass,NULL,NULL);CHECK(other);JS_SetParent(cx,other,NULL);JS_SetPrototype(cx,other,NULL);JS_SetGlobalObject(cx,other);
 CHECK(JS_InitStandardClasses(cx,other));CHECK(JS_DefineFunction(cx,other,"gcNow",Collect,0,0));
 CHECK(JS_ExecuteScript(cx,other,decoded,&result)&&result==JSVAL_TRUE);
 CHECK(Eval(cx,other,JS_GetStringBytes(source),&result)==JS_FALSE);JS_ClearPendingException(cx);
 third=JS_NewObject(cx,&globalClass,NULL,NULL);CHECK(third);JS_SetParent(cx,third,NULL);JS_SetPrototype(cx,third,NULL);JS_SetGlobalObject(cx,third);
 CHECK(JS_InitStandardClasses(cx,third));CHECK(JS_DefineFunction(cx,third,"gcNow",Collect,0,0));
 CHECK(Eval(cx,third,JS_GetStringBytes(source),&result)&&result==JSVAL_TRUE);
 JS_SetGlobalObject(cx,global);TRUE("cachedA===3&&laterLexical===31");
 TRUE("(0,eval)('laterLexical')===31");
 TRUE("(0,eval)('var removable=1');delete removable");
 TRUE("let removable=2;removable===2");
 TRUE("(0,eval)('var retainedVarName=1');delete this.retainedVarName");
 CHECK(!Eval(cx,global,"let retainedVarName=2",&result));JS_ClearPendingException(cx);
 program="var partVar=1;let partLexical=2;partVar+partLexical===3";
 script=JS_CompileScript(cx,global,program,strlen(program),"split-lexical",1);CHECK(script&&(scriptObject=JS_NewScriptObject(cx,script))!=NULL);
 CHECK(JS_ExecuteScriptPart(cx,global,script,JSEXEC_PROLOG,&result));JS_GC(cx);
 TRUE("partVar===undefined");TRUE("try{typeof partLexical;false}catch(e){e instanceof ReferenceError}");
 CHECK(JS_ExecuteScriptPart(cx,global,script,JSEXEC_MAIN,&result)&&result==JSVAL_TRUE);
 TRUE("Object.preventExtensions(this);true");TRUE("let onFixedGlobal=43;onFixedGlobal===43");
 printf("GLOBAL-LEXICAL-COMPILER-NATIVE PASS checks=%u\n",checks);status=0;
 out:
 if(decoder){JS_XDRMemSetData(decoder,NULL,0);JS_XDRDestroy(decoder);}if(encoder)JS_XDRDestroy(encoder);
 JS_RemoveRoot(cx,&source);JS_RemoveRoot(cx,&decodedObject);JS_RemoveRoot(cx,&scriptObject);JS_RemoveRoot(cx,&result);JS_RemoveRoot(cx,&third);JS_RemoveRoot(cx,&other);JS_RemoveRoot(cx,&global);
 JS_EndRequest(cx);JS_DestroyContext(cx);JS_DestroyRuntime(rt);JS_ShutDown();return status;
}
