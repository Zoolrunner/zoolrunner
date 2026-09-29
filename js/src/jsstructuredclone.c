/* Immutable, GC-independent structured-value snapshots.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
#include <stdlib.h>
#include <string.h>
#include "jsapi.h"
#include "jsarray.h"
#include "jsatom.h"
#include "jsbool.h"
#include "jsbinarydata.h"
#include "jscntxt.h"
#include "jsdate.h"
#include "jscollection.h"
#include "jscollectiontable.h"
#include "jslock.h"
#include "jsregexp.h"
#include "jsmath.h"
#include "jsreflect.h"
#include "jspromise.h"
#include "jssymbol.h"
#include "jsweakcollection.h"
#include "jsgc.h"
#include "jsiteres6.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsscope.h"
#include "jsstr.h"
#include "jsstructuredclone.h"

/* This is an opaque in-process storage representation, not a bytecode cache or
 * an untrusted wire format. No node retains a JS value after writing finishes. */
enum CloneTag {
    CLONE_UNDEFINED, CLONE_NULL, CLONE_BOOLEAN, CLONE_NUMBER, CLONE_STRING,
    CLONE_OBJECT, CLONE_ARRAY, CLONE_DATE, CLONE_BOOLEAN_OBJECT,
    CLONE_NUMBER_OBJECT, CLONE_STRING_OBJECT, CLONE_MAP, CLONE_SET, CLONE_REGEXP, CLONE_BUFFER, CLONE_VIEW
};
typedef struct CloneEdge {
    size_t name, value;
} CloneEdge;
typedef struct CloneNode {
    unsigned tag, flags;
    JSBool modern;
    jsdouble number;
    jschar *chars;
    unsigned char *bytes;
    size_t offset, buffer;
    int viewKind;
    size_t length;
    CloneEdge *edges;
    size_t count;
    JSObject *source; /* rooted during write only, then cleared */
} CloneNode;
struct JSStructuredValue {
    CloneNode **nodes;
    size_t count, capacity, root;
};
typedef struct CloneWriter {
    JSTempValueRooter root;
    JSStructuredValue *data;
    JSBool *unsupported;
    unsigned depth;
} CloneWriter;
typedef struct CloneKeys {
    JSTempValueRooter root;
    JSIdArray *ids;
} CloneKeys;

static void JS_DLL_CALLBACK
MarkWriter(JSContext *cx, JSTempValueRooter *root)
{
    CloneWriter *writer = (CloneWriter *)root;
    size_t i;
    for (i = 0; i < writer->data->count; ++i) {
        JSObject *source = writer->data->nodes[i]->source;
        if (source) GC_MARK(cx, source, "structured value source");
    }
}
static void JS_DLL_CALLBACK
MarkKeys(JSContext *cx, JSTempValueRooter *root)
{
    CloneKeys *keys = (CloneKeys *)root;
    jsint i;
    for (i = 0; i < keys->ids->length; ++i) {
        jsval value = ID_TO_VALUE(keys->ids->vector[i]);
        if (JSVAL_IS_GCTHING(value) && !JSVAL_IS_NULL(value))
            GC_MARK(cx, JSVAL_TO_GCTHING(value), "structured value key");
    }
}
void
js_FreeStructuredValue(JSStructuredValue *data)
{
    size_t i;
    if (!data) return;
    for (i = 0; i < data->count; ++i) {
        free(data->nodes[i]->chars);
        free(data->nodes[i]->bytes);
        free(data->nodes[i]->edges);
        free(data->nodes[i]);
    }
    free(data->nodes);
    free(data);
}
static CloneNode *
NewNode(JSContext *cx, JSStructuredValue *data, size_t *index)
{
    CloneNode *node;
    CloneNode **grown;
    size_t capacity;
    if (data->count == data->capacity) {
        capacity = data->capacity ? data->capacity * 2 : 16;
        if (capacity < data->capacity || capacity > (size_t)-1 / sizeof(*grown))
            goto oom;
        grown = (CloneNode **)realloc(data->nodes, capacity * sizeof(*grown));
        if (!grown) goto oom;
        data->nodes = grown;
        data->capacity = capacity;
    }
    node = (CloneNode *)calloc(1, sizeof(*node));
    if (!node) goto oom;
    *index = data->count;
    data->nodes[data->count++] = node;
    return node;
  oom:
    JS_ReportOutOfMemory(cx);
    return NULL;
}
static JSBool
CopyString(JSContext *cx, CloneNode *node, JSString *string)
{
    node->length = JSSTRING_LENGTH(string);
    if (node->length > (size_t)-1 / sizeof(jschar)) {
        JS_ReportOutOfMemory(cx);
        return JS_FALSE;
    }
    if (!node->length) return JS_TRUE;
    node->chars = (jschar *)malloc(node->length * sizeof(jschar));
    if (!node->chars) {
        JS_ReportOutOfMemory(cx);
        return JS_FALSE;
    }
    memcpy(node->chars, JSSTRING_CHARS(string), node->length * sizeof(jschar));
    return JS_TRUE;
}
static JSBool WriteValue(JSContext *, CloneWriter *, jsval, size_t *);

