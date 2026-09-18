/* Template realms, classic JSAPI, Unicode and XDR.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "jsapi.h"
#include "jsxdrapi.h"
#include "jsdbgapi.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static JSClass globalClass = {
    "global", 0,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};
static unsigned checks;
#define CHECK(v) do { ++checks; if (!(v)) { \
    fprintf(stderr,"FAIL template embedding check %u\n",checks); goto out; \
} } while (0)

static JSBool
Evaluate(JSContext *cx, JSObject *global, const char *source, jsval *result)
{
    return JS_EvaluateScript(cx, global, source, strlen(source),
                             "template-embedding", 1, result);
}

static JSBool inHook, hookFailed;
static unsigned hookCalls;
static jsval hookTemplate;

static void
AllocationHook(JSContext *cx, JSObject *obj, JSBool creating, void *closure)
{
    JSObject *global = (JSObject *)closure;
    if (!creating || inHook)
        return;
    inHook = JS_TRUE;
    ++hookCalls;
    JS_GC(cx);
    if (!Evaluate(cx, global, "tag`reentrant template`", &hookTemplate))
        hookFailed = JS_TRUE;
    JS_GC(cx);
    inHook = JS_FALSE;
}

static JSBool
FileBoundaries(JSContext *cx, JSObject *global, const char *executable)
{
    FILE *file;
    JSScript *script;
    jsval result;
    JSString *str;
    unsigned n, i;
    JSBool ok;
    char *filename;
    size_t length = strlen(executable);
    filename = (char *)malloc(length + sizeof ".template-source.js");
    if (!filename) return JS_FALSE;
    memcpy(filename, executable, length);
    memcpy(filename + length, ".template-source.js", sizeof ".template-source.js");
    for (n = 240; n < 780; ++n) {
        file = fopen(filename, "wb");
        if (!file) {free(filename);return JS_FALSE;}
        fputc('`', file);
        for (i = 0; i < n; ++i) fputc('a', file);
        fputs("\r\nb`", file);
        fclose(file);
        /* Let the engine open its own stream: static Windows CRT modules
         * must never exchange FILE pointers. The probe executable itself
         * lives in the test runner's disposable directory. */
        script = JS_CompileFile(cx, global, filename);
        remove(filename);
        if (!script) {free(filename);return JS_FALSE;}
        ok = JS_ExecuteScript(cx, global, script, &result);
        if (ok && JSVAL_IS_STRING(result)) {
            str = JSVAL_TO_STRING(result);
            ok = JS_GetStringLength(str) == n + 2 &&
                 JS_GetStringChars(str)[n] == '\n' &&
                 JS_GetStringChars(str)[n + 1] == 'b';
        } else {
            ok = JS_FALSE;
        }
        JS_DestroyScript(cx, script);
        if (!ok) {
            fprintf(stderr, "FAIL template FILE boundary %u\n", n);
            free(filename);
            return JS_FALSE;
        }
    }
    free(filename);
    return JS_TRUE;
}

