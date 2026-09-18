/* Canonical bytecode PCs and assignment hints across extended atom operands.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "jsapi.h"
#include "jsdbgapi.h"
#include "jsopcode.h"
#include "jsscript.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks, failures, writes, traps;
static JSBool callbackOK;
static unsigned resolves, firstResolveFlags;
static const uint8 lengths[] = {
#define OPDEF(op,val,name,token,length,nuses,ndefs,prec,format) length,
#include "jsopcode.tbl"
#undef OPDEF
};

static JSBool
IsProbe(JSContext *cx, jsval id)
{
    return JSVAL_IS_STRING(id) &&
           !strcmp(JS_GetStringBytes(JSVAL_TO_STRING(id)), "referenceNativeValue");
}

static JSBool
Setter(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    JSStackFrame *iter = NULL, *frame;
    JSScript *script;
    jsbytecode *pc;
    JSOp op;
    jsval value;
    const char *nested = "({nested:1}).nested===1";
    if (!IsProbe(cx, id)) return JS_TRUE;
    ++writes;
    callbackOK = callbackOK && JS_IsAssigning(cx);
    frame = JS_FrameIterator(cx, &iter);
    script = frame ? JS_GetFrameScript(cx, frame) : NULL;
    pc = frame ? JS_GetFramePC(cx, frame) : NULL;
    if (!script || !pc) {
        callbackOK = JS_FALSE;
        return JS_TRUE;
    }
    op = (JSOp)*pc;
    if (op == JSOP_TRAP) op = JS_GetTrapOpcode(cx, script, pc);
    if (op == JSOP_LITOPX) op = (JSOp)pc[1 + LITERAL_INDEX_LEN];
    callbackOK = callbackOK &&
                 (op == JSOP_SETPROP || op == JSOP_SETREF ||
                  op == JSOP_SETNAME || op == JSOP_SETELEM);
    JS_GC(cx);
    if (!JS_EvaluateScript(cx, JS_GetGlobalObject(cx), nested, strlen(nested),
                           "nested-reference", 1, &value)) return JS_FALSE;
    callbackOK = callbackOK && value == JSVAL_TRUE && JS_IsAssigning(cx);
    return JS_TRUE;
}

static JSTrapStatus
Trap(JSContext *cx, JSScript *script, jsbytecode *pc, jsval *rval, void *closure)
{
    ++traps;
    JS_GC(cx);
    return JSTRAP_CONTINUE;
}

static JSBool
Resolve(JSContext *cx, JSObject *obj, jsval id, uintN flags, JSObject **resolved)
{
    *resolved = NULL;
    if (JSVAL_IS_STRING(id) &&
        !strcmp(JS_GetStringBytes(JSVAL_TO_STRING(id)), "referenceNativeMissing")) {
        if (!resolves++) firstResolveFlags = flags;
        JS_GC(cx);
    }
    return JS_TRUE;
}

static JSClass globalClass = {
    "global", JSCLASS_NEW_RESOLVE,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, Setter,
    JS_EnumerateStub, (JSResolveOp)Resolve, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

#define CHECK(c) do { ++checks; if (!(c)) { \
    ++failures; fprintf(stderr,"FAIL reference embedding check %u\n",checks); \
} } while (0)

int main(void)
{
    JSRuntime *rt;
    JSContext *cx;
    JSObject *global;
    JSScript *script;
    jsbytecode *pc;
    jsval result;
    char *source, *cursor;
    unsigned edition, width, i, count;
    JSOp op;
    jsint length;
    const unsigned counts[] = {0, 66000, 131000};

    rt = JS_NewRuntime(64 * 1024 * 1024);
    if (!rt) return 1;
    cx = JS_NewContext(rt, 8192);
    if (!cx) { JS_DestroyRuntime(rt); return 1; }
    JS_BeginRequest(cx);
    global = JS_NewObject(cx, &globalClass, NULL, NULL);
    if (!global) return 1;
    JS_SetGlobalObject(cx, global);
    if (!JS_InitStandardClasses(cx, global)) return 1;
    source = (char *)malloc(4 * 1024 * 1024);
    if (!source) return 1;
    for (edition = 0; edition < 2; ++edition) {
        JS_SetVersion(cx, edition ? JSVERSION_ECMA_2015 : JSVERSION_1_7);
        for (width = 0; width < 3; ++width) {
            cursor = source;
            cursor += sprintf(cursor, "var atom;");
            for (i = 0; i < counts[width]; ++i)
                cursor += sprintf(cursor, "atom='reference%u';", i);
            cursor += sprintf(cursor, "\nthis.referenceNativeValue=1;"
                                      "referenceNativeValue=2;true;");
            script = JS_CompileScript(cx, global, source, strlen(source),
                                       "reference-embedding", 1);
            CHECK(script != NULL);
            if (!script) goto done;
            writes = traps = 0; callbackOK = JS_TRUE;
            CHECK(JS_ExecuteScript(cx, global, script, &result) && result == JSVAL_TRUE);
            CHECK(writes == 2 && callbackOK);
            /* Trap the writes themselves, including the extended prefix. */
            count = 0;
            for (pc = script->code; pc < script->code + script->length; pc += length) {
                op = (JSOp)*pc;
                length = lengths[op];
                if (op == JSOP_LITOPX) {
                    op = (JSOp)pc[1 + LITERAL_INDEX_LEN];
                    length = lengths[op] + JSOP_LITOPX_LENGTH - (1 + ATOM_INDEX_LEN);
                }
                if (JS_PCToLineNumber(cx, script, pc) == 2 &&
                    (op == JSOP_SETPROP || op == JSOP_SETREF ||
                     op == JSOP_SETNAME || op == JSOP_SETELEM)) {
                    /* Atom-fill stores use SETGVAR and do not enter here. */
                    if (!JS_SetTrap(cx, script, pc, Trap, NULL)) goto done;
                    ++count;
                }
                if (length <= 0) goto done;
            }
            writes = traps = 0; callbackOK = JS_TRUE;
            CHECK(JS_ExecuteScript(cx, global, script, &result) && result == JSVAL_TRUE);
            CHECK(writes == 2 && callbackOK && traps == count && count >= 2);
            JS_ClearScriptTraps(cx, script);
            JS_DestroyScript(cx, script);
        }
    }
    JS_SetVersion(cx, JSVERSION_ECMA_2015);
    for (width = 0; width < 2; ++width) {
        for (edition = 0; edition < 2; ++edition) {
            JS_DeleteProperty(cx, global, "referenceNativeMissing");
            resolves = firstResolveFlags = 0;
            cursor = source;
            cursor += sprintf(cursor, "var atom;");
            for (i = 0; i < (width ? 131000U : 0U); ++i)
                cursor += sprintf(cursor, "atom='reference%u';", i);
            cursor += sprintf(cursor, "%sreferenceNativeMissing=3;true;",
                              edition ? "this." : "");
            CHECK(JS_EvaluateScript(cx, global, source, strlen(source),
                                     "reference-resolve", 1, &result) && result == JSVAL_TRUE);
            CHECK(resolves && (firstResolveFlags & JSRESOLVE_ASSIGNING) &&
                  !!(firstResolveFlags & JSRESOLVE_QUALIFIED) == !!edition);
        }
    }
done:
    free(source);
    JS_EndRequest(cx);
    JS_DestroyContext(cx);
    JS_DestroyRuntime(rt);
    JS_ShutDown();
    printf("ES6-REFERENCE-EMBEDDING checks=%u failures=%u\n",checks,failures);
    return failures || checks != 38;
}
