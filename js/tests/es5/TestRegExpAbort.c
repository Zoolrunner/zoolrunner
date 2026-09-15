/* Classic JSAPI cancellation regression; MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "jsapi.h"
#include <stdio.h>
#include <string.h>

static JSBool interrupted;
static JSClass globalClass = {
    "global", 0,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSBool InterruptRegExp(JSContext *cx, JSScript *script)
{
    if (!script) {
        interrupted = JS_TRUE;
        return JS_FALSE;
    }
    return JS_TRUE;
}

int main(void)
{
    JSRuntime *rt = JS_NewRuntime(8 * 1024 * 1024);
    JSContext *cx;
    JSObject *global;
    jsval result;
    JSBool evaluated;
    int status = 1;
    const char *initial = "/(a)(b)(c)(d)(e)(f)(g)(h)(i)(j)/.test('abcdefghij')";
    const char *expensive = "/^(a+)+b$/.test('aaaaaaaaaaaaaaaaaaaaaaaa')";
    const char *recovery = "/c/.test('c') && RegExp.lastMatch === 'c'";
    if (!rt) return 2;
    cx = JS_NewContext(rt, 8192);
    if (!cx) { JS_DestroyRuntime(rt); return 2; }
    JS_BeginRequest(cx);
    global = JS_NewObject(cx, &globalClass, NULL, NULL);
    if (!global) goto out;
    JS_SetGlobalObject(cx, global);
    if (!JS_InitStandardClasses(cx, global)) goto out;
    if (!JS_EvaluateScript(cx, global, initial, strlen(initial),
                           "initial", 1, &result) || result != JSVAL_TRUE) goto out;
    JS_SetBranchCallback(cx, InterruptRegExp);
    evaluated = JS_EvaluateScript(cx, global, expensive, strlen(expensive),
                                  "interrupt", 1, &result);
    JS_SetBranchCallback(cx, NULL);
    if (evaluated || !interrupted) goto out;
    JS_ClearPendingException(cx);
    if (!JS_EvaluateScript(cx, global, recovery, strlen(recovery),
                           "recovery", 1, &result) || result != JSVAL_TRUE) goto out;
    puts("REGEXP-ABORT checks=3 failures=0");
    status = 0;
out:
    JS_EndRequest(cx);
    JS_DestroyContext(cx);
    JS_DestroyRuntime(rt);
    JS_ShutDown();
    return status;
}
