/* ES2015 regressions; MPL 1.1/GPL 2.0/LGPL 2.1. */
'use strict';
var checks=0;function check(v,m){checks++;if(!v)throw Error(m);}
function leaf(n,value){if(!n){gc();return value;}return leaf(n-1,value);}
function Base(n){this.tag=17;return leaf(n,undefined);}
var base=new Base(100000);check(base instanceof Base&&base.tag===17,'base receiver survives collection');
var replacement={};function Replace(){return leaf(100000,replacement);}
check(new Replace()===replacement,'base replacement object');
class Parent {constructor(){this.tag=29;}}
class Derived extends Parent {constructor(){super();return leaf(100000,undefined);}}
var derived=new Derived();check(derived instanceof Derived&&derived.tag===29,'derived undefined returns initialized receiver');
class ObjectReturn extends Parent {constructor(){return leaf(100000,replacement);}}
check(new ObjectReturn()===replacement,'derived object without super');
class Uninitialized extends Parent {constructor(){return leaf(100000,undefined);}}
var err;try{new Uninitialized();}catch(e){err=e;}check(err instanceof ReferenceError,'derived undefined requires initialized this');
class Primitive extends Parent {constructor(){super();return leaf(100000,3);}}
err=null;try{new Primitive();}catch(e){err=e;}check(err instanceof TypeError,'derived primitive rejected');
class ArrowInit extends Parent {constructor(){return (()=>{super();gc();return undefined;})();}}
var arrow=new ArrowInit();check(arrow instanceof ArrowInit&&arrow.tag===29,'tail arrow initializes detached this cell');
var capture;class Capture extends Parent {constructor(){super();capture=()=>this;return leaf(100000,undefined);}}
var captured=new Capture();gc();check(capture()===captured,'captured constructor receiver');
var realm=createTest262Realm();
realm.evalScript('var proxy=new Proxy(function(){},{apply:function(t,r,a){return a;}});function C(){"use strict";return proxy(1);}');
check(Object.getPrototypeOf(new realm.global.C())===Array.prototype,'base constructor original ES2015 resumed realm');
realm.evalScript('class D extends Object {constructor(){return proxy(1);}};this.D=D;');
check(Object.getPrototypeOf(new realm.global.D())===Array.prototype,'derived constructor original ES2015 resumed realm');
function Target(){};function NewTarget(){return leaf(100000,new.target);}
check(Reflect.construct(NewTarget,[],Target)===Target,'new target read before frame retirement');
var events=[];class Finally extends Parent {constructor(){try{return leaf(1000,replacement);}finally{events.push('finally');}}}
check(new Finally()===replacement&&events.join(',')==='finally','constructor finally runs');
function ArrowTarget(){return (()=>{gc();return new.target;})();}
check(Reflect.construct(ArrowTarget,[],Target)===Target,'new target in detached arrow');
realm.evalScript('class Bad extends Object {constructor(){return (()=>3)();}};this.Bad=Bad;');
err=null;try{new realm.global.Bad();}catch(e){err=e;}
check(err instanceof TypeError&&!(err instanceof realm.global.TypeError),'derived return validation in resumed caller realm');
var throwToken={};function throwing(n){if(!n)throw throwToken;return throwing(n-1);}
class Throws extends Parent {constructor(){return throwing(100000);}}
err=null;try{new Throws();}catch(e){err=e;}check(err===throwToken,'tail exception bypasses derived return validation');
print('TAIL-CONSTRUCTORS checks='+checks+' failures=0');
