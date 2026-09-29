/* Native exotic-object embedding regression; MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "jsapi.h"
#include <stdio.h>
#include <string.h>

static JSClass globalClass = {
    "global", JSCLASS_GLOBAL_FLAGS,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};
static JSClass hostClass = {
    "HostFixture", JSCLASS_HAS_RESERVED_SLOTS(2),
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JS_GetHostObjectOps, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};
static JSBool
MakeHost(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSObject *host;
    if (argc != 2 || JSVAL_IS_PRIMITIVE(argv[0]) || JSVAL_IS_PRIMITIVE(argv[1]))
        return JS_FALSE;
    host = JS_NewHostObject(cx, &hostClass, JSVAL_TO_OBJECT(argv[0]),
                            JSVAL_TO_OBJECT(argv[1]), JS_GetGlobalObject(cx));
    if (!host || !JS_IsHostObject(cx, host)) return JS_FALSE;
    *rval = OBJECT_TO_JSVAL(host);
    JS_GC(cx);
    return JS_TRUE;
}
static JSBool
Collect(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JS_GC(cx); *rval = JSVAL_VOID; return JS_TRUE;
}
static void
Report(JSContext *cx, const char *message, JSErrorReport *report)
{
    fprintf(stderr, "%s\n", message);
}

static const char *checks[] = {
    "var data={item:'old'}, back={}, handler={"
      "get:function(t,k,r){collect();return k in data?data[k]:Reflect.get(t,k,r);},"
      "set:function(t,k,v,r){collect();data[k]=String(v);return true;},"
      "has:function(t,k){return k in data;},"
      "deleteProperty:function(t,k){delete data[k];return true;},"
      "ownKeys:function(){return Object.keys(data);},"
      "getOwnPropertyDescriptor:function(t,k){collect();return k in data?"
          "{value:data[k],writable:true,enumerable:true,configurable:true}:undefined;},"
      "defineProperty:function(t,k,d){if(!('value' in d)&&!('writable' in d))return false;"
          "data[k]=String(d.value);return true;},"
      "preventExtensions:function(){return false;}"
    "};var host=makeHost(back,handler);host.item==='old'",
    "data.item='new';host.item==='new'",
    "data.added='live';'added' in host && host.hasOwnProperty('added')",
    "host.write=42;data.write==='42'",
    "Reflect.defineProperty(host,'item',{value:12,configurable:false}) && data.item==='12'",
    "var d=Object.getOwnPropertyDescriptor(host,'item');"
      "d.value==='12' && d.configurable && d.writable && d.enumerable",
    "delete data.item;!('item' in host) && Object.getOwnPropertyDescriptor(host,'item')===undefined",
    "Reflect.defineProperty(host,'item',{writable:false}) && data.item==='undefined'",
    "!Reflect.defineProperty(host,'item',{}) && !Reflect.defineProperty(host,'item',{get:function(){}})",
    "delete host.item;!('item' in data)",
    "Object.keys(host).join(',')==='added,write'",
    "var names=[];for(var k in host)names.push(k);names.join(',')==='added,write'",
    "Object.isExtensible(host) && !Reflect.preventExtensions(host) && Object.isExtensible(host)",
    "var rejected=false;try{Object.preventExtensions(host);}catch(e){rejected=e instanceof TypeError;}rejected",
    "var proto={inherited:1};Object.setPrototypeOf(host,proto);Object.getPrototypeOf(host)===proto",
    "names=[];for(var k in host)names.push(k);names.join(',')==='added,write,inherited'",
    "var it=Reflect.enumerate(host);it.next().value==='added' && it.next().value==='write' && "
      "it.next().value==='inherited' && it.next().done",
    "var s=Symbol('key');data[s]=9;handler.ownKeys=function(){return ['write','added',s];};"
      "Reflect.ownKeys(host)[2]===s && Object.getOwnPropertySymbols(host)[0]===s",
    "Object.keys(host).join(',')==='write,added'",
    "handler.ownKeys=function(){return ['10','2'];};data['10']='ten';data['2']='two';"
      "Object.keys(host).join(',')==='10,2'",
    "names=[];for(var k in host)names.push(k);names.join(',')==='10,2,inherited'",
    "var proxy=new Proxy({},handler), rejected=false;"
      "try{Reflect.defineProperty(proxy,'x',{value:1,configurable:false});}catch(e){rejected=e instanceof TypeError;}rejected",
    "var fixed={};Object.defineProperty(fixed,'x',{value:1});"
      "var liar={get:function(){collect();return 2;}};"
      "makeHost(fixed,liar).x===2",
    "rejected=false;try{new Proxy(fixed,liar).x;}catch(e){rejected=e instanceof TypeError;}rejected",
    "var bad=makeHost({}, {getOwnPropertyDescriptor:function(){return 1;}});"
      "rejected=false;try{Object.getOwnPropertyDescriptor(bad,'x');}catch(e){rejected=e instanceof TypeError;}rejected",
    "bad=makeHost({}, {ownKeys:function(){return [1];}});"
      "rejected=false;try{Reflect.ownKeys(bad);}catch(e){rejected=e instanceof TypeError;}rejected",
    "bad=makeHost({}, {getOwnPropertyDescriptor:function(){return {value:1,get:function(){}};}});"
      "rejected=false;try{Object.getOwnPropertyDescriptor(bad,'x');}catch(e){rejected=e instanceof TypeError;}rejected",
    "var ordinary={x:1}, passthrough=makeHost(ordinary,{});collect();"
      "passthrough.x===1 && Reflect.set(passthrough,'x',2) && ordinary.x===2",
    "Object.getOwnPropertyDescriptor(passthrough,'x').value===2 && "
      "Object.keys(passthrough).join(',')==='x' && Reflect.deleteProperty(passthrough,'x')",
    "typeof host==='object' && typeof passthrough==='object'",
    "rejected=false;try{host();}catch(e){rejected=e instanceof TypeError;}rejected",
    "rejected=false;try{new host();}catch(e){rejected=e instanceof TypeError;}rejected",
    "var isolated=makeHost({a:19},{});collect();isolated.a===19",
    "!Array.isArray(makeHost([],{})) && !Array.isArray(new Proxy(makeHost([],{}),{}))",
    "var receiver={}, h=makeHost({}, {get:function(t,k,r){return r;}});Reflect.get(h,'x',receiver)===receiver"
};
int main(void)
{
    JSRuntime *rt = JS_NewRuntime(8 * 1024 * 1024);
    JSContext *cx;
    JSObject *global;
    jsval result;
    unsigned i;
    int status = 1;
    if (!rt) return 1;
    cx = JS_NewContext(rt, 8192);
    if (!cx) { JS_DestroyRuntime(rt); return 1; }
    JS_BeginRequest(cx);
    JS_SetErrorReporter(cx, Report);
    JS_SetVersion(cx, JSVERSION_ECMA_2015);
    global = JS_NewObject(cx, &globalClass, NULL, NULL);
    if (!global) goto out;
    JS_SetGlobalObject(cx, global);
    if (!JS_InitStandardClasses(cx, global) ||
        !JS_DefineFunction(cx, global, "makeHost", MakeHost, 2, 0) ||
        !JS_DefineFunction(cx, global, "collect", Collect, 0, 0)) goto out;
    for (i = 0; i < sizeof(checks)/sizeof(checks[0]); ++i) {
        if (!JS_EvaluateScript(cx, global, checks[i], strlen(checks[i]),
                              "host-object", i + 1, &result) || result != JSVAL_TRUE) {
            fprintf(stderr, "FAIL host-object check %u\n", i + 1);
            goto out;
        }
    }
    printf("HOST-OBJECT checks=%u failures=0\n", i);
    status = 0;
out:
    JS_EndRequest(cx); JS_DestroyContext(cx);
    JS_DestroyRuntime(rt); JS_ShutDown();
    return status;
}
