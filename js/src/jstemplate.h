/* ES2015 template objects. MPL 1.1/GPL 2.0/LGPL 2.1. */
#ifndef jstemplate_h___
#define jstemplate_h___
#include "jspubtd.h"
JS_BEGIN_EXTERN_C
extern JSBool js_GetTemplateObject(JSContext *cx, JSString *record, jsval *rval);
JS_END_EXTERN_C
#endif
