/* Ordered storage for ES2015 keyed collections; MPL 1.1/GPL 2.0/LGPL 2.1. */
#include <stdlib.h>
#include <string.h>
#include "jsapi.h"
#include "jscntxt.h"
#include "jsdhash.h"
#include "jsgc.h"
#include "jshash.h"
#include "jsnum.h"
#include "jsstr.h"
#include "jscollectiontable.h"

typedef struct CollectionRow {
    jsval key, value;
    JSBool live;
} CollectionRow;
typedef struct CollectionIndex {
    JSDHashEntryHdr hdr;
    jsval key;
    uint32 rowPlusOne;
} CollectionIndex;
struct JSCollectionData {
    JSDHashTable index;
    CollectionRow *rows;
    uint32 used, size, capacity, active, references;
};

static JSBool
SameValueZero(jsval a, jsval b)
{
    if (JSVAL_IS_NUMBER(a) && JSVAL_IS_NUMBER(b)) {
        jsdouble x = JSVAL_IS_INT(a) ? JSVAL_TO_INT(a) : *JSVAL_TO_DOUBLE(a);
        jsdouble y = JSVAL_IS_INT(b) ? JSVAL_TO_INT(b) : *JSVAL_TO_DOUBLE(b);
        return x == y || (JSDOUBLE_IS_NaN(x) && JSDOUBLE_IS_NaN(y));
    }
    if (JSVAL_IS_STRING(a) && JSVAL_IS_STRING(b))
        return js_EqualStrings(JSVAL_TO_STRING(a), JSVAL_TO_STRING(b));
    return a == b;
}
JS_STATIC_DLL_CALLBACK(const void *)
CollectionKey(JSDHashTable *table, JSDHashEntryHdr *entry)
{
    return &((CollectionIndex *)entry)->key;
}
JS_STATIC_DLL_CALLBACK(JSDHashNumber)
CollectionHash(JSDHashTable *table, const void *key)
{
    jsval value = *(const jsval *)key;
    jsuword bits;
    if (JSVAL_IS_NUMBER(value)) {
        union { jsdouble number; uint32 words[2]; } number;
        number.number = JSVAL_IS_INT(value) ? JSVAL_TO_INT(value) : *JSVAL_TO_DOUBLE(value);
        if (number.number == 0) return 0x90123456;
        if (JSDOUBLE_IS_NaN(number.number)) return 0x90765432;
        return number.words[0] ^ number.words[1] ^ 0x90000000;
    }
    if (JSVAL_IS_STRING(value))
        return js_HashString(JSVAL_TO_STRING(value)) ^ 0x70000000;
    bits = (jsuword)value;
    /* Two 16-bit shifts also work on 32-bit hosts without an invalid shift. */
    return (uint32)bits ^ (uint32)((bits >> 16) >> 16) ^ 0x50000000;
}
JS_STATIC_DLL_CALLBACK(JSBool)
CollectionMatch(JSDHashTable *table, const JSDHashEntryHdr *entry, const void *key)
{
    return SameValueZero(((const CollectionIndex *)entry)->key, *(const jsval *)key);
}
static const JSDHashTableOps collectionOps = {
    JS_DHashAllocTable, JS_DHashFreeTable, CollectionKey, CollectionHash,
    CollectionMatch, JS_DHashMoveEntryStub, JS_DHashClearEntryStub,
    JS_DHashFinalizeStub, NULL
};

JSCollectionData *
js_NewCollectionData(void)
{
    JSCollectionData *data = (JSCollectionData *)calloc(1, sizeof(*data));
    if (!data) return NULL;
    if (!JS_DHashTableInit(&data->index, &collectionOps, NULL,
                           sizeof(CollectionIndex), 16)) {
        free(data);
        return NULL;
    }
    data->references = 1;
    return data;
}
void
js_ReleaseCollectionData(JSCollectionData *data)
{
    JS_ASSERT(data && data->references);
    if (--data->references == 0) {
        JS_ASSERT(data->active == 0);
        JS_DHashTableFinish(&data->index);
        free(data->rows);
        free(data);
    }
}
JSBool
js_BeginCollectionIteration(JSCollectionData *data)
{
    if (data->references == (uint32)-1 || data->active == (uint32)-1)
        return JS_FALSE;
    ++data->references;
    ++data->active;
    return JS_TRUE;
}
void
js_EndCollectionIteration(JSCollectionData *data)
{
    JS_ASSERT(data->active);
    --data->active;
    if (!data->active && !data->size) {
        free(data->rows);
        data->rows = NULL;
        data->used = data->capacity = 0;
    }
    js_ReleaseCollectionData(data);
}
uint32 js_CollectionSize(JSCollectionData *data) { return data->size; }

