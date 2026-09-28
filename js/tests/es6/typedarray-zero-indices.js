'use strict';
var checks=0;function check(v,m){checks++;if(!v)throw Error(m);}
var types=[Int8Array,Uint8Array,Uint8ClampedArray,Int16Array,Uint16Array,Int32Array,Uint32Array,Float32Array,Float64Array];
for(var i=0;i<types.length;i++){
 var T=types[i];
 for(var j=0;j<3;j++){
  var n=j===0?-0:j===1?-0.5:{valueOf:function(){gc();return -0;}};
  var a=new T([42,43]);
  check(a.indexOf(42,n)===0&&1/a.indexOf(42,n)===Infinity,'indexOf zero '+i+':'+j);
  check(a.indexOf(43,n)===1&&a.indexOf(undefined,n)===-1,'indexOf real first element '+i+':'+j);
  check(a.lastIndexOf(42,n)===0&&1/a.lastIndexOf(42,n)===Infinity,'lastIndexOf zero '+i+':'+j);
  check(a.lastIndexOf(43,n)===-1&&a.lastIndexOf(undefined,n)===-1,'reverse stops at zero '+i+':'+j);
  var count=0;a.fill({valueOf:function(){count++;return 7;}},n);
  check(a[0]===7&&a[1]===7&&count===2,'fill zero and original per-element conversion '+i+':'+j);
  a=new T([1,2]);check(a.slice(n).join()==='1,2'&&a.subarray(n).join()==='1,2','slice and subarray zero '+i+':'+j);
  a.copyWithin(n,1);check(a.join()==='2,2','copyWithin zero '+i+':'+j);
 }
 var a=new T([1,2]);check(a['-0']===undefined&&!Object.prototype.hasOwnProperty.call(a,'-0'),'string negative-zero remains non-index '+i);
 check(a[-0]===1&&a.indexOf(1,-Infinity)===0&&a.lastIndexOf(2,Infinity)===1,'numeric key and infinite bounds '+i);
}
print('TYPEDARRAY-ZERO-INDICES checks='+checks+' failures=0');
