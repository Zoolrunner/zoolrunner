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
extern JSClass js_Int8ArrayClass;
extern JSObject *js_InitInt8ArrayClass(JSContext *, JSObject *);
extern JSClass js_Uint8ArrayClass;
extern JSObject *js_InitUint8ArrayClass(JSContext *, JSObject *);
extern JSClass js_Uint8ClampedArrayClass;
extern JSObject *js_InitUint8ClampedArrayClass(JSContext *, JSObject *);
extern JSClass js_Int16ArrayClass;
extern JSObject *js_InitInt16ArrayClass(JSContext *, JSObject *);
extern JSClass js_Uint16ArrayClass;
extern JSObject *js_InitUint16ArrayClass(JSContext *, JSObject *);
extern JSClass js_Int32ArrayClass;
extern JSObject *js_InitInt32ArrayClass(JSContext *, JSObject *);
extern JSClass js_Uint32ArrayClass;
extern JSObject *js_InitUint32ArrayClass(JSContext *, JSObject *);
extern JSClass js_Float32ArrayClass;
extern JSObject *js_InitFloat32ArrayClass(JSContext *, JSObject *);
extern JSClass js_Float64ArrayClass;
extern JSObject *js_InitFloat64ArrayClass(JSContext *, JSObject *);
extern JSBool js_IsTypedArray(JSContext *, JSObject *);
extern JSBool js_InitTypedArrayObject(JSContext *, JSObject *);
extern JSObject *js_TypedArrayExpando(JSContext *, JSObject *);
extern JSBool js_TypedArrayLength(JSContext *, JSObject *, jsdouble *);
extern jsuint js_TypedArrayRawLength(JSContext *, JSObject *);
extern JSBool js_TypedArrayIndex(JSContext *, jsid, JSBool *, jsdouble *);
extern JSBool js_TypedArrayIndexValid(JSContext *, JSObject *, jsdouble);
extern JSBool js_TypedArrayWrite(JSContext *, JSObject *, jsdouble, jsval, JSBool *);
extern JSBool js_TypedArrayGet(JSContext *, JSObject *, jsid, jsval, jsval *);
extern JSBool js_IsTypedArrayConstructor(JSNative);
extern JSBool js_TypedArraySet(JSContext *, JSObject *, jsid, jsval, jsval, JSBool *);
/* Internal structured-storage view; borrowed pointers are valid only while the
 * source is rooted and no script, allocation hook or GC callback is invoked. */
typedef struct JSStructuredBinary {
    const unsigned char *bytes;
    JSObject *buffer;
    size_t offset, length;
    int kind; /* -1 DataView, otherwise typedClasses index */
} JSStructuredBinary;
extern int js_StructuredBinaryInfo(JSContext *, JSObject *, JSStructuredBinary *);
extern JSObject *js_ReadStructuredBuffer(JSContext *, JSObject *,
                                         const unsigned char *, size_t);
extern JSObject *js_ReadStructuredView(JSContext *, JSObject *, JSObject *,
                                       size_t, size_t, int);
JS_END_EXTERN_C
#endif
