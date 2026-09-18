/* Modern iterators alongside the classic Iterator/StopIteration interfaces.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
#ifndef jsiteres6_h___
#define jsiteres6_h___
#include "jspubtd.h"
JS_BEGIN_EXTERN_C
extern JSBool js_InitArrayIteratorMethods(JSContext *cx, JSObject *global, JSObject *proto);
extern JSBool js_InitStringIteratorMethod(JSContext *cx, JSObject *global, JSObject *proto);
extern JSBool js_InitArgumentsIterator(JSContext *cx, JSObject *global, JSObject *args);
JS_END_EXTERN_C
#endif
