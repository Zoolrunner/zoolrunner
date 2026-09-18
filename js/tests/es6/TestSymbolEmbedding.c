/* Symbol primitives through the classic JSAPI; MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "jsapi.h"
#include <stdio.h>
#include <string.h>
static JSClass globalClass = {
    "global", 0,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};
static JSClass reservedGlobalClass = {
    "reservedGlobal", JSCLASS_IS_GLOBAL | JSCLASS_HAS_RESERVED_SLOTS(31),
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};
static unsigned checks;
static JSBool Evaluate(JSContext *cx, JSObject *global, const char *source)
{
    jsval result;
    return JS_EvaluateScript(cx, global, source, strlen(source),
                             "symbol-embedding", 1, &result) && result == JSVAL_TRUE;
}
static void Error(JSContext *cx, const char *message, JSErrorReport *report)
{
    fprintf(stderr, "Symbol embedding: %s\n", message);
}
#define CHECK(c) do { ++checks; if (!(c)) { fprintf(stderr,"FAIL Symbol embedding %u\n",checks); goto out; } } while (0)
int main(void)
{
    JSRuntime *rt = JS_NewRuntime(8 * 1024 * 1024);
    JSContext *cx, *other = NULL;
    JSObject *global, *second;
    jsval value, converted;
    JSString *text;
    int status = 1;
    if (!rt) return 1;
    cx = JS_NewContext(rt, 8192);
    if (!cx) { JS_DestroyRuntime(rt); return 1; }
    JS_BeginRequest(cx);
    JS_SetErrorReporter(cx, Error);
    JS_SetVersion(cx, JSVERSION_ECMA_2015);
    global = JS_NewObject(cx, &globalClass, NULL, NULL);
    CHECK(global);
    JS_SetGlobalObject(cx, global);
    CHECK(JS_InitStandardClasses(cx, global));
    CHECK(Evaluate(cx, global, "var s=Symbol('x');typeof s==='symbol'"));
    CHECK(JS_GetProperty(cx, global, "s", &value));
    CHECK(JS_IsSymbolValue(value) && !JSVAL_IS_STRING(value) &&
          JSVAL_IS_PRIMITIVE(value) && JS_TypeOfValue(cx, value) == JSTYPE_SYMBOL);
    CHECK(Evaluate(cx, global, "s===s&&s!==Symbol('x')&&s!=Symbol('x')&&s!='Symbol(x)'&&s!=1&&s!=true"));
    CHECK(Evaluate(cx, global, "String(s)==='Symbol(x)'&&String(Symbol())==='Symbol()'&&String(Symbol(''))==='Symbol()'"));
    CHECK(Evaluate(cx, global, "Boolean(s)===true&&!!s&&s.valueOf()===s&&s.toString()==='Symbol(x)'"));
    CHECK(Evaluate(cx, global, "var box=Object(s);typeof box==='object'&&box.valueOf()===s&&box==s&&box!==s"));
    CHECK(Evaluate(cx, global, "var o={};o[s]=42;o[s]===42&&s in o&&o.hasOwnProperty(s)"));
    CHECK(Evaluate(cx, global, "var p={};p[box]=13;p[s]===13"));
    CHECK(Evaluate(cx, global, "var caught=0;try{Number(s);}catch(e){if(e instanceof TypeError)++caught;}try{''+s;}catch(e){if(e instanceof TypeError)++caught;}try{new String(s);}catch(e){if(e instanceof TypeError)++caught;}try{new Symbol();}catch(e){if(e instanceof TypeError)++caught;}caught===4"));
    CHECK(Evaluate(cx, global, "var r=Symbol.for('registered');r===Symbol.for('registered')&&Symbol.keyFor(r)==='registered'&&Symbol.keyFor(s)===undefined"));
    CHECK(Evaluate(cx, global, "var hints=[],v={};v[Symbol.toPrimitive]=function(h){hints.push(h);return 7;};String(v)==='7'"));
    CHECK(Evaluate(cx, global, "Number(v)===7"));
    CHECK(Evaluate(cx, global, "v+1===8"));
    CHECK(Evaluate(cx, global, "hints.join()==='string,number,default'"));
    CHECK(Evaluate(cx, global, "Object.keys(o).length===0&&Object.getOwnPropertyNames(o).length===0&&Object.getOwnPropertySymbols(o)[0]===s"));
    CHECK(Evaluate(cx, global, "Object.getOwnPropertyDescriptor(o,s).value===42&&o.hasOwnProperty(box)"));
    CHECK(Evaluate(cx, global, "var q={};Object.defineProperty(q,box,{value:8,enumerable:true});q[s]===8&&Object.assign({},q)[s]===8"));
    CHECK(Evaluate(cx, global, "JSON.stringify(s)===undefined&&JSON.stringify([s])==='[null]'&&JSON.stringify(o)==='{}'&&JSON.stringify(box)==='{}'"));
    CHECK(Evaluate(cx, global, "var tagged={};tagged[Symbol.toStringTag]='Custom';Object.prototype.toString.call(tagged)==='[object Custom]'&&Object.prototype.toString.call(s)==='[object Symbol]'"));
    JS_GC(cx);
    CHECK(Evaluate(cx, global, "r===Symbol.for('registered')&&Symbol.keyFor(r)==='registered'&&o[s]===42&&p[s]===13"));
    JS_SetVersion(cx, JSVERSION_1_7);
    CHECK(Evaluate(cx, global, "typeof Symbol('legacy')==='symbol'&&Object(s).valueOf()===s"));
    CHECK(JS_GetProperty(cx, global, "s", &value));
    CHECK(JS_ConvertValue(cx, value, JSTYPE_SYMBOL, &converted) && converted == value);
    text = JS_NewDependentString(cx, JSVAL_TO_STRING(value), 0,
                                 JS_GetStringLength(JSVAL_TO_STRING(value)));
    CHECK(text && JSVAL_IS_STRING(STRING_TO_JSVAL(text)) &&
          !strcmp(JS_GetStringBytes(text), "Symbol(x)"));
    text = JS_ConcatStrings(cx, JSVAL_TO_STRING(value), JSVAL_TO_STRING(JS_GetEmptyStringValue(cx)));
    CHECK(text && JSVAL_IS_STRING(STRING_TO_JSVAL(text)) &&
          !strcmp(JS_GetStringBytes(text), "Symbol(x)"));
    other = JS_NewContext(rt, 8192);
    CHECK(other);
    JS_BeginRequest(other);
    JS_SetVersion(other, JSVERSION_ECMA_2015);
    second = JS_NewObject(other, &reservedGlobalClass, NULL, NULL);
    CHECK(second);
    JS_SetGlobalObject(other, second);
    CHECK(JS_InitStandardClasses(other, second));
    CHECK(JS_GetProperty(cx, global, "r", &value));
    CHECK(JS_DefineProperty(other, second, "shared", value, NULL, NULL, 0));
    CHECK(Evaluate(other, second, "shared===Symbol.for('registered')"));
    JS_ClearScope(other, second);
    CHECK(!JS_IsExceptionPending(other) && JS_InitStandardClasses(other, second));
    CHECK(JS_DefineProperty(other, second, "shared", value, NULL, NULL, 0));
    CHECK(Evaluate(other, second, "shared===Symbol.for('registered')"));
    JS_EndRequest(other);
    JS_DestroyContextNoGC(other); other = NULL;
    CHECK(JS_AddNamedRoot(cx, &value, "registry interval test"));
    JS_EndRequest(cx);
    JS_DestroyContext(cx); cx = NULL;
    cx = JS_NewContext(rt, 8192);
    if (!cx) { JS_DestroyRuntime(rt); return 1; }
    JS_BeginRequest(cx);
    JS_SetVersion(cx, JSVERSION_ECMA_2015);
    global = JS_NewObject(cx, &globalClass, NULL, NULL);
    CHECK(global);
    JS_SetGlobalObject(cx, global);
    CHECK(JS_InitStandardClasses(cx, global));
    CHECK(JS_DefineProperty(cx, global, "saved", value, NULL, NULL, 0));
    JS_RemoveRoot(cx, &value);
    CHECK(Evaluate(cx, global, "saved===Symbol.for('registered')"));
    printf("ES6-SYMBOL-EMBEDDING checks=%u failures=0\n", checks);
    status = 0;
  out:
    if (other) { JS_EndRequest(other); JS_DestroyContextNoGC(other); }
    JS_EndRequest(cx);
    JS_DestroyContext(cx);
    JS_DestroyRuntime(rt);
    JS_ShutDown();
    return status;
}
