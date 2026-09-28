var checks=0;
function check(v,label){checks++;if(!v)throw Error(label)}
function rejects(src){try{eval(src)}catch(e){return e instanceof SyntaxError}return false}
var base={x:7,m:function(){'use strict';return this},get v(){return this.saved},set v(x){this.saved=x}};
function make(){return {saved:9,m(){'use strict';return super.m()},get(){return super.x},computed(k){return super[k]},set(v){return super.v=v},add(v){return super.v+=v},post(){return super.v++},pre(){return --super.v},arrow(){return ()=>super.x},ev(){return eval('super.x')},strictEv(){'use strict';return eval('super.x')},tag(){return super.m`x`},spread(){return super.m(...[])},pattern(){[super.v]=[31];return this.saved},patternObj(){({a:super.v}={a:32});return this.saved},remove(){return delete super.x},forIn(){for(super.v in {a:1,b:2}){}return this.saved},forOf(){for(super.v of [41,42]){}return this.saved},empty(){var n=0;for(super[++n] in {}){}return n}}}
var o=make();Object.setPrototypeOf(o,base);
check(o.m()===o,'method this');check(o.m.call(3)===3,'primitive this');check(o.get()===7,'get');check(o.computed('x')===7,'computed');
check(o.set(12)===12&&o.saved===12,'set');check(o.add(3)===15&&o.saved===15,'compound');check(o.post()===15&&o.saved===16,'postfix');check(o.pre()===15&&o.saved===15,'prefix');
check(o.arrow()()===7,'arrow super');check(o.ev()===7,'direct eval');check(o.strictEv()===7,'strict eval');check(o.tag()===o,'tag receiver');check(o.spread()===o,'spread receiver');check(o.pattern()===31,'array pattern');check(o.patternObj()===32,'object pattern');
var caught=false;try{o.remove()}catch(e){caught=e instanceof ReferenceError}check(caught,'delete');
check(o.forIn()==='b','for-in');check(o.forOf()===42,'for-of');check(o.empty()===0,'empty for-in');
var clone=eval('('+make.toString()+')')();Object.setPrototypeOf(clone,base);check(clone.get()===7&&clone.add(2)===11&&clone.pattern()===31&&clone.forIn()==='b'&&clone.forOf()===42,'source roundtrip');
check(rejects('super.x'),'global rejected');check(rejects('({m(){return function(){return super.x}}})'),'nested function rejected');check(rejects('({m(){return (0,eval)("super.x")}}).m()'),'indirect eval rejected');
var borrowed=o.get;check(borrowed.call({x:99})===7,'home independent of receiver');Object.setPrototypeOf(o,{x:19});check(borrowed()===19,'live home prototype');

var events=[], first={x:3}, second={x:9}, target={m(k){return super[k]},write(v){return super.x=v},change(){return super.x+=(Object.setPrototypeOf(this,second),2)}};
Object.setPrototypeOf(target,first);
check(target.change()===5&&target.x===5&&first.x===3,'compound captured base');
Object.setPrototypeOf(target,first);
check(target.m({toString:function(){events.push('key');Object.setPrototypeOf(target,second);return 'x'}})===9&&events.join()==='key','key conversion before base capture');
var getterHome={get x(){return super.x},set x(v){super.x=v}}, accessorBase={get x(){return this.saved},set x(v){this.saved=v}};
Object.setPrototypeOf(getterHome,accessorBase);getterHome.x=12;check(getterHome.x===12,'accessor home');
var nested={x:15,m(){return ()=>()=>super.x}};Object.setPrototypeOf(nested,{x:21});var nestedArrow=nested.m()();check(nestedArrow.call({x:99})===21,'nested arrow home');
Object.setPrototypeOf(nested,{x:22});check(nestedArrow()===22,'escaped arrow live home');
var nullHome={m(k){return super[k]},write(v){return super.x=v}};Object.setPrototypeOf(nullHome,null);caught=false;events=[];
try{nullHome.m({toString:function(){events.push('key');return 'x'}})}catch(e){caught=e instanceof TypeError}
check(caught&&events.join()==='key','null base key conversion');
var original={m(){return super.x}}, inherited=Object.create(original);Object.setPrototypeOf(original,{x:27});inherited.x=99;check(inherited.m()===27,'inherited method home');
var commaHome={m(){return (0,super.m)()}}, strictBase={m:function(){'use strict';return this}};Object.setPrototypeOf(commaHome,strictBase);check(commaHome.m()===undefined,'comma loses receiver');
print('SUPER-PROPERTIES PASS checks='+checks);
