/* RegExp prototype realms, native construction, XDR and legacy representations.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "jsapi.h"
#include "jsxdrapi.h"
#include "jsdbgapi.h"
#include <stdio.h>
#include <string.h>
static unsigned checks, finalized;
static unsigned poisoned;
static void PoisonRegExp(JSContext *cx, JSObject *obj, JSBool isNew, void *data)
{
    if (isNew && !strcmp(JS_GET_CLASS(cx, obj)->name, "RegExp") &&
        JS_DefineProperty(cx, obj, "lastIndex", JSVAL_ZERO, NULL, NULL,
                          JSPROP_READONLY | JSPROP_PERMANENT))
        ++poisoned;
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
    return JS_EvaluateScript(cx, global, source, strlen(source), "regexp-fields", 1, &result) && result == JSVAL_TRUE;
}
static JSBool Resolve(JSContext *cx, JSObject *global, const char *name)
{
    JSString *str = JS_InternString(cx, name);
    JSBool resolved = JS_FALSE;
    return str && JS_ResolveStandardClass(cx, global, STRING_TO_JSVAL(str), &resolved) && resolved;
}
#define CHECK(c) do { ++checks; if (!(c)) { fprintf(stderr, "FAIL RegExp fields embedding check %u\n", checks); goto out; } } while (0)
int main(void)
{
    JSRuntime *rt = JS_NewRuntime(16 * 1024 * 1024);
    JSContext *cx, *other = NULL;
    JSObject *global, *foreign, *pattern, *clone;
    jsval value, result;
    jschar source[] = {'/', 0x2028};
    const char *program = "var r=/a/g; r.lastIndex=2; r.source==='a' && r.flags==='g' && "
                          "Object.getOwnPropertyDescriptor(r,'lastIndex').value===2 && !r.hasOwnProperty('source')";
    JSScript *script = NULL, *decoded = NULL;
    JSXDRState *encoder = NULL, *decoder = NULL;
    void *data;
    uint32 length;
    int status = 1;
    if (!rt) return 1;
    cx = JS_NewContext(rt, 8192);
    if (!cx) { JS_DestroyRuntime(rt); return 1; }
    JS_BeginRequest(cx); JS_SetVersion(cx, JSVERSION_ECMA_2015);
    global = JS_NewObject(cx, &globalClass, NULL, NULL); CHECK(global);
    JS_SetGlobalObject(cx, global); CHECK(JS_InitStandardClasses(cx, global));
    pattern = JS_NewUCRegExpObject(cx, source, 2, JSREG_GLOB); CHECK(pattern);
    CHECK(JS_DefineProperty(cx, global, "nativePattern", OBJECT_TO_JSVAL(pattern), NULL, NULL, 0));
    CHECK(Evaluate(cx, global, "nativePattern.source==='\\\\/\\\\u2028' && nativePattern.flags==='g' && nativePattern.test('/\\u2028')"));
    CHECK(Evaluate(cx, global, "Object.getOwnPropertyDescriptor(nativePattern,'lastIndex').value===2"));
    CHECK(Evaluate(cx, global, "var sourceGetter=Object.getOwnPropertyDescriptor(RegExp.prototype,'source').get;true"));
    CHECK(JS_GetProperty(cx, global, "sourceGetter", &value));
    clone = JS_CloneFunctionObject(cx, JSVAL_TO_OBJECT(value), global); CHECK(clone);
    CHECK(JS_DefineProperty(cx, global, "clonedGetter", OBJECT_TO_JSVAL(clone), NULL, NULL, 0));
    CHECK(Evaluate(cx, global, "clonedGetter.call(/x/)==='x' && "
                   "(function(){try{clonedGetter.call(RegExp.prototype);}catch(e){return e.name==='TypeError';}return false;})()"));
    other = JS_NewContext(rt, 8192); CHECK(other);
    JS_BeginRequest(other); JS_SetVersion(other, JSVERSION_ECMA_2015);
    foreign = JS_NewObject(other, &globalClass, NULL, NULL); CHECK(foreign);
    JS_SetGlobalObject(other, foreign); CHECK(JS_InitStandardClasses(other, foreign));
    CHECK(Evaluate(other, foreign, "var exports={proto:RegExp.prototype,regex:/foreign/i,"
                   "source:Object.getOwnPropertyDescriptor(RegExp.prototype,'source').get,"
                   "global:Object.getOwnPropertyDescriptor(RegExp.prototype,'global').get};true"));
    CHECK(JS_GetProperty(other, foreign, "exports", &value));
    CHECK(JS_DefineProperty(cx, global, "foreign", value, NULL, NULL, 0));
    JS_EndRequest(other); JS_DestroyContextNoGC(other); other = NULL;
    JS_ClearNewbornRoots(cx); JS_GC(cx); CHECK(finalized == 0);
    CHECK(Evaluate(cx, global, "(function(){var n=0;try{foreign.source.call(foreign.proto);}catch(e){if(e.name==='TypeError')++n;}"
                   "try{foreign.global.call(foreign.proto);}catch(e){if(e.name==='TypeError')++n;}return n===2;})()"));
    CHECK(Evaluate(cx, global, "sourceGetter.call(foreign.regex)==='foreign' && foreign.source.call(/local/)==='local'"));
    CHECK(Evaluate(cx, global, "(function(){try{foreign.source.call(RegExp.prototype);}catch(e){return e.name==='TypeError';}return false;})()"));
    CHECK(Evaluate(cx, global, "delete this.foreign;true"));
    JS_ClearNewbornRoots(cx); JS_GC(cx); CHECK(finalized == 1);
    script = JS_CompileScript(cx, global, program, strlen(program), "regexp-fields-xdr", 1); CHECK(script);
    encoder = JS_XDRNewMem(cx, JSXDR_ENCODE); decoder = JS_XDRNewMem(cx, JSXDR_DECODE);
    CHECK(encoder && decoder && JS_XDRScript(encoder, &script));
    data = JS_XDRMemGetData(encoder, &length); JS_XDRMemSetData(decoder, data, length);
    CHECK(Evaluate(cx, global, "Object.defineProperty(RegExp.prototype,'lastIndex',{value:99,configurable:true});true"));
    JS_SetVersion(cx, JSVERSION_1_7);
    CHECK(JS_XDRScript(decoder, &decoded));
    CHECK(JS_ExecuteScript(cx, global, decoded, &result) && result == JSVAL_TRUE);
    CHECK(JS_GetVersion(cx) == JSVERSION_1_7);
    JS_ClearScope(cx, global); CHECK(JS_InitStandardClasses(cx, global));
    CHECK(Evaluate(cx, global, "RegExp.prototype.source==='' && /a/.hasOwnProperty('source') && RegExp.prototype.exec('')[0]===''"));
    pattern = JS_NewUCRegExpObject(cx, source, 2, JSREG_GLOB); CHECK(pattern);
    CHECK(JS_LookupProperty(cx, pattern, "lastIndex", &value) && value == JSVAL_TRUE);
    CHECK(JS_DefineProperty(cx, global, "legacyPattern", OBJECT_TO_JSVAL(pattern), NULL, NULL, 0));
    CHECK(Evaluate(cx, global, "legacyPattern.hasOwnProperty('source') && typeof Object.getOwnPropertyDescriptor(RegExp.prototype,'source').get==='undefined'"));
    CHECK(Evaluate(cx, global, "var legacy=/a/g;Object.setPrototypeOf(legacy,{value:2});"
                   "legacy.source==='a' && legacy.global===true && legacy.value===2 && "
                   "RegExp.prototype.exec.call(legacy,'a')[0]==='a' && legacy.lastIndex===1"));
    JS_ClearScope(cx, global);
    CHECK(Resolve(cx, global, "Object"));
    JS_SetVersion(cx, JSVERSION_ECMA_2015);
    CHECK(Resolve(cx, global, "RegExp") && JS_InitStandardClasses(cx, global));
    CHECK(Evaluate(cx, global, "var lazy=/lazy/g;lazy.lastIndex===0 && lazy.toString()==='/lazy/g' && "
                   "RegExp.prototype.source==='' && RegExp.prototype.exec('')[0]===''"));
    JS_ClearScope(cx, global); JS_SetVersion(cx, JSVERSION_1_7);
    CHECK(JS_InitStandardClasses(cx, global));
    CHECK(JS_SetObjectHook(rt, PoisonRegExp, NULL));
    JS_DestroyScript(cx, decoded); decoded = NULL;
    JS_XDRMemSetData(decoder, data, length);
    CHECK(!JS_XDRScript(decoder, &decoded) && poisoned == 1);
    JS_SetObjectHook(rt, NULL, NULL);
    JS_ClearPendingException(cx); JS_ClearNewbornRoots(cx); JS_GC(cx);
    CHECK(Evaluate(cx, global, "1+1===2"));
    JS_ClearScope(cx, global); JS_SetVersion(cx, JSVERSION_ECMA_2015);
    CHECK(Resolve(cx, global, "RegExp"));
    CHECK(Evaluate(cx, global, "typeof Object.getOwnPropertyDescriptor(RegExp.prototype,'source').get==='function' && "
                   "new RegExp('first','g').lastIndex===0"));
    printf("ES6-REGEXP-FIELDS-EMBEDDING checks=%u failures=0\n", checks); status = 0;
  out:
    JS_SetObjectHook(rt, NULL, NULL);
    if (decoder) { JS_XDRMemSetData(decoder, NULL, 0); JS_XDRDestroy(decoder); }
    if (encoder) JS_XDRDestroy(encoder);
    if (decoded) JS_DestroyScript(cx, decoded);
    if (script) JS_DestroyScript(cx, script);
    if (other) { JS_EndRequest(other); JS_DestroyContextNoGC(other); }
    JS_EndRequest(cx); JS_DestroyContext(cx); JS_DestroyRuntime(rt); JS_ShutDown();
    return status;
}
