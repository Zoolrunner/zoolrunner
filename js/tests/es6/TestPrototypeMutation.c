/* New prototype mutation preserves classic embedding access boundaries.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "jsapi.h"
#include <stdio.h>
#include <string.h>

static JSClass globalClass = {
    "global", 0,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};
static unsigned checks, writes;
static int accessBehavior;

static JSBool
Access(JSContext *cx, JSObject *obj, jsval id, JSAccessMode mode, jsval *vp)
{
    jsval result;
    const char *freeze = "Object.preventExtensions(target)";
    if ((mode & JSACC_TYPEMASK) != JSACC_PROTO || !(mode & JSACC_WRITE))
        return JS_TRUE;
    ++writes;
    if (accessBehavior == 1) {
        JS_ReportError(cx, "prototype write denied by embedding");
        return JS_FALSE;
    }
    if (accessBehavior == 2) {
        /* The in/out parameter is independent of the requested prototype. */
        *vp = JSVAL_NULL;
        JS_GC(cx);
    } else if (accessBehavior == 3) {
        JS_GC(cx);
        return JS_EvaluateScript(cx, JS_GetGlobalObject(cx), freeze,
                                 strlen(freeze), "access-reentry", 1, &result);
    }
    return JS_TRUE;
}

static JSBool
Evaluate(JSContext *cx, JSObject *global, const char *source)
{
    jsval result;
    return JS_EvaluateScript(cx, global, source, strlen(source),
                             "prototype-embedding", 1, &result) &&
           result == JSVAL_TRUE;
}

#define CHECK(condition) do { ++checks; if (!(condition)) { \
    fprintf(stderr, "FAIL prototype embedding check %u\n", checks); \
    goto out; } } while (0)

int main(void)
{
    JSRuntime *rt = JS_NewRuntime(8 * 1024 * 1024);
    JSContext *cx;
    JSObject *global, *target;
    jsval value;
    unsigned prior;
    int status = 1;
    if (!rt) return 1;
    cx = JS_NewContext(rt, 8192);
    if (!cx) { JS_DestroyRuntime(rt); return 1; }
    JS_BeginRequest(cx);
    JS_SetVersion(cx, JSVERSION_ECMA_2015);
    global = JS_NewObject(cx, &globalClass, NULL, NULL);
    CHECK(global != NULL);
    JS_SetGlobalObject(cx, global);
    CHECK(JS_InitStandardClasses(cx, global));
    CHECK(Evaluate(cx, global, "var original={old:1}, target=Object.create(original);true"));
    JS_SetCheckObjectAccessCallback(rt, Access);
    accessBehavior = 1;
    CHECK(Evaluate(cx, global, "var denied=false;try{Object.setPrototypeOf(target,{next:2});}catch(e){denied=true;}denied&&Object.getPrototypeOf(target)===original"));
    CHECK(writes == 1);
    accessBehavior = 2;
    CHECK(Evaluate(cx, global, "Object.setPrototypeOf(target,{marker:7})===target&&target.marker===7"));
    CHECK(writes == 2);
    JS_GC(cx);
    CHECK(Evaluate(cx, global, "target.marker===7&&!target.hasOwnProperty('marker')"));
    accessBehavior = 3;
    CHECK(Evaluate(cx, global, "var stopped=false;try{Object.setPrototypeOf(target,{marker:9});}catch(e){stopped=e instanceof TypeError;}stopped&&target.marker===7&&!Object.isExtensible(target)"));
    CHECK(writes == 3);
    accessBehavior = 1;
    CHECK(JS_GetProperty(cx, global, "target", &value) && JSVAL_IS_OBJECT(value));
    target = JSVAL_TO_OBJECT(value);
    prior = writes;
    /* The trusted native API retains its historical dispatch contract. */
    CHECK(JS_SetPrototype(cx, target, NULL));
    CHECK(JS_GetPrototype(cx, target) == NULL && writes == prior);
    JS_SetCheckObjectAccessCallback(rt, NULL);
    CHECK(Evaluate(cx, global, "var text=new String('ab');Object.setPrototypeOf(text,null);text.length===2&&text[1]==='b'"));
    JS_GC(cx);
    CHECK(Evaluate(cx, global, "var re=/x/g;Object.setPrototypeOf(re,null);RegExp.prototype.exec.call(re,'x')[0]==='x'&&re.lastIndex===1"));
    printf("ES6-PROTOTYPE-EMBEDDING checks=%u failures=0\n", checks);
    status = 0;
  out:
    JS_SetCheckObjectAccessCallback(rt, NULL);
    JS_EndRequest(cx);
    JS_DestroyContext(cx);
    JS_DestroyRuntime(rt);
    JS_ShutDown();
    return status;
}
