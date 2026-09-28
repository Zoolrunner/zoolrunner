/* Private intrinsic cache for classic embedding globals.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
#ifndef jsrealm_h___
#define jsrealm_h___
#include "jspubtd.h"
JS_BEGIN_EXTERN_C

extern JSBool js_IsModernGlobal(JSContext *cx, JSObject *global);
typedef enum JSRealmIntrinsic {
    JS_INTRINSIC_ITERATOR_PROTO,
    JS_INTRINSIC_ARRAY_ITERATOR_PROTO,
    JS_INTRINSIC_STRING_ITERATOR_PROTO,
    JS_INTRINSIC_ARRAY_VALUES,
    JS_INTRINSIC_MAP_ITERATOR_PROTO,
    JS_INTRINSIC_SET_ITERATOR_PROTO,
    JS_INTRINSIC_ENUMERATOR_PROTO,
    JS_INTRINSIC_TEMPLATE_REGISTRY,
    JS_INTRINSIC_GENERATOR_PROTO,
    JS_INTRINSIC_GENERATOR_FUNCTION_PROTO,
    JS_INTRINSIC_GENERATOR_CONSTRUCTOR,
    JS_INTRINSIC_TYPED_ARRAY_PROTO,
    JS_INTRINSIC_TYPED_ARRAY_CONSTRUCTOR,
    JS_INTRINSIC_GLOBAL_LEXICAL,
    JS_INTRINSIC_LIMIT
} JSRealmIntrinsic;
extern JSObject *js_GetCachedIntrinsic(JSContext *cx, JSObject *global,
                                       JSRealmIntrinsic key);
extern JSBool js_CacheIntrinsic(JSContext *cx, JSObject *global,
                                JSRealmIntrinsic key, JSObject *value);
extern JSObject *js_GetCachedClassObject(JSContext *cx, JSObject *global,
                                         JSProtoKey key);
extern JSBool js_CacheClassObject(JSContext *cx, JSObject *global,
                                  JSProtoKey key, JSObject *constructor);
extern JSObject *js_GlobalLexicalEnvironment(JSContext *cx, JSObject *global,
                                             JSBool create);
extern JSObject *js_NewLexicalEnvironment(JSContext *cx, JSObject *outer);
extern JSBool js_IsLexicalEnvironment(JSContext *cx, JSObject *obj);
extern JSBool js_IsGlobalLexicalEnvironment(JSContext *cx, JSObject *obj);
extern JSBool js_RecordGlobalVarBinding(JSContext *cx, JSObject *env, jsid id);
extern JSBool js_ForgetGlobalVarBinding(JSContext *cx, JSObject *env, jsid id);
extern JSBool js_CanDeclareGlobalLexicalBinding(JSContext *cx, JSObject *env, jsid id);
extern JSBool js_DefineLexicalBinding(JSContext *cx, JSObject *env,
                                            jsid id, JSBool immutable);
extern JSBool js_InitializeLexicalBinding(JSContext *cx, JSObject *env,
                                                jsid id, jsval value);
extern void js_MarkCachedClassObjects(JSContext *cx, JSObject *global);
extern void js_SweepCachedClassObjects(JSRuntime *rt);
extern void js_ClearCachedClassObjects(JSContext *cx, JSObject *global);
extern void js_FinishCachedClassObjects(JSRuntime *rt);
JS_END_EXTERN_C
#endif
