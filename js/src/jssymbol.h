/* ES2015 Symbol primitives and runtime identities.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
#ifndef jssymbol_h___
#define jssymbol_h___
#include "jsstr.h"
JS_BEGIN_EXTERN_C

typedef struct JSSymbol {
    JSString string;
    JSBool registered;
    JSBool hasDescription;
} JSSymbol;

typedef enum JSWellKnownSymbol {
    JS_WKS_HAS_INSTANCE,
    JS_WKS_IS_CONCAT_SPREADABLE,
    JS_WKS_ITERATOR,
    JS_WKS_MATCH,
    JS_WKS_REPLACE,
    JS_WKS_SEARCH,
    JS_WKS_SPECIES,
    JS_WKS_SPLIT,
    JS_WKS_TO_PRIMITIVE,
    JS_WKS_TO_STRING_TAG,
    JS_WKS_UNSCOPABLES,
    JS_WKS_LIMIT
} JSWellKnownSymbol;

extern JSClass js_SymbolClass;
extern JSObject *js_InitSymbolClass(JSContext *cx, JSObject *global);
extern JSSymbol *js_NewSymbol(JSContext *cx, JSString *description);
extern JSSymbol *js_GetWellKnownSymbol(JSContext *cx, JSWellKnownSymbol key);
extern JSBool js_WellKnownSymbolId(JSContext *cx, JSWellKnownSymbol key, jsid *idp);
extern JSBool js_DefineBuiltinTag(JSContext *cx, JSObject *obj, const char *name);
extern JSString *js_SymbolToString(JSContext *cx, JSSymbol *symbol);
extern JSObject *js_SymbolToObject(JSContext *cx, JSSymbol *symbol);
extern void js_MarkSymbolState(JSContext *cx);
extern void js_FinishSymbolState(JSRuntime *rt);

JS_END_EXTERN_C
#endif
