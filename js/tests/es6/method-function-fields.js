/* Methods must not materialize the optional ordinary-function fields. */
var checks=0;
function check(ok,label){++checks;if(!ok)throw Error(label);}
function inEdition(edition,source){var old=version();try{version(edition);return evaluate(source,'method-fields');}finally{version(old);}}
function inspect(fn,label){
 var fields=['caller','arguments'];
 for(var i=0;i<fields.length;++i){
  var key=fields[i];
  check(!Object.prototype.hasOwnProperty.call(fn,key),label+' own '+key);
  check(Object.getOwnPropertyDescriptor(fn,key)===undefined,label+' descriptor '+key);
  var thrown=false;try{fn[key];}catch(e){thrown=e instanceof TypeError;}
  check(thrown,label+' inherited thrower '+key);
  gc();
  check(Object.getOwnPropertyNames(fn).indexOf(key)===-1,label+' names '+key);
  check(Reflect.ownKeys(fn).indexOf(key)===-1,label+' keys '+key);
 }
}
var object={method(){inspect(this.method,'active method');return 3;},
 get value(){return 4;}, set value(v){}, *generator(){yield 1;}};
check(object.method()===3,'method executes');
inspect(object.method,'inactive method');
inspect(Object.getOwnPropertyDescriptor(object,'value').get,'getter');
inspect(Object.getOwnPropertyDescriptor(object,'value').set,'setter');
inspect(object.generator,'generator method');
inspect(()=>1,'arrow');
inspect(function strict(){'use strict';},'strict function');
var C=class {method(){return 5;} static method(){return 6;}};
inspect(C,'class');inspect(C.prototype.method,'class method');inspect(C.method,'static method');
var factory=function(){return {method(){return 3;}};};
var clone=inEdition(2015,'('+factory.toString()+')')();
inspect(clone.method,'source method');check(clone.method()===3,'source method executes');
for(var editionIndex=0;editionIndex<3;++editionIndex){
 var edition=[0,170,2015][editionIndex];
 var fn=inEdition(edition,'(function ordinary(){return 7;})');
 Object.setPrototypeOf(fn,Function.prototype);
 check(fn.hasOwnProperty('caller'),'ordinary optional caller '+edition);
 check(fn.hasOwnProperty('arguments'),'ordinary optional arguments '+edition);
 check(fn()===7,'ordinary still callable '+edition);
}
print('ES6-METHOD-FIELDS checks='+checks+' failures=0');
