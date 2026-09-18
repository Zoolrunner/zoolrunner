/* Private Unicode normalization entry point. Form bits: compose=1, compat=2. */
#ifndef jsnormalization_h___
#define jsnormalization_h___
extern JSString *js_NormalizeString(JSContext *cx, JSString *source, uintN form);
#endif
