/* Native ES2015 module records. MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "jsapi.h"
#include "jscntxt.h"
#include "jsfun.h"
#include "jsemit.h"
#include "jsparse.h"
#include "jsproxy.h"
#include "jsreflect.h"
#include "jssymbol.h"
#include "jsnum.h"
#include <stdlib.h>
#include <string.h>
#include "jsgc.h"
#include "jsinterp.h"
#include "jsmodule.h"
#include "jsobj.h"
#include "jsopcode.h"
#include "jsrealm.h"
#include "jsscope.h"
#include "jsscript.h"
#include "jsstr.h"

typedef enum ModuleState {
    MODULE_UNLINKED, MODULE_LINKING, MODULE_LINKED,
    MODULE_EVALUATING, MODULE_EVALUATED, MODULE_FAILED
} ModuleState;

typedef struct ModuleRequest {
    JSAtom *specifier;
    JSObject *module;
} ModuleRequest;
typedef struct ModuleEntry {
    JSAtom *exportName, *localName, *importName;
    uint32 request;
    JSObject *resolvedModule;
    JSAtom *resolvedName;
} ModuleEntry;

typedef struct ModuleRecord {
    JSScript *script;
    JSObject *environment;
    ModuleState state;
    jsval exception;
    ModuleRequest *requests;
    ModuleEntry *imports, *exports;
    uint32 requestCount, importCount, exportCount;
    JSObject *namespaceObject;
    ModuleEntry *namespaceBindings;
    uint32 namespaceCount;
} ModuleRecord;

static uint32
ModuleMark(JSContext *cx, JSObject *obj, void *arg)
{
    ModuleRecord *record = (ModuleRecord *)JS_GetPrivate(cx, obj);
    uint32 i;
    ModuleEntry *entry;
    if (record) {
        if (record->script) js_MarkScript(cx, record->script);
        if (record->namespaceObject) GC_MARK(cx, record->namespaceObject, "module namespace");
        for (i = 0; i < record->namespaceCount; ++i) {
            entry = &record->namespaceBindings[i];
            GC_MARK_ATOM(cx, entry->exportName);
            GC_MARK_ATOM(cx, entry->resolvedName);
            GC_MARK(cx, entry->resolvedModule, "namespace export target");
        }
        for (i = 0; i < record->requestCount; ++i) {
            GC_MARK_ATOM(cx, record->requests[i].specifier);
            if (record->requests[i].module)
                GC_MARK(cx, record->requests[i].module, "module dependency");
        }
        for (i = 0; i < record->importCount + record->exportCount; ++i) {
            entry = i < record->importCount ? &record->imports[i]
                                            : &record->exports[i - record->importCount];
            if (entry->exportName) GC_MARK_ATOM(cx, entry->exportName);
            if (entry->localName) GC_MARK_ATOM(cx, entry->localName);
            if (entry->importName) GC_MARK_ATOM(cx, entry->importName);
            if (entry->resolvedName) GC_MARK_ATOM(cx, entry->resolvedName);
            if (entry->resolvedModule) GC_MARK(cx, entry->resolvedModule, "resolved module import");
        }
        if (record->environment) GC_MARK(cx, record->environment, "module environment");
        if (JSVAL_IS_GCTHING(record->exception))
            GC_MARK(cx, JSVAL_TO_GCTHING(record->exception), "module exception");
    }
    return 0;
}

static void
ModuleFinalize(JSContext *cx, JSObject *obj)
{
    ModuleRecord *record = (ModuleRecord *)JS_GetPrivate(cx, obj);
    if (record) {
        if (record->script) js_DestroyScript(cx, record->script);
        JS_free(cx, record->namespaceBindings);
        JS_free(cx, record->requests);
        JS_free(cx, record->imports);
        JS_free(cx, record->exports);
        JS_free(cx, record);
    }
}

JSClass js_ModuleClass = {
    "Module Record", JSCLASS_HAS_PRIVATE | JSCLASS_IS_ANONYMOUS |
                     JSCLASS_HAS_CACHED_PROTO(JSProto_Object),
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, ModuleFinalize,
    NULL, NULL, NULL, NULL, NULL, NULL, ModuleMark, NULL
};

static ModuleRecord *
GetModule(JSContext *cx, JSObject *obj)
{
    if (!obj || OBJ_GET_CLASS(cx, obj) != &js_ModuleClass) {
        JS_ReportError(cx, "expected a module record");
        return NULL;
    }
    return (ModuleRecord *)JS_GetPrivate(cx, obj);
}

static JSBool
ModuleSyntaxError(JSContext *cx)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_STRICT_SYNTAX);
    return JS_FALSE;
}

JSBool
js_AddModuleRequest(JSContext *cx, JSObject *module, JSAtom *specifier, uint32 *index)
{
    ModuleRecord *record = GetModule(cx, module);
    ModuleRequest *requests;
    uint32 i;
    if (!record) return JS_FALSE;
    for (i = 0; i < record->requestCount; ++i) {
        if (record->requests[i].specifier == specifier) { *index = i; return JS_TRUE; }
    }
    if (record->requestCount >= (size_t)-1 / sizeof(ModuleRequest) - 1 ||
        record->requestCount == (uint32)-1) {
        JS_ReportOutOfMemory(cx); return JS_FALSE;
    }
    requests = (ModuleRequest *)JS_realloc(cx, record->requests,
                    ((size_t)record->requestCount + 1) * sizeof *requests);
    if (!requests) return JS_FALSE;
    record->requests = requests;
    requests[i].specifier = specifier;
    requests[i].module = NULL;
    *index = i;
    ++record->requestCount;
    return JS_TRUE;
}

static JSBool
AddModuleEntry(JSContext *cx, ModuleEntry **entries, uint32 *count,
                 JSAtom *exportName, JSAtom *localName, uint32 request, JSAtom *importName)
{
    ModuleEntry *next;
    if (*count >= (size_t)-1 / sizeof(ModuleEntry) - 1 || *count == (uint32)-1) {
        JS_ReportOutOfMemory(cx); return JS_FALSE;
    }
    next = (ModuleEntry *)JS_realloc(cx, *entries, ((size_t)*count + 1) * sizeof *next);
    if (!next) return JS_FALSE;
    *entries = next;
    next += *count;
    memset(next, 0, sizeof *next);
    next->exportName = exportName;
    next->localName = localName;
    next->request = request;
    next->importName = importName;
    ++*count;
    return JS_TRUE;
}

JSBool
js_AddModuleImport(JSContext *cx, JSObject *module, uint32 request,
                    JSAtom *importName, JSAtom *localName)
{
    ModuleRecord *record = GetModule(cx, module);
    return record && AddModuleEntry(cx, &record->imports, &record->importCount,
                                    NULL, localName, request, importName);
}

JSBool
js_AddModuleExport(JSContext *cx, JSObject *module, JSAtom *exportName,
                    JSAtom *localName, uint32 request, JSAtom *importName)
{
    ModuleRecord *record = GetModule(cx, module);
    uint32 i;
    if (!record) return JS_FALSE;
    if (exportName) {
        for (i = 0; i < record->exportCount; ++i) {
            if (record->exports[i].exportName == exportName) return ModuleSyntaxError(cx);
        }
    }
    return AddModuleEntry(cx, &record->exports, &record->exportCount,
                           exportName, localName, request, importName);
}

JSBool
js_ValidateModuleExports(JSContext *cx, JSObject *module, JSTreeContext *tc)
{
    ModuleRecord *record = GetModule(cx, module);
    ModuleEntry *entry;
    JSAtomListElement *declared;
    uint32 i;
    if (!record) return JS_FALSE;
    for (i = 0; i < record->exportCount; ++i) {
        entry = &record->exports[i];
        if (entry->request != JS_MODULE_LOCAL) continue;
        ATOM_LIST_SEARCH(declared, &tc->lexicalDecls, entry->localName);
        if (!declared) ATOM_LIST_SEARCH(declared, &tc->varDecls, entry->localName);
        if (!declared) return ModuleSyntaxError(cx);
    }
    return JS_TRUE;
}

JSObject *
js_ModuleEnvironment(JSContext *cx, JSObject *module)
{
    ModuleRecord *record = GetModule(cx, module);
    return record ? record->environment : NULL;
}

JSObject *
js_CompileModule(JSContext *cx, JSObject *global, JSPrincipals *principals, const jschar *source,
                  size_t length, const char *filename, uintN line)
{
    JSObject *module, *outer;
    ModuleRecord *record;
    JSStackFrame frame, *savedFrame;
    JSTempValueRooter root;
    JSVersion savedVersion;
    JSBool ok = JS_FALSE;
    module = js_NewObject(cx, &js_ModuleClass, NULL, global);
    if (!module) return NULL;
    JS_PUSH_TEMP_ROOT_OBJECT(cx, module, &root);
    record = (ModuleRecord *)JS_malloc(cx, sizeof *record);
    if (!record) goto out;
    memset(record, 0, sizeof *record);
    record->exception = JSVAL_VOID;
    if (!JS_SetPrivate(cx, module, record)) { JS_free(cx, record); goto out; }
    outer = js_GlobalLexicalEnvironment(cx, global, JS_TRUE);
    if (!outer || !(record->environment = js_NewLexicalEnvironment(cx, outer))) goto out;
    memset(&frame, 0, sizeof frame);
    frame.scopeChain = frame.varobj = record->environment;
    frame.thisp = module; /* compiler-only handle, never a language this value */
    frame.flags = JSFRAME_MODULE;
    savedFrame = cx->fp;
    frame.down = savedFrame;
    cx->fp = &frame;
    savedVersion = JS_SetVersion(cx, JSVERSION_ECMA_2015);
    record->script = JS_CompileUCScriptForPrincipals(cx, record->environment, principals, source, length,
                                        filename, line);
    JS_SetVersion(cx, savedVersion);
    cx->fp = savedFrame;
    ok = record->script != NULL;
 out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok ? module : NULL;
}