static JSBool
WriteProperties(JSContext *cx, CloneWriter *writer, CloneNode *node)
{
    CloneKeys keys;
    JSObject *obj = node->source, *owner;
    JSProperty *property;
    JSTempValueRooter valuesRoot;
    jsval values[2] = {JSVAL_VOID, JSVAL_VOID};
    JSString *string;
    jsint i;
    jsid id;
    uintN attrs, checkedAttrs;
    JSBool ok = JS_FALSE;
    keys.ids = js_StructuredOwnKeys(cx, obj);
    if (!keys.ids) return JS_FALSE;
    JS_PUSH_TEMP_ROOT_MARKER(cx, MarkKeys, &keys.root);
    JS_PUSH_TEMP_ROOT(cx, 2, values, &valuesRoot);
    if ((size_t)keys.ids->length > (size_t)-1 / sizeof(CloneEdge)) {
        JS_ReportOutOfMemory(cx); goto out;
    }
    if (keys.ids->length) {
        node->edges = (CloneEdge *)calloc(keys.ids->length, sizeof(CloneEdge));
        if (!node->edges) { JS_ReportOutOfMemory(cx); goto out; }
    }
    for (i = 0; i < keys.ids->length; ++i) {
        id = keys.ids->vector[i];
        values[0] = ID_TO_VALUE(id);
        if (JSVAL_IS_SYMBOL(values[0])) continue;
        /* A previous getter may have deleted this property or changed its
         * enumerability. Recheck the own descriptor without invoking a getter. */
        if (!js_CheckOwnAccess(cx, obj, id, JSACC_READ, &values[1], &checkedAttrs) ||
            !js_LookupOwnProperty(cx, obj, id, &owner, &property)) goto out;
        if (!property) continue;
        attrs = ((JSScopeProperty *)property)->attrs;
        OBJ_DROP_PROPERTY(cx, owner, property);
        if (owner != obj || !(attrs & JSPROP_ENUMERATE)) continue;
        string = js_ValueToString(cx, values[0]);
        if (!string) goto out;
        values[0] = STRING_TO_JSVAL(string);
        if (!WriteValue(cx, writer, values[0], &node->edges[node->count].name) ||
            !OBJ_GET_PROPERTY(cx, obj, id, &values[1]) ||
            !WriteValue(cx, writer, values[1], &node->edges[node->count].value))
            goto out;
        ++node->count;
    }
    ok = JS_TRUE;
  out:
    JS_POP_TEMP_ROOT(cx, &valuesRoot);
    JS_POP_TEMP_ROOT(cx, &keys.root);
    JS_DestroyIdArray(cx, keys.ids);
    return ok;
}
static JSBool
WriteCollection(JSContext *cx, CloneWriter *writer, CloneNode *node)
{
    JSCollectionData *data = (JSCollectionData *)JS_GetPrivate(cx, node->source);
    jsval *snapshot;
    JSTempValueRooter root;
    uint32 count, cursor = 0;
    size_t i;
    JSBool ok = JS_FALSE;
    /* Snapshot the complete entry list before serializing any key/value:
     * getters inside a key can mutate the original collection. */
    JS_LOCK_OBJ(cx, node->source);
    count = js_CollectionSize(data);
    if ((size_t)count > (size_t)-1 / (2 * sizeof(jsval)) ||
        (size_t)count > (size_t)-1 / sizeof(CloneEdge)) {
        JS_UNLOCK_OBJ(cx, node->source);
        JS_ReportOutOfMemory(cx); return JS_FALSE;
    }
    if (!count) { JS_UNLOCK_OBJ(cx, node->source); return JS_TRUE; }
    snapshot = (jsval *)malloc((size_t)count * 2 * sizeof(jsval));
    if (!snapshot) {
        JS_UNLOCK_OBJ(cx, node->source);
        JS_ReportOutOfMemory(cx); return JS_FALSE;
    }
    for (i = 0; i < count; ++i) {
        if (!js_CollectionNext(data, &cursor, &snapshot[2*i], &snapshot[2*i+1]))
            break;
    }
    count = (uint32)i;
    JS_UNLOCK_OBJ(cx, node->source);
    JS_PUSH_TEMP_ROOT(cx, (size_t)count * 2, snapshot, &root);
    node->edges = (CloneEdge *)calloc(count, sizeof(CloneEdge));
    if (!node->edges) { JS_ReportOutOfMemory(cx); goto out; }
    for (i = 0; i < count; ++i) {
        if (!WriteValue(cx, writer, snapshot[2*i], &node->edges[i].name)) goto out;
        if (node->tag == CLONE_SET) node->edges[i].value = node->edges[i].name;
        else if (!WriteValue(cx, writer, snapshot[2*i+1], &node->edges[i].value)) goto out;
        ++node->count;
    }
    ok = JS_TRUE;
  out:
    JS_POP_TEMP_ROOT(cx, &root);
    free(snapshot);
    return ok;
}
static JSBool
OrdinaryBuiltin(JSContext *cx, JSObject *obj, JSClass *clasp)
{
    jsval slot;
    if (clasp == &js_MathClass || clasp == &js_ReflectClass ||
        js_IsJSONNamespace(cx, obj) || js_IsRegExpPrototypeObject(cx, obj))
        return JS_TRUE;
    if (clasp == &js_WeakMapClass || clasp == &js_WeakSetClass)
        return JS_GetPrivate(cx, obj) == NULL;
    if (clasp == &js_PromiseClass || clasp == &js_SymbolClass)
        return JS_GetReservedSlot(cx, obj, 0, &slot) && JSVAL_IS_VOID(slot);
    return JS_FALSE;
}
static JSBool
WriteValue(JSContext *cx, CloneWriter *writer, jsval value, size_t *index)
{
    CloneNode *node;
    JSObject *obj = NULL;
    JSClass *clasp = NULL;
    JSTempValueRooter root;
    JSBool ok = JS_FALSE;
    jsuint length;
    size_t i;
    if (writer->depth >= 512 || !JS_CHECK_STACK_SIZE(cx, node)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_OVER_RECURSED);
        return JS_FALSE;
    }
    if (cx->branchCallback && !cx->branchCallback(cx, NULL)) return JS_FALSE;
    if (JSVAL_IS_SYMBOL(value)) { *writer->unsupported = JS_TRUE; return JS_FALSE; }
    if (!JSVAL_IS_PRIMITIVE(value)) {
        obj = JSVAL_TO_OBJECT(value);
        for (i = 0; i < writer->data->count; ++i) {
            if (writer->data->nodes[i]->source == obj) { *index = i; return JS_TRUE; }
        }
        clasp = OBJ_GET_CLASS(cx, obj);
        if (clasp != &js_ObjectClass && clasp != &js_ArrayClass &&
            clasp != &js_DateClass && clasp != &js_BooleanClass &&
            clasp != &js_NumberClass && clasp != &js_StringClass &&
            clasp != &js_MapClass && clasp != &js_SetClass &&
            clasp != &js_RegExpClass && !OrdinaryBuiltin(cx, obj, clasp) &&
            clasp != &js_ArrayBufferClass && clasp != &js_DataViewClass &&
            !js_IsTypedArray(cx, obj)) {
            *writer->unsupported = JS_TRUE;
            return JS_FALSE;
        }
    }
    JS_PUSH_SINGLE_TEMP_ROOT(cx, value, &root);
    ++writer->depth;
    node = NewNode(cx, writer->data, index);
    if (!node) goto out;
    node->source = obj;
    if (JSVAL_IS_VOID(value)) node->tag = CLONE_UNDEFINED;
    else if (JSVAL_IS_NULL(value)) node->tag = CLONE_NULL;
    else if (JSVAL_IS_BOOLEAN(value)) {
        node->tag = CLONE_BOOLEAN; node->number = JSVAL_TO_BOOLEAN(value);
    } else if (JSVAL_IS_NUMBER(value)) {
        node->tag = CLONE_NUMBER;
        node->number = JSVAL_IS_INT(value) ? JSVAL_TO_INT(value) : *JSVAL_TO_DOUBLE(value);
    } else if (JSVAL_IS_STRING(value)) {
        node->tag = CLONE_STRING;
        if (!CopyString(cx, node, JSVAL_TO_STRING(value))) goto out;
    } else if (clasp == &js_ArrayBufferClass || clasp == &js_DataViewClass ||
               js_IsTypedArray(cx, obj)) {
        JSStructuredBinary info;
        int type = js_StructuredBinaryInfo(cx, obj, &info);
        if (type < 0) { *writer->unsupported = JS_TRUE; goto out; }
        if (!type) {
            node->tag = CLONE_OBJECT;
            if (!WriteProperties(cx, writer, node)) goto out;
        } else if (type == 1) {
            node->tag = CLONE_BUFFER; node->length = info.length;
            if (info.length) {
                node->bytes = (unsigned char *)malloc(info.length);
                if (!node->bytes) { JS_ReportOutOfMemory(cx); goto out; }
                memcpy(node->bytes, info.bytes, info.length);
            }
        } else {
            node->tag = CLONE_VIEW; node->length = info.length;
            node->offset = info.offset; node->viewKind = info.kind;
            if (!WriteValue(cx, writer, OBJECT_TO_JSVAL(info.buffer), &node->buffer)) goto out;
        }
    } else if ((clasp == &js_MapClass || clasp == &js_SetClass) && JS_GetPrivate(cx, obj)) {
        node->tag = clasp == &js_MapClass ? CLONE_MAP : CLONE_SET;
        if (!WriteCollection(cx, writer, node)) goto out;
    } else if (clasp == &js_RegExpClass && JS_GetPrivate(cx, obj)) {
        JSRegExp *regexp = (JSRegExp *)JS_GetPrivate(cx, obj);
        node->tag = CLONE_REGEXP;
        node->flags = regexp->flags;
        node->modern = regexp->modern;
        if (!CopyString(cx, node, regexp->source)) goto out;
    } else if (clasp == &js_DateClass &&
               JSVAL_IS_DOUBLE(OBJ_GET_SLOT(cx, obj, JSSLOT_PRIVATE))) {
        /* The historical Date C API deliberately maps invalid dates to zero.
         * Structured storage instead preserves the exact internal time value. */
        node->tag = CLONE_DATE;
        node->number = *JSVAL_TO_DOUBLE(OBJ_GET_SLOT(cx, obj, JSSLOT_PRIVATE));
    } else if (clasp == &js_BooleanClass || clasp == &js_NumberClass || clasp == &js_StringClass) {
        value = OBJ_GET_SLOT(cx, obj, JSSLOT_PRIVATE);
        /* Native allocation hooks can expose a wrapper before its constructor
         * has initialized the slot. Never reinterpret an absent value. */
        if ((clasp == &js_StringClass && !JSVAL_IS_STRING(value)) ||
            (clasp == &js_NumberClass && !JSVAL_IS_NUMBER(value)) ||
            (clasp == &js_BooleanClass && !JSVAL_IS_BOOLEAN(value))) {
            *writer->unsupported = JS_TRUE; goto out;
        }
        if (clasp == &js_StringClass) {
            node->tag = CLONE_STRING_OBJECT;
            if (!CopyString(cx, node, JSVAL_TO_STRING(value))) goto out;
        } else {
            node->tag = clasp == &js_BooleanClass ? CLONE_BOOLEAN_OBJECT : CLONE_NUMBER_OBJECT;
            node->number = JSVAL_IS_BOOLEAN(value) ? JSVAL_TO_BOOLEAN(value) :
                          JSVAL_IS_INT(value) ? JSVAL_TO_INT(value) : *JSVAL_TO_DOUBLE(value);
        }
    } else {
        node->tag = clasp == &js_ArrayClass ? CLONE_ARRAY : CLONE_OBJECT;
        if (node->tag == CLONE_ARRAY) {
            if (!JS_GetArrayLength(cx, obj, &length)) goto out;
            node->length = length;
        }
        if (!WriteProperties(cx, writer, node)) goto out;
    }
    ok = JS_TRUE;
  out:
    --writer->depth;
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}
JSBool
js_WriteStructuredValue(JSContext *cx, jsval value, JSStructuredValue **result,
                         JSBool *unsupported)
{
    CloneWriter writer;
    JSTempValueRooter inputRoot;
    JSBool ok;
    size_t i;
    *result = NULL;
    *unsupported = JS_FALSE;
    writer.data = (JSStructuredValue *)calloc(1, sizeof(JSStructuredValue));
    if (!writer.data) { JS_ReportOutOfMemory(cx); return JS_FALSE; }
    writer.unsupported = unsupported;
    writer.depth = 0;
    JS_PUSH_SINGLE_TEMP_ROOT(cx, value, &inputRoot);
    JS_PUSH_TEMP_ROOT_MARKER(cx, MarkWriter, &writer.root);
    ok = WriteValue(cx, &writer, value, &writer.data->root);
    JS_POP_TEMP_ROOT(cx, &writer.root);
    JS_POP_TEMP_ROOT(cx, &inputRoot);
    if (!ok) { js_FreeStructuredValue(writer.data); return JS_FALSE; }
    for (i = 0; i < writer.data->count; ++i) writer.data->nodes[i]->source = NULL;
    *result = writer.data;
    return JS_TRUE;
}
JSBool
js_ReadStructuredValue(JSContext *cx, JSObject *global,
                        const JSStructuredValue *data, jsval *result)
{
    jsval *values, value;
    JSTempValueRooter roots, globalRoot;
    size_t i, j;
    CloneNode *node;
    JSString *string;
    JSObject *obj, *proto;
    JSClass *clasp;
    JSProtoKey key;
    JSBool ok = JS_FALSE;
    if (data->count > (size_t)-1 / sizeof(jsval)) { JS_ReportOutOfMemory(cx); return JS_FALSE; }
    values = (jsval *)malloc(data->count * sizeof(jsval));
    if (!values) { JS_ReportOutOfMemory(cx); return JS_FALSE; }
    for (i = 0; i < data->count; ++i) values[i] = JSVAL_VOID;
    JS_PUSH_TEMP_ROOT_OBJECT(cx, global, &globalRoot);
    JS_PUSH_TEMP_ROOT(cx, data->count, values, &roots);
    /* Allocate every identity before defining any reference, including cycles. */
    for (i = 0; i < data->count; ++i) {
        node = data->nodes[i];
        switch (node->tag) {
          case CLONE_UNDEFINED: continue;
          case CLONE_NULL: values[i] = JSVAL_NULL; continue;
          case CLONE_BOOLEAN: values[i] = BOOLEAN_TO_JSVAL(node->number != 0); continue;
          case CLONE_NUMBER:
            if (!js_NewNumberValue(cx, node->number, &values[i])) goto out;
            continue;
          case CLONE_STRING:
            string = JS_NewUCStringCopyN(cx, node->chars, node->length);
            if (!string) goto out;
            values[i] = STRING_TO_JSVAL(string); continue;
          case CLONE_VIEW: continue; /* buffers are allocated first */
          case CLONE_BUFFER:
            obj = js_ReadStructuredBuffer(cx, global, node->bytes, node->length);
            if (!obj) goto out;
            values[i] = OBJECT_TO_JSVAL(obj); continue;
          case CLONE_REGEXP:
            obj = js_ReadStructuredRegExp(cx, global, node->chars, node->length,
                                          node->flags, node->modern);
            if (!obj) goto out;
            values[i] = OBJECT_TO_JSVAL(obj); continue;
          case CLONE_MAP: key = JSProto_Map; clasp = &js_MapClass; break;
          case CLONE_SET: key = JSProto_Set; clasp = &js_SetClass; break;
          case CLONE_ARRAY: key = JSProto_Array; clasp = &js_ArrayClass; break;
          case CLONE_DATE: key = JSProto_Date; clasp = &js_DateClass; break;
          case CLONE_BOOLEAN_OBJECT: key = JSProto_Boolean; clasp = &js_BooleanClass; break;
          case CLONE_NUMBER_OBJECT: key = JSProto_Number; clasp = &js_NumberClass; break;
          case CLONE_STRING_OBJECT: key = JSProto_String; clasp = &js_StringClass; break;
          default: key = JSProto_Object; clasp = &js_ObjectClass; break;
        }
        proto = js_BuiltinPrototype(cx, global, key);
        if (!proto) goto out;
        obj = node->tag == CLONE_ARRAY ?
            js_NewArrayObjectWithProto(cx, (jsuint)node->length, NULL, proto, global) :
            js_NewObject(cx, clasp, proto, global);
        if (!obj) goto out;
        values[i] = OBJECT_TO_JSVAL(obj);
        if (node->tag == CLONE_MAP || node->tag == CLONE_SET) {
            JSCollectionData *collection = js_NewCollectionData();
            if (!collection) { JS_ReportOutOfMemory(cx); goto out; }
            if (!JS_SetPrivate(cx, obj, collection)) {
                js_ReleaseCollectionData(collection); goto out;
            }
        } else if (node->tag == CLONE_BOOLEAN_OBJECT)
            OBJ_SET_SLOT(cx, obj, JSSLOT_PRIVATE, BOOLEAN_TO_JSVAL(node->number != 0));
        else if (node->tag == CLONE_NUMBER_OBJECT || node->tag == CLONE_DATE) {
            if (!JS_NewDoubleValue(cx, node->number, &value)) goto out;
            OBJ_SET_SLOT(cx, obj, JSSLOT_PRIVATE, value);
        } else if (node->tag == CLONE_STRING_OBJECT) {
            string = JS_NewUCStringCopyN(cx, node->chars, node->length);
            if (!string) goto out;
            OBJ_SET_SLOT(cx, obj, JSSLOT_PRIVATE, STRING_TO_JSVAL(string));
        }
    }
    for (i = 0; i < data->count; ++i) {
        node = data->nodes[i];
        if (node->tag == CLONE_VIEW) {
            obj = js_ReadStructuredView(cx, global, JSVAL_TO_OBJECT(values[node->buffer]),
                                        node->offset, node->length, node->viewKind);
            if (!obj) goto out;
            values[i] = OBJECT_TO_JSVAL(obj);
        }
    }
    for (i = 0; i < data->count; ++i) {
        node = data->nodes[i];
        for (j = 0; j < node->count; ++j) {
            CloneEdge *edge = &node->edges[j];
            if (node->tag == CLONE_MAP || node->tag == CLONE_SET) {
                JSBool added;
                obj = JSVAL_TO_OBJECT(values[i]);
                JS_LOCK_OBJ(cx, obj);
                added = js_CollectionPut((JSCollectionData *)JS_GetPrivate(cx, obj),
                                          values[edge->name], values[edge->value]);
                JS_UNLOCK_OBJ(cx, obj);
                if (!added) { JS_ReportOutOfMemory(cx); goto out; }
                continue;
            }
            string = JSVAL_TO_STRING(values[edge->name]);
            if (!JS_DefineUCProperty(cx, JSVAL_TO_OBJECT(values[i]),
                                     JSSTRING_CHARS(string), JSSTRING_LENGTH(string),
                                     values[edge->value], NULL, NULL, JSPROP_ENUMERATE))
                goto out;
        }
    }
    *result = values[data->root];
    ok = JS_TRUE;
  out:
    JS_POP_TEMP_ROOT(cx, &roots);
    JS_POP_TEMP_ROOT(cx, &globalRoot);
    free(values);
    return ok;
}
