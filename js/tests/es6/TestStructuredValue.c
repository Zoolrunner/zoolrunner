/* Host structured-storage regression; MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "jsapi.h"
#include <stdio.h>
#include <string.h>
static JSStructuredValue *saved;
static unsigned checks;
static JSClass globalClass = {
    "global", JSCLASS_GLOBAL_FLAGS,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};
static void Report(JSContext *cx, const char *message, JSErrorReport *report)
{ fprintf(stderr, "%s\n", message); }
static JSBool Save(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSStructuredValue *next = NULL;
    JSBool unsupported = JS_FALSE;
    if (!JS_WriteStructuredValue(cx, argc ? argv[0] : JSVAL_VOID, &next, &unsupported)) {
        if (!unsupported) return JS_FALSE;
        *rval = JSVAL_FALSE;
        return JS_TRUE;
    }
    JS_FreeStructuredValue(saved);
    saved = next;
    *rval = JSVAL_TRUE;
    return JS_TRUE;
}
static JSBool Read(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{ return saved && JS_ReadStructuredValue(cx, JS_GetGlobalObject(cx), saved, rval); }
static JSBool Collect(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{ JS_GC(cx); *rval=JSVAL_VOID; return JS_TRUE; }
static JSBool Detach(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    if (!argc || JSVAL_IS_PRIMITIVE(argv[0])) return JS_FALSE;
    *rval=JSVAL_VOID;
    return JS_DetachArrayBuffer(cx,JSVAL_TO_OBJECT(argv[0]));
}
static JSBool RawWrapper(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSObject *raw;
    if (!argc || JSVAL_IS_PRIMITIVE(argv[0])) return JS_FALSE;
    raw=JS_NewObject(cx,JS_GET_CLASS(cx,JSVAL_TO_OBJECT(argv[0])),NULL,JS_GetGlobalObject(cx));
    if(!raw)return JS_FALSE;
    *rval=OBJECT_TO_JSVAL(raw);return JS_TRUE;
}
static unsigned branches;
static JSBool stopBranches;
static JSBool Branch(JSContext *cx, JSScript *script)
{
    if (!script) { ++branches; JS_GC(cx); if(stopBranches)return JS_FALSE; }
    return JS_TRUE;
}
static JSBool DenySecret(JSContext *cx, JSObject *obj, jsval id,
                         JSAccessMode mode, jsval *value)
{
    if (JSVAL_IS_STRING(id) && strcmp(JS_GetStringBytes(JSVAL_TO_STRING(id)),"secret")==0) {
        JS_ReportError(cx,"embedding denied secret read");return JS_FALSE;
    }
    return JS_TRUE;
}
static JSBool Evaluate(JSContext *cx, JSObject *global, const char *source)
{
    jsval result;
    ++checks;
    if (!JS_EvaluateScript(cx, global, source, strlen(source), "structured-value", checks, &result) || result != JSVAL_TRUE) {
        fprintf(stderr, "FAIL structured value check %u: %s\n", checks, source);
        return JS_FALSE;
    }
    return JS_TRUE;
}
static JSObject *Global(JSContext *cx)
{
    JSObject *global=JS_NewObject(cx,&globalClass,NULL,NULL);
    if (!global) return NULL;
    JS_SetGlobalObject(cx,global);
    if (!JS_InitStandardClasses(cx,global) ||
        !JS_DefineFunction(cx,global,"save",Save,1,0) ||
        !JS_DefineFunction(cx,global,"read",Read,0,0) ||
        !JS_DefineFunction(cx,global,"gc",Collect,0,0) ||
        !JS_DefineFunction(cx,global,"detach",Detach,1,0) ||
        !JS_DefineFunction(cx,global,"rawWrapper",RawWrapper,1,0)) return NULL;
    return global;
}
#define CHECK(source) do { if(!Evaluate(cx,global,source)) goto out; } while(0)
int main(void)
{
    JSRuntime *rt=JS_NewRuntime(32L*1024*1024);
    JSContext *cx;
    JSObject *global;
    int status=1;
    jsval input;
    JSStructuredValue *aborted=NULL;
    JSBool unsupported;
    if (!rt) return 1;
    cx=JS_NewContext(rt,8192);
    if (!cx) {JS_DestroyRuntime(rt);return 1;}
    JS_BeginRequest(cx);JS_SetErrorReporter(cx,Report);JS_SetVersion(cx,JSVERSION_ECMA_2015);
    global=Global(cx);if(!global)goto out;
    CHECK("save(undefined)&&read()===undefined&&save(null)&&read()===null");
    CHECK("save(true)&&read()===true&&save(false)&&read()===false");
    CHECK("save(-0)&&1/read()===-Infinity&&save(NaN)&&isNaN(read())&&save(Infinity)&&read()===Infinity");
    CHECK("save('a\\u0000\\ud800z')&&read()==='a\\u0000\\ud800z'");
    CHECK("var a={x:1};a.self=a;save(a);var b=read();b!==a&&b.self===b&&b.x===1");
    CHECK("a.x=9;gc();read().x===1&&read()!==read()");
    CHECK("var x={};save({a:x,b:x});var y=read();y.a===y.b&&y.a!==x");
    CHECK("var a=new Array(10);a[3]=undefined;a.extra=8;save(a);var b=read();b.length===10&&!(0 in b)&&(3 in b)&&b[3]===undefined&&b.extra===8");
    CHECK("var a=Object.create({inherited:9});a.x=1;Object.defineProperty(a,'hidden',{value:2});save(a);var b=read();Object.getPrototypeOf(b)===Object.prototype&&Object.keys(b).join(',')==='x'&&!('inherited' in b)");
    CHECK("var order=[];var a={get z(){order.push('z');return {get a(){order.push('a');gc();return 1;}};},get q(){order.push('q');return 2;}};save(a)&&order.join(',')==='z,a,q'&&read().z.a===1");
    CHECK("var a={get x(){delete this.y;return 1;},y:2};save(a)&&Object.keys(read()).join(',')==='x'");
    CHECK("var a={get x(){Object.defineProperty(this,'y',{enumerable:false});return 1;},y:2};save(a)&&Object.keys(read()).join(',')==='x'");
    CHECK("var a={get x(){this.z=3;return 1;},y:2};save(a)&&Object.keys(read()).join(',')==='x,y'");
    CHECK("var token={};var a={get x(){gc();throw token;}};var caught=false;try{save(a)}catch(e){caught=e===token}caught&&read().y===2");
    CHECK("var a={get x(){save({nested:true});gc();return 7;}};save(a)&&read().x===7");
    CHECK("var a={};Object.defineProperty(a,'__proto__',{value:{safe:true},enumerable:true});save(a);var b=read();Object.getPrototypeOf(b)===Object.prototype&&b.hasOwnProperty('__proto__')&&b.__proto__.safe");
    CHECK("var a={};Object.defineProperty(a,'x',{get:function(){return 8;},enumerable:true});Object.freeze(a);save(a);var d=Object.getOwnPropertyDescriptor(read(),'x');d.value===8&&d.writable&&d.enumerable&&d.configurable");
    CHECK("var a={};a[Symbol('skip')]=function(){};save(a)&&Object.getOwnPropertySymbols(read()).length===0");
    CHECK("!save(Symbol('bad'))&&!save(function(){})&&!save({bad:Symbol('bad')})");
    CHECK("var calls=0;var p=new Proxy({},{ownKeys:function(){++calls;return[];}});!save(p)&&calls===0");
    CHECK("var d=new Date(123456789);d.extra=1;save(d);var c=read();c instanceof Date&&+c===123456789&&!('extra' in c)");
    CHECK("save(new Date(NaN))&&isNaN(+read())");
    CHECK("save(Date.prototype)&&Object.getPrototypeOf(read())===Object.prototype");
    CHECK("save(new Boolean(false))&&read() instanceof Boolean&&read().valueOf()===false");
    CHECK("save(new Number(-0))&&read() instanceof Number&&1/read().valueOf()===-Infinity");
    CHECK("save(new String('x\\u0000\\ud800'))&&read() instanceof String&&read().valueOf()==='x\\u0000\\ud800'&&read().length===3");
    CHECK("var order=[];var a={};Object.defineProperty(a,'9',{get:function(){order.push(9);return 9;},enumerable:true});Object.defineProperty(a,'2',{get:function(){order.push(2);return 2;},enumerable:true});save(a)&&order.join(',')==='2,9'");
    CHECK("var m=new Map();m.set(m,m);save(m);var c=read();c instanceof Map&&c.get(c)===c&&c.size===1");
    CHECK("var x={key:true},m=new Map([[x,x]]);save({m:m,x:x});var c=read();c.m.get(c.x)===c.x");
    CHECK("var m=new Map(),k={get x(){m.clear();gc();return 1;}};m.set(k,2);m.set(3,4);save(m);var c=read();c.size===2&&c.get(3)===4");
    CHECK("var s=new Set();s.add(s);save(s);var c=read();c instanceof Set&&c.has(c)&&c.size===1");
    CHECK("var s=new Set(),k={get x(){s.clear();gc();return 1;}};s.add(k);s.add(7);save(s);read().size===2&&read().has(7)");
    CHECK("var m=new Map([[NaN,-0]]);save(m);var c=read();c.has(NaN)&&1/c.get(NaN)===-Infinity");
    CHECK("save(Map.prototype)&&Object.getPrototypeOf(read())===Object.prototype&&save(Set.prototype)&&Object.getPrototypeOf(read())===Object.prototype");
    CHECK("!save(new WeakMap())&&!save(new WeakSet())&&!save(Promise.resolve(1))");
    CHECK("var r=/a+/gim;r.lastIndex=9;r.extra=1;save(r);var c=read();c instanceof RegExp&&c.source==='a+'&&c.flags==='gim'&&c.lastIndex===0&&!('extra' in c)&&c.test('AAA')");
    CHECK("var r=/x/uy;Object.defineProperty(r,'source',{get:function(){throw 1;}});Object.defineProperty(r,'flags',{get:function(){throw 2;}});save(r);var c=read();c.unicode&&c.sticky&&c.source==='x'");
    CHECK("save(RegExp.prototype)&&Object.getPrototypeOf(read())===Object.prototype");
    CHECK("var a={},p=a;for(var i=0;i<2000;++i)p=p.next={};var safe=false;try{save(a)}catch(e){safe=true}safe");
    CHECK("var b=new ArrayBuffer(8),v=new Uint8Array(b);v[0]=123;save(b);v[0]=9;gc();var c=read();c!==b&&c.byteLength===8&&new Uint8Array(c)[0]===123");
    CHECK("save(new ArrayBuffer(0))&&read().byteLength===0");
    CHECK("var b=new ArrayBuffer(32),a=new Uint16Array(b,4,3),d=new DataView(b,2,8);a[0]=500;save({a:a,d:d,b:b,again:a});var c=read();c.a instanceof Uint16Array&&c.d instanceof DataView&&c.a.buffer===c.b&&c.d.buffer===c.b&&c.again===c.a&&c.a.byteOffset===4&&c.a.length===3&&c.a[0]===500&&c.d.byteOffset===2&&c.d.byteLength===8");
    CHECK("var cs=[Int8Array,Uint8Array,Uint8ClampedArray,Int16Array,Uint16Array,Int32Array,Uint32Array,Float32Array,Float64Array];var ok=true;for(var i=0;i<cs.length;++i){var a=new cs[i](3);a[0]=27;a[1]=-1;save(a);var c=read();ok=ok&&c instanceof cs[i]&&c.length===3&&c[0]===a[0]&&c[1]===a[1];}ok");
    CHECK("var a=new Float64Array(2);a[0]=-0;a[1]=NaN;save(a);var c=read();1/c[0]===-Infinity&&isNaN(c[1])");
    CHECK("var b=new ArrayBuffer(8),a=new Uint8Array(b);detach(b);!save(b)&&!save(a)");
    CHECK("var b=new ArrayBuffer(8),d=new DataView(b);detach(b);!save(d)");
    CHECK("var a=new Uint8Array([1,2]);Object.defineProperty(a,'buffer',{get:function(){throw 1;}});Object.defineProperty(a,'length',{get:function(){throw 2;}});a.extra=function(){};save(a);var c=read();c.length===2&&c[1]===2&&!('extra' in c)");
    CHECK("var b=new ArrayBuffer(8),d=new DataView(b,1,4);Object.defineProperty(d,'byteLength',{get:function(){throw 1;}});save(d)&&read().byteLength===4");
    CHECK("save(ArrayBuffer.prototype)&&Object.getPrototypeOf(read())===Object.prototype&&save(DataView.prototype)&&Object.getPrototypeOf(read())===Object.prototype");
    CHECK("var a=[];a.length=4294967295;a[4294967294]=7;save(a);var c=read();c.length===4294967295&&c[4294967294]===7&&!(0 in c)");
    CHECK("var xs=[Math,JSON,Reflect,Promise.prototype,Symbol.prototype,WeakMap.prototype,WeakSet.prototype];var ok=true;for(var i=0;i<xs.length;++i){xs[i].cloneProbe=i;ok=ok&&save(xs[i])&&read().cloneProbe===i&&Object.getPrototypeOf(read())===Object.prototype;delete xs[i].cloneProbe;}ok");
    CHECK("!save(Object(Symbol('boxed')))");
    CHECK("!save(rawWrapper(new String('x')))&&!save(rawWrapper(new Number(1)))&&!save(rawWrapper(new Boolean(true)))");
    CHECK("var protectedSource={secret:42};true");
    JS_SetCheckObjectAccessCallback(rt,DenySecret);
    CHECK("var denied=false;try{save(protectedSource)}catch(e){denied=String(e).indexOf('embedding denied secret read')>=0}denied");
    JS_SetCheckObjectAccessCallback(rt,NULL);
    JS_SetBranchCallback(cx,Branch);
    CHECK("var a={get x(){gc();return {value:8};}};a.self=a;save(a);var c=read();c.self===c&&c.x.value===8");
    CHECK("var b=new ArrayBuffer(8),v=new Uint8Array(b);v[3]=27;var m=new Map([[b,v]]);save({m:m,b:b,v:v});var c=read();c.m.get(c.b)===c.v&&c.v.buffer===c.b&&c.v[3]===27");
    CHECK("var r=/abcdef[0-9]+/g;save(r);gc();read().test('abcdef123')");
    JS_SetBranchCallback(cx,NULL);
    ++checks;if(!branches){fprintf(stderr,"FAIL no collection checkpoints\n");goto out;}
    if(!JS_GetProperty(cx,global,"protectedSource",&input))goto out;
    stopBranches=JS_TRUE;JS_SetBranchCallback(cx,Branch);unsupported=JS_FALSE;
    ++checks;
    if(JS_WriteStructuredValue(cx,input,&aborted,&unsupported)||aborted||unsupported){fprintf(stderr,"FAIL cancellation protocol\n");goto out;}
    JS_SetBranchCallback(cx,NULL);JS_ClearPendingException(cx);stopBranches=JS_FALSE;
    CHECK("var a={x:42};a.self=a;save(a)");
    /* The snapshot must survive collection and destruction of its source realm. */
    JS_EndRequest(cx);JS_DestroyContext(cx);
    cx=JS_NewContext(rt,8192);if(!cx)goto no_context;
    JS_BeginRequest(cx);JS_SetErrorReporter(cx,Report);JS_SetVersion(cx,JSVERSION_ECMA_2015);
    global=Global(cx);if(!global)goto out;JS_GC(cx);
    CHECK("var a=read();a.x===42&&a.self===a&&Object.getPrototypeOf(a)===Object.prototype");
    CHECK("var original=Object.prototype;Object=function(){throw 1;};var a=read();a.self===a&&a.__proto__===original");
    status=0;printf("STRUCTURED-VALUE checks=%u failures=0\n",checks);
  out:
    JS_SetBranchCallback(cx,NULL);JS_SetCheckObjectAccessCallback(rt,NULL);
    JS_EndRequest(cx);JS_DestroyContext(cx);
  no_context:
    JS_FreeStructuredValue(saved);JS_DestroyRuntime(rt);JS_ShutDown();return status;
}
