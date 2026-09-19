/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* Native ECMAScript 5.1 JSON support. See LICENSE for project licensing. */
#include <string.h>
#include "jsapi.h"
#include "jsarray.h"
#include "jsatom.h"
#include "jsbool.h"
#include "jscntxt.h"
#include "jsfun.h"
#include "jsgc.h"
#include "jsinterp.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsstr.h"
#include "jssymbol.h"

/* Grow UTF-16 output with checked arithmetic on both 32- and 64-bit hosts. */
typedef struct JSONBuffer {
    jschar *chars;
    size_t length, capacity;
} JSONBuffer;

static JSBool
Append(JSContext *cx, JSONBuffer *b, const jschar *chars, size_t n)
{
    size_t capacity, needed;
    jschar *p;

    if (n > JSSTRING_LENGTH_MASK - b->length)
        goto oom;
    needed = b->length + n;
    if (needed > b->capacity) {
        capacity = b->capacity ? b->capacity : 64;
        while (capacity < needed) {
            if (capacity > JSSTRING_LENGTH_MASK / 2) {
                capacity = needed;
                break;
            }
            capacity *= 2;
        }
        if (capacity > (size_t)-1 / sizeof(jschar))
            goto oom;
        p = (jschar *)JS_realloc(cx, b->chars, capacity * sizeof(jschar));
        if (!p)
            return JS_FALSE;
        b->chars = p;
        b->capacity = capacity;
    }
    if (n)
        memcpy(b->chars + b->length, chars, n * sizeof(jschar));
    b->length = needed;
    return JS_TRUE;
oom:
    JS_ReportOutOfMemory(cx);
    return JS_FALSE;
}

static JSBool
Put(JSContext *cx, JSONBuffer *b, jschar c)
{
    return Append(cx, b, &c, 1);
}

static JSBool
Word(JSContext *cx, JSONBuffer *b, const char *s)
{
    while (*s) {
        if (!Put(cx, b, (jschar)*s++))
            return JS_FALSE;
    }
    return JS_TRUE;
}

typedef struct JSONParser {
    JSContext *cx;
    const jschar *at, *end;
} JSONParser;

static void
White(JSONParser *p)
{
    while (p->at < p->end && (*p->at == ' ' || *p->at == '\t' ||
                             *p->at == '\r' || *p->at == '\n'))
        p->at++;
}

static JSBool
BadJSON(JSONParser *p)
{
    JS_ReportErrorNumber(p->cx, js_GetErrorMessage, NULL, JSMSG_BAD_JSON);
    return JS_FALSE;
}

static JSBool
ParseString(JSONParser *p, jsval *vp)
{
    JSONBuffer b;
    jschar c;
    uintN i, digit, hex;
    JSString *str;
    JSBool ok = JS_FALSE;

    memset(&b, 0, sizeof(b));
    p->at++; /* opening quote */
    while (p->at < p->end) {
        c = *p->at++;
        if (c == '"') {
            str = JS_NewUCStringCopyN(p->cx, b.chars ? b.chars : js_empty_ucstr,
                                      b.length);
            if (str) {
                *vp = STRING_TO_JSVAL(str);
                ok = JS_TRUE;
            }
            goto out;
        }
        if (c < 0x20)
            goto bad;
        if (c == '\\') {
            if (p->at == p->end)
                goto bad;
            c = *p->at++;
            switch (c) {
              case '"': case '\\': case '/': break;
              case 'b': c = '\b'; break;
              case 'f': c = '\f'; break;
              case 'n': c = '\n'; break;
              case 'r': c = '\r'; break;
              case 't': c = '\t'; break;
              case 'u':
                if (p->end - p->at < 4)
                    goto bad;
                hex = 0;
                for (i = 0; i < 4; i++) {
                    c = *p->at++;
                    if (c >= '0' && c <= '9') digit = c - '0';
                    else if (c >= 'a' && c <= 'f') digit = c - 'a' + 10;
                    else if (c >= 'A' && c <= 'F') digit = c - 'A' + 10;
                    else goto bad;
                    hex = hex * 16 + digit;
                }
                c = (jschar)hex;
                break;
              default: goto bad;
            }
        }
        if (!Put(p->cx, &b, c))
            goto out;
    }
bad:
    BadJSON(p);
out:
    JS_free(p->cx, b.chars);
    return ok;
}

static JSBool ParseValue(JSONParser *p, jsval *vp);

