/* Modern iterators alongside the classic Iterator/StopIteration interfaces.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
#ifndef jsiteres6_h___
#define jsiteres6_h___
#include "jspubtd.h"
JS_BEGIN_EXTERN_C
extern JSBool js_InitArrayIteratorMethods(JSContext *cx, JSObject *global, JSObject *proto);
extern JSBool js_InitStringIteratorMethod(JSContext *cx, JSObject *global, JSObject *proto);
extern JSBool js_InitArgumentsIterator(JSContext *cx, JSObject *global, JSObject *args);
extern JSObject *js_BuiltinToObject(JSContext *cx, JSObject *global, jsval value);
extern JSObject *js_BuiltinGlobal(JSContext *cx, jsval *argv);
extern JSObject *js_BuiltinPrototype(JSContext *cx, JSObject *global, JSProtoKey key);
extern JSObject *js_GetIteratorPrototype(JSContext *cx, JSObject *global);
extern JSBool js_IteratorResult(JSContext *cx, JSObject *global, jsval value,
                                JSBool done, jsval *rval);
extern void js_IteratorCloseThrow(JSContext *cx, JSObject *iterator);
extern JSObject *js_ForOfStart(JSContext *cx, jsval value);
extern JSObject *js_PatternStart(JSContext *cx, jsval value);
extern JSBool js_PatternRest(JSContext *cx, JSObject *state, jsval *value);
extern JSBool js_PatternStep(JSContext *cx, JSObject *state, JSBool readValue, jsval *value);
extern JSBool js_ForOfNext(JSContext *cx, JSObject *state, JSBool *more);
extern JSBool js_ForOfClose(JSContext *cx, JSObject *state, JSBool throwing);
extern JSBool js_CreateArrayIterator(JSContext *, jsval *, jsval *, uintN);
JS_END_EXTERN_C
#endif
