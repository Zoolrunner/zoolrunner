/* Weak collection reachability and classic callback marking guarantees.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "jsapi.h"
#include <stdio.h>
#include <string.h>
static unsigned checks, finalized[16];
static JSObject *callbackKey, *callbackValue, *callbackOwner;
static JSBool callbackClosed;
static void ProbeFinalize(JSContext *cx, JSObject *obj)
{
    jsval value;
    if (JS_GetReservedSlot(cx, obj, 0, &value) && JSVAL_IS_INT(value) &&
        JSVAL_TO_INT(value) >= 0 && JSVAL_TO_INT(value) < 16)
        ++finalized[JSVAL_TO_INT(value)];
}
static JSClass probeClass = {
    "probe", JSCLASS_HAS_RESERVED_SLOTS(1),
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, ProbeFinalize,
    JSCLASS_NO_OPTIONAL_MEMBERS
};
static JSClass globalClass = {
    "global", 0,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};
static JSBool Probe(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSObject *probe = JS_NewObject(cx, &probeClass, NULL, NULL);
    if (!probe) return JS_FALSE;
    *rval = OBJECT_TO_JSVAL(probe);
    return JS_SetReservedSlot(cx, probe, 0, argc ? argv[0] : JSVAL_ZERO);
}
static JSBool Callback(JSContext *cx, JSGCStatus status)
{
    if (status == JSGC_MARK_END && callbackKey) {
        if (callbackOwner) JS_MarkGCThing(cx, callbackOwner, "host-owned weak collection", NULL);
        JS_MarkGCThing(cx, callbackKey, "host-owned weak key", NULL);
        /* This must hold immediately, before returning to the collector. */
        callbackClosed = !JS_IsAboutToBeFinalized(cx, callbackValue);
    }
    return JS_TRUE;
}
static JSBool Evaluate(JSContext *cx, JSObject *global, const char *source)
{
    jsval value;
    return JS_EvaluateScript(cx, global, source, strlen(source), "weak-gc", 1, &value) && value == JSVAL_TRUE;
}
static void Collect(JSContext *cx)
{
    JS_ClearNewbornRoots(cx);
    JS_GC(cx);
    JS_ClearNewbornRoots(cx);
    JS_GC(cx);
}
#define CHECK(expr) do { ++checks; if (!(expr)) { \
    fprintf(stderr,"FAIL weak GC line %d check %u\n",__LINE__,checks); goto out; } } while (0)
