/* Preserve classic object environments while modern syntactic with filters.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "jsapi.h"
#include <stdio.h>
#include <string.h>
static unsigned checks;
static JSClass globalClass = {
    "global", 0,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};
static JSBool Evaluate(JSContext *cx, JSObject *scope, const char *source)
{
    jsval result;
    return JS_EvaluateScript(cx, scope, source, strlen(source),
                             "unscopables-embedding", 1, &result) && result == JSVAL_TRUE;
}
#define CHECK(c) do { ++checks; if (!(c)) { fprintf(stderr,"FAIL unscopables embedding %u\n",checks); goto out; } } while (0)
int main(void)
{
    JSRuntime *rt=JS_NewRuntime(8*1024*1024);
    JSContext *cx;
    JSObject *global, *scope;
    jsval value;
    int status=1;
    if (!rt) return 1;
    cx=JS_NewContext(rt,8192);
    if (!cx) {JS_DestroyRuntime(rt);return 1;}
    JS_BeginRequest(cx);
    JS_SetVersion(cx,JSVERSION_ECMA_2015);
    global=JS_NewObject(cx,&globalClass,NULL,NULL);
    CHECK(global);
    JS_SetGlobalObject(cx,global);
    CHECK(JS_InitStandardClasses(cx,global));
    CHECK(Evaluate(cx,global,
        "var x=1,env={x:2},calls=0;env[Symbol.unscopables]={x:true};"
        "var scope={x:3,method:function(){'use strict';return this;}};"
        "Object.defineProperty(scope,Symbol.unscopables,{get:function(){++calls;throw 'embedding getter';}});true"));
    CHECK(JS_GetProperty(cx,global,"scope",&value));
    scope=JSVAL_TO_OBJECT(value);
    CHECK(JS_SetParent(cx,scope,global));
    CHECK(Evaluate(cx,scope,"x===3 && method()===scope"));
    CHECK(Evaluate(cx,global,"calls===0"));
    CHECK(Evaluate(cx,global,"(function(){with(env){return x===1;}})()"));
    JS_GC(cx);
    CHECK(Evaluate(cx,global,"(function(){var values=9;with([]){return values===9;}})()"));
    JS_SetVersion(cx,JSVERSION_DEFAULT);
    CHECK(Evaluate(cx,global,"(function(){with(env){return x===2;}})()"));
    CHECK(Evaluate(cx,global,"(function(){var values=9;with([]){return values===Array.prototype.values;}})()"));
    CHECK(Evaluate(cx,scope,"x===3 && calls===0"));
    JS_SetVersion(cx,JSVERSION_1_7);
    CHECK(Evaluate(cx,global,"(function(){with(env){return x===2;}})()"));
    JS_SetVersion(cx,JSVERSION_ECMA_2015);
    CHECK(Evaluate(cx,global,"(function(){with(env){return x===1;}})()"));
    printf("ES6-UNSCOPABLES-EMBEDDING checks=%u failures=0\n",checks);
    status=0;
  out:
    JS_EndRequest(cx);
    JS_DestroyContext(cx);
    JS_DestroyRuntime(rt);
    JS_ShutDown();
    return status;
}
