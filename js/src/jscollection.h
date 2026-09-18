/* ES2015 keyed collections; MPL 1.1/GPL 2.0/LGPL 2.1. */
#ifndef jscollection_h___
#define jscollection_h___
#include "jspubtd.h"
JS_BEGIN_EXTERN_C
extern JSClass js_MapClass, js_SetClass;
extern JSObject *js_InitMapClass(JSContext *cx, JSObject *global);
extern JSObject *js_InitSetClass(JSContext *cx, JSObject *global);
JS_END_EXTERN_C
#endif