/* A fresh array exposes dependency names without exposing mutable metadata. */
JSObject *
js_GetModuleRequests(JSContext *cx, JSObject *module)
{
    ModuleRecord *record = GetModule(cx, module);
    JSObject *array;
    uint32 i;
    jsval value;
    JSTempValueRooter moduleRoot, arrayRoot;
    JSBool ok = JS_TRUE;
    if (!record) return NULL;
    JS_PUSH_TEMP_ROOT_OBJECT(cx, module, &moduleRoot);
    array = JS_NewArrayObject(cx, 0, NULL);
    if (array) {
        JS_PUSH_TEMP_ROOT_OBJECT(cx, array, &arrayRoot);
        for (i = 0; i < record->requestCount && ok; ++i) {
            value = ATOM_KEY(record->requests[i].specifier);
            ok = JS_DefineElement(cx, array, i, value, NULL, NULL, JSPROP_ENUMERATE);
        }
        JS_POP_TEMP_ROOT(cx, &arrayRoot);
    }
    JS_POP_TEMP_ROOT(cx, &moduleRoot);
    return ok ? array : NULL;
}

/* Host resolution is explicit and stable for the lifetime of a linked graph. */
JSBool
js_SetModuleDependency(JSContext *cx, JSObject *module, JSString *specifier,
                         JSObject *dependency)
{
    ModuleRecord *record = GetModule(cx, module);
    uint32 i;
    if (!record || !GetModule(cx, dependency)) return JS_FALSE;
    if (record->state != MODULE_UNLINKED) {
        JS_ReportError(cx, "cannot replace a linked module dependency"); return JS_FALSE;
    }
    for (i = 0; i < record->requestCount; ++i) {
        if (js_EqualStrings(ATOM_TO_STRING(record->requests[i].specifier), specifier)) {
            record->requests[i].module = dependency;
            return JS_TRUE;
        }
    }
    JS_ReportError(cx, "module does not request this dependency");
    return JS_FALSE;
}

