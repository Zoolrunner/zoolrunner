/* Template scanner boundaries and decompilation. MPL 1.1/GPL 2.0/LGPL 2.1. */
(function(){
    var checks=0,failures=0;
    function check(value,label){
        ++checks;
        if(!value){++failures;print('FAIL template boundary: '+label);}
    }
    var breaks=['\n','\r','\r\n','\u2028','\u2029'];
    for(var n=240;n<780;n++){
        var pad=new Array(n+1).join('a');
        for(var i=0;i<breaks.length;i++){
            var raw=breaks[i],want=i<3?'\n':raw;
            check(eval('`'+pad+raw+'z`')===pad+want+'z','ordinary '+n+'/'+i);
            check(eval('(function(t){return t.raw[0];})`'+pad+raw+'z`')===pad+want+'z',
                  'tagged '+n+'/'+i);
        }
    }
    var samples=['', 'hello', '${(1,2)}', '${({toString:function(){return "x";}})}',
                 'a\\n${3}b', '\u2028', '\u2029'];
    for(var i=0;i<samples.length;i++){
        var f=eval('(function(){return `'+samples[i]+'`;})');
        var g=eval('('+f.toString()+')');
        check(f()===g(),'ordinary decompile '+i);
    }
    var f=eval('(function(){`use strict`;with({x:3}){return x;}})');
    check(eval('('+f.toString()+')')()===3,'directive');
    var rawCases=['\x01abcd\x02','\0\x01\x02\u00e9\ud800',
                  '\u2028\u2029','\ud83d\ude00','a\\`b\\${c}d','a\\\nb'];
    for(var i=0;i<rawCases.length;i++){
        var source='(function(){function outer(){function inner(t){return t.raw[0];}'+
                   'return inner`'+rawCases[i]+'`;}return outer();})';
        var original=eval(source),restored=eval('('+original.toString()+')');
        check(original()===restored(),'nested raw '+i);
    }
    var entries=[];
    for(var i=0;i<66000;++i)entries.push('atom="template'+i+'"');
    var filler='var atom;'+entries.join(';')+';';
    var f=eval('(function(){'+filler+'function tag(t,v){gc();return t.raw[0]+v+t[1];}'+
               'return tag`a\\n${3}b`+`c${4}d`;})');
    check(f()==='a\\n3bc4d','wide literal');
    check(eval('('+f.toString()+')')()==='a\\n3bc4d','wide decompile');
    print('ES6-TEMPLATE-BOUNDARIES checks='+checks+' failures='+failures);
    if(failures)throw Error('template boundary failures: '+failures);
})();
