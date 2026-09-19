/* ES2015 class and derived-constructor regressions. */
(function () {
var checks=0;
function assert(v,label){checks++;if(!v)throw Error('CLASS FAIL '+checks+': '+label)}
function throws(type,f,label){var caught=false;try{f()}catch(e){caught=e instanceof type}assert(caught,label)}
class Base {constructor(x){this.x=x;this.target=new.target}m(){return this.x}static m(){return this.name}}
class Derived extends Base {constructor(x){var init=()=>super(x);init();this.y=2}m(){return super.m()+this.y}static m(){return super.m()+'!'}}
var o=new Derived(4);
assert(o.m()===6&&o instanceof Derived&&o instanceof Base,'inheritance');
assert(o.target===Derived&&Derived.m()==='Derived!','new.target and static super');
assert(Object.getPrototypeOf(Derived)===Base,'constructor prototype');
assert(Object.getPrototypeOf(Derived.prototype)===Base.prototype,'instance prototype');
throws(TypeError,function(){Derived()},'class call');
throws(TypeError,function(){Derived.call({})},'class call receiver');
throws(TypeError,function(){Reflect.apply(Derived,{},[])},'class reflect apply');
throws(TypeError,function(){new Derived.prototype.m()},'method not constructor');
var pd=Object.getOwnPropertyDescriptor(Derived,'prototype');
assert(!pd.writable&&!pd.enumerable&&!pd.configurable,'prototype descriptor');
pd=Object.getOwnPropertyDescriptor(Derived.prototype,'m');
assert(pd.writable&&!pd.enumerable&&pd.configurable,'method descriptor');
pd=Object.getOwnPropertyDescriptor(Derived,'name');
assert(pd.value==='Derived'&&!pd.writable&&!pd.enumerable&&pd.configurable,'name descriptor');
class Default extends Base {}
assert(new Default(5).x===5&&Default.length===0,'default forwarding');
class Null extends null {constructor(){return Object.create(new.target.prototype)}}
assert(Object.getPrototypeOf(Null.prototype)===null&&new Null() instanceof Null,'extends null');
throws(TypeError,function(){new (class extends null {})()},'default extends null');
throws(TypeError,function(){return class extends undefined {}},'invalid heritage');
throws(TypeError,function(){return class extends (()=>{}) {}},'nonconstructor heritage');
throws(ReferenceError,function(){new (class extends Base {constructor(){this.x=1;super()}})()},'this before super');
throws(ReferenceError,function(){new (class extends Base {constructor(){}})()},'missing super');
throws(TypeError,function(){new (class extends Base {constructor(){return 1}})()},'primitive derived return');
assert(new (class extends Base {constructor(){return {x:9}}})().x===9,'object derived return');
var calls=0;class Counter {constructor(){calls++}}
throws(ReferenceError,function(){new (class extends Counter {constructor(){super();super()}})()},'double super');
assert(calls===2,'second base executes before binding error');
var later;class Escaped extends Base {constructor(){later=()=>super(7);return {}}}
new Escaped();var late=later();assert(late.x===7&&late instanceof Escaped,'escaped initializer');
class EvalInit extends Base {constructor(){eval('super(8)');assert(eval('this')===this,'eval this')}}
assert(new EvalInit().x===8,'eval super');
class ArrowEval extends Base {constructor(){(()=>eval('super(9)'))()}}
assert(new ArrowEval().x===9,'arrow eval super');
var log=[];function key(x){log.push(x);return x}
class Ordered {[key('a')](){}static [key('b')](){}get [key('c')](){return 1}}
assert(log.join(',')==='a,b,c','computed key order');
throws(ReferenceError,function(){return class TDZ {[TDZ](){}}},'class binding TDZ');
var Named=class Inner {m(){return Inner}};
assert(new Named().m()===Named,'inner binding');
throws(TypeError,function(){new (class Inner {m(){Inner=3}})().m()},'immutable inner binding');
var Saved=Ordered;Ordered=3;assert(Ordered===3&&typeof Saved==='function','mutable declaration binding');
var Anonymous=class {};assert(Anonymous.name==='Anonymous','binding inferred name');
var symbol=Symbol('named'), holder={[symbol]:class {}};assert(holder[symbol].name==='[named]','symbol inferred name');
var Own=class {static name(){return 10}};assert(Own.name()===10,'static name retained');
assert(!(class {}).hasOwnProperty('name'),'bare anonymous name in ES2015');
var keys=Object.getOwnPropertyNames(class Named {static a(){}});
assert(keys.join(',')==='length,prototype,a,name','name installed last');
class Access {get value(){return this._v}set value(v){this._v=v;return 999}}
var a=new Access();assert((a.value=12)===12&&a.value===12,'setter result');
class Generator {*values(){yield this;yield 2}}
var g=new Generator(),it=g.values();assert(it.next().value===g&&it.next().value===2,'generator method');
class Replace extends Base {m(){return super.m()}}
Object.setPrototypeOf(Replace.prototype,{m(){return 17}});assert(new Replace(1).m()===17,'live home prototype');
var reads=0,Target=new Proxy(function(){},{get(t,k,r){if(k==='prototype')reads++;return Reflect.get(t,k,r)}});
Reflect.construct(class extends Base {constructor(){return {}}},[],Target);assert(reads===0,'no eager derived receiver');
var seen,Super=new Proxy(Base,{construct(t,args,target){seen=target;return Reflect.construct(t,args,target)}});
class ProxyChild extends Super {}var child=new ProxyChild(18);assert(child.x===18&&seen===ProxyChild,'proxy super new.target');
var text=Derived.toString(),Copy=eval('('+text+')');assert(new Copy(19).m()===21,'class source roundtrip');
function factory(){class A {m(){return 20}}return A}var Factory=eval('('+factory.toString()+')');assert(new (Factory())().m()===20,'enclosing source roundtrip');
var lazy=function(){};var method={m(){return super.prototype}};Object.setPrototypeOf(method,lazy);assert(method.m()===lazy.prototype,'super lazily resolved prototype');
var invalid=['class C {constructor(){} constructor(){}}','class C {get constructor(){}}','class C {*constructor(){}}','class C {static prototype(){}}','class C {m(){super()}}','class C {constructor(){super()}}','class C extends Base {constructor(){function f(){super()}}}','class C extends Base {constructor(){new super()}}','if (true) class C {}'];
for(var i=0;i<invalid.length;i++)throws(SyntaxError,function(){eval(invalid[i])},'early error '+i);
print('CLASS PASS checks='+checks);
})();