int main(int argc, char **argv)
{
    JSRuntime *rt;
    JSContext *cx;
    JSObject *a = NULL, *b = NULL, *c = NULL;
    JSObject *scriptObject = NULL, *decodedObject = NULL;
    JSString *source = NULL;
    JSScript *script = NULL, *decoded = NULL;
    JSXDRState *encoder = NULL, *decoder = NULL;
    jsval result, saved = JSVAL_VOID;
    uint32 length;
    void *data;
    jschar text[1024];
    size_t i, j;
    int status = 1;
    JSVersion versions[] = {JSVERSION_DEFAULT, JSVERSION_1_5, JSVERSION_1_7};
    const char *program =
        "function tag(t){return t;}function template(){return tag`A%B${3}C`; }"
        "var t=template();t[0].charCodeAt(1)===0xd800 && "
        "t.raw[0].charCodeAt(1)===0xd800 && t===tag`A%B${4}C` && "
        "Object.isFrozen(t) && Object.isFrozen(t.raw)";
    rt = JS_NewRuntime(4 * 1024 * 1024);
    if (!rt) return 1;
    cx = JS_NewContext(rt, 8192);
    if (!cx) {JS_DestroyRuntime(rt);return 1;}
    JS_BeginRequest(cx);
    JS_AddNamedRoot(cx, &a, "template global A");
    JS_AddNamedRoot(cx, &b, "template global B");
    JS_AddNamedRoot(cx, &c, "template global C");
    JS_AddNamedRoot(cx, &saved, "saved template object");
    JS_AddNamedRoot(cx, &source, "template decompiled source");
    JS_AddNamedRoot(cx, &scriptObject, "template script");
    JS_AddNamedRoot(cx, &decodedObject, "decoded template script");
    hookTemplate = JSVAL_VOID;
    JS_AddNamedRoot(cx, &hookTemplate, "reentrant template object");
    JS_SetVersion(cx, JSVERSION_ECMA_2015);
    a = JS_NewObject(cx, &globalClass, NULL, NULL);
    CHECK(a != NULL);
    JS_SetGlobalObject(cx, a);
    CHECK(JS_InitStandardClasses(cx, a));
    b = JS_NewObject(cx, &globalClass, NULL, NULL);
    CHECK(b && JS_SetParent(cx, b, NULL) && JS_SetPrototype(cx, b, NULL));
    JS_SetGlobalObject(cx, b);
    CHECK(JS_InitStandardClasses(cx, b));
    JS_SetGlobalObject(cx, a);
    for (i = 0; program[i]; ++i)
        text[i] = program[i] == '%' ? 0xd800 : (unsigned char)program[i];
    script = JS_CompileUCScript(cx, a, text, i, "template-unicode", 1);
    CHECK(script != NULL);
    CHECK((scriptObject = JS_NewScriptObject(cx, script)) != NULL);
    CHECK(JS_ExecuteScript(cx, a, script, &result) && result == JSVAL_TRUE);
    CHECK(JS_GetProperty(cx, a, "t", &saved));
    encoder = JS_XDRNewMem(cx, JSXDR_ENCODE);
    decoder = JS_XDRNewMem(cx, JSXDR_DECODE);
    CHECK(encoder && decoder && JS_XDRScript(encoder, &script));
    data = JS_XDRMemGetData(encoder, &length);
    JS_XDRMemSetData(decoder, data, length);
    CHECK(JS_XDRScript(decoder, &decoded));
    CHECK((decodedObject = JS_NewScriptObject(cx, decoded)) != NULL);
    JS_GC(cx);
    CHECK(JS_ExecuteScript(cx, a, decoded, &result) && result == JSVAL_TRUE);
    CHECK(JS_GetProperty(cx, a, "t", &result) && result == saved);
    source = JS_DecompileScript(cx, decoded, "template-roundtrip", 0);
    CHECK(source && JS_EvaluateUCScript(cx, a, JS_GetStringChars(source),
          JS_GetStringLength(source), "decompiled", 1, &result) && result == JSVAL_TRUE);
    /* Identical raw strings in different realms have distinct cached arrays. */
    JS_SetGlobalObject(cx, b);
    CHECK(JS_ExecuteScript(cx, b, decoded, &result) && result == JSVAL_TRUE);
    CHECK(JS_GetProperty(cx, b, "t", &result) && result != saved);
    /* The executing function's realm, not the context global or tag's realm,
     * determines the template object. */
    CHECK(JS_GetProperty(cx, a, "template", &result) &&
          JS_DefineProperty(cx, b, "otherTemplate", result, NULL, NULL, 0));
    CHECK(Evaluate(cx, b, "otherTemplate()", &result) && result == saved);
    CHECK(JS_GetProperty(cx, b, "tag", &result) &&
          JS_SetProperty(cx, a, "tag", &result));
    CHECK(JS_CallFunctionName(cx, a, "template", 0, NULL, &result) && result == saved);
    JS_GC(cx);
    CHECK(JS_CallFunctionName(cx, a, "template", 0, NULL, &result) && result == saved);
    CHECK(Evaluate(cx, b,
          "function reentrantTemplate(){return tag`reentrant template`;}true", &result)
          && result == JSVAL_TRUE);
    JS_SetObjectHook(rt, AllocationHook, b);
    CHECK(JS_CallFunctionName(cx, b, "reentrantTemplate", 0, NULL, &result));
    JS_SetObjectHook(rt, NULL, NULL);
    CHECK(hookCalls && !hookFailed && result == hookTemplate);
    c = JS_NewObject(cx, &globalClass, NULL, NULL);
    CHECK(c && JS_SetParent(cx, c, NULL) && JS_SetPrototype(cx, c, NULL));
    JS_SetGlobalObject(cx, c);
    CHECK(JS_InitStandardClasses(cx, c));
    CHECK(Evaluate(cx, c,
          "function tag(t){return t;}"
          "function reentrantTemplate(){return tag`reentrant template`;}true", &result)
          && result == JSVAL_TRUE);
    hookCalls = 0;
    hookTemplate = JSVAL_VOID;
    JS_SetObjectHook(rt, AllocationHook, c);
    CHECK(JS_CallFunctionName(cx, c, "reentrantTemplate", 0, NULL, &result));
    JS_SetObjectHook(rt, NULL, NULL);
    CHECK(hookCalls && !hookFailed && result == hookTemplate);
    CHECK(argc > 0 && FileBoundaries(cx, a, argv[0]));
    for (j = 0; j < sizeof versions / sizeof versions[0]; ++j) {
        JS_SetVersion(cx, versions[j]);
        CHECK(!Evaluate(cx, a, "`legacy must reject`", &result));
        JS_ClearPendingException(cx);
    }
    status = 0;
    printf("ES6-TEMPLATE-EMBEDDING PASS checks=%u\n", checks);
out:
    JS_SetObjectHook(rt, NULL, NULL);
    JS_RemoveRoot(cx, &hookTemplate);
    if (decoder) {JS_XDRMemSetData(decoder, NULL, 0);JS_XDRDestroy(decoder);}
    if (encoder) JS_XDRDestroy(encoder);
    if (decoded && !decodedObject) JS_DestroyScript(cx, decoded);
    if (script && !scriptObject) JS_DestroyScript(cx, script);
    JS_RemoveRoot(cx, &decodedObject);
    JS_RemoveRoot(cx, &scriptObject);
    JS_RemoveRoot(cx, &source);
    JS_RemoveRoot(cx, &saved);
    JS_RemoveRoot(cx, &c);
    JS_RemoveRoot(cx, &b);
    JS_RemoveRoot(cx, &a);
    JS_EndRequest(cx);
    JS_DestroyContext(cx);
    JS_DestroyRuntime(rt);
    JS_ShutDown();
    return status;
}
