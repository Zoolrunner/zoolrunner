/* MPL 1.1/GPL 2.0/LGPL 2.1. */
/* Native storage normalization must not replace an ES2015 assignment value. */
#include "jsapi.h"
#include <stdio.h>
#include <string.h>
static unsigned checks, writes;
static JSClass globalClass = {
    "global", 0, JS_PropertyStub, JS_PropertyStub,
    JS_PropertyStub, JS_PropertyStub, JS_EnumerateStub,
    JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};
static JSBool Normalize(JSContext *cx, JSObject *obj, jsval id, jsval *value)
{
    ++writes;
    JS_GC(cx);
    *value = INT_TO_JSVAL(5);
    return JS_TRUE;
}
static JSBool Eval(JSContext *cx, JSObject *global, const char *source)
{
    jsval value;
    return JS_EvaluateScript(cx, global, source, strlen(source),
                             "native-setter-result", 1, &value) && value == JSVAL_TRUE;
}
#define CHECK(x) do { ++checks; if (!(x)) {fprintf(stderr,"FAIL check %u\n",checks);goto out;} } while(0)
int main(void)
{
    JSRuntime *rt = JS_NewRuntime(8*1024*1024);
    JSContext *cx;
    JSObject *global, *host = NULL;
    jsval value;
    int status = 1;
    if (!rt) return 1;
    cx = JS_NewContext(rt,8192);
    if (!cx) {JS_DestroyRuntime(rt);return 1;}
    JS_BeginRequest(cx);
    JS_SetVersion(cx,JSVERSION_ECMA_2015);
    JS_AddNamedRoot(cx,&host,"setter host");
    global = JS_NewObject(cx,&globalClass,NULL,NULL);
    CHECK(global);JS_SetGlobalObject(cx,global);CHECK(JS_InitStandardClasses(cx,global));
    host = JS_NewObject(cx,&globalClass,NULL,global);CHECK(host);
    CHECK(JS_DefineProperty(cx,global,"host",OBJECT_TO_JSVAL(host),NULL,NULL,0));
    CHECK(JS_DefineProperty(cx,host,"value",INT_TO_JSVAL(0),NULL,Normalize,JSPROP_ENUMERATE));
    CHECK(JS_DefineProperty(cx,global,"nativeValue",INT_TO_JSVAL(0),NULL,Normalize,JSPROP_ENUMERATE));
    CHECK(Eval(cx,global,"(host.value=17)===17 && host.value===5"));
    CHECK(Eval(cx,global,"(host['value']=18)===18 && host.value===5"));
    CHECK(Eval(cx,global,"var token={};(host.value=token)===token && host.value===5"));
    CHECK(Eval(cx,global,"var result;with(host){result=(value=19);}result===19 && host.value===5"));
    CHECK(Eval(cx,global,"(nativeValue=20)===20 && nativeValue===5"));
    CHECK(Eval(cx,global,"(host.value+=2)===7 && host.value===5"));
    CHECK(Eval(cx,global,"host.value++===5 && host.value===5"));
    CHECK(Eval(cx,global,"++host.value===6 && host.value===5"));
    CHECK(Eval(cx,global,"++host['value']===6 && host.value===5"));
    value = INT_TO_JSVAL(23);CHECK(JS_SetProperty(cx,host,"value",&value)&&value==INT_TO_JSVAL(5));
    JS_SetVersion(cx,JSVERSION_1_7);
    CHECK(Eval(cx,global,"(host.value=17)===5 && host.value===5"));
    CHECK(writes>=11);
    printf("NATIVE-SETTER-RESULT checks=%u failures=0\n",checks);status=0;
out:
    JS_RemoveRoot(cx,&host);JS_EndRequest(cx);JS_DestroyContext(cx);JS_DestroyRuntime(rt);JS_ShutDown();return status;
}
