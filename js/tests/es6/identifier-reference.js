/* Resolve identifier references once, before RHS callbacks mutate bindings.
 * MPL 1.1/GPL 2.0/LGPL 2.1. ES2015 binding-resolution regression. */
(function(){
    var checks=0,failures=0;
    function test(label,body){
        ++checks;
        try{if(body()===true)return;}catch(e){print('DETAIL '+label+': '+e);}
        ++failures;print('FAIL identifier reference: '+label);
    }
    var operations=['=','+=','-=','*=','/=','%=','<<=','>>=','>>>=','&=','|=','^='];
    var global=Function('return this')();
    for(var i=0;i<operations.length;++i)(function(op){
        var expected=eval('(function(){var x=3;x '+op+' 2;return x;})()');
        test(op+' deleted with binding',function(){
            return eval('(function(){var scope={x:3};with(scope){(function(){"use strict";'+
                        'x '+op+' (delete scope.x,2);})();}return scope.x;})()')===expected;
        });
        test(op+' deleted global binding',function(){
            Object.defineProperty(global,'referenceGlobalProbe',{value:3,writable:true,configurable:true});
            try{
                eval('(function(){"use strict";referenceGlobalProbe '+op+' (delete global.referenceGlobalProbe,2);})()');
                return global.referenceGlobalProbe===expected;
            }finally{delete global.referenceGlobalProbe;}
        });
    })(operations[i]);
    test('unresolvable strict write remains unresolvable',function(){
        delete global.referenceGlobalProbe;
        try{
            eval('(function(){"use strict";referenceGlobalProbe=(global.referenceGlobalProbe=1,2);})()');
        }catch(e){return e instanceof ReferenceError && global.referenceGlobalProbe===1;}
        finally{delete global.referenceGlobalProbe;}
        return false;
    });
    test('unresolvable sloppy write retains global target',function(){
        var scope={};delete global.referenceGlobalProbe;
        try{
            with(scope){referenceGlobalProbe=(scope.referenceGlobalProbe=1,2);}
            return scope.referenceGlobalProbe===1 && global.referenceGlobalProbe===2;
        }finally{delete global.referenceGlobalProbe;}
    });
    test('strict resolved proxy scope does not repeat has trap',function(){
        var calls=[],target={referenceProxyProbe:1};
        var proxy=new Proxy(target,{
            has:function(t,k){if(k==='referenceProxyProbe')calls.push('has');return k in t;},
            set:function(t,k,v){if(k==='referenceProxyProbe')calls.push('set');t[k]=v;return true;}
        });
        with(proxy){(function(){'use strict';referenceProxyProbe=2;})();}
        return target.referenceProxyProbe===2 && calls.join()==='has,set';
    });
    test('getter deletion retains strict binding',function(){
        var scope={get referenceGetterProbe(){delete this.referenceGetterProbe;gc();return 3;}};
        with(scope){(function(){'use strict';referenceGetterProbe+=2;})();}
        return scope.referenceGetterProbe===5;
    });
    test('compound proxy checks binding value but does not resolve again',function(){
        var calls=[],target={referenceProxyProbe:3};
        var proxy=new Proxy(target,{
            has:function(t,k){if(k==='referenceProxyProbe')calls.push('has');return k in t;},
            get:function(t,k){if(k==='referenceProxyProbe')calls.push('get');return t[k];},
            set:function(t,k,v){if(k==='referenceProxyProbe')calls.push('set');t[k]=v;return true;}
        });
        with(proxy){(function(){'use strict';referenceProxyProbe+=2;})();}
        return target.referenceProxyProbe===5 && calls.join()==='has,has,get,set';
    });
    test('unscopables callback removes non-strict resolved binding',function(){
        var scope={referenceVanishingProbe:3},runs=0;
        Object.defineProperty(scope,Symbol.unscopables,{get:function(){
            delete this.referenceVanishingProbe;gc();return {};
        }});
        with(scope){referenceVanishingProbe+=(runs++,2);}
        return runs===1 && scope.hasOwnProperty('referenceVanishingProbe') &&
               isNaN(scope.referenceVanishingProbe);
    });
    test('unscopables callback removes strict resolved binding before read',function(){
        var scope={referenceVanishingProbe:3},runs=0;
        Object.defineProperty(scope,Symbol.unscopables,{get:function(){
            delete this.referenceVanishingProbe;gc();return {};
        }});
        try{with(scope){(function(){'use strict';referenceVanishingProbe+=(runs++,2);})();}}
        catch(e){return e instanceof ReferenceError && runs===0;}
        return false;
    });
    test('unscopables mutation does not redirect simple assignment',function(){
        var scope={referenceVanishingProbe:3};
        scope[Symbol.unscopables]={};
        with(scope){(function(){'use strict';referenceVanishingProbe=
            (scope[Symbol.unscopables].referenceVanishingProbe=true,gc(),9);})();}
        return scope.referenceVanishingProbe===9;
    });
    test('decompiled simple and compound references retain resolution',function(){
        var original=function(scope){with(scope){(function(){'use strict';
            referenceDecompileProbe+=(delete scope.referenceDecompileProbe,gc(),2);
            referenceDecompileProbe=(delete scope.referenceDecompileProbe,gc(),7);
        })();}return scope.referenceDecompileProbe;};
        var restored=eval('('+original.toString()+')');
        return original({referenceDecompileProbe:3})===7 &&
               restored({referenceDecompileProbe:3})===7;
    });
    print('ES6-IDENTIFIER-REFERENCE checks='+checks+' failures='+failures);
    if(failures)throw Error('identifier reference failures: '+failures);
})();
