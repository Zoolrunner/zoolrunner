/* Proxy realms, revoker closure cloning and classic embedding lifetimes.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "jsapi.h"
#include <stdio.h>
#include <string.h>
static unsigned checks, finalized;
static JSBool Collect(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JS_GC(cx); *rval = JSVAL_VOID; return JS_TRUE;
}
static void Finalize(JSContext *cx, JSObject *obj) { ++finalized; }
static JSClass globalClass = {
    "global", 0,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, Finalize,
    JSCLASS_NO_OPTIONAL_MEMBERS
};
static unsigned payloadsFinalized;
static void FinalizePayload(JSContext *cx, JSObject *obj) { ++payloadsFinalized; }
static JSClass payloadClass = {
    "payload", 0,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, FinalizePayload,
    JSCLASS_NO_OPTIONAL_MEMBERS
};
static JSBool NewPayload(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSObject *payload = JS_NewObject(cx, &payloadClass, NULL, NULL);
    if (!payload) return JS_FALSE;
    *rval = OBJECT_TO_JSVAL(payload);
    return JS_TRUE;
}
static JSBool Evaluate(JSContext *cx, JSObject *global, const char *source)
{
    jsval result;
    return JS_EvaluateScript(cx, global, source, strlen(source), "proxy-realms", 1, &result) &&
           result == JSVAL_TRUE;
}
#define CHECK(c) do { ++checks; if (!(c)) { \
    fprintf(stderr,"FAIL Proxy embedding check %u\n",checks); goto out; } } while (0)
int main(void)
{
    JSRuntime *rt = JS_NewRuntime(16 * 1024 * 1024);
    JSContext *cx, *other = NULL;
    JSObject *global, *second, *clone;
    unsigned lookup;
    jsval exported, oldProto = JSVAL_VOID, result;
    JSBool rooted = JS_FALSE;
    int status = 1;
    const char *prototypeSource = "Proxy";
    if (!rt) return 1;
    cx = JS_NewContext(rt, 8192);
    if (!cx) { JS_DestroyRuntime(rt); return 1; }
    JS_BeginRequest(cx);
    JS_SetVersion(cx, JSVERSION_ECMA_2015);
    global = JS_NewObject(cx, &globalClass, NULL, NULL);
    CHECK(global);
    JS_SetGlobalObject(cx, global);
    CHECK(JS_InitStandardClasses(cx, global));
    CHECK(Evaluate(cx, global, "Object.prototype.realmMarker=11;Array.prototype.realmMarker=12;true"));
    CHECK(JS_DefineFunction(cx, global, "collect", Collect, 0, 0));
    other = JS_NewContext(rt, 8192);
    CHECK(other);
    JS_BeginRequest(other);
    JS_SetVersion(other, JSVERSION_ECMA_2015);
    second = JS_NewObject(other, &globalClass, NULL, NULL);
    CHECK(second);
    JS_SetGlobalObject(other, second);
    CHECK(JS_InitStandardClasses(other, second));
    CHECK(JS_DefineFunction(other, second, "collect", Collect, 0, 0));
    CHECK(Evaluate(other, second,
        "Object.prototype.realmMarker=21;Array.prototype.realmMarker=22;"
        "var exported={Proxy:Proxy,revocable:Proxy.revocable,reflect:Reflect,"
        "objectProto:Object.prototype,arrayProto:Array.prototype,"
        "target:new Proxy({answer:7},{get:function(t,k,r){collect();return t[k];}})};"
        "Object=null;Array=null;Proxy=null;Reflect=null;true"));
    CHECK(JS_GetProperty(other, second, "exported", &exported));
    CHECK(JS_DefineProperty(cx, global, "foreign", exported, NULL, NULL, 0));
    JS_EndRequest(other);
    JS_DestroyContextNoGC(other); other = NULL;
    JS_ClearNewbornRoots(cx);
    JS_GC(cx);
    CHECK(finalized == 0);
    CHECK(Evaluate(cx, global, "foreign.target.answer===7"));
    CHECK(Evaluate(cx, global,
        "Object.getPrototypeOf(foreign.revocable({},{}))===foreign.objectProto"));
    CHECK(Evaluate(cx, global,
        "Object.getPrototypeOf(foreign.reflect.ownKeys(new Proxy({a:1},{})))===foreign.arrayProto"));
    CHECK(Evaluate(cx, global,
        "(function(){var p=new foreign.Proxy({}, {getOwnPropertyDescriptor:function(){collect();"
        "return {value:3,configurable:true};}});"
        "return Object.getOwnPropertyDescriptor(p,'a').realmMarker===11;})()"));
    CHECK(Evaluate(cx, global,
        "(function(){var p=new foreign.Proxy(function(){},{apply:function(t,r,a){collect();return a.realmMarker;}});"
        "return Reflect.apply(p,null,[])===12 && foreign.reflect.apply(p,null,[])===22;})()"));
    CHECK(Evaluate(cx, global,
        "(function(){var p=new foreign.Proxy(function(){},{construct:function(t,a,n){collect();return a;}});"
        "return Reflect.construct(p,[]).realmMarker===12 && foreign.reflect.construct(p,[]).realmMarker===22;})()"));
    CHECK(Evaluate(cx, global,
        "var pair=foreign.revocable({answer:9},{});pair.revoke.length===0"));
    CHECK(JS_GetProperty(cx, global, "pair", &result));
    CHECK(JS_GetProperty(cx, JSVAL_TO_OBJECT(result), "revoke", &exported));
    clone = JS_CloneFunctionObject(cx, JSVAL_TO_OBJECT(exported), global);
    CHECK(clone);
    CHECK(JS_DefineProperty(cx, global, "clonedRevoke", OBJECT_TO_JSVAL(clone), NULL, NULL, 0));
    CHECK(Evaluate(cx, global,
        "clonedRevoke.length===0 && (clonedRevoke(),pair.revoke(),true)"));
    CHECK(Evaluate(cx, global,
        "(function(){try{return pair.proxy.answer===9;}catch(e){return e instanceof TypeError;}})()"));
    JS_SetVersion(cx, JSVERSION_DEFAULT);
    CHECK(Evaluate(cx, global, "foreign.target.answer===7"));
    CHECK(Evaluate(cx, global,
        "foreign.reflect.ownKeys(new foreign.Proxy({a:1},{})).realmMarker===22"));
    JS_SetVersion(cx, JSVERSION_ECMA_2015);
    CHECK(Evaluate(cx, global, "var savedProxy=Proxy;delete this.Proxy;true"));
    CHECK(JS_EnumerateStandardClasses(cx, global));
    CHECK(Evaluate(cx, global, "!Object.prototype.hasOwnProperty.call(this,'Proxy') && (this.Proxy=savedProxy,true)"));
    CHECK(Evaluate(cx, global, "pair=null;clonedRevoke=null;true"));
    CHECK(JS_DeleteProperty(cx, global, "foreign"));
    JS_ClearNewbornRoots(cx);
    JS_GC(cx);
    CHECK(finalized == 1);
    CHECK(JS_EvaluateScript(cx, global, prototypeSource, strlen(prototypeSource),
                            "proxy-constructor", 1, &oldProto));
    CHECK(JS_AddNamedRoot(cx, &oldProto, "old Proxy constructor"));
    rooted = JS_TRUE;
    JS_ClearScope(cx, global);
    CHECK(JS_InitStandardClasses(cx, global));
    CHECK(JS_EvaluateScript(cx, global, prototypeSource, strlen(prototypeSource),
                            "new-proxy-constructor", 1, &result) && result != oldProto);
    CHECK(Evaluate(cx, global, "new Proxy({a:3},{}).a===3"));
    CHECK(JS_DefineFunction(cx, global, "payload", NewPayload, 0, 0));
    CHECK(JS_DefineFunction(cx, global, "collect", Collect, 0, 0));
    CHECK(Evaluate(cx, global,
        "var held=Proxy.revocable(payload(),payload());true"));
    JS_ClearNewbornRoots(cx); JS_GC(cx);
    CHECK(payloadsFinalized == 0);
    CHECK(Evaluate(cx, global, "held.revoke();true"));
    JS_ClearNewbornRoots(cx); JS_GC(cx);
    CHECK(payloadsFinalized == 2);
    CHECK(Evaluate(cx, global,
        "var held=Proxy.revocable(payload(),payload());"
        "var nested=new Proxy(held.proxy,{});collect();"
        "held.revoke();held=null;nested=null;true"));
    JS_ClearNewbornRoots(cx); JS_GC(cx);
    CHECK(payloadsFinalized == 4);
    CHECK(Evaluate(cx, global,
        "var handler=payload(), target=payload();target.a=19;"
        "Object.defineProperty(handler,'get',{get:function(){held.revoke();collect();"
        "return function(t,k){collect();return t[k];};}});"
        "held=Proxy.revocable(target,handler);target=null;handler=null;held.proxy.a===19"));
    JS_ClearNewbornRoots(cx); JS_GC(cx);
    CHECK(payloadsFinalized == 6);
    CHECK(JS_EvaluateScript(cx, global, "new Proxy([], {})", 17,
                            "proxy-array", 1, &result));
    CHECK(JS_IsArrayObject(cx, JSVAL_TO_OBJECT(result)));
    CHECK(Evaluate(cx, global,
        "var lookupTarget=payload();lookupTarget.a=27;"
        "var lookupProxy=new Proxy(lookupTarget,{});lookupTarget=null;true"));
    CHECK(JS_GetProperty(cx, global, "lookupProxy", &exported));
    for (lookup = 0; lookup < 1000; ++lookup) {
        if (!JS_LookupProperty(cx, JSVAL_TO_OBJECT(exported), "a", &result) ||
            result != JSVAL_TRUE) break; /* Classic non-native lookup sentinel. */
    }
    CHECK(lookup == 1000);
    CHECK(Evaluate(cx, global, "lookupProxy=null;true"));
    JS_ClearNewbornRoots(cx); JS_GC(cx);
    CHECK(payloadsFinalized == 7);
    printf("ES6-PROXY-EMBEDDING checks=%u failures=0\n",checks);
    status = 0;
  out:
    if (rooted) JS_RemoveRoot(cx, &oldProto);
    if (other) { JS_EndRequest(other); JS_DestroyContextNoGC(other); }
    JS_EndRequest(cx);
    JS_DestroyContext(cx);
    JS_DestroyRuntime(rt);
    JS_ShutDown();
    return status;
}
