/* Private lexical record lifetime, isolation and callback regressions.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "jsapi.h"
#include "../../src/jsrealm.h"
#include <stdio.h>
#include <string.h>
static JSClass globalClass={"global",0,JS_PropertyStub,JS_PropertyStub,JS_PropertyStub,JS_PropertyStub,JS_EnumerateStub,JS_ResolveStub,JS_ConvertStub,JS_FinalizeStub,JSCLASS_NO_OPTIONAL_MEMBERS};
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"LEXICAL-STORE FAIL %u\n",checks);goto out;}}while(0)
static JSBool Eval(JSContext *cx,JSObject *scope,const char *code,jsval *value)
{return JS_EvaluateScript(cx,scope,code,strlen(code),"lexical-store",1,value);}
int main(void){
 JSRuntime *rt=JS_NewRuntime(4*1024*1024);JSContext *cx;
 JSObject *global=NULL,*env=NULL,*second=NULL,*other=NULL;
 jsval value=JSVAL_VOID,reader=JSVAL_VOID;jsid id;JSBool found;int status=1;
 if(!rt)return 1;cx=JS_NewContext(rt,8192);if(!cx){JS_DestroyRuntime(rt);return 1;}
 JS_BeginRequest(cx);JS_SetVersion(cx,JSVERSION_ECMA_2015);
 JS_AddNamedRoot(cx,&global,"global");JS_AddNamedRoot(cx,&env,"env");JS_AddNamedRoot(cx,&second,"second");JS_AddNamedRoot(cx,&other,"other");JS_AddNamedRoot(cx,&value,"value");JS_AddNamedRoot(cx,&reader,"reader");
 global=JS_NewObject(cx,&globalClass,NULL,NULL);CHECK(global);JS_SetGlobalObject(cx,global);CHECK(JS_InitStandardClasses(cx,global));
 CHECK(Eval(cx,global,"Object.defineProperty(this,'Global Lexical Environment',{get:function(){throw Error('private lookup')},configurable:true});Object.preventExtensions(this);true",&value)&&value==JSVAL_TRUE);
 env=js_GlobalLexicalEnvironment(cx,global,JS_TRUE);CHECK(env && JS_GetParent(cx,env)==global && JS_GetPrototype(cx,env)==NULL);
 JS_GC(cx);CHECK(js_GlobalLexicalEnvironment(cx,global,JS_FALSE)==env);
 CHECK(Eval(cx,env,"(function(){return future})",&reader));
 CHECK(JS_ValueToId(cx,STRING_TO_JSVAL(JS_InternString(cx,"future")),&id));
 CHECK(js_DefineLexicalBinding(cx,env,id,JS_FALSE));
 CHECK(Eval(cx,env,"try{future;false}catch(e){e instanceof ReferenceError}",&value)&&value==JSVAL_TRUE);
 CHECK(Eval(cx,env,"try{typeof future;false}catch(e){e instanceof ReferenceError}",&value)&&value==JSVAL_TRUE);
 CHECK(Eval(cx,env,"try{future=1;false}catch(e){e instanceof ReferenceError}",&value)&&value==JSVAL_TRUE);
 CHECK(js_InitializeLexicalBinding(cx,env,id,INT_TO_JSVAL(7)));
 JS_GC(cx);CHECK(JS_CallFunctionValue(cx,global,reader,0,NULL,&value)&&value==INT_TO_JSVAL(7));
 CHECK(Eval(cx,env,"future=9;future",&value)&&value==INT_TO_JSVAL(9));
 CHECK(JS_HasProperty(cx,global,"future",&found)&&!found);
 CHECK(!js_DefineLexicalBinding(cx,env,id,JS_FALSE));JS_ClearPendingException(cx);
 CHECK(JS_ValueToId(cx,STRING_TO_JSVAL(JS_InternString(cx,"constant")),&id));
 CHECK(js_DefineLexicalBinding(cx,env,id,JS_TRUE));
 CHECK(Eval(cx,env,"try{constant=1;false}catch(e){e instanceof ReferenceError}",&value)&&value==JSVAL_TRUE);
 CHECK(js_InitializeLexicalBinding(cx,env,id,INT_TO_JSVAL(3)));
 CHECK(Eval(cx,env,"try{constant=4;false}catch(e){e instanceof TypeError}",&value)&&value==JSVAL_TRUE);
 CHECK(Eval(cx,env,"constant",&value)&&value==INT_TO_JSVAL(3));
 CHECK(!js_InitializeLexicalBinding(cx,env,id,INT_TO_JSVAL(4)));JS_ClearPendingException(cx);
 CHECK(Eval(cx,env,"(function(){'use strict';return this===undefined})",&value));
 CHECK(JS_DefineProperty(cx,env,"fn",value,NULL,NULL,0));
 CHECK(Eval(cx,env,"fn()",&value)&&value==JSVAL_TRUE);
 second=JS_NewObject(cx,&globalClass,NULL,NULL);CHECK(second);JS_SetParent(cx,second,NULL);JS_SetPrototype(cx,second,NULL);
 other=js_GlobalLexicalEnvironment(cx,second,JS_TRUE);CHECK(other&&other!=env);
 JS_GC(cx);CHECK(JS_HasProperty(cx,other,"future",&found)&&!found);
 reader=JSVAL_VOID;value=JSVAL_VOID;env=NULL;JS_GC(cx);
 env=js_GlobalLexicalEnvironment(cx,global,JS_FALSE);CHECK(env);
 CHECK(Eval(cx,env,"future===9 && constant===3",&value)&&value==JSVAL_TRUE);
 CHECK(JS_ValueToId(cx,STRING_TO_JSVAL(JS_InternString(cx,"reserved")),&id));
 CHECK(js_RecordGlobalVarBinding(cx,env,id));
 CHECK(!js_DefineLexicalBinding(cx,env,id,JS_FALSE));JS_ClearPendingException(cx);
 JS_GC(cx);CHECK(!js_CanDeclareGlobalLexicalBinding(cx,env,id));JS_ClearPendingException(cx);
 CHECK(js_ForgetGlobalVarBinding(cx,env,id));
 CHECK(js_DefineLexicalBinding(cx,env,id,JS_FALSE));
 CHECK(JS_ValueToId(cx,STRING_TO_JSVAL(JS_InternString(cx,"undefined")),&id));
 CHECK(!js_CanDeclareGlobalLexicalBinding(cx,env,id));JS_ClearPendingException(cx);
 other=js_NewLexicalEnvironment(cx,env);CHECK(other && !js_IsGlobalLexicalEnvironment(cx,other));
 CHECK(js_DefineLexicalBinding(cx,other,id,JS_FALSE));
 CHECK(js_InitializeLexicalBinding(cx,other,id,INT_TO_JSVAL(3)));
 CHECK(Eval(cx,other,"undefined===3 && future===9",&value)&&value==JSVAL_TRUE);
 CHECK(JS_ValueToId(cx,STRING_TO_JSVAL(JS_InternString(cx,"future")),&id));
 CHECK(js_DefineLexicalBinding(cx,other,id,JS_FALSE));
 CHECK(js_InitializeLexicalBinding(cx,other,id,INT_TO_JSVAL(4)));
 CHECK(Eval(cx,other,"(function(){return future})",&reader));
 other=NULL;value=JSVAL_VOID;JS_GC(cx);
 CHECK(JS_CallFunctionValue(cx,global,reader,0,NULL,&value)&&value==INT_TO_JSVAL(4));
 CHECK(Eval(cx,env,"future===9 && undefined===void 0",&value)&&value==JSVAL_TRUE);
 printf("GLOBAL-LEXICAL-STORE PASS checks=%u\n",checks);status=0;
 out:JS_RemoveRoot(cx,&reader);JS_RemoveRoot(cx,&value);JS_RemoveRoot(cx,&other);JS_RemoveRoot(cx,&second);JS_RemoveRoot(cx,&env);JS_RemoveRoot(cx,&global);JS_EndRequest(cx);JS_DestroyContext(cx);JS_DestroyRuntime(rt);JS_ShutDown();return status;
}
