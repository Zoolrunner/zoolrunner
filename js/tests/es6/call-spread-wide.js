var wideSpreadChecks=0;
function check(v){wideSpreadChecks++;if(!v)throw Error('wide spread '+wideSpreadChecks)}
var many=[];for(var i=0;i<66000;i++)many.push(i);
function probe(){return arguments.length===66000 && arguments[65536]===65536 && arguments[65999]===65999}
check(probe(...many));
function Large(){this.good=probe(...arguments);this.target=new.target}
var large=new Large(...many);check(large.good && large.target===Large);
var proxy=new Proxy(Large,{});large=new proxy(...many);check(large.good && large.target===proxy);
var bound=Large.bind(null);large=new bound(...many);check(large.good && large.target===Large);
check((function(){var scoped=7;return (eval)(...['scoped'])})()===7);
check((function(){var scoped=7;return eval.bind(null)(...['typeof scoped'])})()==='undefined');
var restore=eval('('+probe.toString()+')');check(restore(...many));
print('CALL-SPREAD-WIDE PASS checks='+wideSpreadChecks);
