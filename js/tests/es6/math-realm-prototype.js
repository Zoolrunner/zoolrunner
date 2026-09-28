var checks=0;
function check(v,s){++checks;if(!v)throw Error(s);}
check(Object.getPrototypeOf(Math)===Object.prototype,'initial Math prototype');
var realms=[createTest262Realm(),createTest262Realm()];
for(var i=0;i<realms.length;++i){
 var r=realms[i];
 check(r.evalScript('Object.getPrototypeOf(Math)===Object.prototype'),'eager Math prototype');
 check(r.evalScript('Object.getOwnPropertyNames(Object.getPrototypeOf(Math)).indexOf("PI")===-1'),'no hidden Math object');
 check(r.evalScript('Math.hasOwnProperty("PI")&&Math.hasOwnProperty("abs")&&Math.abs(-2)===2'),'own constants and methods');
 check(r.evalScript('Object.prototype.toString.call(Math)==="[object Math]"'),'tag');
 check(r.global.Math!==Math&&r.global.Object.prototype!==Object.prototype,'realm isolation');
 var child=r.createRealm();check(child.evalScript('Object.getPrototypeOf(Math)===Object.prototype'),'nested realm');
 check(child.global.Math!==r.global.Math,'nested identity');
 gc();check(r.evalScript('Object.getPrototypeOf(Math)===Object.prototype'),'after GC');
}
realms[0].evalScript('Object.prototype.realmFlag=1');
check(realms[0].global.Math.realmFlag===1&&realms[1].global.Math.realmFlag===undefined,'local inheritance');
realms[0].evalScript('Math=7');
var fresh=realms[0].createRealm();check(fresh.evalScript('Object.getPrototypeOf(Math)===Object.prototype&&Math.abs(-3)===3'),'new realm after replacement');
print('MATH-REALM-PROTOTYPE checks='+checks+' failures=0');
