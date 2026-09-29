/* Native structured-value snapshots for embedding storage.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
#ifndef jsstructuredclone_h___
#define jsstructuredclone_h___
#include "jsapi.h"
JS_BEGIN_EXTERN_C
extern JSBool js_WriteStructuredValue(JSContext *, jsval,
                                      JSStructuredValue **, JSBool *);
extern JSBool js_ReadStructuredValue(JSContext *, JSObject *,
                                     const JSStructuredValue *, jsval *);
extern void js_FreeStructuredValue(JSStructuredValue *);
/* Own string/symbol keys in standard order, without a mutable Reflect lookup. */
extern JSIdArray *js_StructuredOwnKeys(JSContext *, JSObject *);
extern JSBool js_IsJSONNamespace(JSContext *, JSObject *);
JS_END_EXTERN_C
#endif
