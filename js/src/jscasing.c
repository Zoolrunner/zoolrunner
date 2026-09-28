/* Modern Unicode casing; legacy tables and locale hooks remain independent.
 * MPL 1.1/GPL 2.0/LGPL 2.1. See unicode/README.md for data provenance. */
#include "jsapi.h"
#include "jscntxt.h"
#include "jsfun.h"
#include "jsnum.h"
#include "jsstr.h"
#include "jscasing.h"

typedef struct CasingMapping {
    uint32 point, lowerOffset, upperOffset;
    uint8 lowerLength, upperLength;
} CasingMapping;
typedef struct CasingRange { uint32 first, last; } CasingRange;
#include "jscasing-data.h"
#define COUNT(a) (sizeof(a) / sizeof((a)[0]))

static const CasingMapping *
Mapping(uint32 point)
{
    size_t low = 0, high = COUNT(casingMappings), middle;
    while (low < high) {
        middle = low + (high - low) / 2;
        if (point < casingMappings[middle].point) high = middle;
        else if (point > casingMappings[middle].point) low = middle + 1;
        else return &casingMappings[middle];
    }
    return NULL;
}
static JSBool
InRanges(uint32 point, const CasingRange *ranges, size_t count)
{
    size_t low = 0, middle;
    while (low < count) {
        middle = low + (count - low) / 2;
        if (point < ranges[middle].first) count = middle;
        else if (point > ranges[middle].last) low = middle + 1;
        else return JS_TRUE;
    }
    return JS_FALSE;
}
static uint32
NextPoint(const jschar *text, size_t length, size_t *position)
{
    uint32 point = text[(*position)++], second;
    if (point >= 0xd800 && point <= 0xdbff && *position < length) {
        second = text[*position];
        if (second >= 0xdc00 && second <= 0xdfff) {
            ++*position;
            point = 0x10000 + ((point - 0xd800) << 10) + second - 0xdc00;
        }
    }
    return point;
}
static JSBool
AppendPoint(JSContext *cx, jschar **buffer, size_t *length, size_t *capacity, uint32 point)
{
    size_t count = point > 0xffff ? 2 : 1, grown;
    size_t limit = JS_MIN((size_t)JSSTRING_LENGTH_MASK, ((size_t)-1)/sizeof(jschar)-1);
    jschar *storage;
    if (count > limit - *length) { JS_ReportOutOfMemory(cx); return JS_FALSE; }
    if (*length + count > *capacity) {
        grown = *capacity > limit / 2 ? limit : *capacity * 2;
        if (grown < *length + count) grown = *length + count;
        storage = (jschar *)JS_realloc(cx, *buffer, (grown + 1) * sizeof(jschar));
        if (!storage) return JS_FALSE;
        *buffer = storage; *capacity = grown;
    }
    if (point > 0xffff) {
        point -= 0x10000;
        (*buffer)[(*length)++] = (jschar)(0xd800 + (point >> 10));
        (*buffer)[(*length)++] = (jschar)(0xdc00 + (point & 0x3ff));
    } else (*buffer)[(*length)++] = (jschar)point;
    return JS_TRUE;
}

static JSBool
ConvertCase(JSContext *cx, jsval *argv, jsval *rval, JSBool upper, JSBool locale)
{
    JSString *source, *result;
    jschar *buffer = NULL;
    const CasingMapping *mapping;
    const uint32 *sequence;
    uint32 point, following, sigma = 0x3c2;
    size_t position = 0, look, length, used = 0, capacity = 0, count, i, work = 0;
    JSBool preceded = JS_FALSE, followed, ignorable, ok = JS_FALSE;
    if (JSVAL_IS_NULL(argv[-1]) || JSVAL_IS_VOID(argv[-1])) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_CANT_CONVERT_TO, "null or undefined", "string");
        return JS_FALSE;
    }
    source = js_ValueToString(cx, argv[-1]);
    if (!source) return JS_FALSE;
    argv[-1] = STRING_TO_JSVAL(source);
    if (locale && cx->localeCallbacks) {
        JSLocaleToUpperCase callback = upper ? cx->localeCallbacks->localeToUpperCase : cx->localeCallbacks->localeToLowerCase;
        if (callback) return callback(cx, source, rval);
    }
    length = JSSTRING_LENGTH(source);
    while (position < length) {
        if ((work++ & 1023) == 0 && cx->branchCallback && !cx->branchCallback(cx, NULL)) goto out;
        point = NextPoint(JSSTRING_CHARS(source), length, &position);
        ignorable = InRanges(point, casingCase_Ignorable, COUNT(casingCase_Ignorable));
        mapping = Mapping(point);
        sequence = &point; count = 1;
        if (mapping) {
            count = upper ? mapping->upperLength : mapping->lowerLength;
            if (count) sequence = casingSequences + (upper ? mapping->upperOffset : mapping->lowerOffset);
            else count = 1;
        }
        if (!upper && point == 0x3a3 && preceded) {
            /* Context is read from the original text, ignoring even characters
             * that are both Cased and Case_Ignorable. Each ignored run is
             * scanned at most once by lookahead, so this remains linear. */
            look = position; followed = JS_FALSE;
            while (look < length) {
                if ((work++ & 1023) == 0 && cx->branchCallback && !cx->branchCallback(cx, NULL)) goto out;
                following = NextPoint(JSSTRING_CHARS(source), length, &look);
                if (!InRanges(following, casingCase_Ignorable, COUNT(casingCase_Ignorable))) {
                    followed = InRanges(following, casingCased, COUNT(casingCased));
                    break;
                }
            }
            if (!followed) { sequence = &sigma; count = 1; }
        }
        for (i = 0; i < count; ++i)
            if (!AppendPoint(cx, &buffer, &used, &capacity, sequence[i])) goto out;
        if (!ignorable) preceded = InRanges(point, casingCased, COUNT(casingCased));
    }
    if (!buffer) { *rval = STRING_TO_JSVAL(cx->runtime->emptyString); return JS_TRUE; }
    buffer[used] = 0;
    result = js_NewString(cx, buffer, used, 0);
    if (!result) goto out;
    buffer = NULL; *rval = STRING_TO_JSVAL(result); ok = JS_TRUE;
  out:
    JS_free(cx, buffer);
    return ok;
}
#define CASE_METHOD(name, upper, locale) \
static JSBool name(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) \
{ return ConvertCase(cx, argv, rval, upper, locale); }
CASE_METHOD(LowerCase, JS_FALSE, JS_FALSE)
CASE_METHOD(UpperCase, JS_TRUE, JS_FALSE)
CASE_METHOD(LocaleLowerCase, JS_FALSE, JS_TRUE)
CASE_METHOD(LocaleUpperCase, JS_TRUE, JS_TRUE)
#undef CASE_METHOD
JSBool
js_InitModernCasing(JSContext *cx, JSObject *proto)
{
    return JS_DefineFunction(cx, proto, "toLowerCase", LowerCase, 0, JSFUN_STRICT | JSFUN_NO_CONSTRUCT) &&
           JS_DefineFunction(cx, proto, "toUpperCase", UpperCase, 0, JSFUN_STRICT | JSFUN_NO_CONSTRUCT) &&
           JS_DefineFunction(cx, proto, "toLocaleLowerCase", LocaleLowerCase, 0, JSFUN_STRICT | JSFUN_NO_CONSTRUCT) &&
           JS_DefineFunction(cx, proto, "toLocaleUpperCase", LocaleUpperCase, 0, JSFUN_STRICT | JSFUN_NO_CONSTRUCT);
}
