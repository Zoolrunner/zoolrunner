/* Private binary-data implementation. MPL 1.1/GPL 2.0/LGPL 2.1. */
#ifndef jsbinarydata_h___
#define jsbinarydata_h___
#include "jspubtd.h"
JS_BEGIN_EXTERN_C
extern JSClass js_ArrayBufferClass;
extern JSClass js_DataViewClass;
extern JSObject *js_InitArrayBufferClass(JSContext *cx, JSObject *global);
extern JSObject *js_InitDataViewClass(JSContext *cx, JSObject *global);
extern JSBool js_ArrayBufferConstructor(JSContext *, JSObject *, uintN, jsval *, jsval *);
extern JSBool js_DataViewConstructor(JSContext *, JSObject *, uintN, jsval *, jsval *);
extern JSBool js_DetachArrayBuffer(JSContext *cx, JSObject *obj);
JS_END_EXTERN_C
#endif
