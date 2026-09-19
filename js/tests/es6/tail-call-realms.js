/* ES2015 regressions; MPL 1.1/GPL 2.0/LGPL 2.1. */
'use strict';
var checks=0;function check(v,m){checks++;if(!v)throw Error(m);}
var realm=createTest262Realm();
realm.evalScript('function a(n){"use strict";return n?peer(n-1):[];}');
function b(n){return n?realm.global.a(n-1):[];}realm.global.peer=b;
var result=realm.global.a(100000);
check(Object.getPrototypeOf(result)===realm.global.Array.prototype,'foreign allocation realm after mutual tails');
realm.evalScript('function err(n){"use strict";if(!n)throw new TypeError("foreign");return peerError(n-1);}');
function err(n){return realm.global.err(n);}realm.global.peerError=err;
var caught;try{realm.global.err(100000);}catch(e){caught=e;}
check(caught instanceof realm.global.TypeError&&!(caught instanceof TypeError),'foreign error realm');
realm.evalScript('var indirect=eval;function ev(){"use strict";var hidden=1;return indirect("typeof hidden");}');
check(realm.global.ev()==='undefined','foreign indirect eval');
realm.evalScript('function direct(){"use strict";var hidden=2;return eval("hidden");}');
check(realm.global.direct()===2,'foreign direct eval');
function leaf(){return this;}realm.global.leaf=leaf;
realm.evalScript('function raw(){"use strict";return leaf.call(19);}');
check(realm.global.raw()===19,'raw receiver across forwarding realms');
realm.evalScript('function revoked(){"use strict";var x=Proxy.revocable(function(){},{});x.revoke();return x.proxy();}');
var revokedError;try{realm.global.revoked();}catch(e){revokedError=e;}
check(revokedError instanceof TypeError,'original ES2015 resumed caller realm for proxy error');
realm.evalScript('var proxy=new Proxy(function(){},{apply:function(t,r,a){return a;}});function proxyArgs(){"use strict";return proxy(1);}');
check(Object.getPrototypeOf(realm.global.proxyArgs())===Array.prototype,'original ES2015 resumed caller realm for proxy arguments');
function localProxyArgs(){return realm.global.proxy(1);}
check(Object.getPrototypeOf(localProxyArgs())===Array.prototype,'foreign proxy uses calling realm for arguments');
print('TAIL-CALL-REALMS checks='+checks+' failures=0');