/* No live traversal can have an index into the rows while compacting. */
static void
Compact(JSCollectionData *data)
{
    uint32 source, target = 0;
    CollectionIndex *entry;
    if (data->active || data->used <= 64 || data->size >= data->used / 2)
        return;
    for (source = 0; source < data->used; ++source) {
        if (!data->rows[source].live) continue;
        if (source != target) data->rows[target] = data->rows[source];
        entry = (CollectionIndex *)JS_DHashTableOperate(&data->index,
                              &data->rows[target].key, JS_DHASH_LOOKUP);
        JS_ASSERT(JS_DHASH_ENTRY_IS_BUSY(&entry->hdr));
        entry->rowPlusOne = target + 1;
        ++target;
    }
    JS_ASSERT(target == data->size);
    data->used = target;
}
JSBool
js_CollectionGet(JSCollectionData *data, jsval key, jsval *value)
{
    CollectionIndex *entry = (CollectionIndex *)JS_DHashTableOperate(&data->index,
                                                             &key, JS_DHASH_LOOKUP);
    if (!JS_DHASH_ENTRY_IS_BUSY(&entry->hdr)) return JS_FALSE;
    *value = data->rows[entry->rowPlusOne - 1].value;
    return JS_TRUE;
}
JSBool
js_CollectionPut(JSCollectionData *data, jsval key, jsval value)
{
    CollectionIndex *entry;
    CollectionRow *rows;
    uint32 capacity, slot;
    if (JSVAL_IS_NUMBER(key) &&
        (JSVAL_IS_INT(key) ? JSVAL_TO_INT(key) == 0 : *JSVAL_TO_DOUBLE(key) == 0))
        key = JSVAL_ZERO;
    Compact(data);
    entry = (CollectionIndex *)JS_DHashTableOperate(&data->index, &key, JS_DHASH_LOOKUP);
    if (JS_DHASH_ENTRY_IS_BUSY(&entry->hdr)) {
        data->rows[entry->rowPlusOne - 1].value = value;
        return JS_TRUE;
    }
    if (data->used == data->capacity) {
        if (data->capacity > ((uint32)-1) / 2) return JS_FALSE;
        capacity = data->capacity ? data->capacity * 2 : 16;
        if ((size_t)capacity > ((size_t)-1) / sizeof(CollectionRow)) return JS_FALSE;
        rows = (CollectionRow *)realloc(data->rows, (size_t)capacity * sizeof(CollectionRow));
        if (!rows) return JS_FALSE;
        data->rows = rows;
        data->capacity = capacity;
    }
    entry = (CollectionIndex *)JS_DHashTableOperate(&data->index, &key, JS_DHASH_ADD);
    if (!entry) return JS_FALSE;
    slot = data->used;
    entry->key = key;
    entry->rowPlusOne = slot + 1;
    data->rows[slot].key = key;
    data->rows[slot].value = value;
    data->rows[slot].live = JS_TRUE;
    ++data->used;
    ++data->size;
    return JS_TRUE;
}
JSBool
js_CollectionDelete(JSCollectionData *data, jsval key)
{
    CollectionIndex *entry = (CollectionIndex *)JS_DHashTableOperate(&data->index,
                                                              &key, JS_DHASH_LOOKUP);
    uint32 slot;
    if (!JS_DHASH_ENTRY_IS_BUSY(&entry->hdr)) return JS_FALSE;
    slot = entry->rowPlusOne - 1;
    JS_DHashTableRawRemove(&data->index, &entry->hdr);
    data->rows[slot].live = JS_FALSE;
    data->rows[slot].key = data->rows[slot].value = JSVAL_VOID;
    --data->size;
    Compact(data);
    return JS_TRUE;
}
JS_STATIC_DLL_CALLBACK(JSDHashOperator)
ClearIndex(JSDHashTable *table, JSDHashEntryHdr *entry, uint32 number, void *arg)
{
    return JS_DHASH_REMOVE;
}
void
js_CollectionClear(JSCollectionData *data)
{
    uint32 i;
    JS_DHashTableEnumerate(&data->index, ClearIndex, NULL);
    if (data->active) {
        for (i = 0; i < data->used; ++i) {
            data->rows[i].live = JS_FALSE;
            data->rows[i].key = data->rows[i].value = JSVAL_VOID;
        }
    } else {
        free(data->rows);
        data->rows = NULL;
        data->used = data->capacity = 0;
    }
    data->size = 0;
}
JSBool
js_CollectionNext(JSCollectionData *data, uint32 *cursor, jsval *key, jsval *value)
{
    JS_ASSERT(data->active);
    while (*cursor < data->used) {
        CollectionRow *row = &data->rows[(*cursor)++];
        if (!row->live) continue;
        *key = row->key;
        *value = row->value;
        return JS_TRUE;
    }
    return JS_FALSE;
}
void
js_MarkCollectionData(JSContext *cx, JSCollectionData *data)
{
    uint32 i;
    if (!data) return;
    for (i = 0; i < data->used; ++i) {
        CollectionRow *row = &data->rows[i];
        if (!row->live) continue;
        if (JSVAL_IS_GCTHING(row->key) && !JSVAL_IS_NULL(row->key))
            GC_MARK(cx, JSVAL_TO_GCTHING(row->key), "collection key");
        if (JSVAL_IS_GCTHING(row->value) && !JSVAL_IS_NULL(row->value))
            GC_MARK(cx, JSVAL_TO_GCTHING(row->value), "collection value");
    }
}