static JSBool
ParseContainer(JSONParser *p, jsval *vp, JSBool array)
{
    JSContext *cx = p->cx;
    JSObject *obj;
    jsval roots[2];
    JSTempValueRooter root;
    jsuint index = 0;
    JSBool ok = JS_FALSE;
    jschar close = array ? ']' : '}';
    jsid id;

    obj = array ? js_NewArrayObject(cx, 0, NULL) :
                  js_NewObject(cx, &js_ObjectClass, NULL, NULL);
    if (!obj)
        return JS_FALSE;
    *vp = OBJECT_TO_JSVAL(obj); /* caller roots the result while we recurse */
    roots[0] = roots[1] = JSVAL_VOID;
    JS_PUSH_TEMP_ROOT(cx, 2, roots, &root);
    p->at++;
    White(p);
    if (p->at < p->end && *p->at == close) {
        p->at++;
        ok = JS_TRUE;
        goto out;
    }
    for (;;) {
        if (!array) {
            if (p->at == p->end || *p->at != '"')
                goto bad;
            if (!ParseString(p, &roots[0]))
                goto out;
            White(p);
            if (p->at == p->end || *p->at++ != ':')
                goto bad;
        }
        if (!ParseValue(p, &roots[1]))
            goto out;
        if (array) {
            if (!JS_DefineElement(cx, obj, index++, roots[1], NULL, NULL,
                                  JSPROP_ENUMERATE))
                goto out;
        } else {
            if (!JS_ValueToId(cx, roots[0], &id) ||
                !OBJ_DEFINE_PROPERTY(cx, obj, id, roots[1], NULL, NULL,
                                     JSPROP_ENUMERATE, NULL))
                goto out;
        }
        White(p);
        if (p->at == p->end)
            goto bad;
        if (*p->at == close) {
            p->at++;
            ok = JS_TRUE;
            goto out;
        }
        if (*p->at++ != ',')
            goto bad;
        White(p);
    }
bad:
    BadJSON(p);
out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}

static JSBool
ParseValue(JSONParser *p, jsval *vp)
{
    const jschar *start;
    const char *word;
    JSString *str;
    jsdouble d;
    jschar c;

    if (!JS_CHECK_STACK_SIZE(p->cx, c)) {
        JS_ReportErrorNumber(p->cx, js_GetErrorMessage, NULL, JSMSG_OVER_RECURSED);
        return JS_FALSE;
    }
    White(p);
    if (p->at == p->end)
        return BadJSON(p);
    c = *p->at;
    if (c == '"') return ParseString(p, vp);
    if (c == '[' || c == '{') return ParseContainer(p, vp, c == '[');
    if (c == 't' || c == 'f' || c == 'n') {
        word = c == 't' ? "true" : c == 'f' ? "false" : "null";
        while (*word) {
            if (p->at == p->end || *p->at++ != (jschar)*word++)
                return BadJSON(p);
        }
        *vp = c == 'n' ? JSVAL_NULL : BOOLEAN_TO_JSVAL(c == 't');
        return JS_TRUE;
    }
    start = p->at;
    if (*p->at == '-') p->at++;
    if (p->at == p->end) return BadJSON(p);
    if (*p->at == '0') p->at++;
    else {
        if (*p->at < '1' || *p->at > '9') return BadJSON(p);
        do { p->at++; } while (p->at < p->end && *p->at >= '0' && *p->at <= '9');
    }
    if (p->at < p->end && *p->at == '.') {
        p->at++;
        if (p->at == p->end || *p->at < '0' || *p->at > '9') return BadJSON(p);
        do { p->at++; } while (p->at < p->end && *p->at >= '0' && *p->at <= '9');
    }
    if (p->at < p->end && (*p->at == 'e' || *p->at == 'E')) {
        p->at++;
        if (p->at < p->end && (*p->at == '+' || *p->at == '-')) p->at++;
        if (p->at == p->end || *p->at < '0' || *p->at > '9') return BadJSON(p);
        do { p->at++; } while (p->at < p->end && *p->at >= '0' && *p->at <= '9');
    }
    str = JS_NewUCStringCopyN(p->cx, start, p->at - start);
    if (!str) return JS_FALSE;
    *vp = STRING_TO_JSVAL(str);
    return JS_ValueToNumber(p->cx, *vp, &d) && JS_NewNumberValue(p->cx, d, vp);
}

/* Walk uses snapshots of array length and own enumerable keys. User callbacks
 * can remove or add properties, throw, reenter JSON, or trigger collection. */