typedef struct ResolveEntry {
    JSObject *module;
    JSAtom *name;
    struct ResolveEntry *previous;
} ResolveEntry;

typedef struct ResolvedBinding {
    JSObject *module;
    JSAtom *name;
    JSBool ambiguous;
} ResolvedBinding;

static JSBool
ResolveExport(JSContext *cx, JSObject *module, JSAtom *name,
                ResolveEntry *previous, ResolvedBinding *result)
{
    ModuleRecord *record = GetModule(cx, module);
    ResolveEntry entry, *cursor;
    ModuleEntry *exportEntry, *importEntry;
    ResolvedBinding candidate;
    uint32 i, j;
    int stackDummy;
    if (!record) return JS_FALSE;
    if (!JS_CHECK_STACK_SIZE(cx, stackDummy)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_OVER_RECURSED);
        return JS_FALSE;
    }
    memset(result, 0, sizeof *result);
    for (cursor = previous; cursor; cursor = cursor->previous)
        if (cursor->module == module && cursor->name == name) return JS_TRUE;
    entry.module = module; entry.name = name; entry.previous = previous;
    for (i = 0; i < record->exportCount; ++i) {
        exportEntry = &record->exports[i];
        if (exportEntry->exportName != name) continue;
        if (exportEntry->request != JS_MODULE_LOCAL) {
            return ResolveExport(cx, record->requests[exportEntry->request].module,
                                  exportEntry->importName, &entry, result);
        }
        for (j = 0; j < record->importCount; ++j) {
            importEntry = &record->imports[j];
            if (importEntry->localName == exportEntry->localName && importEntry->importName)
                return ResolveExport(cx, record->requests[importEntry->request].module,
                                      importEntry->importName, &entry, result);
        }
        result->module = module;
        result->name = exportEntry->localName;
        return JS_TRUE;
    }
    if (JSSTRING_LENGTH(ATOM_TO_STRING(name)) == 7 &&
        JSSTRING_CHARS(ATOM_TO_STRING(name))[0] == 'd' &&
        JSSTRING_CHARS(ATOM_TO_STRING(name))[1] == 'e' &&
        JSSTRING_CHARS(ATOM_TO_STRING(name))[2] == 'f' &&
        JSSTRING_CHARS(ATOM_TO_STRING(name))[3] == 'a' &&
        JSSTRING_CHARS(ATOM_TO_STRING(name))[4] == 'u' &&
        JSSTRING_CHARS(ATOM_TO_STRING(name))[5] == 'l' &&
        JSSTRING_CHARS(ATOM_TO_STRING(name))[6] == 't')
        return JS_TRUE;
    for (i = 0; i < record->exportCount; ++i) {
        exportEntry = &record->exports[i];
        if (exportEntry->exportName) continue;
        if (!ResolveExport(cx, record->requests[exportEntry->request].module,
                            name, &entry, &candidate)) return JS_FALSE;
        if (candidate.ambiguous ||
            (result->module && candidate.module &&
             (result->module != candidate.module || result->name != candidate.name))) {
            result->module = NULL; result->ambiguous = JS_TRUE;
            return JS_TRUE;
        }
        if (candidate.module) *result = candidate;
    }
    return JS_TRUE;
}

