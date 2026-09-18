/* Native casing callbacks, foreign functions and legacy policy isolation.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "jsapi.h"
#include <stdio.h>
#include <string.h>
static unsigned checks, interrupted;
static JSBool stopOnBranch;
static JSClass globalClass = {
    "global", 0,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};
static JSBool Branch(JSContext *cx, JSScript *script)
{
    if (!script) { ++interrupted; JS_GC(cx); if (stopOnBranch) return JS_FALSE; }
    return JS_TRUE;
}
static JSBool Locale(JSContext *cx, JSString *source, jsval *rval)
{
    JSString *result;
    JS_GC(cx);
    result = JS_NewStringCopyZ(cx,"embedding locale");
    if (!result) return JS_FALSE;
    *rval = STRING_TO_JSVAL(result); return JS_TRUE;
}
static JSBool Evaluate(JSContext *cx, JSObject *global, const char *source)
{
    jsval value;
    return JS_EvaluateScript(cx,global,source,strlen(source),"casing",1,&value) && value == JSVAL_TRUE;
}
#define CHECK(c) do { ++checks; if (!(c)) { fprintf(stderr,"FAIL casing embedding check %u\n",checks); goto out; } } while (0)
int main(void)
{
    JSRuntime *rt = JS_NewRuntime(16*1024*1024);
    JSContext *cx, *other = NULL;
    JSObject *global, *foreign, *clone;
    jsval value, method;
    JSBool rooted = JS_FALSE;
    JSLocaleCallbacks callbacks;
    int status = 1;
    if (!rt) return 1;
    cx = JS_NewContext(rt,8192);
    if (!cx) { JS_DestroyRuntime(rt); return 1; }
    JS_BeginRequest(cx); JS_SetVersion(cx,JSVERSION_ECMA_2015);
    global = JS_NewObject(cx,&globalClass,NULL,NULL); CHECK(global);
    JS_SetGlobalObject(cx,global); CHECK(JS_InitStandardClasses(cx,global));
    CHECK(Evaluate(cx,global,"var text='A\\u03a3'+new Array(8193).join('\\u0301');true"));
    JS_SetBranchCallback(cx,Branch);
    CHECK(Evaluate(cx,global,"text.toLowerCase()==='a\\u03c2'+text.slice(2)"));
    CHECK(interrupted >= 8);
    stopOnBranch = JS_TRUE;
    CHECK(!Evaluate(cx,global,"text.toLowerCase();true"));
    stopOnBranch = JS_FALSE; JS_ClearPendingException(cx);
    CHECK(Evaluate(cx,global,"'\\u00df'.toUpperCase()==='SS'"));
    JS_SetBranchCallback(cx,NULL);
    memset(&callbacks,0,sizeof callbacks);
    callbacks.localeToLowerCase = Locale; callbacks.localeToUpperCase = Locale;
    JS_SetLocaleCallbacks(cx,&callbacks);
    CHECK(Evaluate(cx,global,"'X'.toLocaleLowerCase()==='embedding locale'&&'x'.toLocaleUpperCase()==='embedding locale'"));
    CHECK(Evaluate(cx,global,"'X'.toLowerCase()==='x'&&'\\u00df'.toUpperCase()==='SS'"));
    JS_SetLocaleCallbacks(cx,NULL);
    other = JS_NewContext(rt,8192); CHECK(other);
    JS_BeginRequest(other); JS_SetVersion(other,JSVERSION_ECMA_2015);
    foreign = JS_NewObject(other,&globalClass,NULL,NULL); CHECK(foreign);
    JS_SetGlobalObject(other,foreign); CHECK(JS_InitStandardClasses(other,foreign));
    CHECK(Evaluate(other,foreign,"var exports={upper:String.prototype.toUpperCase,lower:String.prototype.toLowerCase};true"));
    CHECK(JS_GetProperty(other,foreign,"exports",&value));
    CHECK(JS_DefineProperty(cx,global,"foreign",value,NULL,NULL,0));
    JS_EndRequest(other); JS_DestroyContextNoGC(other); other = NULL;
    JS_ClearNewbornRoots(cx); JS_GC(cx);
    CHECK(Evaluate(cx,global,"foreign.upper.call('\\u00df')==='SS'&&foreign.lower.call('A\\u03a3')==='a\\u03c2'"));
    CHECK(JS_GetProperty(cx,JSVAL_TO_OBJECT(value),"upper",&method));
    CHECK(JS_AddNamedRoot(cx,&method,"modern casing")); rooted = JS_TRUE;
    clone = JS_CloneFunctionObject(cx,JSVAL_TO_OBJECT(method),global); CHECK(clone);
    CHECK(JS_DefineProperty(cx,global,"clonedUpper",OBJECT_TO_JSVAL(clone),NULL,NULL,0));
    CHECK(Evaluate(cx,global,"clonedUpper.call('\\u00df')==='SS'"));
    JS_ClearScope(cx,global); JS_SetVersion(cx,JSVERSION_1_7);
    CHECK(JS_InitStandardClasses(cx,global));
    CHECK(Evaluate(cx,global,"'\\u00df'.toUpperCase()==='\\u00df'&&'A\\u03a3'.toLowerCase()==='a\\u03c3'"));
    clone = JS_CloneFunctionObject(cx,JSVAL_TO_OBJECT(method),global); CHECK(clone);
    CHECK(JS_DefineProperty(cx,global,"clonedModernUpper",OBJECT_TO_JSVAL(clone),NULL,NULL,0));
    CHECK(Evaluate(cx,global,"clonedModernUpper.call('\\u00df')==='SS'"));
    JS_RemoveRoot(cx,&method); rooted = JS_FALSE;
    printf("ES6-CASING-EMBEDDING checks=%u failures=0\n",checks); status = 0;
  out:
    JS_SetBranchCallback(cx,NULL); JS_SetLocaleCallbacks(cx,NULL);
    if (rooted) JS_RemoveRoot(cx,&method);
    if (other) { JS_EndRequest(other); JS_DestroyContextNoGC(other); }
    JS_EndRequest(cx); JS_DestroyContext(cx); JS_DestroyRuntime(rt); JS_ShutDown();
    return status;
}
