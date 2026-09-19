/* Tail calls, embedding callbacks, collection and script serialization.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "jsapi.h"
#include "jsdbgapi.h"
#include "jsxdrapi.h"
#include <stdio.h>
#include <string.h>
static unsigned checks,branches,entries,exits,wrongFrames;
static JSBool abortBranches,installHook;
static void *call(JSContext*,JSStackFrame*,JSBool,JSBool*,void*);
static JSClass globalClass={"global",0,JS_PropertyStub,JS_PropertyStub,JS_PropertyStub,JS_PropertyStub,JS_EnumerateStub,JS_ResolveStub,JS_ConvertStub,JS_FinalizeStub,JSCLASS_NO_OPTIONAL_MEMBERS};
static JSBool branch(JSContext *cx,JSScript *script){++branches;if(installHook&&branches==1)JS_SetCallHook(JS_GetRuntime(cx),call,NULL);if(branches%2000==0)JS_GC(cx);return !abortBranches||branches<50;}
static void *call(JSContext *cx, JSStackFrame *fp, JSBool before,
                  JSBool *ok, void *closure)
{
    JSStackFrame *iterator = NULL;
    if (JS_FrameIterator(cx, &iterator) != fp) ++wrongFrames;
    if (before) {
        ++entries;
        JS_GC(cx);
    } else ++exits;
    return (void *)1;
}
static JSBool eval(JSContext *cx,JSObject *g,const char *s){jsval v;return JS_EvaluateScript(cx,g,s,strlen(s),"tail-call",1,&v)&&v==JSVAL_TRUE;}
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"TAIL-CALL-NATIVE FAIL %u\n",checks);goto out;}}while(0)
int main(void){
 JSRuntime *rt=JS_NewRuntime(16*1024*1024);JSContext *cx;JSObject *global,*owner=NULL,*decodedOwner=NULL;
 JSScript *script=NULL,*decoded=NULL;JSXDRState *encoder=NULL,*decoder=NULL;JSString *source=NULL;jsval result;void *bytes;uint32 length;int status=1;
 const char *program="function tail(n){'use strict';return n?tail(n-1):true;}tail(100000);";
 if(!rt)return 1;cx=JS_NewContext(rt,8192);if(!cx){JS_DestroyRuntime(rt);return 1;}
 JS_BeginRequest(cx);JS_SetVersion(cx,JSVERSION_ECMA_2015);JS_AddNamedRoot(cx,&owner,"tail script");JS_AddNamedRoot(cx,&decodedOwner,"decoded tail script");JS_AddNamedRoot(cx,&source,"tail source");
 global=JS_NewObject(cx,&globalClass,NULL,NULL);CHECK(global);JS_SetGlobalObject(cx,global);CHECK(JS_InitStandardClasses(cx,global));
 JS_SetBranchCallback(cx,branch);CHECK(eval(cx,global,program));CHECK(branches>=100000);
 script=JS_CompileScript(cx,global,program,strlen(program),"tail-cache",1);CHECK(script);owner=JS_NewScriptObject(cx,script);CHECK(owner);
 source=JS_DecompileScript(cx,script,"tail-source",0);CHECK(source);
 encoder=JS_XDRNewMem(cx,JSXDR_ENCODE);decoder=JS_XDRNewMem(cx,JSXDR_DECODE);CHECK(encoder&&decoder&&JS_XDRScript(encoder,&script));bytes=JS_XDRMemGetData(encoder,&length);JS_XDRMemSetData(decoder,bytes,length);
 CHECK(JS_XDRScript(decoder,&decoded));decodedOwner=JS_NewScriptObject(cx,decoded);CHECK(decodedOwner);JS_GC(cx);
 CHECK(JS_ExecuteScript(cx,global,decoded,&result)&&result==JSVAL_TRUE);
 CHECK(eval(cx,global,JS_GetStringBytes(source)));
 branches=0;abortBranches=JS_TRUE;CHECK(!eval(cx,global,"tail(100000)"));CHECK(branches==50);JS_ClearPendingException(cx);abortBranches=JS_FALSE;
 JS_SetCallHook(rt,call,NULL);CHECK(eval(cx,global,"tail(20)"));CHECK(entries==21&&exits==21);JS_SetCallHook(rt,NULL,NULL);
 entries=exits=branches=0;installHook=JS_TRUE;CHECK(eval(cx,global,"tail(20)"));CHECK(entries==20&&exits==20);installHook=JS_FALSE;JS_SetCallHook(rt,NULL,NULL);
 CHECK(eval(cx,global,"tail(100000)"));
 CHECK(wrongFrames==0);
 printf("TAIL-CALL-NATIVE checks=%u failures=0\n",checks);status=0;
 out:JS_SetBranchCallback(cx,NULL);JS_SetCallHook(rt,NULL,NULL);if(decoder){JS_XDRMemSetData(decoder,NULL,0);JS_XDRDestroy(decoder);}if(encoder)JS_XDRDestroy(encoder);
 JS_RemoveRoot(cx,&source);JS_RemoveRoot(cx,&decodedOwner);JS_RemoveRoot(cx,&owner);JS_EndRequest(cx);JS_DestroyContext(cx);JS_DestroyRuntime(rt);JS_ShutDown();return status;
}
