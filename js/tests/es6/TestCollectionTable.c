/* Ordered collection storage regression; MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "jsapi.h"
#include "jscollectiontable.h"
#include <stdio.h>
#include <string.h>

static JSClass globalClass = {
    "global", 0,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};
static unsigned checks;
#define CHECK(expr) do { ++checks; if (!(expr)) { \
    fprintf(stderr, "FAIL collection storage line %d check %u\n", __LINE__, checks); \
    return 1; } } while (0)

/* Independent small ordered-array model; keys below are nonnegative integers. */
static unsigned modelKeys[128], modelValues[128], modelSize;
static unsigned Find(unsigned key)
{
    unsigned i;
    for (i = 0; i < modelSize && modelKeys[i] != key; ++i) {}
    return i;
}
static unsigned randomState = 0x173821u;
static unsigned Random(void)
{
    randomState = randomState * 1664525u + 1013904223u;
    return randomState;
}
int main(void)
{
    JSRuntime *rt;
    JSContext *cx;
    JSObject *global;
    JSCollectionData *data;
    jsval k, v, roots[8];
    uint32 cursor;
    unsigned i, j, key, value, op, found;
    const char *source;
    rt = JS_NewRuntime(8L * 1024 * 1024);
    CHECK(rt != NULL);
    cx = JS_NewContext(rt, 8192);
    CHECK(cx != NULL);
    JS_BeginRequest(cx);
    JS_SetVersion(cx, JSVERSION_ECMA_2015);
    global = JS_NewObject(cx, &globalClass, NULL, NULL);
    CHECK(global != NULL && JS_InitStandardClasses(cx, global));
    for (i = 0; i < 8; ++i) {
        roots[i] = JSVAL_VOID;
        CHECK(JS_AddRoot(cx, &roots[i]));
    }
    data = js_NewCollectionData();
    CHECK(data != NULL);
    for (i = 0; i < 50000; ++i) {
        op = Random() >> 24;
        key = (Random() >> 16) % 128;
        value = (Random() >> 16);
        found = Find(key);
        if (op < 128) {
            CHECK(js_CollectionPut(data, INT_TO_JSVAL(key), INT_TO_JSVAL(value)));
            if (found == modelSize) modelKeys[modelSize++] = key;
            modelValues[found] = value;
        } else if (op < 220) {
            CHECK(js_CollectionDelete(data, INT_TO_JSVAL(key)) == (found < modelSize));
            if (found < modelSize) {
                --modelSize;
                for (j = found; j < modelSize; ++j) {
                    modelKeys[j] = modelKeys[j + 1];
                    modelValues[j] = modelValues[j + 1];
                }
            }
        } else if (op == 255) {
            js_CollectionClear(data);
            modelSize = 0;
        } else {
            CHECK(js_CollectionGet(data, INT_TO_JSVAL(key), &v) == (found < modelSize));
            if (found < modelSize) CHECK(v == INT_TO_JSVAL(modelValues[found]));
        }
        CHECK(js_CollectionSize(data) == modelSize);
        if (i % 37 == 0) {
            CHECK(js_BeginCollectionIteration(data));
            cursor = 0;
            for (j = 0; j < modelSize; ++j) {
                CHECK(js_CollectionNext(data, &cursor, &k, &v));
                CHECK(k == INT_TO_JSVAL(modelKeys[j]) && v == INT_TO_JSVAL(modelValues[j]));
            }
            CHECK(!js_CollectionNext(data, &cursor, &k, &v));
            js_EndCollectionIteration(data);
        }
    }
    js_CollectionClear(data);
    CHECK(js_CollectionPut(data, INT_TO_JSVAL(1), INT_TO_JSVAL(10)));
    CHECK(js_CollectionPut(data, INT_TO_JSVAL(2), INT_TO_JSVAL(20)));
    CHECK(js_BeginCollectionIteration(data));
    cursor = 0;
    CHECK(js_CollectionNext(data, &cursor, &k, &v) && k == INT_TO_JSVAL(1));
    CHECK(js_CollectionDelete(data, INT_TO_JSVAL(2)));
    CHECK(js_CollectionPut(data, INT_TO_JSVAL(1), INT_TO_JSVAL(11)));
    CHECK(js_CollectionPut(data, INT_TO_JSVAL(2), INT_TO_JSVAL(22)));
    CHECK(js_CollectionNext(data, &cursor, &k, &v) && k == INT_TO_JSVAL(2) && v == INT_TO_JSVAL(22));
    js_CollectionClear(data);
    CHECK(js_CollectionPut(data, INT_TO_JSVAL(3), INT_TO_JSVAL(30)));
    CHECK(js_CollectionNext(data, &cursor, &k, &v) && k == INT_TO_JSVAL(3));
    CHECK(!js_CollectionNext(data, &cursor, &k, &v));
    js_EndCollectionIteration(data);
    js_CollectionClear(data);
    source = "[NaN,NaN,-0,0,'same',('sa'+'me'),Symbol('x'),Symbol('x')]";
    CHECK(JS_EvaluateScript(cx, global, source, strlen(source), "keys", 1, &v));
    roots[0] = v;
    for (i = 1; i < 8; ++i)
        CHECK(JS_GetElement(cx, JSVAL_TO_OBJECT(roots[0]), i, &roots[i]));
    CHECK(JS_GetElement(cx, JSVAL_TO_OBJECT(roots[0]), 0, &k));
    CHECK(js_CollectionPut(data, k, JSVAL_TRUE));
    CHECK(js_CollectionGet(data, roots[1], &v) && v == JSVAL_TRUE);
    CHECK(js_CollectionPut(data, roots[2], JSVAL_FALSE));
    CHECK(js_CollectionGet(data, roots[3], &v) && v == JSVAL_FALSE);
    CHECK(js_CollectionPut(data, roots[4], JSVAL_TRUE));
    CHECK(js_CollectionGet(data, roots[5], &v) && v == JSVAL_TRUE);
    CHECK(js_CollectionPut(data, roots[6], JSVAL_TRUE));
    CHECK(!js_CollectionGet(data, roots[7], &v));
    CHECK(js_CollectionPut(data, roots[7], JSVAL_FALSE));
    CHECK(js_CollectionSize(data) == 5);
    CHECK(js_BeginCollectionIteration(data));
    cursor = 0;
    CHECK(js_CollectionNext(data, &cursor, &k, &v));
    CHECK(js_CollectionNext(data, &cursor, &k, &v) && k == JSVAL_ZERO);
    /* Storage survives the owner's finalizer until the last iterator releases. */
    js_ReleaseCollectionData(data);
    CHECK(js_CollectionNext(data, &cursor, &k, &v) && k == roots[4]);
    js_EndCollectionIteration(data);
    for (i = 0; i < 8; ++i) JS_RemoveRoot(cx, &roots[i]);
    JS_EndRequest(cx);
    JS_DestroyContext(cx);
    JS_DestroyRuntime(rt);
    JS_ShutDown();
    printf("ES6-COLLECTION-STORAGE checks=%u failures=0\n", checks);
    return 0;
}