typedef struct ModuleVisit {
    JSObject *module;
    struct ModuleVisit *previous;
} ModuleVisit;

typedef struct ExportNames {
    JSAtom **names;
    size_t count;
} ExportNames;

static JSBool
IsDefaultExport(JSAtom *name)
{
    static const jschar word[] = {'d','e','f','a','u','l','t'};
    JSString *str = ATOM_TO_STRING(name);
    return JSSTRING_LENGTH(str) == 7 && !memcmp(JSSTRING_CHARS(str), word, sizeof word);
}

static JSBool
CollectExportNames(JSContext *cx, JSObject *module, ModuleVisit *previous,
                     JSBool includeDefault, ExportNames *result)
{
    ModuleRecord *record = GetModule(cx, module);
    ModuleVisit visit, *cursor;
    JSAtom *name, **next;
    uint32 i;
    size_t j;
    int stackDummy;
    if (!record) return JS_FALSE;
    if (!JS_CHECK_STACK_SIZE(cx, stackDummy)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_OVER_RECURSED);
        return JS_FALSE;
    }
    for (cursor = previous; cursor; cursor = cursor->previous)
        if (cursor->module == module) return JS_TRUE;
    visit.module = module; visit.previous = previous;
    for (i = 0; i < record->exportCount; ++i) {
        name = record->exports[i].exportName;
        if (!name) {
            if (!CollectExportNames(cx, record->requests[record->exports[i].request].module,
                                     &visit, JS_FALSE, result)) return JS_FALSE;
            continue;
        }
        if (!includeDefault && IsDefaultExport(name)) continue;
        for (j = 0; j < result->count; ++j) if (result->names[j] == name) break;
        if (j < result->count) continue;
        if (result->count >= (size_t)-1 / sizeof(JSAtom *) - 1) {
            JS_ReportOutOfMemory(cx); return JS_FALSE;
        }
        next = (JSAtom **)JS_realloc(cx, result->names, (result->count + 1) * sizeof *next);
        if (!next) return JS_FALSE;
        result->names = next; next[result->count++] = name;
    }
    return JS_TRUE;
}

static int
CompareExportNames(const void *a, const void *b)
{
    JSString *left = ATOM_TO_STRING(*(JSAtom * const *)a);
    JSString *right = ATOM_TO_STRING(*(JSAtom * const *)b);
    size_t i, n = JSSTRING_LENGTH(left), m = JSSTRING_LENGTH(right);
    for (i = 0; i < n && i < m; ++i) {
        if (JSSTRING_CHARS(left)[i] != JSSTRING_CHARS(right)[i])
            return JSSTRING_CHARS(left)[i] < JSSTRING_CHARS(right)[i] ? -1 : 1;
    }
    return n == m ? 0 : n < m ? -1 : 1;
}

