/* Unicode normalization for JavaScript strings. The platform's historical
 * XPCOM normalizer remains independent. See unicode/README.md for provenance. */
#include <string.h>
#include "jsapi.h"
#include "jscntxt.h"
#include "jsnum.h"
#include "jsstr.h"
#include "jsnormalization.h"

typedef struct {
    uint32 point, canonicalOffset, compatibilityOffset;
    uint8 canonicalLength, compatibilityLength;
} NormalizationMapping;
typedef struct { uint32 first, last; uint8 combining; } CombiningRange;
typedef struct { uint32 first, second, composite; } CompositionPair;
typedef struct { uint32 point; uint8 combining; } NormalizationChar;
#include "jsnormalization-data.h"

#define TABLE_COUNT(table) (sizeof(table) / sizeof((table)[0]))

static uint8
CombiningClass(uint32 point)
{
    size_t low = 0, high = TABLE_COUNT(normalizationClasses), middle;
    const CombiningRange *range;
    while (low < high) {
        middle = low + (high - low) / 2;
        range = &normalizationClasses[middle];
        if (point < range->first)
            high = middle;
        else if (point > range->last)
            low = middle + 1;
        else
            return range->combining;
    }
    return 0;
}

static const NormalizationMapping *
Decomposition(uint32 point)
{
    size_t low = 0, high = TABLE_COUNT(normalizationMappings), middle;
    while (low < high) {
        middle = low + (high - low) / 2;
        if (point < normalizationMappings[middle].point)
            high = middle;
        else if (point > normalizationMappings[middle].point)
            low = middle + 1;
        else
            return &normalizationMappings[middle];
    }
    return NULL;
}

static uint32
Compose(uint32 first, uint32 second)
{
    size_t low, high, middle;
    const CompositionPair *pair;
    uint32 syllable;

    if (first >= 0x1100 && first < 0x1113 &&
        second >= 0x1161 && second < 0x1176)
        return 0xac00 + (first - 0x1100) * 588 + (second - 0x1161) * 28;
    if (first >= 0xac00 && first <= 0xd7a3 &&
        second > 0x11a7 && second < 0x11c3) {
        syllable = first - 0xac00;
        if (syllable % 28 == 0)
            return first + second - 0x11a7;
    }
    low = 0;
    high = TABLE_COUNT(normalizationPairs);
    while (low < high) {
        middle = low + (high - low) / 2;
        pair = &normalizationPairs[middle];
        if (first < pair->first || (first == pair->first && second < pair->second))
            high = middle;
        else if (first > pair->first || second > pair->second)
            low = middle + 1;
        else
            return pair->composite;
    }
    return 0;
}

static JSBool
AppendDecomposition(JSContext *cx, NormalizationChar **buffer, size_t *length,
                    size_t *capacity, const uint32 *points, size_t count)
{
    size_t limit, required, grown, i;
    NormalizationChar *storage;

    limit = JS_MIN((size_t)JSVAL_INT_MAX,
                   ((size_t)-1) / sizeof(NormalizationChar));
    if (count > limit - *length) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_BAD_STRING_LENGTH);
        return JS_FALSE;
    }
    required = *length + count;
    if (required > *capacity) {
        grown = *capacity ? *capacity * 2 : 32;
        if (grown < required)
            grown = required;
        if (grown > limit)
            grown = limit;
        storage = (NormalizationChar *)JS_realloc(cx, *buffer,
                                                  grown * sizeof(*storage));
        if (!storage)
            return JS_FALSE;
        *buffer = storage;
        *capacity = grown;
    }
    for (i = 0; i < count; ++i) {
        (*buffer)[*length].point = points[i];
        (*buffer)[*length].combining = CombiningClass(points[i]);
        ++*length;
    }
    return JS_TRUE;
}

