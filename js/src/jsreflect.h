/* ES2015 Reflect; MPL 1.1/GPL 2.0/LGPL 2.1. */
#ifndef jsreflect_h___
#define jsreflect_h___
#include "jspubtd.h"
JS_BEGIN_EXTERN_C
extern JSClass js_ReflectClass;
extern JSObject *js_InitReflectClass(JSContext *cx, JSObject *global);
#define REFLECT_NATIVE(name) \
extern JSBool name(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
REFLECT_NATIVE(js_ReflectDefineProperty);
REFLECT_NATIVE(js_ReflectDeleteProperty);
REFLECT_NATIVE(js_ReflectEnumerate);
REFLECT_NATIVE(js_ReflectGetOwnPropertyDescriptor);
REFLECT_NATIVE(js_ReflectGetPrototypeOf);
REFLECT_NATIVE(js_ReflectOwnKeys);
REFLECT_NATIVE(js_ReflectIsExtensible);
REFLECT_NATIVE(js_ReflectPreventExtensions);
REFLECT_NATIVE(js_ReflectSetPrototypeOf);
REFLECT_NATIVE(js_ReflectSet);
#undef REFLECT_NATIVE
JS_END_EXTERN_C
#endif
