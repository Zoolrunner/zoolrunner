/* Private intrinsic cache for classic embedding globals.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
#ifndef jsrealm_h___
#define jsrealm_h___
#include "jspubtd.h"
JS_BEGIN_EXTERN_C
extern JSObject *js_GetCachedClassObject(JSContext *cx, JSObject *global,
                                         JSProtoKey key);
extern JSBool js_CacheClassObject(JSContext *cx, JSObject *global,
                                  JSProtoKey key, JSObject *constructor);
extern void js_MarkCachedClassObjects(JSContext *cx, JSObject *global);
extern void js_SweepCachedClassObjects(JSRuntime *rt);
extern void js_ClearCachedClassObjects(JSContext *cx, JSObject *global);
extern void js_FinishCachedClassObjects(JSRuntime *rt);
JS_END_EXTERN_C
#endif
