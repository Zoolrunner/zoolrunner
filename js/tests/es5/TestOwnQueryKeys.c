/* Classic JSAPI/edition boundary regression; MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "jsapi.h"
#include <stdio.h>
#include <string.h>

static JSClass globalClass = {
    "global", 0,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSBool
CheckEdition(JSRuntime *rt, JSVersion version)
{
    JSContext *cx = JS_NewContext(rt, 8192);
    JSObject *global;
    jsval key = JSVAL_VOID, value;
    jsid id;
    JSBool ok = JS_FALSE;
    const char *source;
    if (!cx) return JS_FALSE;
    JS_BeginRequest(cx);
    JS_SetVersion(cx, version);
    global = JS_NewObject(cx, &globalClass, NULL, NULL);
    if (!global) goto out;
    JS_SetGlobalObject(cx, global);
    if (!JS_InitStandardClasses(cx, global)) goto out;
    if (!JS_AddRoot(cx, &key)) goto out;
    source = "var key=['loaded']; var o={loaded:1}; key";
    if (!JS_EvaluateScript(cx, global, source, strlen(source),
                           "own-query", 1, &key)) goto rooted;
    /* Native API object ids must retain their classic identity, even though
     * default-script property queries now perform standard key conversion. */
    if (!JS_ValueToId(cx, key, &id) || !JS_IdToValue(cx, id, &value)) goto rooted;
    if (version == JSVERSION_ECMA_2015) {
        if (!JSVAL_IS_STRING(value) ||
            strcmp(JS_GetStringBytes(JSVAL_TO_STRING(value)), "loaded")) goto rooted;
    } else if (value != key) goto rooted;
    source = version == JSVERSION_1_7
        ? "!o.hasOwnProperty(key) && !o.propertyIsEnumerable(key)"
        : "o.hasOwnProperty(key) && o.propertyIsEnumerable(key)";
    if (!JS_EvaluateScript(cx, global, source, strlen(source),
                           "own-query", 1, &value) || value != JSVAL_TRUE) goto rooted;
    ok = JS_TRUE;
rooted:
    JS_RemoveRoot(cx, &key);
out:
    JS_EndRequest(cx);
    JS_DestroyContext(cx);
    return ok;
}

int main(void)
{
    JSRuntime *rt = JS_NewRuntime(8 * 1024 * 1024);
    JSBool ok;
    if (!rt) return 1;
    ok = CheckEdition(rt, JSVERSION_DEFAULT) &&
         CheckEdition(rt, JSVERSION_1_7) &&
         CheckEdition(rt, JSVERSION_ECMA_2015);
    JS_DestroyRuntime(rt);
    JS_ShutDown();
    printf("OWN-QUERY-EMBEDDING editions=3 failures=%d\n", !ok);
    return ok ? 0 : 1;
}