static ModuleRecord *
NamespaceRecord(JSContext *cx, JSObject *handler)
{
    jsval module;
    if (!JS_GetReservedSlot(cx, handler, 0, &module) || JSVAL_IS_PRIMITIVE(module))
        return NULL;
    return GetModule(cx, JSVAL_TO_OBJECT(module));
}

static ModuleEntry *
NamespaceBinding(JSContext *cx, ModuleRecord *record, jsval key)
{
    jsid id;
    uint32 i;
    if (!js_ValueToPropertyId(cx, key, &id)) return NULL;
    for (i = 0; i < record->namespaceCount; ++i)
        if (ATOM_TO_JSID(record->namespaceBindings[i].exportName) == id)
            return &record->namespaceBindings[i];
    return NULL;
}

static JSBool
NamespaceRead(JSContext *cx, ModuleEntry *entry, jsval *value)
{
    return OBJ_GET_PROPERTY(cx, js_ModuleEnvironment(cx, entry->resolvedModule),
                             ATOM_TO_JSID(entry->resolvedName), value);
}

static JSBool
NamespaceGet(JSContext *cx, JSObject *handler, uintN argc, jsval *argv, jsval *rval)
{
    ModuleRecord *record = NamespaceRecord(cx, handler);
    ModuleEntry *entry;
    if (!record) return JS_FALSE;
    entry = NamespaceBinding(cx, record, argv[1]);
    return entry ? NamespaceRead(cx, entry, rval) : js_ReflectGet(cx, handler, argc, argv, rval);
}

static JSBool
NamespaceDescriptor(JSContext *cx, JSObject *handler, uintN argc, jsval *argv, jsval *rval)
{
    ModuleRecord *record = NamespaceRecord(cx, handler);
    ModuleEntry *entry;
    jsval roots[2];
    JSTempValueRooter root;
    JSBool ok;
    if (!record) return JS_FALSE;
    entry = NamespaceBinding(cx, record, argv[1]);
    if (!entry) return js_ReflectGetOwnPropertyDescriptor(cx, handler, argc, argv, rval);
    roots[0] = roots[1] = JSVAL_VOID;
    JS_PUSH_TEMP_ROOT(cx, 2, roots, &root);
    ok = NamespaceRead(cx, entry, &roots[0]) &&
         js_ReflectGetOwnPropertyDescriptor(cx, handler, argc, argv, &roots[1]) &&
         JS_SetProperty(cx, JSVAL_TO_OBJECT(roots[1]), "value", &roots[0]);
    if (ok) *rval = roots[1];
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}

static JSBool
NamespaceSet(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    *rval = JSVAL_FALSE;
    return JS_TRUE;
}

/* ES2015 9.4.6.10 rejects exported keys and reports success for
 * other keys without removing the namespace's intrinsic symbol properties. */
static JSBool
NamespaceDelete(JSContext *cx, JSObject *handler, uintN argc, jsval *argv, jsval *rval)
{
    ModuleRecord *record = NamespaceRecord(cx, handler);
    if (!record) return JS_FALSE;
    *rval = BOOLEAN_TO_JSVAL(!NamespaceBinding(cx, record, argv[1]));
    return JS_TRUE;
}

/* ES2015 9.4.6.6 rejects even a no-op definition. */
static JSBool
NamespaceDefine(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    *rval = JSVAL_FALSE;
    return JS_TRUE;
}

/* ES2015 26.3.2 is generic and enumerates keys, not binding values. */
static JSBool
NamespaceIterator(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    argv[0] = argv[-1];
    return js_ReflectEnumerate(cx, obj, 1, argv, rval);
}

