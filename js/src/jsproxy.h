/* ES2015 Proxy private dispatch. MPL 1.1/GPL 2.0/LGPL 2.1. */
#ifndef jsproxy_h___
#define jsproxy_h___
#include "jspubtd.h"
JS_BEGIN_EXTERN_C
extern JSBool js_ProxyTailCall(JSContext *, JSObject *, jsval, uintN, jsval *, jsval *);
extern JSClass js_ProxyClass;
extern JSObject *js_NewProxyObject(JSContext *, jsval, jsval, JSObject *);
extern JSObject *js_ProxyOperationGlobal(JSContext *cx);
extern JSObject *js_InitProxyClass(JSContext *cx, JSObject *global);
extern JSBool js_IsProxy(JSContext *cx, JSObject *obj);
extern JSBool js_ProxyConstructor(JSContext *cx, JSObject *obj, uintN argc,
                                  jsval *argv, jsval *rval);
extern JSBool js_ProxyGet(JSContext *cx, JSObject *obj, jsid id,
                         jsval receiver, jsval *rval);
extern JSBool js_ProxySet(JSContext *cx, JSObject *obj, jsid id, jsval value,
                         jsval receiver, JSBool *accepted);
extern JSBool js_ProxyHas(JSContext *cx, JSObject *obj, jsid id, JSBool *found);
extern JSBool js_ProxyDelete(JSContext *cx, JSObject *obj, jsid id, JSBool *accepted);
extern JSBool js_ProxyGetOwnDescriptor(JSContext *cx, JSObject *obj, jsid id,
                                      JSObject *global, jsval *rval);
extern JSBool js_ProxyDefineOwn(JSContext *cx, JSObject *obj, jsid id,
                                jsval descriptor, JSBool *accepted);
extern JSBool js_ProxyGetPrototype(JSContext *cx, JSObject *obj, JSObject **proto);
extern JSBool js_ProxySetPrototype(JSContext *cx, JSObject *obj, JSObject *proto,
                                   JSBool *accepted);
extern JSBool js_ProxyIsExtensible(JSContext *cx, JSObject *obj, JSBool *result);
extern JSBool js_ProxyPreventExtensions(JSContext *cx, JSObject *obj, JSBool *result);
extern JSIdArray *js_ProxyOwnKeys(JSContext *cx, JSObject *obj);
extern JSBool js_ProxyEnumerate(JSContext *cx, JSObject *obj, jsval *rval);
extern JSBool js_ProxyIteratorNext(JSContext *cx, jsval iterator, JSBool *done, jsval *rval);
extern JSBool js_ProxyCall(JSContext *cx, JSObject *obj, jsval receiver,
                          uintN argc, jsval *argv, jsval *rval);
extern JSBool js_ProxyConstruct(JSContext *cx, JSObject *obj, uintN argc,
                               jsval *argv, JSObject *newTarget, jsval *rval);
extern JSBool js_ProxyTarget(JSContext *cx, JSObject *obj, JSObject **target);
extern JSBool js_ProxyLookupForAccess(JSContext *cx, JSObject *obj, jsid id,
                                      JSObject **owner, JSProperty **property);
/* Internal descriptor records shared with the ES5 native implementation. */
extern JSBool js_ConvertProxyDescriptor(JSContext *cx, jsval value,
                                       JSBool complete, JSBool ownOnly,
                                       JSObject *global, jsval *rval);
extern JSBool js_CompatibleProxyDescriptor(JSContext *cx, JSBool extensible,
                                          jsval descriptor, jsval current, JSBool *compatible);
JS_END_EXTERN_C
#endif
