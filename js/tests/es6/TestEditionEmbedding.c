/* Classic JSAPI edition/XDR regression; MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "jsapi.h"
#include "jsxdrapi.h"
#include <stdio.h>
#include <string.h>

static JSClass globalClass = {
    "global", 0,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSBool
Edition(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    *rval = INT_TO_JSVAL(JS_GetVersion(cx));
    return JS_TRUE;
}

static JSBool
Nested(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    const char *source = "'use strict'; ({x:1,x:2}).x===2 && edition()===2015";
    return JS_EvaluateScript(cx, JS_GetGlobalObject(cx), source, strlen(source),
                             "native-callback", 1, rval);
}

static unsigned checks;
static JSBool deniedWatch;

static JSBool
CheckAccess(JSContext *cx, JSObject *obj, jsval id, JSAccessMode mode, jsval *value)
{
    if (mode == JSACC_WATCH) {
        deniedWatch = JS_TRUE;
        JS_ReportError(cx, "embedding denied accessor");
        return JS_FALSE;
    }
    return JS_TRUE;
}

#define CHECK(condition) do { \
    ++checks; \
    if (!(condition)) { \
        fprintf(stderr, "FAIL edition embedding check %u\n", checks); \
        goto out; \
    } \
} while (0)

int main(void)
{
    JSRuntime *rt;
    JSContext *cx;
    JSObject *global;
    JSScript *script = NULL, *decoded = NULL;
    JSXDRState *encoder = NULL, *decoder = NULL;
    jsval result;
    uint32 length;
    void *data;
    int status = 1;
    const char *legacy =
        "function legacyRegExp(){return /legacy/g;}"
        "function legacyEdition(){return edition()===170 && /a/('a')[0]==='a';}true";
    const char *modern =
        "'use strict';"
        "function modernEdition(){let radixValue=0b101+0o17;var caught=false;"
        "try{var nested;[[nested]]=[null];}catch(error){caught=error instanceof TypeError;}"
        "const constantValue=1;var immutable=false;"
        "try{constantValue++;}catch(error){immutable=error instanceof TypeError;}"
        "try{constantValue+=1;immutable=false;}catch(error){immutable=immutable && error instanceof TypeError;}"
        "return radixValue===20 && Number('0b101')===5 && caught && "
        "immutable && "
        "(function(){var a=[];for(var i=0;i<3;++i)a.push(/fresh/g);"
        "a[0].lastIndex=9;return a[0]!==a[1] && a[1].lastIndex===0;})() && "
        "legacyRegExp()===legacyRegExp() && "
        "(function(Array,Object){return [].length===0 && ({answer:42}).answer===42;})(null,null) && "
        "edition()===2015 && ({x:1,x:2}).x===2"
        " && legacyEdition() && nativeNested() && edition()===2015;}"
        "modernEdition() && eval('modernEdition()') && "
        "Function('return edition()===2015')() && "
        "eval('('+modernEdition.toString()+')')()";

    rt = JS_NewRuntime(8 * 1024 * 1024);
    if (!rt) return 1;
    cx = JS_NewContext(rt, 8192);
    if (!cx) { JS_DestroyRuntime(rt); return 1; }
    JS_BeginRequest(cx);
    global = JS_NewObject(cx, &globalClass, NULL, NULL);
    if (!global) goto out;
    JS_SetGlobalObject(cx, global);
    if (!JS_InitStandardClasses(cx, global) ||
        !JS_DefineFunction(cx, global, "edition", Edition, 0, 0) ||
        !JS_DefineFunction(cx, global, "nativeNested", Nested, 0, 0)) goto out;
    CHECK(JS_GetVersion(cx) == JSVERSION_DEFAULT);
    CHECK(JS_StringToVersion("ECMAv6") == JSVERSION_ECMA_2015 &&
          !strcmp(JS_VersionToString(JSVERSION_ECMA_2015), "ECMAv6"));
    JS_SetVersion(cx, JSVERSION_1_7);
    CHECK(JS_EvaluateScript(cx, global, legacy, strlen(legacy), "legacy", 1, &result)
          && result == JSVAL_TRUE);
    JS_SetVersion(cx, JSVERSION_ECMA_2015);
    script = JS_CompileScript(cx, global, modern, strlen(modern), "modern", 1);
    CHECK(script != NULL);
    encoder = JS_XDRNewMem(cx, JSXDR_ENCODE);
    decoder = JS_XDRNewMem(cx, JSXDR_DECODE);
    CHECK(encoder && decoder && JS_XDRScript(encoder, &script));
    data = JS_XDRMemGetData(encoder, &length);
    JS_XDRMemSetData(decoder, data, length);
    JS_SetVersion(cx, JSVERSION_DEFAULT);
    CHECK(JS_XDRScript(decoder, &decoded));
    CHECK(JS_ExecuteScript(cx, global, decoded, &result) && result == JSVAL_TRUE);
    CHECK(JS_GetVersion(cx) == JSVERSION_DEFAULT);
    JS_GC(cx);
    CHECK(JS_CallFunctionName(cx, global, "modernEdition", 0, NULL, &result) &&
          result == JSVAL_TRUE);
    CHECK(JS_GetVersion(cx) == JSVERSION_DEFAULT);
    JS_SetVersion(cx, JSVERSION_ECMA_2015);
    JS_SetCheckObjectAccessCallback(rt, CheckAccess);
    {
        const char *source = "var denied=false;try{({get __proto__(){return 1;}});}"
                             "catch(e){denied=true;}denied";
        CHECK(JS_EvaluateScript(cx, global, source, strlen(source), "access-check", 1,
                                &result) && result == JSVAL_TRUE && deniedWatch);
    }
    JS_SetCheckObjectAccessCallback(rt, NULL);
    {
        const char *names[] = {"parameter"};
        const char *body = "let parameter=2;return parameter;";
        JSFunction *function;
        jsval argument = INT_TO_JSVAL(1);

        CHECK(JS_CompileFunction(cx, global, "modernParameter", 1, names,
                                 body, strlen(body), "modern-parameter", 1) == NULL);
        JS_ClearPendingException(cx);
        JS_SetVersion(cx, JSVERSION_1_7);
        function = JS_CompileFunction(cx, global, "legacyParameter", 1, names,
                                      body, strlen(body), "legacy-parameter", 1);
        CHECK(function && JS_CallFunction(cx, global, function, 1, &argument,
                                          &result) && result == INT_TO_JSVAL(2) &&
              JS_GetVersion(cx) == JSVERSION_1_7);
    }
    status = 0;
    printf("ES6-EDITION-EMBEDDING checks=%u failures=0\n", checks);
out:
    if (decoder) { JS_XDRMemSetData(decoder, NULL, 0); JS_XDRDestroy(decoder); }
    if (encoder) JS_XDRDestroy(encoder);
    if (decoded) JS_DestroyScript(cx, decoded);
    if (script) JS_DestroyScript(cx, script);
    JS_EndRequest(cx);
    JS_DestroyContext(cx);
    JS_DestroyRuntime(rt);
    JS_ShutDown();
    return status;
}
