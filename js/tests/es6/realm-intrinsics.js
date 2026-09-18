/* Intrinsics survive mutable globals; legacy scripts keep their old lookup. */
var checks = 0;
function check(value, label) { ++checks; if (!value) throw Error('FAIL realm intrinsics: ' + label); }
var originalObject = Object, originalArray = Array;
var arrayProto = Array.prototype, objectProto = Object.prototype;
var getProto = Object.getPrototypeOf, define = Object.defineProperty;
var calls = 0;
function Replacement() { ++calls; }
Array = Replacement; Object = Replacement;
check(getProto([]) === arrayProto, 'array literal intrinsic');
check(getProto({}) === objectProto, 'object literal intrinsic');
check((function(Array, Object) { return getProto([]) === arrayProto && getProto({}) === objectProto; })(null, null), 'parameters do not shadow intrinsics');
check((function() { with ({Array:null,Object:null}) { return getProto([]) === arrayProto && getProto({}) === objectProto; } })(), 'with does not shadow intrinsics');
check(calls === 0, 'replacement constructor never called');
check(getProto(new Array()) === Replacement.prototype && calls === 1, 'explicit new uses replacement');
var fn = function() { return [1, {answer:42}]; };
var recreated = eval('(' + fn.toString() + ')');
check(recreated()[1].answer === 42 && getProto(recreated()) === arrayProto, 'decompile and recompile');
for (var i = 0; i < 20; ++i) { gc(); check(getProto([]) === arrayProto && getProto({}) === objectProto, 'GC retains intrinsic ' + i); }
delete this.Array; delete this.Object;
check(getProto([]) === arrayProto && getProto({}) === objectProto, 'deleted global names');
define(this, 'Array', {configurable:true,get:function(){throw 'observable Array lookup';}});
define(this, 'Object', {configurable:true,get:function(){throw 'observable Object lookup';}});
check(getProto([]) === arrayProto && getProto({}) === objectProto, 'global getters not invoked');
delete this.Array; delete this.Object;
this.Array = originalArray; this.Object = originalObject;
var constructors = ['String', 'Number', 'Boolean'];
for (i = 0; i < constructors.length; ++i) {
    var name = constructors[i], saved = this[name], proto = saved.prototype;
    this[name] = Replacement;
    var boxed = i === 0 ? originalObject('x') : i === 1 ? originalObject(1) : originalObject(true);
    check(getProto(boxed) === proto, 'boxing intrinsic ' + name);
    this[name] = saved;
}
version(0);
var legacy = eval('(function(){return [];})');
version(170);
var legacy170 = eval('(function(){return [];})');
version(2015);
Array = Replacement;
check(getProto(legacy()) === Replacement.prototype, 'default legacy constructor lookup');
check(getProto(legacy170()) === Replacement.prototype, '1.7 legacy constructor lookup');
check(getProto([]) === arrayProto, 'modern caller restores intrinsic lookup');
Array = originalArray;
print('ES6-REALM-INTRINSICS checks=' + checks + ' failures=0');
