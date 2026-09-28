/* Binary storage, conversion ordering and observable constructor hooks.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
(function () {
    var checks = 0;
    function check(value, label) { ++checks; if (!value) throw new Error(label); }
    function throws(kind, action) {
        var caught = false;
        try { action(); } catch (e) { caught = e instanceof kind; }
        check(caught, 'expected ' + kind.name);
    }
    var b = new ArrayBuffer(32), v = new DataView(b, 1, 24), i;
    check(b.byteLength === 32 && v.buffer === b && v.byteOffset === 1 && v.byteLength === 24, 'slots');
    check(ArrayBuffer.isView(v) && !ArrayBuffer.isView(b) && !ArrayBuffer.isView(DataView.prototype), 'isView');
    check(!ArrayBuffer.isView(new Proxy(v, {})), 'proxy lacks internal slots');
    for (i = 0; i < 24; i++) check(v.getUint8(i) === 0, 'zero initialized');
    v.setUint32(0, 0x12345678);
    check(v.getUint8(0) === 0x12 && v.getUint8(3) === 0x78, 'big endian bytes');
    check(v.getUint32(0, true) === 0x78563412, 'little endian read');
    v.setInt32(4, -2147483648, true);
    check(v.getInt32(4, true) === -2147483648 && v.getUint32(4, true) === 2147483648, 'signed integer');
    v.setInt16(8, 65535); v.setInt8(10, 255);
    check(v.getInt16(8) === -1 && v.getInt8(10) === -1, 'signed narrowing');
    v.setUint8(11, -257); check(v.getUint8(11) === 255, 'modulo');
    v.setFloat32(12, 1.5, true); v.setFloat64(16, -0, false);
    check(v.getFloat32(12, true) === 1.5, 'float32');
    check(1 / v.getFloat64(16) === -Infinity, 'negative zero');
    v.setFloat64(16, NaN, true); check(isNaN(v.getFloat64(16, true)), 'NaN');
    v.setFloat32(12, Infinity); check(v.getFloat32(12) === Infinity, 'infinity');
    v.setFloat32(12, 3.4028235677973366e38); check(v.getFloat32(12) === Infinity, 'float32 overflow rounding');
    v.setFloat32(12, 1 + Math.pow(2, -24)); check(v.getFloat32(12) === 1, 'float32 ties to even');
    v.setFloat32(12, -Math.pow(2, -150)); check(1 / v.getFloat32(12) === -Infinity, 'float32 subnormal tie');
    v.setFloat32(12, Math.pow(2, -149)); check(v.getFloat32(12) === Math.pow(2, -149), 'float32 least subnormal');
    throws(TypeError, function () { ArrayBuffer(1); });
    throws(TypeError, function () { DataView(b); });
    throws(RangeError, function () { new ArrayBuffer(); });
    throws(RangeError, function () { new ArrayBuffer(1.5); });
    throws(RangeError, function () { new ArrayBuffer(-1); });
    check(new DataView(b).byteLength === 32 && new DataView(b, 1.9).byteOffset === 1, 'corrected optional offset');
    throws(TypeError, function () { new DataView(new Proxy(b, {})); });
    throws(TypeError, function () { DataView.prototype.getInt8.call({}, 0); });
    throws(TypeError, function () { Object.getOwnPropertyDescriptor(ArrayBuffer.prototype, 'byteLength').get.call(ArrayBuffer.prototype); });
    throws(RangeError, function () { v.getInt8(NaN); });
    throws(RangeError, function () { v.getInt8(0.5); });
    throws(RangeError, function () { v.getFloat64(17); });
    var order = '';
    throws(RangeError, function () { v.setInt8(99, {valueOf:function () { order += 'v'; return 1; }}); });
    check(order === 'v', 'value conversion before bounds');
    order = '';
    v.setInt8({valueOf:function () { order += 'i'; return 0; }}, {valueOf:function () { order += 'v'; gc(); return 4; }});
    check(order === 'iv' && v.getInt8(0) === 4, 'conversion ordering and GC');
    var proto = {}, target = new Proxy(function () {}, {get:function (o, p) { if (p === 'prototype') { order += 'p'; gc(); return proto; } return o[p]; }});
    order = '';
    var custom = Reflect.construct(ArrayBuffer, [{valueOf:function () { order += 'n'; return 3; }}], target);
    check(order === 'np' && Object.getPrototypeOf(custom) === proto, 'allocation order');
    check(Object.getOwnPropertyDescriptor(ArrayBuffer.prototype, 'byteLength').get.call(custom) === 3, 'custom prototype slots');
    order = '';
    throws(RangeError, function () { Reflect.construct(ArrayBuffer, [Math.pow(2, 53) - 1], target); });
    check(order === 'p', 'prototype before allocation failure');
    var copy = b.slice(1, 5);
    check(copy !== b && copy.byteLength === 4 && new DataView(copy).getInt8(0) === 4, 'slice copies');
    new DataView(copy).setInt8(0, 9); check(v.getInt8(0) === 4, 'copy independent');
    var size;
    b.constructor = {};
    b.constructor[Symbol.species] = function (n) { size = n; gc(); return new ArrayBuffer(n + 2); };
    copy = b.slice(-4);
    check(size === 4 && copy.byteLength === 6, 'species size');
    b.constructor[Symbol.species] = function () { return b; };
    throws(TypeError, function () { b.slice(0, 1); });
    b.constructor[Symbol.species] = function () { return new ArrayBuffer(0); };
    throws(TypeError, function () { b.slice(0, 1); });
    delete b.constructor;
    check(Object.prototype.toString.call(b) === '[object ArrayBuffer]' && Object.prototype.toString.call(v) === '[object DataView]', 'tags');
    var descriptor = Object.getOwnPropertyDescriptor(DataView.prototype, 'getInt8');
    check(descriptor.writable && descriptor.configurable && !descriptor.enumerable, 'method descriptor');
    throws(TypeError, function () { new v.getInt8(); });
    check(ArrayBuffer.length === 1 && DataView.length === 1 && v.getInt8.length === 1 && v.setInt8.length === 2, 'arities');
    print('ES6-BINARY-DATA checks=' + checks + ' failures=0');
}());
