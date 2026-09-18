/* Native host diagnostic for the normalization kernel, including GC.
 * Compile this file and jsnormalization.c with sanitizers; the existing engine
 * library itself need not be instrumented. This does not audit the whole engine. */
#include "jsapi.h"
#include "jsnormalization.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static JSClass globalClass = {
    "global", 0, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};
static unsigned long checks;

static int
verify(JSContext *cx, const jschar *input, size_t inputLength,
       const jschar *expected, size_t expectedLength, unsigned form)
{
    JSString *source, *result;
    ++checks;
    source = JS_NewUCStringCopyN(cx, input, inputLength);
    if (!source)
        return 0;
    result = js_NormalizeString(cx, source, form);
    return result && JS_GetStringLength(result) == expectedLength &&
           !memcmp(JS_GetStringChars(result), expected, expectedLength * sizeof(jschar));
}

static size_t
encode(unsigned long point, jschar *chars)
{
    if (point <= 0xffff) { chars[0] = (jschar)point; return 1; }
    point -= 0x10000;
    chars[0] = (jschar)(0xd800 + (point >> 10));
    chars[1] = (jschar)(0xdc00 + (point & 1023));
    return 2;
}

int main(int argc, char **argv)
{
    JSRuntime *runtime;
    JSContext *cx;
    JSObject *global;
    FILE *input;
    char line[16384], *cursor, *next;
    jschar fields[5][2048], single[2];
    size_t lengths[5], amount;
    unsigned char *listed;
    unsigned row = 0, column, form, target;
    unsigned long point, identity = 0;
    int part1 = 0, status = 1;
    static const unsigned expected[4][5] = {
        {2,2,2,4,4}, {1,1,1,3,3}, {4,4,4,4,4}, {3,3,3,3,3}
    };
    if (argc != 2) return 2;
    input = fopen(argv[1], "r");
    if (!input) return 2;
    listed = (unsigned char *)calloc(0x110000, 1);
    if (!listed) { fclose(input); return 2; }
    runtime = JS_NewRuntime(8 * 1024 * 1024);
    if (!runtime) { free(listed); fclose(input); return 2; }
    cx = JS_NewContext(runtime, 8192);
    if (!cx) { JS_DestroyRuntime(runtime); free(listed); fclose(input); return 2; }
    JS_BeginRequest(cx);
    JS_SetVersion(cx, JSVERSION_ECMA_2015);
    global = JS_NewObject(cx, &globalClass, NULL, NULL);
    if (!global) goto out;
    JS_SetGlobalObject(cx, global);
    if (!JS_InitStandardClasses(cx, global)) goto out;
    while (fgets(line, sizeof line, input)) {
        if (!strchr(line, '\n') && !feof(input)) goto out;
        if (line[0] == '@') { part1 = !strncmp(line, "@Part1", 6); continue; }
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;
        cursor = line;
        for (column = 0; column < 5; ++column) {
            lengths[column] = 0;
            for (;;) {
                while (*cursor == ' ' || *cursor == '\t') ++cursor;
                if (*cursor == ';') { ++cursor; break; }
                point = strtoul(cursor, &next, 16);
                if (next == cursor || point > 0x10ffff || lengths[column] + 2 > 2048) goto out;
                if (part1 && column == 0) listed[point] = 1;
                lengths[column] += encode(point, fields[column] + lengths[column]);
                cursor = next;
            }
        }
        for (form = 0; form < 4; ++form) {
            for (column = 0; column < 5; ++column) {
                target = expected[form][column];
                if (!verify(cx, fields[column], lengths[column],
                            fields[target], lengths[target], form)) {
                    fprintf(stderr, "FAIL normalization row %u form %u column %u\n", row, form, column);
                    goto out;
                }
            }
        }
        ++row;
        if (row % 256 == 0) JS_GC(cx);
    }
    if (ferror(input)) goto out;
    for (point = 0; point <= 0x10ffff; ++point) {
        if (listed[point]) continue;
        amount = encode(point, single);
        for (form = 0; form < 4; ++form) {
            if (!verify(cx, single, amount, single, amount, form)) {
                fprintf(stderr, "FAIL normalization identity %lu form %u\n", point, form);
                goto out;
            }
        }
        ++identity;
        if (point % 4096 == 0) JS_GC(cx);
    }
    printf("NORMALIZATION-KERNEL rows=%u identity=%lu checks=%lu failures=0\n", row, identity, checks);
    status = 0;
  out:
    JS_EndRequest(cx);
    JS_DestroyContext(cx);
    JS_DestroyRuntime(runtime);
    JS_ShutDown();
    free(listed);
    fclose(input);
    return status;
}
