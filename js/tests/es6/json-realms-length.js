/* Modern JSON length conversion and method-realm allocations. */
var checks = 0;
function same(a,b,label) { checks++; if (a !== b) throw Error(label+': '+a+' != '+b); }
function collect() { if (typeof gc === 'function') gc(); }
var methods = ['parse', 'stringify', 'replacer'];
function run(method, array) {
    if (method === 'parse') return JSON.parse('{"a":0,"b":0}',function(k,v){
        collect(); if(k === 'a') this.b = array; return v;
    });
    if (method === 'stringify') return JSON.stringify(array);
    return JSON.stringify({x:1}, array);
}
for (var m=0;m<methods.length;m++) {
    var lengths=[4294967296,9007199254740991,Infinity,-1,-Infinity,NaN,-0];
    for (var n=0;n<lengths.length;n++) {
        var length=lengths[n], sentinel={}, reads=0, conversions=0, first=0;
        var array=new Proxy([], {get:function(t,k){
            if(k === 'length') { reads++; return {valueOf:function(){conversions++;collect();return length;}}; }
            if(k === '0') { first++;collect();throw sentinel; }
            return t[k];
        }});
        var caught;
        caught=undefined;
        try { run(methods[m],array); } catch(e) { caught=e; }
        same(caught,length>0 ? sentinel : undefined,methods[m]+' conversion '+length);
        same(reads,1,'length read once');
        same(conversions,1,'length converted once');
        same(first,length>0 ? 1 : 0,'positive lengths visit index zero');
    }
}
var realm=createTest262Realm(), foreign=realm.global;
var parse=foreign.JSON.parse, stringify=foreign.JSON.stringify;
var proto=foreign.Object.prototype, arrayProto=foreign.Array.prototype;
foreign.Object=function(){throw Error('mutable Object binding');};
foreign.Array=function(){throw Error('mutable Array binding');};
collect();
var parsed=parse('{"a":[{},[]]}');
same(Object.getPrototypeOf(parsed),proto,'foreign object');
same(Object.getPrototypeOf(parsed.a),arrayProto,'foreign array');
same(Object.getPrototypeOf(parsed.a[0]),proto,'nested foreign object');
same(Object.getPrototypeOf(parsed.a[1]),arrayProto,'nested foreign array');
parse('0',function(k,v){collect();same(Object.getPrototypeOf(this),proto,'reviver root holder');return v;});
stringify(0,function(k,v){collect();same(Object.getPrototypeOf(this),proto,'replacer root holder');return v;});
same(JSON.stringify([1,2],null,2),'[\n  1,\n  2\n]','indentation retained');
print('JSON-REALMS-LENGTH checks='+checks+' failures=0');