int main(void)
{
    JSRuntime *rt = JS_NewRuntime(16L * 1024 * 1024);
    JSContext *cx, *other = NULL;
    JSObject *global, *second;
    jsval value;
    int status = 1;
    if (!rt) return 1;
    cx = JS_NewContext(rt, 8192);
    if (!cx) { JS_DestroyRuntime(rt); return 1; }
    JS_BeginRequest(cx);
    JS_SetVersion(cx, JSVERSION_ECMA_2015);
    global = JS_NewObject(cx, &globalClass, NULL, NULL);
    CHECK(global);
    JS_SetGlobalObject(cx, global);
    CHECK(JS_InitStandardClasses(cx, global));
    CHECK(JS_DefineFunction(cx, global, "probe", Probe, 1, 0));
    CHECK(Evaluate(cx, global,"var w=new WeakMap(),k=probe(1),v=probe(2);w.set(k,v);v=null;true"));
    Collect(cx);
    CHECK(!finalized[1] && !finalized[2]);
    CHECK(Evaluate(cx,global,"w.get(k) instanceof Object && w.has(k)"));
    CHECK(Evaluate(cx,global,"k=null;true"));
    Collect(cx);
    CHECK(finalized[1]==1 && finalized[2]==1);
    CHECK(Evaluate(cx,global,"k=probe(3);v=probe(4);v.back=k;w.set(k,v);k=v=null;true"));
    Collect(cx);
    CHECK(finalized[3]==1 && finalized[4]==1);
    CHECK(Evaluate(cx,global,"k=probe(5);v=probe(6);w.set(k,v);w=v=null;true"));
    Collect(cx);
    CHECK(!finalized[5] && finalized[6]==1);
    CHECK(Evaluate(cx,global,"var a=new WeakMap(),b=new WeakMap();k=probe(7);v=probe(8);a.set(k,v);b.set(v,probe(9));v=null;true"));
    Collect(cx);
    CHECK(!finalized[7] && !finalized[8] && !finalized[9]);
    CHECK(Evaluate(cx,global,"b.has(a.get(k))"));
    CHECK(Evaluate(cx,global,"k=null;true"));
    Collect(cx);
    CHECK(finalized[7]==1 && finalized[8]==1 && finalized[9]==1);
    CHECK(Evaluate(cx,global,"w=new WeakMap();k=probe(10);v=probe(11);w.set(k,v);true"));
    CHECK(JS_GetProperty(cx,global,"k",&value));callbackKey=JSVAL_TO_OBJECT(value);
    CHECK(JS_GetProperty(cx,global,"v",&value));callbackValue=JSVAL_TO_OBJECT(value);
    CHECK(Evaluate(cx,global,"k=v=null;true"));
    JS_SetGCCallback(cx,Callback);
    Collect(cx);
    CHECK(callbackClosed && !finalized[10] && !finalized[11]);
    callbackKey=callbackValue=NULL;
    JS_SetGCCallback(cx,NULL);
    Collect(cx);
    CHECK(finalized[10]==1 && finalized[11]==1);
    CHECK(Evaluate(cx,global,"var ws=new WeakSet();k=probe(12);ws.add(k);k=null;true"));
    Collect(cx);
    CHECK(finalized[12]==1);
    CHECK(Evaluate(cx,global,"a=new WeakMap();b=new WeakMap();k=probe(13);a.set(k,b);b.set(k,probe(14));b=null;true"));
    Collect(cx);
    CHECK(!finalized[13] && !finalized[14]);
    CHECK(Evaluate(cx,global,"a.get(k).has(k)"));
    CHECK(Evaluate(cx,global,"k=null;true"));
    Collect(cx);
    CHECK(finalized[13]==1 && finalized[14]==1);
    CHECK(Evaluate(cx,global,"w=new WeakMap();k=probe(15);v=probe(0);w.set(k,v);true"));
    CHECK(JS_GetProperty(cx,global,"w",&value));callbackOwner=JSVAL_TO_OBJECT(value);
    CHECK(JS_GetProperty(cx,global,"k",&value));callbackKey=JSVAL_TO_OBJECT(value);
    CHECK(JS_GetProperty(cx,global,"v",&value));callbackValue=JSVAL_TO_OBJECT(value);
    CHECK(Evaluate(cx,global,"w=k=v=null;true"));
    callbackClosed=JS_FALSE;
    JS_SetGCCallback(cx,Callback);
    Collect(cx);
    CHECK(callbackClosed && !finalized[15] && !finalized[0]);
    callbackKey=callbackValue=callbackOwner=NULL;
    JS_SetGCCallback(cx,NULL);
    Collect(cx);
    CHECK(finalized[15]==1 && finalized[0]==1);
    JS_SetVersion(cx,JSVERSION_1_7);
    CHECK(Evaluate(cx,global,
        "var closed=0;function legacyGenerator(){try{yield 1;}finally{closed++;}}"
        "w=new WeakMap();k={};v=legacyGenerator();v.next();w.set(k,v);v=null;true"));
    Collect(cx);
    CHECK(Evaluate(cx,global,"closed===0"));
    CHECK(Evaluate(cx,global,"k=null;true"));
    Collect(cx);
    CHECK(Evaluate(cx,global,"closed===1"));
    JS_SetVersion(cx,JSVERSION_ECMA_2015);
    other=JS_NewContext(rt,8192);
    CHECK(other);
    JS_BeginRequest(other);
    JS_SetVersion(other,JSVERSION_ECMA_2015);
    second=JS_NewObject(other,&globalClass,NULL,NULL);
    CHECK(second);
    JS_SetGlobalObject(other,second);
    CHECK(JS_InitStandardClasses(other,second));
    CHECK(JS_DefineFunction(other,second,"probe",Probe,1,0));
    CHECK(Evaluate(other,second,
        "var key={},map=new WeakMap();map.set(key,{marker:42,probe:probe(0)});"
        "var exported={key:key,map:map,get:WeakMap.prototype.get};true"));
    CHECK(JS_GetProperty(other,second,"exported",&value));
    CHECK(JS_DefineProperty(cx,global,"foreign",value,NULL,NULL,0));
    JS_EndRequest(other);
    JS_DestroyContextNoGC(other);other=NULL;
    Collect(cx);
    CHECK(finalized[0]==1);
    CHECK(Evaluate(cx,global,"foreign.get.call(foreign.map,foreign.key).marker===42"));
    CHECK(JS_DeleteProperty(cx,global,"foreign"));
    Collect(cx);
    CHECK(finalized[0]==2);
    CHECK(Evaluate(cx,global,
        "w=new WeakMap();var liveKeys=[];for(var i=0;i<3000;i++){"
        "k={};w.set(k,probe(1));if(i%3===0)liveKeys.push(k);}k=null;true"));
    Collect(cx);
    CHECK(finalized[1]==2001);
    CHECK(Evaluate(cx,global,
        "(function(){for(var i=0;i<liveKeys.length;i++){if(!w.has(liveKeys[i]) || "
        "typeof w.get(liveKeys[i])!=='object')return false;}return true;})()"));
    CHECK(Evaluate(cx,global,"liveKeys=null;true"));
    Collect(cx);
    CHECK(finalized[1]==3001);
    printf("ES6-WEAK-COLLECTION-GC checks=%u failures=0\n",checks);
    status=0;
  out:
    callbackKey=callbackValue=callbackOwner=NULL;
    JS_SetGCCallback(cx,NULL);
    if (other) { JS_EndRequest(other); JS_DestroyContextNoGC(other); }
    JS_EndRequest(cx);
    JS_DestroyContext(cx);
    JS_DestroyRuntime(rt);
    JS_ShutDown();
    return status;
}
