/* ES2015 Promise internals. MPL 1.1/GPL 2.0/LGPL 2.1. */
#ifndef jspromise_h___
#define jspromise_h___
#include "jspubtd.h"
JS_BEGIN_EXTERN_C
extern JSClass js_PromiseClass;
extern JSObject *js_InitPromiseClass(JSContext *cx, JSObject *global);
extern JSBool js_PromiseConstructor(JSContext *cx, JSObject *obj, uintN argc,
                                    jsval *argv, jsval *rval);
JS_END_EXTERN_C
#endif