static JSBool
CanonicalOrder(JSContext *cx, NormalizationChar *buffer, size_t length)
{
    NormalizationChar *scratch = NULL, *storage;
    size_t capacity = 0, start, end, i, count, total, positions[256];
    uintN combining;
    JSBool ordered;

    start = 0;
    while (start < length) {
        if (buffer[start].combining == 0) {
            ++start;
            continue;
        }
        end = start + 1;
        ordered = JS_TRUE;
        while (end < length && buffer[end].combining != 0) {
            if (buffer[end - 1].combining > buffer[end].combining)
                ordered = JS_FALSE;
            ++end;
        }
        if (!ordered) {
            count = end - start;
            if (count > capacity) {
                storage = (NormalizationChar *)JS_realloc(cx, scratch,
                                                          count * sizeof(*scratch));
                if (!storage) {
                    JS_free(cx, scratch);
                    return JS_FALSE;
                }
                scratch = storage;
                capacity = count;
            }
            memset(positions, 0, sizeof positions);
            for (i = start; i < end; ++i)
                ++positions[buffer[i].combining];
            total = 0;
            for (combining = 1; combining < 256; ++combining) {
                count = positions[combining];
                positions[combining] = total;
                total += count;
            }
            /* Stable counting sort avoids quadratic insertion of long runs. */
            for (i = start; i < end; ++i)
                scratch[positions[buffer[i].combining]++] = buffer[i];
            memcpy(buffer + start, scratch, (end - start) * sizeof(*scratch));
        }
        start = end;
    }
    JS_free(cx, scratch);
    return JS_TRUE;
}

JSString *
js_NormalizeString(JSContext *cx, JSString *source, uintN form)
{
    NormalizationChar *buffer = NULL;
    size_t length = 0, capacity = 0, i, sourceLength, count, offset;
    size_t starter, outputLength, position;
    const NormalizationMapping *mapping;
    const jschar *sourceChars;
    const uint32 *points;
    uint32 point, hangul[3], composite;
    uint8 previousClass;
    jschar *chars = NULL;
    JSString *result = NULL;
    JSTempValueRooter tvr;

    JS_PUSH_TEMP_ROOT_STRING(cx, source, &tvr);
    sourceLength = JSSTRING_LENGTH(source);
    sourceChars = JSSTRING_CHARS(source);
    for (i = 0; i < sourceLength; ++i) {
        point = sourceChars[i];
        if (point >= 0xd800 && point <= 0xdbff && i + 1 < sourceLength &&
            sourceChars[i + 1] >= 0xdc00 && sourceChars[i + 1] <= 0xdfff) {
            point = 0x10000 + ((point - 0xd800) << 10) +
                    (sourceChars[++i] - 0xdc00);
        }
        points = &point;
        count = 1;
        if (point >= 0xac00 && point <= 0xd7a3) {
            point -= 0xac00;
            hangul[0] = 0x1100 + point / 588;
            hangul[1] = 0x1161 + (point % 588) / 28;
            hangul[2] = 0x11a7 + point % 28;
            points = hangul;
            count = point % 28 ? 3 : 2;
        } else if ((mapping = Decomposition(point)) != NULL) {
            count = (form & 2) ? mapping->compatibilityLength : mapping->canonicalLength;
            if (count) {
                offset = (form & 2) ? mapping->compatibilityOffset : mapping->canonicalOffset;
                points = normalizationSequence + offset;
            } else {
                count = 1;
            }
        }
        if (!AppendDecomposition(cx, &buffer, &length, &capacity, points, count))
            goto out;
    }
    if (!CanonicalOrder(cx, buffer, length))
        goto out;
    if (form & 1) {
        starter = (size_t)-1;
        outputLength = 0;
        previousClass = 0;
        for (i = 0; i < length; ++i) {
            composite = 0;
            if (starter != (size_t)-1 &&
                (previousClass == 0 || previousClass < buffer[i].combining))
                composite = Compose(buffer[starter].point, buffer[i].point);
            if (composite) {
                buffer[starter].point = composite;
            } else {
                if (buffer[i].combining == 0)
                    starter = outputLength;
                previousClass = buffer[i].combining;
                buffer[outputLength++] = buffer[i];
            }
        }
        length = outputLength;
    }
    outputLength = 0;
    for (i = 0; i < length; ++i) {
        count = buffer[i].point > 0xffff ? 2 : 1;
        if (count > (size_t)JSVAL_INT_MAX - outputLength) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_BAD_STRING_LENGTH);
            goto out;
        }
        outputLength += count;
    }
    chars = (jschar *)JS_malloc(cx, (outputLength + 1) * sizeof(jschar));
    if (!chars)
        goto out;
    position = 0;
    for (i = 0; i < length; ++i) {
        point = buffer[i].point;
        if (point <= 0xffff) {
            chars[position++] = (jschar)point;
        } else {
            point -= 0x10000;
            chars[position++] = (jschar)(0xd800 + (point >> 10));
            chars[position++] = (jschar)(0xdc00 + (point & 0x3ff));
        }
    }
    chars[position] = 0;
    result = js_NewString(cx, chars, outputLength, 0);
    if (result)
        chars = NULL;
  out:
    JS_free(cx, chars);
    JS_free(cx, buffer);
    JS_POP_TEMP_ROOT(cx, &tvr);
    return result;
}
