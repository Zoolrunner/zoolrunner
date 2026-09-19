/* Contextual grammar, legacy accessors and source/cache round trips.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "jsapi.h"
#include "jsxdrapi.h"
#include <stdio.h>
#include <string.h>
static unsigned checks;
static JSClass globalClass={"global",0,JS_PropertyStub,JS_PropertyStub,
 JS_PropertyStub,JS_PropertyStub,JS_EnumerateStub,JS_ResolveStub,JS_ConvertStub,
 JS_FinalizeStub,JSCLASS_NO_OPTIONAL_MEMBERS};
static JSBool ParseError(JSContext *cx,JSObject *global,const char *source)
{
    JSScript *script=JS_CompileScript(cx,global,source,strlen(source),"invalid-class",1);
    jsval error,name;
    JSBool ok=JS_FALSE;
    if(script){JS_DestroyScript(cx,script);return JS_FALSE;}
    if(JS_GetPendingException(cx,&error)&&!JSVAL_IS_PRIMITIVE(error)&&
       JS_GetProperty(cx,JSVAL_TO_OBJECT(error),"name",&name)&&JSVAL_IS_STRING(name))
        ok=!strcmp(JS_GetStringBytes(JSVAL_TO_STRING(name)),"SyntaxError");
    JS_ClearPendingException(cx);return ok;
}
#define CHECK(c) do{++checks;if(!(c)){fprintf(stderr,"FAIL contextual escape check %u\n",checks);goto out;}}while(0)
int main(void)
{
    JSRuntime *rt=JS_NewRuntime(8*1024*1024);
    JSContext *cx;
    JSObject *global=NULL,*scriptObject=NULL,*decodedObject=NULL;
    JSScript *script=NULL,*decoded=NULL;
    JSString *source=NULL;
    jsval result=JSVAL_VOID;
    JSXDRState *encoder=NULL,*decoder=NULL;
    void *data;
    uint32 length;
    unsigned i;
    int status=1;
    const char *program="class C {st\\u0061tic(){return 1;} static g\\u0065t(){return 2;} get g\\u0065tter(){return 3;}}var c=new C;c.static()===1&&C.get()===2&&c.getter===3";
    const char *legacy="var old={g\\u0065t x(){return 1;},s\\u0065t y(v){this.v=v;}};old.y=3;old.x===1&&old.v===3";
    const char *bad[]={"class C{st\\u0061tic f(){}}","class C{g\\u0065t x(){}}",
      "class C{s\\u0065t x(v){}}","class C{a b(){}}","class C{['a'] b(){}}",
      "class C{a *b(){}}","({get a b(){}})","({set a b(x){}})","({get a *b(){}})","({g\\u0065t x(){}})","({s\\u0065t x(v){}})"};
    if(!rt)return 1;
    cx=JS_NewContext(rt,8192);if(!cx){JS_DestroyRuntime(rt);return 1;}
    JS_BeginRequest(cx);
    JS_SetOptions(cx,JS_GetOptions(cx)|JSOPTION_DONT_REPORT_UNCAUGHT);
    JS_AddNamedRoot(cx,&global,"contextual global");JS_AddNamedRoot(cx,&scriptObject,"contextual script");
    JS_AddNamedRoot(cx,&decodedObject,"contextual decoded");JS_AddNamedRoot(cx,&source,"contextual source");
    JS_SetVersion(cx,JSVERSION_ECMA_2015);
    global=JS_NewObject(cx,&globalClass,NULL,NULL);CHECK(global);
    JS_SetGlobalObject(cx,global);CHECK(JS_InitStandardClasses(cx,global));
    for(i=0;i<sizeof(bad)/sizeof(bad[0]);++i)CHECK(ParseError(cx,global,bad[i]));
    script=JS_CompileScript(cx,global,program,strlen(program),"contextual",1);
    CHECK(script&&(scriptObject=JS_NewScriptObject(cx,script))!=NULL);
    CHECK(JS_ExecuteScript(cx,global,script,&result)&&result==JSVAL_TRUE);
    encoder=JS_XDRNewMem(cx,JSXDR_ENCODE);decoder=JS_XDRNewMem(cx,JSXDR_DECODE);
    CHECK(encoder&&decoder&&JS_XDRScript(encoder,&script));
    data=JS_XDRMemGetData(encoder,&length);JS_XDRMemSetData(decoder,data,length);
    JS_SetVersion(cx,JSVERSION_1_7);CHECK(JS_XDRScript(decoder,&decoded));
    CHECK((decodedObject=JS_NewScriptObject(cx,decoded))!=NULL);
    JS_ClearNewbornRoots(cx);JS_GC(cx);
    /* Clear the original lexical C so an independent execution can declare it. */
    JS_ClearScope(cx,global);JS_SetVersion(cx,JSVERSION_ECMA_2015);
    CHECK(JS_InitStandardClasses(cx,global));JS_SetVersion(cx,JSVERSION_1_7);
    CHECK(JS_ExecuteScript(cx,global,decoded,&result)&&result==JSVAL_TRUE);
    CHECK(JS_GetVersion(cx)==JSVERSION_1_7);
    CHECK(JS_EvaluateScript(cx,global,legacy,strlen(legacy),"legacy",1,&result)&&result==JSVAL_TRUE);
    source=JS_DecompileScript(cx,decoded,"contextual-source",0);CHECK(source);
    JS_ClearScope(cx,global);JS_SetVersion(cx,JSVERSION_ECMA_2015);CHECK(JS_InitStandardClasses(cx,global));
    CHECK(JS_EvaluateUCScript(cx,global,JS_GetStringChars(source),JS_GetStringLength(source),"roundtrip",1,&result)&&result==JSVAL_TRUE);
    printf("ES6-CONTEXTUAL-ESCAPES checks=%u failures=0\n",checks);status=0;
out:
    if(decoder){JS_XDRMemSetData(decoder,NULL,0);JS_XDRDestroy(decoder);}
    if(encoder)JS_XDRDestroy(encoder);
    JS_RemoveRoot(cx,&source);JS_RemoveRoot(cx,&decodedObject);JS_RemoveRoot(cx,&scriptObject);JS_RemoveRoot(cx,&global);
    JS_EndRequest(cx);JS_DestroyContext(cx);JS_DestroyRuntime(rt);JS_ShutDown();return status;
}