static JSClass namespaceHandlerClass = {
    "Module Namespace Handler", JSCLASS_HAS_RESERVED_SLOTS(1) |
        JSCLASS_IS_ANONYMOUS | JSCLASS_HAS_CACHED_PROTO(JSProto_Object),
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

JSObject *
js_GetModuleNamespace(JSContext *cx, JSObject *module)
{
    ModuleRecord *record = GetModule(cx, module);
    ExportNames names;
    ResolvedBinding resolved;
    ModuleEntry *entry;
    JSObject *global, *handler, *target;
    JSFunction *iteratorFunction;
    JSString *tagString;
    jsval roots[4];
    JSTempValueRooter root;
    jsid tag;
    size_t i;
    JSBool ok = JS_FALSE;
    if (!record) return NULL;
    if (record->state == MODULE_UNLINKED && !js_InstantiateModule(cx, module)) return NULL;
    if (record->namespaceObject) return record->namespaceObject;
    roots[0] = OBJECT_TO_JSVAL(module);
    roots[1] = roots[2] = roots[3] = JSVAL_VOID;
    JS_PUSH_TEMP_ROOT(cx, 4, roots, &root);
    memset(&names, 0, sizeof names);
    if (!CollectExportNames(cx, module, NULL, JS_TRUE, &names)) goto out;
    if (names.count > 1)
        qsort(names.names, names.count, sizeof(JSAtom *), CompareExportNames);
    if (names.count > (uint32)-1 || names.count > (size_t)-1 / sizeof(ModuleEntry)) {
        JS_ReportOutOfMemory(cx); goto out;
    }
    if (names.count) {
        record->namespaceBindings = (ModuleEntry *)JS_malloc(cx, names.count * sizeof(ModuleEntry));
        if (!record->namespaceBindings) goto out;
    }
    global = OBJ_GET_PARENT(cx, module);
    target = js_NewObject(cx, &js_ObjectClass, NULL, global);
    if (!target) goto out;
    roots[1] = OBJECT_TO_JSVAL(target);
    if (!JS_SetPrototype(cx, target, NULL)) goto out;
    for (i = 0; i < names.count; ++i) {
        if (!ResolveExport(cx, module, names.names[i], NULL, &resolved)) goto out;
        if (!resolved.module || resolved.ambiguous) continue;
        entry = &record->namespaceBindings[record->namespaceCount++];
        memset(entry, 0, sizeof *entry);
        entry->exportName = names.names[i];
        entry->resolvedModule = resolved.module;
        entry->resolvedName = resolved.name;
        if (!js_DefineNativeProperty(cx, target, ATOM_TO_JSID(entry->exportName), JSVAL_VOID,
                                      NULL, NULL, JSPROP_ENUMERATE | JSPROP_PERMANENT,
                                      0, 0, NULL)) goto out;
    }
    tagString = JS_NewStringCopyZ(cx, "Module");
    if (!tagString) goto out;
    roots[3] = STRING_TO_JSVAL(tagString);
    if (!js_WellKnownSymbolId(cx, JS_WKS_TO_STRING_TAG, &tag) ||
        !OBJ_DEFINE_PROPERTY(cx, target, tag, roots[3], NULL, NULL,
                             JSPROP_READONLY, NULL)) goto out;
    iteratorFunction = JS_NewFunction(cx, NamespaceIterator, 1,
                         JSFUN_STRICT | JSFUN_NO_CONSTRUCT, global, "[Symbol.iterator]");
    if (!iteratorFunction) goto out;
    roots[3] = OBJECT_TO_JSVAL(JS_GetFunctionObject(iteratorFunction));
    /* One native argument slot is reserved for generic Reflect.enumerate. */
    if (!JS_DefineProperty(cx, JSVAL_TO_OBJECT(roots[3]), "length", JSVAL_ZERO,
                           NULL, NULL, JSPROP_READONLY) ||
        !js_WellKnownSymbolId(cx, JS_WKS_ITERATOR, &tag) ||
        !OBJ_DEFINE_PROPERTY(cx, target, tag, roots[3], NULL, NULL, 0, NULL) ||
        !js_ReflectPreventExtensions(cx, NULL, 1, &roots[1], &roots[3])) goto out;
    handler = js_NewObject(cx, &namespaceHandlerClass, NULL, global);
    if (!handler) goto out;
    roots[2] = OBJECT_TO_JSVAL(handler);
    if (!JS_SetPrototype(cx, handler, NULL) ||
        !JS_SetReservedSlot(cx, handler, 0, roots[0]) ||
        !JS_DefineFunction(cx, handler, "get", NamespaceGet, 3, JSFUN_NO_CONSTRUCT) ||
        !JS_DefineFunction(cx, handler, "getOwnPropertyDescriptor", NamespaceDescriptor, 2, JSFUN_NO_CONSTRUCT) ||
        !JS_DefineFunction(cx, handler, "set", NamespaceSet, 4, JSFUN_NO_CONSTRUCT) ||
        !JS_DefineFunction(cx, handler, "setPrototypeOf", NamespaceSet, 2, JSFUN_NO_CONSTRUCT) ||
        !JS_DefineFunction(cx, handler, "deleteProperty", NamespaceDelete, 2, JSFUN_NO_CONSTRUCT) ||
        !JS_DefineFunction(cx, handler, "defineProperty", NamespaceDefine, 3, JSFUN_NO_CONSTRUCT)) goto out;
    record->namespaceObject = js_NewProxyObject(cx, roots[1], roots[2], global);
    ok = record->namespaceObject != NULL;
 out:
    if (!ok) {
        JS_free(cx, record->namespaceBindings);
        record->namespaceBindings = NULL;
        record->namespaceCount = 0;
    }
    JS_free(cx, names.names);
    JS_POP_TEMP_ROOT(cx, &root);
    return ok ? record->namespaceObject : NULL;
}

static JSBool
ModuleImportGet(JSContext *cx, JSObject *environment, jsval property, jsval *value)
{
    jsval owner;
    jsid id;
    ModuleRecord *record;
    ModuleEntry *entry;
    uint32 i;
    if (!JS_GetReservedSlot(cx, environment, 0, &owner) ||
        !JSVAL_IS_OBJECT(owner) || !JSVAL_TO_OBJECT(owner) ||
        !(record = GetModule(cx, JSVAL_TO_OBJECT(owner))) ||
        !js_ValueToPropertyId(cx, property, &id)) return JS_FALSE;
    for (i = 0; i < record->importCount; ++i) {
        entry = &record->imports[i];
        if (ATOM_TO_JSID(entry->localName) != id) continue;
        if (!entry->importName) {
            JSObject *namespaceObject = js_GetModuleNamespace(cx, record->requests[entry->request].module);
            if (!namespaceObject) return JS_FALSE;
            *value = OBJECT_TO_JSVAL(namespaceObject);
            return JS_TRUE;
        }
        if (!entry->resolvedModule || !entry->resolvedName) {
            JS_ReportError(cx, "module import has not been initialized"); return JS_FALSE;
        }
        return OBJ_GET_PROPERTY(cx, js_ModuleEnvironment(cx, entry->resolvedModule),
                                 ATOM_TO_JSID(entry->resolvedName), value);
    }
    JS_ReportError(cx, "unknown module import binding");
    return JS_FALSE;
}

static JSBool
ModuleImportSet(JSContext *cx, JSObject *obj, jsval property, jsval *value)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_BAD_DESCRIPTOR);
    return JS_FALSE;
}

