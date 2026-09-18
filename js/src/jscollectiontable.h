/* Ordered storage for ES2015 keyed collections; MPL 1.1/GPL 2.0/LGPL 2.1. */
#ifndef jscollectiontable_h___
#define jscollectiontable_h___
#include "jsapi.h"
JS_BEGIN_EXTERN_C
typedef struct JSCollectionData JSCollectionData;
/* Callers synchronize access. These operations never call user code or GC;
 * allocation failures are reported by the caller after releasing its lock. */
extern JSCollectionData *js_NewCollectionData(void);
extern void js_ReleaseCollectionData(JSCollectionData *data);
extern JSBool js_BeginCollectionIteration(JSCollectionData *data);
extern void js_EndCollectionIteration(JSCollectionData *data);
extern uint32 js_CollectionSize(JSCollectionData *data);
extern JSBool js_CollectionGet(JSCollectionData *data, jsval key, jsval *value);
extern JSBool js_CollectionPut(JSCollectionData *data, jsval key, jsval value);
extern JSBool js_CollectionDelete(JSCollectionData *data, jsval key);
extern void js_CollectionClear(JSCollectionData *data);
extern JSBool js_CollectionNext(JSCollectionData *data, uint32 *cursor,
                                jsval *key, jsval *value);
extern void js_MarkCollectionData(JSContext *cx, JSCollectionData *data);
/* Collector-only operations; weak tables have no public traversals. */
extern JSBool js_MarkWeakCollectionValues(JSContext *cx, JSCollectionData *data);
extern void js_SweepWeakCollectionKeys(JSContext *cx, JSCollectionData *data);
JS_END_EXTERN_C
#endif
