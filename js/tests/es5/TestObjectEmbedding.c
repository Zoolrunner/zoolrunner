/* Optional classic JSAPI regression; MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "jsapi.h"
#include "jsxdrapi.h"
#include <stdio.h>
#include <string.h>

static JSBool
ResolveStandard(JSContext *cx, JSObject *obj, jsval id)
{
    JSBool resolved;
    return JS_ResolveStandardClass(cx, obj, id, &resolved);
}

static JSClass globalClass = {
    "global", 0, /* Historical embedders need not set JSCLASS_GLOBAL_FLAGS. */
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, ResolveStandard, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};
static JSObject *protectedObject;
static unsigned checks;
static JSBool denyConstructor;

static JSBool
CheckAccess(JSContext *cx, JSObject *obj, jsval id, JSAccessMode mode,
            jsval *value)
{
    if (denyConstructor && JSVAL_IS_STRING(id) &&
        strcmp(JS_GetStringBytes(JSVAL_TO_STRING(id)), "constructor") == 0) {
        JS_ReportError(cx, "embedding denied constructor access");
        return JS_FALSE;
    }
    if (obj == protectedObject) {
        JS_ReportError(cx, "embedding denied access");
        return JS_FALSE;
    }
    return JS_TRUE;
}

static JSBool
Evaluate(JSContext *cx, JSObject *global, const char *source)
{
    jsval result;
    ++checks;
    if (!JS_EvaluateScript(cx, global, source, strlen(source),
                           "embedding-test", 1, &result) ||
        result != JSVAL_TRUE) {
        fprintf(stderr, "FAIL embedding check %u\n", checks);
        return JS_FALSE;
    }
    return JS_TRUE;
}

static JSBool
CheckBootstrap(JSRuntime *rt)
{
    JSContext *cx = JS_NewContext(rt, 8192);
    JSObject *global;
    JSBool ok = JS_FALSE;
    if (!cx) return JS_FALSE;
    JS_BeginRequest(cx);
    global = JS_NewObject(cx, &globalClass, NULL, NULL);
    if (!global) goto out;
    JS_SetGlobalObject(cx, global);
    /* A DOM security context may not permit constructor reads during startup.
     * Internal class initialization must not invoke that script-visible getter.
     */
    denyConstructor = JS_TRUE;
    JS_SetCheckObjectAccessCallback(rt, CheckAccess);
    if (!Evaluate(cx, global,
        "typeof Object.keys==='function' && "
        "Object.hasOwnProperty.call({x:1},'x') && "
        "Object.getPrototypeOf(Object)===Function.prototype && "
        "Object.getPrototypeOf(Function.prototype)===Object.prototype")) goto out;
    denyConstructor = JS_FALSE;
    JS_SetCheckObjectAccessCallback(rt, NULL);
    ok = JS_TRUE;
out:
    denyConstructor = JS_FALSE;
    JS_SetCheckObjectAccessCallback(rt, NULL);
    JS_EndRequest(cx);
    JS_DestroyContext(cx);
    return ok;
}

