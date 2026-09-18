/* Reflect intrinsic realms, constructor fallback and classic globals.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "jsapi.h"
#include <stdio.h>
#include <string.h>
static unsigned checks, finalized, writes, reentries;
static JSBool reenter, denyAccess;
static JSBool Access(JSContext *cx, JSObject *obj, jsval id, JSAccessMode mode, jsval *vp)
{
    if (denyAccess) {
        JS_ReportError(cx, "Reflect host access denied");
        return JS_FALSE;
    }
    return JS_TRUE;
}
static JSBool Enumerate(JSContext *cx, JSObject *obj)
{
    jsval result;
    const char *source = "(function(){try{activeIterator.next();}catch(e){return e instanceof TypeError;}return false;})()";
    if (!reenter) return JS_TRUE;
    reenter = JS_FALSE;
    ++reentries;
    JS_GC(cx);
    return JS_EvaluateScript(cx, JS_GetGlobalObject(cx), source, strlen(source),
                             "enumerator-reentry", 1, &result) && result == JSVAL_TRUE;
}
static JSClass hostClass = {
    "ReflectHost", 0,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    Enumerate, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    NULL, Access, NULL, NULL, NULL, NULL, NULL, NULL
};
static JSBool Collect(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JS_GC(cx);
    *rval = JSVAL_VOID;
    return JS_TRUE;
}
static JSBool Write(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    ++writes;
    JS_GC(cx);
    return JS_TRUE;
}
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
    return JS_EvaluateScript(cx, global, source, strlen(source), "reflect-realms", 1, &result) &&
           result == JSVAL_TRUE;
}
#define CHECK(c) do { ++checks; if (!(c)) { \
    fprintf(stderr,"FAIL Reflect embedding check %u\n",checks); goto out; } } while (0)
int main(void)
{
    JSRuntime *rt = JS_NewRuntime(16 * 1024 * 1024);
    JSContext *cx, *other = NULL;
    JSObject *global, *second, *host;
    jsval exported, oldProto = JSVAL_VOID, result;
    JSBool rooted = JS_FALSE;
    int status = 1;
    const char *prototypeSource = "Object.getPrototypeOf(Reflect.enumerate({}))";
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
    host = JS_DefineObject(cx, global, "host", &hostClass, NULL, 0);
    CHECK(host);
    CHECK(JS_DefineProperty(cx, host, "value", INT_TO_JSVAL(1), NULL, Write, JSPROP_ENUMERATE));
    CHECK(Evaluate(cx, global, "Reflect.set(host,'value',7) && Reflect.get(host,'value')===7"));
    CHECK(writes == 1);
    CHECK(Evaluate(cx, global, "var activeIterator=Reflect.enumerate(host);true"));
    reenter = JS_TRUE;
    CHECK(Evaluate(cx, global, "activeIterator.next().value==='value' && activeIterator.next().value==='realmMarker' && activeIterator.next().done"));
    CHECK(reentries == 1);
    denyAccess = JS_TRUE;
    CHECK(Evaluate(cx, global,
        "(function(){try{Reflect.getOwnPropertyDescriptor(host,'value');}catch(e){"
        "return String(e).indexOf('Reflect host access denied')>=0;}return false;})()"));
    CHECK(Evaluate(cx, global,
        "(function(){try{Reflect.defineProperty(host,'value',{value:99});}catch(e){"
        "return String(e).indexOf('Reflect host access denied')>=0;}return false;})()"));
    CHECK(Evaluate(cx, global,
        "(function(){try{Reflect.set(host,'value',99);}catch(e){"
        "return String(e).indexOf('Reflect host access denied')>=0;}return false;})()"));
    denyAccess = JS_FALSE;
    CHECK(Evaluate(cx, global, "host.value===7"));
    CHECK(Evaluate(cx, global,
        "function Target(){collect();this.value=9;}"
        "function Alternate(){};Alternate.prototype={marker:17};"
        "var instance=Reflect.construct(Target,[],Alternate);"
        "instance.value===9 && Object.getPrototypeOf(instance)===Alternate.prototype"));
    other = JS_NewContext(rt, 8192);
    CHECK(other);
    JS_BeginRequest(other);
    JS_SetVersion(other, JSVERSION_ECMA_2015);
    second = JS_NewObject(other, &globalClass, NULL, NULL);
    CHECK(second);
    JS_SetGlobalObject(other, second);
    CHECK(JS_InitStandardClasses(other, second));
    CHECK(Evaluate(other, second,
        "Object.prototype.realmMarker=21;Array.prototype.realmMarker=22;"
        "function Foreign(){};Foreign.prototype=null;"
        "var exported={reflect:Reflect,ctor:Foreign,bound:Foreign.bind(null),"
        "objectProto:Object.prototype,arrayProto:Array.prototype,errorProto:TypeError.prototype,"
        "enumProto:Object.getPrototypeOf(Reflect.enumerate({}))};"
        "Object=null;Array=null;Reflect=null;TypeError=null;true"));
    CHECK(JS_GetProperty(other, second, "exported", &exported));
    CHECK(JS_DefineProperty(cx, global, "foreign", exported, NULL, NULL, 0));
    JS_EndRequest(other);
    JS_DestroyContextNoGC(other); other = NULL;
    JS_ClearNewbornRoots(cx);
    JS_GC(cx);
    CHECK(finalized == 0);
    CHECK(Evaluate(cx, global,
        "Object.getPrototypeOf(foreign.reflect.ownKeys({a:1}))===foreign.arrayProto"));
    CHECK(Evaluate(cx, global,
        "Object.getPrototypeOf(foreign.reflect.getOwnPropertyDescriptor({a:1},'a'))===foreign.objectProto"));
    CHECK(Evaluate(cx, global,
        "Object.getPrototypeOf(foreign.reflect.enumerate({}))===foreign.enumProto"));
    CHECK(Evaluate(cx, global,
        "foreign.reflect.enumerate({a:1}).next().realmMarker===21"));
    CHECK(Evaluate(cx, global,
        "Object.getPrototypeOf(Reflect.construct(Object,[],foreign.ctor))===foreign.objectProto"));
    CHECK(Evaluate(cx, global,
        "Object.getPrototypeOf(Reflect.construct(Array,[],foreign.ctor))===foreign.arrayProto"));
    CHECK(Evaluate(cx, global,
        "Object.getPrototypeOf(Reflect.construct(TypeError,[],foreign.ctor))===foreign.errorProto"));
    CHECK(Evaluate(cx, global,
        "Object.getPrototypeOf(Reflect.construct(Object,[],foreign.bound))===foreign.objectProto"));
    JS_SetVersion(cx, JSVERSION_DEFAULT);
    CHECK(Evaluate(cx, global,
        "foreign.reflect.get({a:7},'a')===7 && foreign.reflect.set({},'a',8)"));
    CHECK(Evaluate(cx, global,
        "foreign.reflect.enumerate({a:1}).next().realmMarker===21"));
    JS_SetVersion(cx, JSVERSION_ECMA_2015);
    CHECK(Evaluate(cx, global, "var savedReflect=Reflect;delete this.Reflect;true"));
    CHECK(JS_EnumerateStandardClasses(cx, global));
    CHECK(Evaluate(cx, global, "!Object.prototype.hasOwnProperty.call(this,'Reflect') && (this.Reflect=savedReflect,true)"));
    CHECK(JS_DeleteProperty(cx, global, "foreign"));
    JS_ClearNewbornRoots(cx);
    JS_GC(cx);
    CHECK(finalized == 1);
    CHECK(JS_EvaluateScript(cx, global, prototypeSource, strlen(prototypeSource),
                            "iterator-prototype", 1, &oldProto));
    CHECK(JS_AddNamedRoot(cx, &oldProto, "old iterator prototype"));
    rooted = JS_TRUE;
    JS_ClearScope(cx, global);
    CHECK(JS_InitStandardClasses(cx, global));
    CHECK(JS_EvaluateScript(cx, global, prototypeSource, strlen(prototypeSource),
                            "new-iterator-prototype", 1, &result) && result != oldProto);
    CHECK(Evaluate(cx, global, "Reflect.enumerate({a:3}).next().value==='a'"));
    printf("ES6-REFLECT-EMBEDDING checks=%u failures=0\n",checks);
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
