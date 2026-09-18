/* Extended atom indices and computed initializers must still decompile. */
var fields=[],symbol=Symbol('wide');
for(var i=0;i<66000;++i)fields.push('["p'+i+'"]:'+i);
fields.push('["__proto__"]:9','[symbol]:function(){return 7;}');
var source='(function(){return {'+fields.join(',')+'};})';
var makeWide=eval(source),value=makeWide();
if(value.p65999!==65999||value.__proto__!==9||value[symbol].name!=='[wide]')throw Error('wide computed execution');
value=null;gc();
var restored=eval('('+makeWide.toString()+')');value=restored();
if(value.p65999!==65999||value.__proto__!==9||value[symbol]()!==7)throw Error('wide computed decompilation');
print('ES6-COMPUTED-PROPERTIES-WIDE checks=2 failures=0');
