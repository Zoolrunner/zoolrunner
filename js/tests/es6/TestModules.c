/* Explicit module host API and script lifecycle. MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "jsapi.h"
#include "jsdbgapi.h"
#include "jsscript.h"
#include <stdio.h>
#include <string.h>

static unsigned checks, created, destroyed, hookErrors;
static JSScript *scripts[128];
static JSClass globalClass = {
    "global", JSCLASS_GLOBAL_FLAGS,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};
static void report(JSContext *cx, const char *message, JSErrorReport *error)
{
    fprintf(stderr, "%.200s\n", message);
}
static void newScript(JSContext *cx, const char *filename, uintN line,
                      JSScript *script, JSFunction *fun, void *data)
{
    unsigned i;
    if (!filename || strncmp(filename, "module-", 7)) return;
    for (i = 0; i < 128 && scripts[i]; ++i) {}
    if (i == 128) { ++hookErrors; return; }
    scripts[i] = script;
    ++created;
    JS_GC(cx);
}
static void destroyScript(JSContext *cx, JSScript *script, void *data)
{
    unsigned i;
    if (!script->filename || strncmp(script->filename, "module-", 7)) return;
    for (i = 0; i < 128 && scripts[i] != script; ++i) {}
    if (i == 128) { ++hookErrors; return; }
    scripts[i] = NULL;
    ++destroyed;
}
static JSObject *compile(JSContext *cx, JSObject *global, const char *source)
{
    jschar text[4096];
    size_t i, n = strlen(source);
    if (n >= 4096) return NULL;
    for (i = 0; i < n; ++i) text[i] = (unsigned char)source[i];
    return JS_CompileUCModule(cx, global, NULL, text, n, "module-native", 1);
}
#define CHECK(x) do { ++checks; if (!(x)) { \
    fprintf(stderr, "MODULE-NATIVE FAIL %u\n", checks); goto out; } } while (0)
int main(void)
{
    JSRuntime *rt = JS_NewRuntime(32 * 1024 * 1024);
    JSContext *cx;
    JSObject *global = NULL, *a = NULL, *b = NULL, *ns = NULL, *requests = NULL;
    JSString *name = NULL;
    jsval value = JSVAL_VOID;
    JSVersion version;
    jsuint length;
    int status = 1;
    if (!rt) return 1;
    cx = JS_NewContext(rt, 8192);
    if (!cx) { JS_DestroyRuntime(rt); return 1; }
    JS_BeginRequest(cx);
    JS_SetErrorReporter(cx, report);
    JS_SetOptions(cx, JS_GetOptions(cx) | JSOPTION_DONT_REPORT_UNCAUGHT);
    JS_AddNamedRoot(cx, &global, "global");
    JS_AddNamedRoot(cx, &a, "module a");
    JS_AddNamedRoot(cx, &b, "module b");
    JS_AddNamedRoot(cx, &ns, "namespace");
    JS_AddNamedRoot(cx, &requests, "requests");
    JS_AddNamedRoot(cx, &name, "specifier");
    JS_AddNamedRoot(cx, &value, "result");
    global = JS_NewObject(cx, &globalClass, NULL, NULL);
    CHECK(global);
    JS_SetGlobalObject(cx, global);
    CHECK(JS_InitStandardClasses(cx, global));
    version = JS_GetVersion(cx);
    JS_SetNewScriptHook(rt, newScript, NULL);
    JS_SetDestroyScriptHook(rt, destroyScript, NULL);
    a = compile(cx, global, "export let x=1;export function inc(step=1){x+=step;return x}");
    CHECK(a && JS_GetVersion(cx) == version);
    b = compile(cx, global, "import {x,inc} from 'a';import 'a';export {x,inc};export var initial=x;");
    CHECK(b && JS_GetVersion(cx) == version);
    requests = JS_GetModuleRequests(cx, b);
    CHECK(requests && JS_GetArrayLength(cx, requests, &length) && length == 1);
    CHECK(JS_GetElement(cx, requests, 0, &value) && JSVAL_IS_STRING(value));
    name = JSVAL_TO_STRING(value);
    CHECK(!strcmp(JS_GetStringBytes(name), "a"));
    CHECK(JS_SetModuleDependency(cx, b, name, a));
    CHECK(JS_InstantiateModule(cx, b));
    ns = JS_GetModuleNamespace(cx, b);
    CHECK(ns && JS_GetModuleNamespace(cx, b) == ns);
    JS_GC(cx);
    CHECK(JS_EvaluateModule(cx, b) && JS_GetVersion(cx) == version);
    CHECK(JS_GetProperty(cx, ns, "x", &value) && value == INT_TO_JSVAL(1));
    CHECK(JS_CallFunctionName(cx, ns, "inc", 0, NULL, &value) && value == INT_TO_JSVAL(2));
    CHECK(JS_EvaluateModule(cx, b));
    CHECK(JS_GetProperty(cx, ns, "initial", &value) && value == INT_TO_JSVAL(1));
    CHECK(JS_GetProperty(cx, global, "x", &value) && JSVAL_IS_VOID(value));
    CHECK(JS_GetProperty(cx, global, "inc", &value) && JSVAL_IS_VOID(value));
    CHECK(!JS_SetModuleDependency(cx, b, name, a));
    JS_ClearPendingException(cx);
    a = b = requests = NULL;
    name = NULL;
    JS_GC(cx);
    CHECK(JS_CallFunctionName(cx, ns, "inc", 0, NULL, &value) && value == INT_TO_JSVAL(3));
    CHECK(hookErrors == 0 && created >= 4);
    status = 0;
 out:
    JS_RemoveRoot(cx, &value);
    JS_RemoveRoot(cx, &name);
    JS_RemoveRoot(cx, &requests);
    JS_RemoveRoot(cx, &ns);
    JS_RemoveRoot(cx, &b);
    JS_RemoveRoot(cx, &a);
    JS_RemoveRoot(cx, &global);
    JS_EndRequest(cx);
    JS_DestroyContext(cx);
    JS_DestroyRuntime(rt);
    JS_ShutDown();
    if (!status && (hookErrors || created != destroyed)) {
        fprintf(stderr, "MODULE-HOOK FAIL new=%u destroyed=%u errors=%u\n",
                created, destroyed, hookErrors);
        status = 1;
    }
    if (!status) printf("MODULE-NATIVE PASS checks=%u scripts=%u\n", checks, created);
    return status;
}
