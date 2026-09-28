/* Annex B accessor realms, embedding checks and historical globals.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "jsapi.h"
#include <stdio.h>
#include <string.h>
static unsigned checks, reads, writes;
static JSBool deny;
static JSBool Resolve(JSContext *cx, JSObject *obj, jsval id, uintN flags, JSObject **objp)
{
    JSBool resolved;
    if (!JS_ResolveStandardClass(cx, obj, id, &resolved)) return JS_FALSE;
    if (resolved) *objp = obj;
    return JS_TRUE;
}
static JSClass globalClass = {
    "global", JSCLASS_NEW_RESOLVE,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, (JSResolveOp)Resolve, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};
static JSBool Access(JSContext *cx, JSObject *obj, jsval id, JSAccessMode mode, jsval *vp)
{
    if ((mode & JSACC_TYPEMASK) != JSACC_PROTO) return JS_TRUE;
    if (mode & JSACC_WRITE) { ++writes; *vp = JSVAL_NULL; }
    else ++reads;
    JS_GC(cx);
    if (deny) { JS_ReportError(cx, "Annex host access denied"); return JS_FALSE; }
    return JS_TRUE;
}
static JSClass hostClass = {
    "AnnexHost", 0,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    NULL, Access, NULL, NULL, NULL, NULL, NULL, NULL
};
static JSBool Evaluate(JSContext *cx, JSObject *global, const char *source)
{
    jsval result;
    return JS_EvaluateScript(cx, global, source, strlen(source), "annex-builtins", 1, &result) && result == JSVAL_TRUE;
}
#define CHECK(c) do { ++checks; if (!(c)) { fprintf(stderr, "FAIL Annex embedding check %u\n", checks); goto out; } } while (0)
int main(void)
{
    JSRuntime *rt = JS_NewRuntime(16 * 1024 * 1024);
    JSContext *cx, *other = NULL;
    JSObject *global, *foreign, *host, *clone;
    jsval value;
    int status = 1;
    if (!rt) return 1;
    cx = JS_NewContext(rt, 8192);
    if (!cx) { JS_DestroyRuntime(rt); return 1; }
    JS_BeginRequest(cx); JS_SetVersion(cx, JSVERSION_ECMA_2015);
    global = JS_NewObject(cx, &globalClass, NULL, NULL);
    CHECK(global); JS_SetGlobalObject(cx, global);
    CHECK(JS_InitStandardClasses(cx, global));
    CHECK(Evaluate(cx, global, "var getter=Object.getOwnPropertyDescriptor(Object.prototype,'__proto__').get;"
                   "var setter=Object.getOwnPropertyDescriptor(Object.prototype,'__proto__').set;true"));
    host = JS_DefineObject(cx, global, "host", &hostClass, NULL, 0);
    CHECK(host);
    CHECK(Evaluate(cx, global, "getter.call(host)===Object.prototype"));
    CHECK(reads == 1);
    CHECK(Evaluate(cx, global, "var requested={};setter.call(host,requested);getter.call(host)===requested"));
    CHECK(writes == 1 && reads == 2);
    deny = JS_TRUE;
    CHECK(Evaluate(cx, global, "(function(){try{getter.call(host);}catch(e){return String(e).indexOf('Annex host access denied')>=0;}return false;})()"));
    CHECK(Evaluate(cx, global, "(function(){try{setter.call(host,{});}catch(e){return String(e).indexOf('Annex host access denied')>=0;}return false;})()"));
    deny = JS_FALSE;
    CHECK(Evaluate(cx, global, "getter.call(host)===requested"));
    CHECK(JS_GetProperty(cx, global, "getter", &value));
    clone = JS_CloneFunctionObject(cx, JSVAL_TO_OBJECT(value), global);
    CHECK(clone);
    CHECK(JS_DefineProperty(cx, global, "clonedGetter", OBJECT_TO_JSVAL(clone), NULL, NULL, 0));
    CHECK(Evaluate(cx, global, "getter.call(clonedGetter)===Function.prototype && clonedGetter.call(host)===requested"));
    other = JS_NewContext(rt, 8192); CHECK(other);
    JS_BeginRequest(other); JS_SetVersion(other, JSVERSION_ECMA_2015);
    foreign = JS_NewObject(other, &globalClass, NULL, NULL); CHECK(foreign);
    JS_SetGlobalObject(other, foreign); CHECK(JS_InitStandardClasses(other, foreign));
    CHECK(Evaluate(other, foreign, "var exports={get:Object.getOwnPropertyDescriptor(Object.prototype,'__proto__').get,"
                   "number:Number.prototype,string:String.prototype,boolean:Boolean.prototype,symbol:Symbol.prototype};true"));
    CHECK(JS_GetProperty(other, foreign, "exports", &value));
    CHECK(JS_DefineProperty(cx, global, "foreign", value, NULL, NULL, 0));
    JS_EndRequest(other); JS_DestroyContextNoGC(other); other = NULL;
    JS_ClearNewbornRoots(cx); JS_GC(cx);
    CHECK(Evaluate(cx, global, "foreign.get.call(1)===foreign.number && foreign.get.call('x')===foreign.string &&"
                   "foreign.get.call(true)===foreign.boolean && foreign.get.call(Symbol())===foreign.symbol"));
    CHECK(Evaluate(cx, global, "var savedString=String; delete this.escape;delete this.unescape;delete this.String;"
                   "!('escape' in this) && !('unescape' in this) && !('String' in this)"));
    JS_SetVersion(cx, JSVERSION_1_7);
    CHECK(JS_EnumerateStandardClasses(cx, global));
    CHECK(Evaluate(cx, global, "!('escape' in this) && !('unescape' in this) && !('String' in this)"));
    /* Reinitializing a cleared global starts a new, explicitly legacy lifetime. */
    JS_ClearScope(cx, global); CHECK(JS_InitStandardClasses(cx, global));
    CHECK(Evaluate(cx, global, "typeof Object.getOwnPropertyDescriptor(Object.prototype,'__proto__').get==='undefined'"));
    JS_SetVersion(cx, JSVERSION_ECMA_2015);
    CHECK(Evaluate(cx, global, "delete this.escape;typeof escape==='function'"));
    printf("ES6-ANNEX-EMBEDDING checks=%u failures=0\n", checks); status = 0;
  out:
    if (other) { JS_EndRequest(other); JS_DestroyContextNoGC(other); }
    JS_EndRequest(cx); JS_DestroyContext(cx); JS_DestroyRuntime(rt); JS_ShutDown();
    return status;
}
