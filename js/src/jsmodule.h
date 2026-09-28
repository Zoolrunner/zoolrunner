/* Native ES2015 module records. MPL 1.1/GPL 2.0/LGPL 2.1. */
#ifndef jsmodule_h___
#define jsmodule_h___
#include "jsprvtd.h"
JS_BEGIN_EXTERN_C
extern JSClass js_ModuleClass;
#define JS_MODULE_LOCAL ((uint32)-1)
extern JSBool js_AddModuleRequest(JSContext *, JSObject *, JSAtom *, uint32 *);
extern JSBool js_AddModuleImport(JSContext *, JSObject *, uint32, JSAtom *, JSAtom *);
extern JSBool js_AddModuleExport(JSContext *, JSObject *, JSAtom *, JSAtom *, uint32, JSAtom *);
extern JSBool js_ValidateModuleExports(JSContext *, JSObject *, JSTreeContext *);
extern JSObject *js_CompileModule(JSContext *cx, JSObject *global, JSPrincipals *principals,
                                  const jschar *source, size_t length,
                                  const char *filename, uintN line);
extern JSObject *js_GetModuleRequests(JSContext *, JSObject *);
extern JSBool js_SetModuleDependency(JSContext *, JSObject *, JSString *, JSObject *);
extern JSObject *js_GetModuleNamespace(JSContext *, JSObject *);
extern JSObject *js_ModuleEnvironment(JSContext *cx, JSObject *module);
extern JSBool js_InstantiateModule(JSContext *cx, JSObject *module);
extern JSBool js_EvaluateModule(JSContext *cx, JSObject *module);
JS_END_EXTERN_C
#endif
