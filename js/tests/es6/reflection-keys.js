/* Modern key ordering, snapshots and reflection array results.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
(function () {
    var checks=0;
    function check(value,label){++checks;if(!value)throw new Error(label);}
    var o={a:'A',1:'B',c:'C',2:'D'};
    check(Object.keys(o).join(',')==='1,2,a,c','numeric first');
    check(Object.getOwnPropertyNames(o).join(',')==='1,2,a,c','own names');
    check(JSON.stringify(o)==='{"1":"B","2":"D","a":"A","c":"C"}','JSON key ordering');
    var seen=[];
    JSON.parse('{"a":1,"2":2,"1":3}',function(k,v){seen.push(k);return v;});
    check(seen.join(',')==='1,2,a,','reviver key ordering');
    o={z:1};o['9007199254740991']=2;o['4294967296']=3;o['1']=4;o['4294967295']=5;o['01']=6;o['-0']=7;
    check(Object.keys(o).join(',')==='1,4294967295,4294967296,9007199254740991,z,01,-0','ES2015 integer indices');
    o={first:1,last:2};delete o.first;o.first=3;
    check(Object.keys(o).join(',')==='last,first','reinserted string');
    var getterCalls=0;
    Object.defineProperty(o,'hidden',{value:1});
    Object.defineProperty(o,'getter',{enumerable:true,get:function(){++getterCalls;throw 'getter';}});
    check(Object.keys(o).join(',')==='last,first,getter'&&getterCalls===0,'no value reads');
    check(Object.getOwnPropertyNames(o).join(',')==='last,first,hidden,getter','nonenumerable own names');
    var s1=Symbol('first'),s2=Symbol('second');o[s1]=1;o[s2]=2;
    check(Object.keys(o).join(',')==='last,first,getter','exclude symbols');
    var symbols=Object.getOwnPropertySymbols(o);
    check(symbols.length===2&&symbols[0]===s1&&symbols[1]===s2,'symbol order');
    check(Object.keys('ab').join(',')==='0,1','string boxing');
    check(Object.keys(Symbol()).length===0,'symbol boxing');
    var order='',proxy=new Proxy({a:1,1:2,2:3},{ownKeys:function(){return ['a','2','1'];},getOwnPropertyDescriptor:function(t,k){order+=k;gc();return Object.getOwnPropertyDescriptor(t,k);}});
    check(Object.keys(proxy).join(',')==='a,2,1'&&order==='a21','proxy order untouched');
    check(Object.getOwnPropertyNames(proxy).join(',')==='a,2,1','proxy own names');
    var inherited=Object.create({inherited:1});inherited.own=2;
    check(Object.keys(inherited).join(',')==='own','own properties only');
    var snapshot=Object.keys(inherited);delete inherited.own;inherited.later=3;
    check(snapshot.join(',')==='own','independent snapshot');
    print('ES6-REFLECTION-KEYS checks='+checks+' failures=0');
}());