static JSBool
Walk(JSContext *cx, JSObject *holder, jsval key, jsval reviver, jsval *vp)
{
    jsval roots[4], args[2], ignored;
    JSTempValueRooter root;
    JSObject *obj;
    jsuint i, length;
    jsid id;
    JSString *str;
    JSBool array, accepted, ok = JS_FALSE;

    if (!JS_CHECK_STACK_SIZE(cx, roots)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_OVER_RECURSED);
        return JS_FALSE;
    }
    roots[0] = roots[1] = roots[2] = roots[3] = JSVAL_VOID;
    JS_PUSH_TEMP_ROOT(cx, 4, roots, &root);
    if (!JS_ValueToId(cx, key, &id) || !OBJ_GET_PROPERTY(cx, holder, id, &roots[0]))
        goto out;
    if (!JSVAL_IS_PRIMITIVE(roots[0])) {
        obj = JSVAL_TO_OBJECT(roots[0]);
        if (!js_IsArray(cx, obj, &array)) goto out;
        if (array) {
            if (!js_GetLengthProperty(cx, obj, &length)) goto out;
        } else {
            if (!js_ObjectKeys(cx, NULL, 1, roots, &roots[1]) ||
                !js_GetLengthProperty(cx, JSVAL_TO_OBJECT(roots[1]), &length))
                goto out;
        }
        for (i = 0; i < length; i++) {
            if (array) {
                if (!JS_NewNumberValue(cx, i, &roots[2])) goto out;
            } else if (!JS_GetElement(cx, JSVAL_TO_OBJECT(roots[1]), i, &roots[2]))
                goto out;
            str = js_ValueToString(cx, roots[2]);
            if (!str) goto out;
            roots[2] = STRING_TO_JSVAL(str);
            if (!Walk(cx, obj, roots[2], reviver, &roots[3]) ||
                !JS_ValueToId(cx, roots[2], &id))
                goto out;
            /* Rejected redefinitions leave the property intact. Callback
             * exceptions still propagate; a false definition result does not. */
            if (JSVAL_IS_VOID(roots[3])) {
                if (!OBJ_DELETE_PROPERTY(cx, obj, id, &ignored)) goto out;
            } else if (!js_CreateDataProperty(cx, obj, id, roots[3], &accepted))
                goto out;
        }
    }
    args[0] = key;
    args[1] = roots[0];
    ok = js_InternalCall(cx, holder, reviver, 2, args, vp);
out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}

static JSBool
json_parse(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSONParser p;
    JSString *text;
    JSObject *holder;
    jsval roots[2];
    JSTempValueRooter root;
    JSBool ok = JS_FALSE;

    text = js_ValueToString(cx, argc ? argv[0] : JSVAL_VOID);
    if (!text) return JS_FALSE;
    argv[0] = STRING_TO_JSVAL(text);
    p.cx = cx;
    p.at = JSSTRING_CHARS(text);
    p.end = p.at + JSSTRING_LENGTH(text);
    if (!ParseValue(&p, rval)) return JS_FALSE;
    White(&p);
    if (p.at != p.end) return BadJSON(&p);
    if (argc < 2 || !js_IsCallable(cx, argv[1])) return JS_TRUE;
    roots[0] = *rval;
    roots[1] = JSVAL_VOID;
    JS_PUSH_TEMP_ROOT(cx, 2, roots, &root);
    holder = js_NewObject(cx, &js_ObjectClass, NULL, NULL);
    if (!holder) goto out;
    roots[1] = OBJECT_TO_JSVAL(holder);
    if (!JS_DefineProperty(cx, holder, "", roots[0], NULL, NULL, JSPROP_ENUMERATE))
        goto out;
    ok = Walk(cx, holder, STRING_TO_JSVAL(cx->runtime->emptyString), argv[1], rval);
out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}

/* The active ancestor chain, rather than a set of previously seen objects,
 * detects cycles while permitting shared subobjects. Each ancestor is rooted
 * by its Serialize invocation. */
typedef struct JSONAncestor {
    JSObject *obj;
    struct JSONAncestor *up;
} JSONAncestor;

typedef struct JSONWriter {
    JSContext *cx;
    JSONBuffer output;
    jsval replacer, keys;
    jschar gap[10];
    size_t gapLength, depth;
    JSONAncestor *ancestors;
} JSONWriter;

