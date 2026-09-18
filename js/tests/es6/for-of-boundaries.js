/* For-of source notes, wide jumps and extended atom indices.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
(function(){
    var checks=0;
    function check(ok,label){++checks;if(!ok)throw Error('FAIL for-of boundary: '+label)}
    var padding=new Array(7001).join('n+=0;');
    var f=Function('var n=0,a=[];for(let x of [3,4]){'+padding+
                   'a.push(()=>x)}return a[0]()+a[1]()+n');
    check(f()===7,'wide loop execution');
    check(eval('('+f.toString()+')')()===7,'wide loop source');
    var atoms=[];
    for(var i=0;i<65540;i++)atoms.push('pad="for-of-atom-'+i+'";');
    f=Function('var pad;'+atoms.join('')+'var o={},n=0;'+
               'for(o.target of [3,4])n+=o.target;return n');
    check(f()===7,'extended assignment atom');
    check(eval('('+f.toString()+')')()===7,'extended assignment source');
    f=Function('var n=0,closed=0,source={};source[Symbol.iterator]=function(){'+
               'return {next:function(){return {value:3}},'+
               'return:function(){closed++;return {}}}};'+
               'try{for(let x of source){'+padding+'throw x}}'+
               'catch(e){return e===3&&closed===1}return false');
    check(f(),'wide exception range');
    check(eval('('+f.toString()+')')(),'wide exception source');
    var names=[];
    for(i=0;i<65535;i++)names.push('local'+i);
    var rejected=false;
    try { Function('let '+names.join()+ ';for(var x of []){}'); }
    catch(e) { rejected=e.name === "InternalError" && e.message.indexOf("too large") >= 0; }
    check(rejected,'oversized iterator stack rejected');
    print('ES6-FOR-OF-BOUNDARIES checks='+checks+' failures=0');
})();
