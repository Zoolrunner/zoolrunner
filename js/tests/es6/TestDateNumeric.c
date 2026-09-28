/* Date arithmetic policy across legacy embeddings and cloned methods.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "jsapi.h"
#include "jsdate.h"
#include <stdio.h>
#include <string.h>
static unsigned checks;
static JSClass globalClass={"global",0,JS_PropertyStub,JS_PropertyStub,
 JS_PropertyStub,JS_PropertyStub,JS_EnumerateStub,JS_ResolveStub,JS_ConvertStub,
 JS_FinalizeStub,JSCLASS_NO_OPTIONAL_MEMBERS};
static JSBool Evaluate(JSContext *cx, JSObject *global, const char *source)
{
    jsval value;
    return JS_EvaluateScript(cx,global,source,strlen(source),"date-numeric",1,&value)&&value==JSVAL_TRUE;
}
static JSBool Collect(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JS_GC(cx);*rval=JSVAL_VOID;return JS_TRUE;
}
#define CHECK(c) do{++checks;if(!(c)){fprintf(stderr,"FAIL Date numeric check %u\n",checks);goto out;}}while(0)
int main(void)
{
    JSRuntime *rt=JS_NewRuntime(16*1024*1024);
    JSContext *cx;
    JSObject *global,*legacy,*clone,*date;
    jsval methods,method;
    const char *names[]={"utc","setTime","setHours","setMonth","setYear","parse"};
    unsigned i;
    int status=1;
    if(!rt)return 1;
    cx=JS_NewContext(rt,8192);if(!cx){JS_DestroyRuntime(rt);return 1;}
    JS_BeginRequest(cx);JS_SetVersion(cx,JSVERSION_ECMA_2015);
    global=JS_NewObject(cx,&globalClass,NULL,NULL);CHECK(global);
    JS_SetGlobalObject(cx,global);CHECK(JS_InitStandardClasses(cx,global));
    CHECK(JS_DefineFunction(cx,global,"collect",Collect,0,0));
    CHECK(Evaluate(cx,global,"var exports={ctor:Date,utc:Date.UTC,parse:Date.parse,setTime:Date.prototype.setTime,setHours:Date.prototype.setHours,setMonth:Date.prototype.setMonth,setYear:Date.prototype.setYear};true"));
    CHECK(JS_GetProperty(cx,global,"exports",&methods));
    JS_SetVersion(cx,JSVERSION_1_7);
    legacy=JS_NewObject(cx,&globalClass,NULL,NULL);CHECK(legacy);
    CHECK(JS_SetParent(cx,legacy,NULL));
    CHECK(JS_DefineProperty(cx,global,"legacy",OBJECT_TO_JSVAL(legacy),NULL,NULL,0));
    JS_SetGlobalObject(cx,legacy);CHECK(JS_InitStandardClasses(cx,legacy));
    CHECK(JS_DefineProperty(cx,legacy,"modern",methods,NULL,NULL,0));
    CHECK(JS_DefineFunction(cx,legacy,"collect",Collect,0,0));
    CHECK(Evaluate(cx,legacy,"1/new Date(-0.1).getTime()===-Infinity && 1/new modern.ctor(-0.1).getTime()===Infinity"));
    CHECK(Evaluate(cx,legacy,"Date.UTC(1970,0,0)===0 && modern.utc(1970,0,0)===-86400000"));
    CHECK(Evaluate(cx,legacy,"var d=new Date(NaN),calls=0;d.setHours({valueOf:function(){calls++;return 1;}});calls===0"));
    CHECK(Evaluate(cx,legacy,"modern.setHours.call(d,{valueOf:function(){calls++;collect();d.setTime(0);return 1;}});calls===1&&isNaN(d.getTime())"));
    for(i=0;i<sizeof(names)/sizeof(names[0]);++i){
        CHECK(JS_GetProperty(cx,JSVAL_TO_OBJECT(methods),names[i],&method));
        clone=JS_CloneFunctionObject(cx,JSVAL_TO_OBJECT(method),legacy);CHECK(clone);
        CHECK(JS_DefineProperty(cx,legacy,names[i],OBJECT_TO_JSVAL(clone),NULL,NULL,0));
    }
    JS_ClearNewbornRoots(cx);JS_GC(cx);
    CHECK(Evaluate(cx,legacy,"utc(1970,0,0)===-86400000 && 1/setTime.call(new Date(1),-0.1)===Infinity"));
    CHECK(Evaluate(cx,legacy,"d=new Date(NaN);calls=0;setHours.call(d,{valueOf:function(){calls++;collect();d.setTime(0);return 1;}});calls===1&&isNaN(d.getTime())"));
    CHECK(Evaluate(cx,legacy,"d=new Date(NaN);setMonth.call(d,{valueOf:function(){collect();d.setTime(0);return 1;}});isNaN(d.getTime())"));
    CHECK(Evaluate(cx,legacy,"d=new Date(0);setYear.call(d,-0.9);d.getFullYear()===1900"));
    date=js_NewDateObjectMsec(cx,1234);CHECK(date);
    CHECK(JS_DefineProperty(cx,legacy,"nativeDate",OBJECT_TO_JSVAL(date),NULL,NULL,0));
    CHECK(Evaluate(cx,legacy,"setTime.call(nativeDate,-0.1)===0 && 1/nativeDate.getTime()===Infinity"));
    CHECK(js_DateIsValid(cx,date)&&js_DateGetMsecSinceEpoch(cx,date)==0);
    JS_SetVersion(cx,JSVERSION_ECMA_2015);
    CHECK(Evaluate(cx,legacy,"Date.UTC(1970,0,0)===0 && utc(1970,0,0)===-86400000"));
    CHECK(Evaluate(cx,legacy,"Date.parse('1970-01-01T00:00:00')===0 && parse('1970-01-01T00:00:00')===new Date(1970,0,1).getTime()"));
    CHECK(Evaluate(cx,legacy,"new Date('1970-01-01T00:00:00').getTime()===0 && new modern.ctor('1970-01-01T00:00:00').getTime()===new Date(1970,0,1).getTime()"));
    printf("ES6-DATE-NUMERIC checks=%u failures=0\n",checks);status=0;
out:
    JS_EndRequest(cx);JS_DestroyContext(cx);JS_DestroyRuntime(rt);JS_ShutDown();return status;
}
