/* Sticky native flags, constructor realms/cloning and XDR.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "jsapi.h"
#include "jsxdrapi.h"
#include <stdio.h>
#include <string.h>
static unsigned checks, finalized;
static void Finalize(JSContext *cx, JSObject *obj) { ++finalized; }
static JSClass globalClass = {
    "global", 0,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, Finalize,
    JSCLASS_NO_OPTIONAL_MEMBERS
};
static JSBool Evaluate(JSContext *cx, JSObject *global, const char *source)
{
    jsval result;
    return JS_EvaluateScript(cx,global,source,strlen(source),"regexp-constructor",1,&result) && result==JSVAL_TRUE;
}
#define CHECK(c) do { ++checks; if (!(c)) { fprintf(stderr,"FAIL RegExp constructor embedding check %u\n",checks);goto out; } } while (0)
int main(void)
{
    JSRuntime *rt=JS_NewRuntime(16*1024*1024);
    JSContext *cx,*other=NULL;
    JSObject *global,*foreign,*pattern,*clone;
    jsval value,args[2],result;
    JSScript *script=NULL,*decoded=NULL;
    JSXDRState *encoder=NULL,*decoder=NULL;
    void *data;
    uint32 length;
    int status=1;
    const char *program="var cached=/a\\/b/y;cached.lastIndex=1;cached.exec('xa/b')[0]==='a/b' && cached.lastIndex===4 && "
                        "cached.sticky && eval(cached.toString()).sticky";
    if (!rt) return 1;
    cx=JS_NewContext(rt,8192);
    if (!cx) { JS_DestroyRuntime(rt);return 1; }
    JS_BeginRequest(cx);JS_SetVersion(cx,JSVERSION_ECMA_2015);
    global=JS_NewObject(cx,&globalClass,NULL,NULL);CHECK(global);
    JS_SetGlobalObject(cx,global);CHECK(JS_InitStandardClasses(cx,global));
    CHECK(JSXDR_BYTECODE_VERSION==(0xb973c0de - 56));
    pattern=JS_NewRegExpObject(cx,"a",1,JSREG_STICKY);CHECK(pattern);
    CHECK(JS_DefineProperty(cx,global,"nativePattern",OBJECT_TO_JSVAL(pattern),NULL,NULL,0));
    CHECK(Evaluate(cx,global,"nativePattern.sticky && nativePattern.exec('ba')===null && "
                   "(nativePattern.lastIndex=1,nativePattern.exec('ba')[0]==='a' && nativePattern.lastIndex===2)"));
    args[0]=STRING_TO_JSVAL(JS_InternString(cx,"b"));args[1]=STRING_TO_JSVAL(JS_InternString(cx,"y"));
    pattern=JS_ConstructObjectWithArguments(cx,JS_GET_CLASS(cx,pattern),NULL,global,2,args);CHECK(pattern);
    CHECK(JS_DefineProperty(cx,global,"nativeConstructed",OBJECT_TO_JSVAL(pattern),NULL,NULL,0));
    CHECK(Evaluate(cx,global,"nativeConstructed.sticky && nativeConstructed.source==='b' && nativeConstructed.lastIndex===0"));
    other=JS_NewContext(rt,8192);CHECK(other);
    JS_BeginRequest(other);JS_SetVersion(other,JSVERSION_ECMA_2015);
    foreign=JS_NewObject(other,&globalClass,NULL,NULL);CHECK(foreign);
    JS_SetGlobalObject(other,foreign);CHECK(JS_InitStandardClasses(other,foreign));
    CHECK(Evaluate(other,foreign,"function Other(){};Other.prototype=3;var exports={ctor:RegExp,proto:RegExp.prototype,"
                   "other:Other,bound:Other.bind(null),proxy:new Proxy(Other,{})};true"));
    CHECK(JS_GetProperty(other,foreign,"exports",&value));
    CHECK(JS_DefineProperty(cx,global,"foreign",value,NULL,NULL,0));
    JS_EndRequest(other);JS_DestroyContextNoGC(other);other=NULL;
    JS_ClearNewbornRoots(cx);JS_GC(cx);CHECK(finalized==0);
    CHECK(Evaluate(cx,global,"Object.getPrototypeOf(foreign.ctor('x','y'))===foreign.proto && "
                   "Object.getPrototypeOf(new foreign.ctor('x','y'))===foreign.proto"));
    CHECK(Evaluate(cx,global,"Object.getPrototypeOf(Reflect.construct(RegExp,['x','y'],foreign.other))===foreign.proto"));
    CHECK(Evaluate(cx,global,"Object.getPrototypeOf(Reflect.construct(RegExp,['x','y'],foreign.bound))===foreign.proto"));
    CHECK(Evaluate(cx,global,"Object.getPrototypeOf(Reflect.construct(RegExp,['x','y'],foreign.proxy))===foreign.proto"));
    CHECK(JS_GetProperty(cx,JSVAL_TO_OBJECT(value),"ctor",&value));
    clone=JS_CloneFunctionObject(cx,JSVAL_TO_OBJECT(value),global);CHECK(clone);
    CHECK(JS_DefineProperty(cx,global,"clonedCtor",OBJECT_TO_JSVAL(clone),NULL,NULL,0));
    /* Classic JSAPI function clones create their own ordinary prototype. */
    CHECK(Evaluate(cx,global,"var cloned=new clonedCtor('x','y');Object.getPrototypeOf(cloned)===clonedCtor.prototype && "
                   "Object.getOwnPropertyDescriptor(foreign.proto,'source').get.call(cloned)==='x' && "
                   "Object.getOwnPropertyDescriptor(foreign.proto,'sticky').get.call(cloned) && cloned.lastIndex===0"));
    script=JS_CompileScript(cx,global,program,strlen(program),"sticky-xdr",1);CHECK(script);
    encoder=JS_XDRNewMem(cx,JSXDR_ENCODE);decoder=JS_XDRNewMem(cx,JSXDR_DECODE);
    CHECK(encoder&&decoder&&JS_XDRScript(encoder,&script));
    data=JS_XDRMemGetData(encoder,&length);JS_XDRMemSetData(decoder,data,length);
    JS_SetVersion(cx,JSVERSION_1_7);
    CHECK(JS_XDRScript(decoder,&decoded));
    CHECK(JS_ExecuteScript(cx,global,decoded,&result)&&result==JSVAL_TRUE);
    CHECK(JS_GetVersion(cx)==JSVERSION_1_7);
    CHECK(Evaluate(cx,global,"var r=foreign.ctor('a','y');r.sticky && r.toString()==='/a/y'"));
    JS_ClearScope(cx,global);JS_ClearNewbornRoots(cx);JS_GC(cx);CHECK(finalized==1);
    CHECK(JS_InitStandardClasses(cx,global));
    CHECK(Evaluate(cx,global,"var rejected=false;try{new RegExp('a','y');}catch(e){rejected=e.name==='SyntaxError';}"
                   "rejected && RegExp.prototype.source==='' && typeof RegExp.prototype.sticky==='undefined'"));
    pattern=JS_NewRegExpObject(cx,"a",1,JSREG_STICKY);CHECK(pattern);
    CHECK(JS_DefineProperty(cx,global,"legacyNative",OBJECT_TO_JSVAL(pattern),NULL,NULL,0));
    CHECK(Evaluate(cx,global,"legacyNative.exec('ba')===null && (legacyNative.lastIndex=1,legacyNative.exec('ba')[0]==='a') && legacyNative.toString()==='/a/y'"));
    printf("ES6-REGEXP-CONSTRUCTOR-EMBEDDING checks=%u failures=0\n",checks);status=0;
  out:
    if (decoder) {JS_XDRMemSetData(decoder,NULL,0);JS_XDRDestroy(decoder);}
    if (encoder) JS_XDRDestroy(encoder);
    if (decoded) JS_DestroyScript(cx,decoded);
    if (script) JS_DestroyScript(cx,script);
    if (other) {JS_EndRequest(other);JS_DestroyContextNoGC(other);}
    JS_EndRequest(cx);JS_DestroyContext(cx);JS_DestroyRuntime(rt);JS_ShutDown();return status;
}
