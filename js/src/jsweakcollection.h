/* ES2015 weak keyed collections; MPL 1.1/GPL 2.0/LGPL 2.1. */
#ifndef jsweakcollection_h___
#define jsweakcollection_h___
#include "jspubtd.h"
JS_BEGIN_EXTERN_C
extern JSClass js_WeakMapClass, js_WeakSetClass;
extern JSObject *js_InitWeakMapClass(JSContext *cx, JSObject *global);
extern JSObject *js_InitWeakSetClass(JSContext *cx, JSObject *global);
extern JSBool js_MarkWeakCollections(JSContext *cx);
extern void js_SweepWeakCollections(JSContext *cx);
extern void js_FinishWeakCollections(JSRuntime *rt);
JS_END_EXTERN_C
#endif
