var checks=0;
function same(a,b,label){++checks;if(a!==b)throw Error(label+': '+a+' != '+b);}
function reference(fn,label){var caught=false;try{fn();}catch(e){caught=e instanceof ReferenceError;}same(caught,true,label);}
var expressions=['this;','(this);','(this,0);','void this;','typeof this;','!this;','~this;','+this;','-this;','this===this;','delete this;','if(true)this;','for(this;false;){}','(()=>{this;})();','(()=>{void this;})();','(()=>{delete this;})();','eval("this;");'];
for(var i=0;i<expressions.length;++i){
 var C=Function('return class extends Object {constructor(){'+expressions[i]+'return {};}}')();
 reference(function(){new C;},'uninitialized '+expressions[i]);
 var D=Function('return class extends Object {constructor(){super();'+expressions[i]+'this.ok=1;}}')();
 same(new D().ok,1,'initialized '+expressions[i]);
}
var Null=class extends null {constructor(){(()=>{this;})();return {};}};
reference(function(){new Null;},'null heritage arrow this');
var events=[];
class Later extends Object {
 constructor(){
  var read=()=>{this;};
  reference(read,'captured this before super');
  try{this;}catch(e){same(e instanceof ReferenceError,true,'direct discarded this throws');}
  finally{events.push('finally');if(typeof gc==='function')gc();}
  super();same(read(),undefined,'same captured binding after super');this.ok=2;
 }
}
same(new Later().ok,2,'initialization after caught read');same(events.join(','),'finally','finally preserved');
var Source=Function('return class extends Object {constructor(){this;return {};}}')();
var Round=eval('('+Source.toString()+')');reference(function(){new Round;},'source roundtrip preserves discarded read');
var ordinary={check:function(){this;void this;delete this;return this;}};
same(ordinary.check(),ordinary,'ordinary method this remains valid');
same((function(){'use strict';this;return this;}).call(null),null,'strict null receiver remains valid');
print('DERIVED-THIS-EFFECTS checks='+checks+' failures=0');
