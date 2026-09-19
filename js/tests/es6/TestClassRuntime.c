/* Class constructor prototype wiring, derived receivers and embedding behavior.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
#include "jsapi.h"
#include "jsfun.h"
#include "jscntxt.h"
#include "jsinterp.h"
#include <stdio.h>
#include <string.h>
static unsigned checks;
static JSClass globalClass={"global",JSCLASS_GLOBAL_FLAGS,JS_PropertyStub,JS_PropertyStub,JS_PropertyStub,JS_PropertyStub,JS_EnumerateStub,JS_ResolveStub,JS_ConvertStub,JS_FinalizeStub,JSCLASS_NO_OPTIONAL_MEMBERS};
static JSBool MakeClass(JSContext *cx,JSObject *obj,uintN argc,jsval *argv,jsval *rval){
 JSFunction *fun;
 if(!argc||(fun=JS_ValueToFunction(cx,argv[0]))==NULL)return JS_FALSE;
 fun->kind|=JSFUN_KIND_CLASS|JSFUN_KIND_HOME_OBJECT;fun->flags&=~JSFUN_NO_CONSTRUCT;
 if(argc>2&&argv[2]==JSVAL_TRUE){fun->kind|=JSFUN_KIND_DERIVED;fun->flags|=JSFUN_HEAVYWEIGHT;}
 if(!js_InitClassConstructor(cx,JSVAL_TO_OBJECT(argv[0]),argc>1?argv[1]:JSVAL_VOID,argc>1))return JS_FALSE;
 *rval=argv[0];return JS_TRUE;
}
static JSBool SuperInitialize(JSContext *cx,JSObject *obj,uintN argc,jsval *argv,jsval *rval){
 JSObject *constructor,*target,*cell;JSStackFrame *caller=cx->fp->down,*frame=cx->fp;
 jsval roots[3],*base,*oldsp;JSTempValueRooter root;void *mark;uintN i;JSBool ok;
 if(!js_GetSuperCallEnvironment(cx,caller,&constructor,&target,&cell))return JS_FALSE;
 roots[0]=OBJECT_TO_JSVAL(constructor);roots[1]=OBJECT_TO_JSVAL(target);roots[2]=OBJECT_TO_JSVAL(cell);
 JS_PUSH_TEMP_ROOT(cx,3,roots,&root);
 base=js_AllocStack(cx,argc+2,&mark);if(!base){JS_POP_TEMP_ROOT(cx,&root);return JS_FALSE;}
 base[0]=roots[0];base[1]=JSVAL_NULL;for(i=0;i<argc;i++)base[i+2]=argv[i];oldsp=frame->sp;frame->sp=base+argc+2;
 ok=js_InternalInvokeConstructorWithNewTarget(cx,base,argc,target);
 if(ok){*rval=base[0];ok=js_BindDerivedThis(cx,cell,JSVAL_TO_OBJECT(*rval));}
 frame->sp=oldsp;js_FreeStack(cx,mark);JS_POP_TEMP_ROOT(cx,&root);return ok;
}
static JSBool Collect(JSContext *cx,JSObject *obj,uintN argc,jsval *argv,jsval *rval){JS_GC(cx);*rval=JSVAL_VOID;return JS_TRUE;}
static void Report(JSContext *cx,const char *message,JSErrorReport *error){fprintf(stderr,"%.200s\n",message);}
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"CLASS-RUNTIME FAIL %u\n",checks);goto out;}}while(0)
int main(void){
 JSRuntime *rt=JS_NewRuntime(8*1024*1024);JSContext *cx;JSObject *global=NULL;jsval result=JSVAL_VOID;unsigned i;int status=1;
 const char *cases[]={
 "Object.getPrototypeOf(C)===Function.prototype",
 "Object.getPrototypeOf(C.prototype)===Object.prototype&&C.prototype.constructor===C",
 "(function(){var d=Object.getOwnPropertyDescriptor(C,'prototype');return !d.writable&&!d.enumerable&&!d.configurable})()",
 "(function(){var d=Object.getOwnPropertyDescriptor(C.prototype,'constructor');return d.writable&&!d.enumerable&&d.configurable})()",
 "(function(){var o=new C(7);return o.value===7&&o.target===C&&o instanceof C})()",
 "throws(function(){C(1)})",
 "throws(function(){C.call({},1)})",
 "throws(function(){C.apply({},[1])})",
 "throws(function(){Reflect.apply(C,{},[1])})",
 "throws(function(){C.bind({})(1)})",
 "throws(function(){new Proxy(C,{})(1)})",
 "(function(){var B=C.bind(null,8),o=new B();return o.value===8&&o.target===C&&o instanceof C})()",
 "(function(){function T(){}var o=Reflect.construct(C,[9],T);return o.value===9&&o.target===T&&o instanceof T})()",
 "(function(){var P=new Proxy(C,{}),o=new P(10);return o.value===10&&o.target===P&&o instanceof C})()",
 "(function(){var N=makeClass(function(){'use strict';return 3},null);return Object.getPrototypeOf(N)===Function.prototype&&Object.getPrototypeOf(N.prototype)===null&&Object.getPrototypeOf(new N())===N.prototype})()",
 "(function(){var D=makeClass(function(){'use strict';this.x=1},C);return Object.getPrototypeOf(D)===C&&Object.getPrototypeOf(D.prototype)===C.prototype&&(new D()).x===1})()",
 "throws(function(){makeClass(function(){'use strict'},undefined)})",
 "throws(function(){makeClass(function(){'use strict'},3)})",
 "throws(function(){makeClass(function(){'use strict'},()=>{})})",
 "throws(function(){var B=function(){};B.prototype=3;makeClass(function(){'use strict'},B)})",
 "(function(){var n=0,B=new Proxy(function(){},{get:function(t,k,r){if(k==='prototype'){n++;collect();return null}return Reflect.get(t,k,r)}}),D=makeClass(function(){'use strict'},B);return n===1&&Object.getPrototypeOf(D)===B&&Object.getPrototypeOf(D.prototype)===null})()",
 "(function(){var value={marker:2},D=makeClass(function(){'use strict';collect();return value});return new D()===value})()"
 ,"(function(){var D=makeClass(function(){'use strict';superInitialize(17);this.other=2},C,true),o=new D();return o.value===17&&o.other===2&&o.target===D&&o instanceof D&&o instanceof C})()"
 ,"(function(){var D=makeClass(function(){'use strict';return {}},C,true);return typeof new D()==='object'})()"
 ,"throws(function(){new (makeClass(function(){'use strict';return 3},C,true))()})"
 ,"refThrows(function(){new (makeClass(function(){'use strict';return this},C,true))()})"
 ,"refThrows(function(){new (makeClass(function(){'use strict'},C,true))()})"
 ,"refThrows(function(){new (makeClass(function(){'use strict';eval('this')},C,true))()})"
 ,"refThrows(function(){new (makeClass(function(){'use strict';var f=()=>this;f()},C,true))()})"
 ,"(function(){var D=makeClass(function(){'use strict';var f=()=>this;superInitialize(18);return f()},C,true);return new D().value===18})()"
 ,"(function(){var D=makeClass(function(){'use strict';var f=()=>{superInitialize(19);return this};return f()},C,true);return new D().value===19})()"
 ,"(function(){var late,D=makeClass(function(){'use strict';late=()=>{superInitialize(20);return this};return {}},C,true);new D();collect();var o=late();return o.value===20&&o.target===D&&o instanceof D})()"
 ,"refThrows(function(){new (makeClass(function(){'use strict';superInitialize(1);superInitialize(2)},C,true))()})"
 ,"(function(){var D=makeClass(function(){'use strict';superInitialize(21)},C,true);function T(){}var o=Reflect.construct(D,[],T);return o.value===21&&o.target===T&&o instanceof T})()"
 ,"(function(){var D=makeClass(function(){'use strict';superInitialize(22)},C,true),P=new Proxy(D,{}),o=new P();return o.value===22&&o.target===P})()"
 ,"throws(function(){new (makeClass(function(){'use strict';superInitialize()},null,true))()})"
 ,"(function(){var reads=0,D=makeClass(function(){'use strict';return {}},C,true),T=new Proxy(function(){},{get:function(t,k,r){if(k==='prototype')reads++;return Reflect.get(t,k,r)}});Reflect.construct(D,[],T);return reads===0})()"
 ,"(function(){var calls=0,B=makeClass(function(){'use strict';calls++}),D=makeClass(function(){'use strict';superInitialize();superInitialize()},B,true);return refThrows(function(){new D()})&&calls===2})()"
 ,"(function(){var D=makeClass(function(){'use strict';superInitialize(23);return eval('this')},C,true);return new D().value===23})()"
 ,"(function(){var D=makeClass(function(){'use strict';var f=()=>()=>{superInitialize(24);return this};return f()()},C,true);return new D().value===24})()"
 ,"(function(){var B=makeClass(function(){'use strict';this.x=1}),D=makeClass(({m(){'use strict';superInitialize();this.y=super.value}}).m,B,true);B.prototype.value=25;return new D().y===25})()"
 };
 const char *setup="function throws(f){try{f()}catch(e){return e instanceof TypeError}return false}function refThrows(f){try{f()}catch(e){return e instanceof ReferenceError}return false}var C=makeClass(function C(v){'use strict';collect();this.value=v;this.target=new.target});";
 if(!rt)return 1;cx=JS_NewContext(rt,8192);if(!cx){JS_DestroyRuntime(rt);return 1;}JS_BeginRequest(cx);JS_SetErrorReporter(cx,Report);JS_SetVersion(cx,JSVERSION_ECMA_2015);JS_AddNamedRoot(cx,&global,"global");JS_AddNamedRoot(cx,&result,"result");global=JS_NewObject(cx,&globalClass,NULL,NULL);CHECK(global);JS_SetGlobalObject(cx,global);CHECK(JS_InitStandardClasses(cx,global));CHECK(JS_DefineFunction(cx,global,"makeClass",MakeClass,1,0));CHECK(JS_DefineFunction(cx,global,"collect",Collect,0,0));CHECK(JS_DefineFunction(cx,global,"superInitialize",SuperInitialize,0,0));CHECK(JS_EvaluateScript(cx,global,setup,strlen(setup),"class-setup",1,&result));
 for(i=0;i<sizeof(cases)/sizeof(cases[0]);i++){CHECK(JS_EvaluateScript(cx,global,cases[i],strlen(cases[i]),"class-runtime",i+1,&result)&&result==JSVAL_TRUE);}
 printf("CLASS-RUNTIME PASS checks=%u\n",checks);status=0;
 out:JS_RemoveRoot(cx,&result);JS_RemoveRoot(cx,&global);JS_EndRequest(cx);JS_DestroyContext(cx);JS_DestroyRuntime(rt);JS_ShutDown();return status;
}
