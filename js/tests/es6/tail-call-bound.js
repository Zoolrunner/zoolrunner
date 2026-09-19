/* ES2015 regressions; MPL 1.1/GPL 2.0/LGPL 2.1. */
'use strict';
var checks=0;function check(v,m){checks++;if(!v)throw Error(m);}
var aThis={name:'a'},bThis={name:'b'},seenA=0,seenB=0;
function a(prefix,n,value){if(this!==aThis||prefix!=='prefix')throw Error('bound a');seenA++;return n?boundB(n-1,value+1):value;}
function b(n,value){if(this!==bThis)throw Error('bound b');seenB++;return n?boundA(n-1,value+1):value;}
var boundA=a.bind(aThis,'prefix'),boundB=b.bind(bThis);
check(boundA(100000,2)===100002&&seenA===50001&&seenB===50000,'bound mutual recursion and prefix');
function nested(a,b,n){if(a!==1||b!==2||this!==17)throw Error('nested bind');return n?nestedBound(n-1):true;}
var nestedBound=nested.bind(17,1).bind(99,2);
check(nestedBound(100000),'nested binding argument and receiver order');
function spread(n){return n?boundSpread(...[n-1]):true;}var boundSpread=spread.bind(null);
check(boundSpread(100000),'bound spread tail');
print('TAIL-CALL-BOUND checks='+checks+' failures=0');