static JSBool
Quote(JSONWriter *w, JSString *str)
{
    const jschar *chars = JSSTRING_CHARS(str);
    size_t i, n = JSSTRING_LENGTH(str);
    jschar c;
    static const char hex[] = "0123456789abcdef";
    const char *escape;

    if (!Put(w->cx, &w->output, '"')) return JS_FALSE;
    for (i = 0; i < n; i++) {
        c = chars[i];
        escape = NULL;
        switch (c) {
          case '"': escape = "\\\""; break;
          case '\\': escape = "\\\\"; break;
          case '\b': escape = "\\b"; break;
          case '\f': escape = "\\f"; break;
          case '\n': escape = "\\n"; break;
          case '\r': escape = "\\r"; break;
          case '\t': escape = "\\t"; break;
        }
        if (escape) {
            if (!Word(w->cx, &w->output, escape)) return JS_FALSE;
        } else if (c < 0x20) {
            if (!Word(w->cx, &w->output, "\\u00") ||
                !Put(w->cx, &w->output, hex[c >> 4]) ||
                !Put(w->cx, &w->output, hex[c & 15])) return JS_FALSE;
        } else if (!Put(w->cx, &w->output, c)) return JS_FALSE;
    }
    return Put(w->cx, &w->output, '"');
}

static JSBool
Indent(JSONWriter *w)
{
    size_t i;
    if (!w->gapLength) return JS_TRUE;
    if (!Put(w->cx, &w->output, '\n')) return JS_FALSE;
    for (i = 0; i < w->depth; i++) {
        if (!Append(w->cx, &w->output, w->gap, w->gapLength)) return JS_FALSE;
    }
    return JS_TRUE;
}

static JSBool Serialize(JSONWriter *w, JSObject *holder, jsval key,
                        JSBool *present);

static JSBool
Container(JSONWriter *w, JSObject *obj, JSBool array)
{
    JSContext *cx = w->cx;
    JSONAncestor here, *ancestor;
    jsval roots[2], value;
    JSTempValueRooter root;
    JSString *str;
    jsuint length, i, count = 0;
    size_t mark;
    JSBool present, ok = JS_FALSE;

    for (ancestor = w->ancestors; ancestor; ancestor = ancestor->up) {
        if (ancestor->obj == obj) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_JSON_CYCLE);
            return JS_FALSE;
        }
    }
    here.obj = obj;
    here.up = w->ancestors;
    w->ancestors = &here;
    w->depth++;
    roots[0] = roots[1] = JSVAL_VOID;
    JS_PUSH_TEMP_ROOT(cx, 2, roots, &root);
    if (array) {
        if (!js_GetLengthProperty(cx, obj, &length)) goto out;
    } else {
        roots[0] = w->keys;
        value = OBJECT_TO_JSVAL(obj);
        if (JSVAL_IS_VOID(roots[0]) &&
            !js_ObjectKeys(cx, NULL, 1, &value, &roots[0])) goto out;
        if (!js_GetLengthProperty(cx, JSVAL_TO_OBJECT(roots[0]), &length)) goto out;
    }
    if (!Put(cx, &w->output, array ? '[' : '{')) goto out;
    for (i = 0; i < length; i++) {
        if (array) {
            if (!JS_NewNumberValue(cx, i, &roots[1])) goto out;
        } else if (!JS_GetElement(cx, JSVAL_TO_OBJECT(roots[0]), i, &roots[1]))
            goto out;
        str = js_ValueToString(cx, roots[1]);
        if (!str) goto out;
        roots[1] = STRING_TO_JSVAL(str);
        mark = w->output.length;
        if ((count && !Put(cx, &w->output, ',')) || !Indent(w)) goto out;
        if (!array && (!Quote(w, str) || !Put(cx, &w->output, ':') ||
                       (w->gapLength && !Put(cx, &w->output, ' ')))) goto out;
        if (!Serialize(w, obj, roots[1], &present)) goto out;
        if (!present) {
            if (array) {
                if (!Word(cx, &w->output, "null")) goto out;
            } else {
                w->output.length = mark;
                continue;
            }
        }
        count++;
    }
    w->depth--;
    ok = (!count || Indent(w)) && Put(cx, &w->output, array ? ']' : '}');
    w->depth++;
out:
    JS_POP_TEMP_ROOT(cx, &root);
    w->depth--;
    w->ancestors = here.up;
    return ok;
}

