/* ES2015 integer-indexed views. MPL 1.1/GPL 2.0/LGPL 2.1. */
(function () {
    var checks = 0;
    function check(value, label) { ++checks; if (!value) throw new Error(label); }
    function throws(kind, callback, label) {
        var caught = false;
        try { callback(); } catch (error) { caught = error instanceof kind; }
        check(caught, label);
    }
    var constructors = [Int8Array, Uint8Array, Uint8ClampedArray, Int16Array,
                        Uint16Array, Int32Array, Uint32Array, Float32Array, Float64Array];
    constructors.forEach(function (C) {
        var a = new C([1, 2, 3]), result = [], key;
        check(a.length === 3 && a[0] === 1 && a[2] === 3, C.name + ' elements');
        check(a.byteLength === 3 * C.BYTES_PER_ELEMENT && a.byteOffset === 0, C.name + ' view');
        check(ArrayBuffer.isView(a) && !ArrayBuffer.isView(C.prototype), C.name + ' isView');
        check(Object.getPrototypeOf(a) === C.prototype, C.name + ' prototype');
        check(Object.prototype.toString.call(a) === '[object ' + C.name + ']', C.name + ' tag');
        for (var value of a) result.push(value);
        check(result.join() === '1,2,3', C.name + ' iteration');
        var desc = Object.getOwnPropertyDescriptor(a, '1');
        check(desc.value === 2 && desc.writable && desc.enumerable && !desc.configurable, C.name + ' index descriptor');
        check(!Reflect.defineProperty(a, '1', {configurable: true}) &&
              !Reflect.defineProperty(a, '1', {writable: false}) &&
              !Reflect.defineProperty(a, '1', {get: function () { return 9; }}), C.name + ' rejected descriptors');
        Object.defineProperty(a, '1', {value: 4});
        check(a[1] === 4 && !Reflect.deleteProperty(a, '1'), C.name + ' index definition');
        a['01'] = 'ordinary';
        Object.defineProperty(a, 'extra', {get: function () { return this === a ? 12 : 13; }, configurable: true});
        check(a['01'] === 'ordinary' && a.extra === 12, C.name + ' ordinary properties');
        check(typeof Object.getOwnPropertyDescriptor(a, 'extra').get === 'function', C.name + ' getter descriptor');
        var symbol = Symbol('extra'); a[symbol] = 11;
        var keys = Reflect.ownKeys(a);
        check(keys.length === 6 && keys[0] === '0' && keys[3] === '01' && keys[4] === 'extra' && keys[5] === symbol, C.name + ' own key order');
        var conversions = 0;
        check(!Reflect.set(a, '-0', {valueOf: function () { ++conversions; return 1; }}) && conversions === 1, C.name + ' invalid index conversion');
        check(a['-0'] === undefined && !('-0' in a), C.name + ' negative zero');
        Object.defineProperty(a, 'length', {value: 1});
        result = [];
        for (var v of a) result.push(v);
        check(result.join() === '1,4,3', C.name + ' iterator internal length');
        var view = new C(a.buffer, -0, 2);
        check(1 / view.byteOffset === Infinity && view.length === 2, C.name + ' offset normalization');
        view[0] = 9; check(a[0] === 9, C.name + ' shared storage');
        Object.preventExtensions(a);
        check(!Object.isExtensible(a) && Reflect.set(a, '0', 7) && a[0] === 7 &&
              !Reflect.defineProperty(a, 'newProperty', {value: 1}), C.name + ' prevent extensions');
        throws(TypeError, function () { Object.freeze(a); }, C.name + ' nonempty freeze');
        throws(TypeError, function () { C(1); }, C.name + ' no call');
        throws(TypeError, function () { new C(undefined); }, C.name + ' ES2015 explicit undefined');
        throws(RangeError, function () { new C(1.5); }, C.name + ' fractional length');
        check(new C().length === 0, C.name + ' empty');
    });
    check(new Int8Array([255])[0] === -1 && new Uint16Array([-1])[0] === 65535 &&
          new Int32Array([4294967295])[0] === -1, 'integer wrapping');
    check(Array.prototype.join.call(new Uint8ClampedArray([0.5,1.5,2.5,3.5,-1,256,NaN])) === '0,2,2,4,0,255,0', 'clamp ties to even');
    check(1/new Float64Array([-0])[0] === -Infinity && new Float32Array([1/3])[0] === Math.fround(1/3), 'floating storage');
    var a = new Uint8Array([1, 2, 3, 4]);
    Object.defineProperty(a, 'length', {value: 0});
    check(a.join('-') === '1-2-3-4' && a.toString() === '1,2,3,4', 'join uses internal length');
    check(a.map(function (v, i, source) { check(source === a, 'map receiver'); return v + i; }).join() === '1,3,5,7', 'map');
    check(a.filter(function (v) { return v % 2; }).join() === '1,3', 'filter');
    check(a.every(function (v) { return v > 0; }) && a.some(function (v) { return v === 3; }), 'every/some');
    check(a.find(function (v) { return v > 2; }) === 3 && a.findIndex(function (v) { return v > 2; }) === 2, 'find');
    check(a.reduce(function (x,y) { return x+y; }) === 10 && a.reduceRight(function (x,y) { return x-y; }) === -2, 'reductions');
    check(a.indexOf(3) === 2 && a.lastIndexOf(3,-1) === 2 && a.indexOf(9) === -1, 'search');
    var seen = [], receiver = {};
    a.forEach(function (v,i,source) { check(this === receiver && source === a, 'forEach receiver'); seen.push(v+i); }, receiver);
    check(seen.join() === '1,3,5,7', 'forEach');
    check(a.slice(1,-1).join() === '2,3', 'slice');
    var view = a.subarray(1,3); view[0] = 8;
    check(view.buffer === a.buffer && a[1] === 8 && view.join() === '8,3', 'subarray');
    a.set(a.subarray(0,3),1);
    check(a.join() === '1,1,8,3', 'overlapping set');
    check(a.copyWithin(1,0,2) === a && a.join() === '1,1,1,3', 'copyWithin');
    check(a.fill(7,1,3) === a && a.join() === '1,7,7,3', 'fill');
    check(a.reverse() === a && a.join() === '3,7,7,1', 'reverse');
    var floats = new Float64Array([NaN,0,-0,10,-1,2]); floats.sort();
    check(floats[0] === -1 && 1/floats[1] === -Infinity && 1/floats[2] === Infinity &&
          floats[3] === 2 && floats[4] === 10 && isNaN(floats[5]), 'numeric sort');
    check(new Int8Array([1,3,2]).sort(function (x,y) { return y-x; }).join() === '3,2,1', 'custom sort');
    check(Int16Array.of(1,2,3).join() === '1,2,3' && Int16Array.from([1,2],function(v,i){return v+i}).join() === '1,3', 'from/of');
    var events = [], iterable = {};
    iterable[Symbol.iterator] = function () { var i=0;return {next:function(){events.push('n');return {value:++i,done:i>2}}}; };
    check(Uint8Array.from(iterable,function(v){events.push('m');return v;}).join() === '1,2' && events.join() === 'n,n,n,m,m', 'collect before mapping');
    a = new Uint8Array([1]); a.extra = 1; a.extra = 2;
    Object.defineProperty(a, 'access', {get:function(){return this.extra},set:function(v){this.extra=v}});
    var child = Object.create(a); child.access=9;
    check(child.access===9 && a.extra===2 && !child.hasOwnProperty('0'), 'inherited receiver');
    var proto=Object.create(Object.getPrototypeOf(a)); proto['-1']=17; Object.setPrototypeOf(a,proto);
    check(a[-1]===undefined && child[-1]===17 && !('-1' in a), 'integer Get versus Has receiver policy');
    a = new Uint8Array(1); throws(TypeError,function(){Object.freeze(a)},'freeze failure');
    check(!Object.isExtensible(a), 'freeze prevents extensions before rejection');
    var buffer = new ArrayBuffer(16), short = new Uint8Array(buffer,4,2), calls = 0;
    function BufferSpecies(){++calls;throw new Error('must not call buffer constructor');}
    BufferSpecies.prototype = {bufferPrototype:true}; buffer.constructor={}; buffer.constructor[Symbol.species]=BufferSpecies;
    var copied = new Uint8Array(short);
    check(calls===0 && copied.length===2 && Object.getPrototypeOf(copied.buffer)===BufferSpecies.prototype &&
          Object.getOwnPropertyDescriptor(ArrayBuffer.prototype,'byteLength').get.call(copied.buffer)===12, 'ES2015 buffer clone allocation');
    var raw = new Uint8Array(16), rawView = new DataView(raw.buffer);
    rawView.setUint32(0,0x7ff80001,false); rawView.setUint32(4,0x23456789,false);
    var floating = new Float64Array(raw.buffer,0,1);
    var rawCopy = new Uint8Array(floating.slice().buffer);
    check(rawCopy.slice(0,8).join()===raw.slice(0,8).join(), 'slice preserves raw bytes');
    var sink = new Float64Array(1); sink.set(floating);
    check(new Uint8Array(sink.buffer).join()===raw.slice(0,8).join(), 'set preserves raw bytes');
    var base=Object.getPrototypeOf(Uint8Array), methods=Object.getPrototypeOf(Uint8Array.prototype);
    check(base.name==='TypedArray' && base.length===0 && base.prototype===methods &&
          methods.constructor===base && Uint8Array.length===3, 'intrinsic graph');
    throws(TypeError,function(){new base(1)},'abstract constructor');
    throws(TypeError,function(){methods.map.call([],function(v){return v})},'method brand');
    throws(TypeError,function(){new methods.map(function(v){return v})},'method nonconstructible');
    var order=[], converted=0;
    a=new Uint8Array(2);
    a.fill({valueOf:function(){order.push('value');return ++converted;}},
           {valueOf:function(){order.push('start');return 0;}},
           {valueOf:function(){order.push('end');return 2;}});
    check(order.join()==='start,end,value,value' && a.join()==='1,2', 'ES2015 fill conversion order');
    throws(TypeError,function(){Object.assign(new Uint8Array(0),{0:1})},'assign rejects invalid index');
    throws(TypeError,function(){Array.prototype.push.call(new Uint8Array(0),1)},'generic Array Set throws');
    var locked=new Uint8Array(1); Object.preventExtensions(locked);
    throws(TypeError,function(){Object.assign(locked,{ordinary:1})},'assign rejects nonextensible expando');
    check(Object.assign(locked,{0:7})===locked && locked[0]===7,'assign valid nonextensible index');
    print('ES6-TYPED-ARRAYS PASS checks=' + checks);
})();