int main(void)
{
    JSRuntime *rt;
    JSContext *cx;
    JSObject *global, *clone, *scope;
    JSFunction *handler;
    JSScript *script = NULL, *decoded = NULL;
    JSXDRState *encoder = NULL, *decoder = NULL;
    uint32 length;
    jsval result;
    int status = 1;
    const char *literal =
        "'use strict'; var a=[,,undefined,,];"
        "function xdrStrict(x){x=9;return this===undefined && arguments[0]===1;}"
        "a.length===4 && !(0 in a) && (2 in a) && !(3 in a) && xdrStrict(1)";

    rt = JS_NewRuntime(8 * 1024 * 1024);
    if (!rt) return 1;
    if (!CheckBootstrap(rt)) { JS_DestroyRuntime(rt); JS_ShutDown(); return 1; }
    cx = JS_NewContext(rt, 8192);
    if (!cx) { JS_DestroyRuntime(rt); return 1; }
    JS_BeginRequest(cx);
    global = JS_NewObject(cx, &globalClass, NULL, NULL);
    if (!global) goto out;
    JS_SetGlobalObject(cx, global);
    if (!Evaluate(cx, global,
        "typeof JSON==='object' && JSON.parse('[1]')[0]===1 && "
        "Object.getPrototypeOf(JSON)===Object.prototype")) goto out;
    if (!JS_InitStandardClasses(cx, global)) goto out;
    /* Event handlers and embedders put ordinary objects on the scope chain. */
    scope = JS_NewObject(cx, NULL, NULL, global);
    if (!scope || !JS_DefineProperty(cx, global, "embeddingScope",
                                     OBJECT_TO_JSVAL(scope), NULL, NULL, 0)) goto out;
    if (!Evaluate(cx, global,
        "embeddingScope.method=function(){'use strict';return this;};true")) goto out;
    handler = JS_CompileFunction(cx, scope, "handler", 0, NULL,
                                "return method();", 16, "object-scope", 1);
    ++checks;
    if (!handler || !JS_CallFunction(cx, scope, handler, 0, NULL, &result) ||
        result != OBJECT_TO_JSVAL(scope)) {
        fprintf(stderr, "FAIL object scope lost its implicit receiver\n");
        goto out;
    }
    if (!Evaluate(cx, global,
        "JSON.stringify({a:1})==='{\"a\":1}' && "
        "Object.prototype.toString.call(JSON)==='[object JSON]'")) goto out;
    if (!Evaluate(cx, global,
        "var boundForClone=(function(a,b){return this.x+a+b;}).bind({x:4},5); true") ||
        !JS_GetProperty(cx, global, "boundForClone", &result)) goto out;
    clone = JS_CloneFunctionObject(cx, JSVAL_TO_OBJECT(result), global);
    if (!clone || !JS_DefineProperty(cx, global, "clonedBound",
                                      OBJECT_TO_JSVAL(clone), NULL, NULL, 0)) goto out;
    JS_GC(cx);
    if (!Evaluate(cx, global, "clonedBound(6)===15 && clonedBound.length===1")) goto out;
    protectedObject = JS_NewObject(cx, NULL, NULL, global);
    if (!protectedObject ||
        !JS_DefineProperty(cx, global, "protectedObject",
                           OBJECT_TO_JSVAL(protectedObject), NULL, NULL, 0))
        goto out;
    if (!JS_DefineProperty(cx, protectedObject, "x", INT_TO_JSVAL(1),
                           NULL, NULL, JSPROP_ENUMERATE)) goto out;
    JS_SetCheckObjectAccessCallback(rt, CheckAccess);
    if (!Evaluate(cx, global,
        "var denied=false; try { Object.getPrototypeOf(protectedObject); }"
        "catch(e) { denied=String(e).indexOf('embedding denied access')>=0; } denied") ||
        !Evaluate(cx, global,
        "var denied=false; try { Object.getOwnPropertyDescriptor(protectedObject,'x'); }"
        "catch(e) { denied=String(e).indexOf('embedding denied access')>=0; } denied"))
        goto out;
    if (!Evaluate(cx, global,
        "var denied=false; try { Object.defineProperty(protectedObject,'x',{value:2}); }"
        "catch(e) { denied=String(e).indexOf('embedding denied access')>=0; } denied"))
        goto out;
    JS_SetCheckObjectAccessCallback(rt, NULL);
    if (!Evaluate(cx, global, "Object.getPrototypeOf(protectedObject)===Object.prototype"))
        goto out;
    if (!JS_SealObject(cx, protectedObject, JS_FALSE) ||
        !Evaluate(cx, global,
        "!Object.isExtensible(protectedObject) && "
        "Object.preventExtensions(protectedObject)===protectedObject")) goto out;
    if (!Evaluate(cx, global,
        "var undefined=7; var Infinity=9; var NaN=11;"
        "undefined===void 0 && Infinity===1/0 && NaN!==NaN")) goto out;
    JS_SetVersion(cx, JSVERSION_1_7);
    if (!Evaluate(cx, global, "const [legacyDestructured]=[12]; true") ||
        !Evaluate(cx, global,
        "var denied=false; try { eval('var legacyDestructured;'); }"
        "catch(e) { denied=e instanceof TypeError; } denied")) goto out;
    JS_SetVersion(cx, JSVERSION_DEFAULT);
    if (!Evaluate(cx, global,
        "var strictCloneSource=function(x){'use strict';x=9;return this===null && arguments[0]===1;};true") ||
        !JS_GetProperty(cx, global, "strictCloneSource", &result)) goto out;
    clone = JS_CloneFunctionObject(cx, JSVAL_TO_OBJECT(result), global);
    if (!clone || !JS_DefineProperty(cx, global, "strictClone",
                                      OBJECT_TO_JSVAL(clone), NULL, NULL, 0)) goto out;
    JS_GC(cx);
    if (!Evaluate(cx, global, "strictClone.call(null,1)")) goto out;
    script = JS_CompileScript(cx, global, literal, strlen(literal), "elisions", 1);
    encoder = JS_XDRNewMem(cx, JSXDR_ENCODE);
    decoder = JS_XDRNewMem(cx, JSXDR_DECODE);
    if (!script || !encoder || !decoder || !JS_XDRScript(encoder, &script)) goto out;
    {
        void *data = JS_XDRMemGetData(encoder, &length);
        JS_XDRMemSetData(decoder, data, length);
    }
    if (!JS_XDRScript(decoder, &decoded)) goto out;
    ++checks;
    if (!JS_ExecuteScript(cx, global, decoded, &result) || result != JSVAL_TRUE) {
        fprintf(stderr, "FAIL XDR strict function and sparse literal round trip\n");
        goto out;
    }
    status = 0;
    printf("ES5-EMBEDDING checks=%u failures=0\n", checks);
out:
    /* Decoder borrows the encoder's buffer; only the encoder frees it. */
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