static JSBool
Serialize(JSONWriter *w, JSObject *holder, jsval key, JSBool *present)
{
    JSContext *cx = w->cx;
    jsval roots[2], args[2];
    JSTempValueRooter root;
    JSObject *obj;
    JSClass *clasp;
    JSString *str;
    jsid id;
    jsdouble d;
    JSBool array, ok = JS_FALSE;

    if (!JS_CHECK_STACK_SIZE(cx, roots)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_OVER_RECURSED);
        return JS_FALSE;
    }
    *present = JS_TRUE;
    roots[0] = roots[1] = JSVAL_VOID;
    JS_PUSH_TEMP_ROOT(cx, 2, roots, &root);
    if (!JS_ValueToId(cx, key, &id) || !OBJ_GET_PROPERTY(cx, holder, id, &roots[0]))
        goto out;
    if (!JSVAL_IS_PRIMITIVE(roots[0])) {
        obj = JSVAL_TO_OBJECT(roots[0]);
        if (!JS_GetProperty(cx, obj, "toJSON", &roots[1])) goto out;
        if (js_IsCallable(cx, roots[1]) &&
            !js_InternalCall(cx, obj, roots[1], 1, &key, &roots[0])) goto out;
    }
    if (!JSVAL_IS_VOID(w->replacer)) {
        args[0] = key;
        args[1] = roots[0];
        if (!js_InternalCall(cx, holder, w->replacer, 2, args, &roots[0])) goto out;
    }
    if (!JSVAL_IS_PRIMITIVE(roots[0])) {
        obj = JSVAL_TO_OBJECT(roots[0]);
        clasp = OBJ_GET_CLASS(cx, obj);
        if (clasp == &js_NumberClass) {
            if (!JS_ValueToNumber(cx, roots[0], &d) ||
                !JS_NewNumberValue(cx, d, &roots[0])) goto out;
        } else if (clasp == &js_StringClass) {
            str = js_ValueToString(cx, roots[0]);
            if (!str) goto out;
            roots[0] = STRING_TO_JSVAL(str);
        } else if (clasp == &js_BooleanClass) {
            roots[0] = OBJ_GET_SLOT(cx, obj, JSSLOT_PRIVATE);
        }
    }
    if (JSVAL_IS_NULL(roots[0])) ok = Word(cx, &w->output, "null");
    else if (JSVAL_IS_BOOLEAN(roots[0]))
        ok = Word(cx, &w->output, JSVAL_TO_BOOLEAN(roots[0]) ? "true" : "false");
    else if (JSVAL_IS_STRING(roots[0])) ok = Quote(w, JSVAL_TO_STRING(roots[0]));
    else if (JSVAL_IS_NUMBER(roots[0])) {
        d = JSVAL_IS_INT(roots[0]) ? JSVAL_TO_INT(roots[0]) : *JSVAL_TO_DOUBLE(roots[0]);
        if (!JSDOUBLE_IS_FINITE(d)) ok = Word(cx, &w->output, "null");
        else {
            str = js_ValueToString(cx, roots[0]);
            if (!str) goto out;
            roots[1] = STRING_TO_JSVAL(str);
            ok = Append(cx, &w->output, JSSTRING_CHARS(str), JSSTRING_LENGTH(str));
        }
    } else if (!JSVAL_IS_PRIMITIVE(roots[0]) && !js_IsCallable(cx, roots[0])) {
        obj = JSVAL_TO_OBJECT(roots[0]);
        if (!js_IsArray(cx, obj, &array)) goto out;
        ok = Container(w, obj, array);
    } else {
        *present = JS_FALSE;
        ok = JS_TRUE;
    }
out:
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}

