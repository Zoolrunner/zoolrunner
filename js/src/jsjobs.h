/* Private per-thread ECMAScript job queue. MPL 1.1/GPL 2.0/LGPL 2.1. */
#ifndef jsjobs_h___
#define jsjobs_h___
#include "jspubtd.h"
typedef struct JSJobQueue {
    JSObject *head, *tail;
    JSBool draining, registered;
} JSJobQueue;
JS_BEGIN_EXTERN_C
extern void js_MarkJobs(JSContext *cx);
extern void js_ClearJobs(JSContext *cx);
JS_END_EXTERN_C
#endif
