/* Error-source reconstruction must account for hidden completion resets. */
var sourceChecks=0;
function checked(source) {
    var value=(0,eval)(source);
    sourceChecks++;
    if(value!==true)throw Error('completion source: '+source+' => '+value);
}
checked('try { null.value } catch(e) { e instanceof TypeError }');
checked('if(false){}; try { ({}).missing() } catch(e) { e instanceof TypeError }');
checked('try { var x={};x[{toString:function(){return {}},valueOf:function(){return {}}}] } catch(e) { e instanceof TypeError }');
checked('try { 3 } finally { try { null.value } catch(e) { if(!(e instanceof TypeError))throw e } }; true');
checked('try { throw 1 } catch(e) { try { null.value } catch(e) { e instanceof TypeError } }');
checked('try { try { null.value } finally { 9 } } catch(e) { e instanceof TypeError }');
checked('try { for(var i=0;i<2;i++){if(i===1)null.value} } catch(e) { e instanceof TypeError }');
checked('(function(){try{return 7}finally{try{null.value}catch(e){if(!(e instanceof TypeError))throw e}}})()===7');
checked('(function*(){try{yield 1;null.value}catch(e){yield e instanceof TypeError}})().next().value===1');
var iterator=(function*(){try{yield 1;null.value}catch(e){yield e instanceof TypeError}})();
iterator.next();sourceChecks++;if(iterator.next().value!==true)throw Error('generator error source');
print('COMPLETION-ERROR-SOURCE PASS checks='+sourceChecks);
