var r=createTest262Realm(),s=r.createRealm();
var b=r.evalScript('var buffer=new ArrayBuffer(16);var view=new Uint8Array(buffer);var data=new DataView(buffer);view[0]=7;buffer');
s.detachArrayBuffer(b);
if(r.evalScript('view.length!==0'))throw Error('detached lengths');
if(!r.evalScript('(function(){try{data.getUint8(0)}catch(e){return e instanceof TypeError}return false})()'))throw Error('detached view');
if(!r.evalScript('(function(){try{buffer.byteLength}catch(e){return e instanceof TypeError}return false})()'))throw Error('detached buffer getter');
var error;try{s.detachArrayBuffer({})}catch(e){error=e}if(!(error instanceof s.global.TypeError))throw Error('error realm');
s.detachArrayBuffer(b);gc();
print('DETACH-HOST checks=5 failures=0');