JSBool
js_InstantiateModule(JSContext *cx, JSObject *module)
{
    ModuleRecord *record = GetModule(cx, module);
    JSScript *script;
    JSObject *environment, *bindings, *function, *owner;
    JSProperty *existing;
    JSScopeProperty *property;
    JSAtom *atom;
    jsbytecode *pc;
    jsint length;
    jsatomid index;
    JSOp op;
    uint32 i;
    ResolvedBinding resolution;
    ModuleEntry *entry;
    jsid id;
    jsval value = JSVAL_VOID;
    JSTempValueRooter moduleRoot, valueRoot;
    JSBool ok = JS_FALSE;
    int stackDummy;
    if (!JS_CHECK_STACK_SIZE(cx, stackDummy)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_OVER_RECURSED);
        return JS_FALSE;
    }
    if (!record || !record->script) return JS_FALSE;
    if (record->state == MODULE_FAILED) {
        JS_SetPendingException(cx, record->exception);
        return JS_FALSE;
    }
    if (record->state != MODULE_UNLINKED) return JS_TRUE;
    for (i = 0; i < record->requestCount; ++i) {
        if (!record->requests[i].module) {
            JS_ReportError(cx, "module dependencies have not been resolved");
            return JS_FALSE;
        }
    }
    JS_PUSH_TEMP_ROOT_OBJECT(cx, module, &moduleRoot);
    JS_PUSH_TEMP_ROOT(cx, 1, &value, &valueRoot);
    record->state = MODULE_LINKING;
    for (i = 0; i < record->requestCount; ++i)
        if (!js_InstantiateModule(cx, record->requests[i].module)) goto bad;
    script = record->script;
    environment = record->environment;
    /* Slot zero belongs to the module owner in non-global lexical records. */
    if (!JS_SetReservedSlot(cx, environment, 0, OBJECT_TO_JSVAL(module))) goto bad;
    for (i = 0; i < record->exportCount; ++i) {
        entry = &record->exports[i];
        if (entry->request != JS_MODULE_LOCAL && entry->exportName &&
            (!ResolveExport(cx, module, entry->exportName, NULL, &resolution) ||
             !resolution.module || resolution.ambiguous)) {
            ModuleSyntaxError(cx); goto bad;
        }
    }
    for (i = 0; i < record->importCount; ++i) {
        entry = &record->imports[i];
        if (!entry->importName) {
            if (!js_GetModuleNamespace(cx, record->requests[entry->request].module)) goto bad;
        } else {
            if (!ResolveExport(cx, record->requests[entry->request].module,
                                 entry->importName, NULL, &resolution)) goto bad;
            if (!resolution.module || resolution.ambiguous) { ModuleSyntaxError(cx); goto bad; }
            entry->resolvedModule = resolution.module;
            entry->resolvedName = resolution.name;
        }
        if (!js_DefineNativeProperty(cx, environment, ATOM_TO_JSID(entry->localName),
                                      JSVAL_VOID, ModuleImportGet, ModuleImportSet,
                                      JSPROP_ENUMERATE | JSPROP_PERMANENT | JSPROP_SHARED,
                                      0, 0, NULL)) goto bad;
    }
    if (script->globalLexicalIndex != (uint32)-1) {
        bindings = ATOM_TO_OBJECT(js_GetAtom(cx, &script->atomMap, script->globalLexicalIndex));
        for (property = OBJ_SCOPE(bindings)->lastProp; property; property = property->parent) {
            if (!js_DefineLexicalBinding(cx, environment, property->id,
                                          (property->flags & SPROP_IS_CONST) != 0)) goto bad;
        }
    }
    /* Declaration bytecode has no user callbacks; initialize it before any
     * dependency's evaluation can observe these bindings. */
    for (pc = script->code; pc < script->main; pc += length) {
        op = js_GetEffectiveOpcode(cx, script, pc, &length, &index);
        if (length <= 0) goto bad;
        if (op != JSOP_DEFVAR && op != JSOP_DEFFUN) {
            JS_ReportError(cx, "unexpected module declaration opcode");
            goto bad;
        }
        atom = js_GetAtom(cx, &script->atomMap, index);
        value = JSVAL_VOID;
        if (op == JSOP_DEFFUN) {
            function = ATOM_TO_OBJECT(atom);
            atom = ((JSFunction *)JS_GetPrivate(cx, function))->atom;
            function = js_CloneFunctionObject(cx, function, environment);
            if (!function) goto bad;
            value = OBJECT_TO_JSVAL(function);
        }
        id = ATOM_TO_JSID(atom);
        /* Repeated var declarations share one mutable module binding. */
        if (!js_LookupOwnProperty(cx, environment, id, &owner, &existing)) goto bad;
        if (existing) OBJ_DROP_PROPERTY(cx, owner, existing);
        if (!existing) {
            if (!js_DefineLexicalBinding(cx, environment, id, JS_FALSE) ||
                !js_InitializeLexicalBinding(cx, environment, id, value)) goto bad;
        } else if (op == JSOP_DEFFUN && !OBJ_SET_PROPERTY(cx, environment, id, &value)) {
            goto bad;
        }
    }
    record->state = MODULE_LINKED;
    ok = JS_TRUE;
    goto out;
 bad:
    record->state = MODULE_FAILED;
    if (JS_IsExceptionPending(cx)) JS_GetPendingException(cx, &record->exception);
 out:
    JS_POP_TEMP_ROOT(cx, &valueRoot);
    JS_POP_TEMP_ROOT(cx, &moduleRoot);
    return ok;
}

JSBool
js_EvaluateModule(JSContext *cx, JSObject *module)
{
    ModuleRecord *record = GetModule(cx, module);
    jsval result;
    JSBool ok = JS_TRUE;
    uint32 i;
    JSTempValueRooter root;
    int stackDummy;
    if (!JS_CHECK_STACK_SIZE(cx, stackDummy)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_OVER_RECURSED);
        return JS_FALSE;
    }
    if (!record || !js_InstantiateModule(cx, module)) return JS_FALSE;
    if (record->state == MODULE_EVALUATING || record->state == MODULE_EVALUATED)
        return JS_TRUE;
    JS_PUSH_TEMP_ROOT_OBJECT(cx, module, &root);
    record->state = MODULE_EVALUATING;
    for (i = 0; i < record->requestCount && ok; ++i)
        ok = js_EvaluateModule(cx, record->requests[i].module);
    if (ok) ok = js_Execute(cx, record->environment, record->script, NULL,
                    JSFRAME_MODULE | JSFRAME_MODULE_THIS, &result);
    if (ok) record->state = MODULE_EVALUATED;
    else {
        record->state = MODULE_FAILED;
        if (JS_IsExceptionPending(cx)) JS_GetPendingException(cx, &record->exception);
    }
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}
