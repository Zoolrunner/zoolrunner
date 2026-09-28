/* Pattern cover grammar, iteration keys and contextual arrow formals. */
var checks = 0;
function check(value, label) { ++checks; if (!value) throw Error(label); }
function rejects(source) {
    var threw = false;
    try { Function(source); } catch(e) { threw = e instanceof SyntaxError; }
    check(threw, source);
}
var a, b;
({__proto__: a, __proto__: b} = {['__proto__']: 7});
check(a === 7 && b === 7, 'duplicate proto assignment keys');
var {__proto__: c, __proto__: d} = {['__proto__']: 8};
check(c === 8 && d === 8, 'duplicate proto binding keys');
rejects('return {__proto__: null, __proto__: null}');
rejects('return ({nested: {__proto__: null, __proto__: null}})');
check(({__proto__: null, ['__proto__']: 9}).__proto__ === 9, 'computed key is ordinary');
rejects('function* g(){ (x = yield) => {}; }');
rejects('function* g(){ (x = yield 1) => {}; }');
check(Function('var yield=3;return ((x=yield)=>x)()')() === 3, 'yield identifier outside generator');
check(Function('function* g(){return ((x=function*(){yield 4})=>x)()}return g().next().value().next().value')() === 4,
      'nested generator in default');
var values = [], callbacks = [];
for (let [x] in {ab: 1, cd: 2}) { values.push(x); callbacks.push(()=>x); }
check(values.join() === 'a,c', 'destructure property names');
check(callbacks[0]() === 'a' && callbacks[1]() === 'c', 'fresh lexical bindings');
check(typeof x === 'undefined', 'lexical name does not escape');
for (var [repeat, repeat] in {ab: null}) check(repeat === 'b', 'duplicate var names');
for (const {length: size} in {abc: null}) check(size === 3, 'object pattern');
for ([a, ...b] in {xyz: null}) check(a === 'x' && b.join() === 'y,z', 'assignment rest');
function collect(object) { var out=[]; for(let [key] in object) out.push(key); return out.join(); }
check(eval('('+collect.toString()+')')({ab:1,cd:2}) === 'a,c', 'iteration decompilation');
check(/\8/.exec('789')[0] === '8', 'identity escape eight');
check(/7\89/.exec('67890')[0] === '789', 'following decimal digit');
check(/\9/.exec('890')[0] === '9', 'identity escape nine');
check(/(.)(.)(.)(.)(.)(.)(.)(.)\8\8/.test('0123456777'), 'existing backreference');
rejects('return /\\8/u');
print('LATER-PATTERN-EDGES checks=' + checks + ' failures=0');