static JSBool
json_stringify(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSONWriter w;
    jsval roots[4], other;
    JSTempValueRooter root;
    JSObject *holder, *array;
    JSClass *clasp;
    JSString *str;
    jsuint length, i, j, count = 0;
    jsdouble d;
    JSBool duplicate, present, isArray = JS_FALSE, ok = JS_FALSE;

    memset(&w, 0, sizeof(w));
    w.cx = cx;
    w.replacer = w.keys = JSVAL_VOID;
    roots[0] = roots[1] = roots[2] = roots[3] = JSVAL_VOID;
    JS_PUSH_TEMP_ROOT(cx, 4, roots, &root);
    if (argc > 1 && js_IsCallable(cx, argv[1])) w.replacer = argv[1];
    else if (argc > 1 && !JSVAL_IS_PRIMITIVE(argv[1])) {
        if (!js_IsArray(cx, JSVAL_TO_OBJECT(argv[1]), &isArray)) goto out;
    }
    if (isArray) {
        array = JSVAL_TO_OBJECT(argv[1]);
        holder = js_NewArrayObject(cx, 0, NULL);
        if (!holder) goto out;
        roots[0] = w.keys = OBJECT_TO_JSVAL(holder);
        if (!js_GetLengthProperty(cx, array, &length)) goto out;
        for (i = 0; i < length; i++) {
            if (!JS_GetElement(cx, array, i, &roots[1])) goto out;
            if (!JSVAL_IS_PRIMITIVE(roots[1])) {
                clasp = OBJ_GET_CLASS(cx, JSVAL_TO_OBJECT(roots[1]));
                if (clasp != &js_StringClass && clasp != &js_NumberClass) continue;
            } else if (!JSVAL_IS_STRING(roots[1]) && !JSVAL_IS_NUMBER(roots[1])) continue;
            str = js_ValueToString(cx, roots[1]);
            if (!str) goto out;
            roots[1] = STRING_TO_JSVAL(str);
            duplicate = JS_FALSE;
            for (j = 0; j < count; j++) {
                if (!JS_GetElement(cx, holder, j, &other)) goto out;
                if (js_CompareStrings(str, JSVAL_TO_STRING(other)) == 0) {
                    duplicate = JS_TRUE;
                    break;
                }
            }
            if (!duplicate && !JS_DefineElement(cx, holder, count++, roots[1],
                                                NULL, NULL, JSPROP_ENUMERATE)) goto out;
        }
    }
    roots[1] = argc > 2 ? argv[2] : JSVAL_VOID;
    if (!JSVAL_IS_PRIMITIVE(roots[1])) {
        clasp = OBJ_GET_CLASS(cx, JSVAL_TO_OBJECT(roots[1]));
        if (clasp == &js_NumberClass) {
            if (!JS_ValueToNumber(cx, roots[1], &d) ||
                !JS_NewNumberValue(cx, d, &roots[1])) goto out;
        } else if (clasp == &js_StringClass) {
            str = js_ValueToString(cx, roots[1]);
            if (!str) goto out;
            roots[1] = STRING_TO_JSVAL(str);
        }
    }
    if (JSVAL_IS_NUMBER(roots[1])) {
        if (!JS_ValueToNumber(cx, roots[1], &d)) goto out;
        d = js_DoubleToInteger(d);
        w.gapLength = d <= 0 ? 0 : d >= 10 ? 10 : (size_t)d;
        for (i = 0; i < w.gapLength; i++) w.gap[i] = ' ';
    } else if (JSVAL_IS_STRING(roots[1])) {
        str = JSVAL_TO_STRING(roots[1]);
        w.gapLength = JS_MIN(JSSTRING_LENGTH(str), 10);
        memcpy(w.gap, JSSTRING_CHARS(str), w.gapLength * sizeof(jschar));
    }
    holder = js_NewObject(cx, &js_ObjectClass, NULL, NULL);
    if (!holder) goto out;
    roots[2] = OBJECT_TO_JSVAL(holder);
    if (!JS_DefineProperty(cx, holder, "", argc ? argv[0] : JSVAL_VOID,
                           NULL, NULL, JSPROP_ENUMERATE)) goto out;
    if (!Serialize(&w, holder, STRING_TO_JSVAL(cx->runtime->emptyString), &present))
        goto out;
    *rval = JSVAL_VOID;
    if (present) {
        str = JS_NewUCStringCopyN(cx, w.output.chars, w.output.length);
        if (!str) goto out;
        *rval = STRING_TO_JSVAL(str);
    }
    ok = JS_TRUE;
out:
    JS_free(cx, w.output.chars);
    JS_POP_TEMP_ROOT(cx, &root);
    return ok;
}

static JSClass json_class = {
    "JSON", 0,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

JSObject *
js_InitJSONClass(JSContext *cx, JSObject *global)
{
    JSObject *obj;
    JSTempValueRooter root;
    JSBool ok;

    obj = js_NewObject(cx, &json_class, NULL, global);
    if (!obj) return NULL;
    JS_PUSH_TEMP_ROOT_OBJECT(cx, obj, &root);
    ok = JS_DefineFunction(cx, obj, "parse", json_parse, 2, JSFUN_NO_CONSTRUCT) &&
         JS_DefineFunction(cx, obj, "stringify", json_stringify, 3, JSFUN_NO_CONSTRUCT) &&
         js_DefineBuiltinTag(cx, obj, "JSON") &&
         JS_DefineProperty(cx, global, "JSON", OBJECT_TO_JSVAL(obj), NULL, NULL, 0);
    JS_POP_TEMP_ROOT(cx, &root);
    return ok ? obj : NULL;
}
