/* ES2015 context regression. MPL 1.1/GPL 2.0/LGPL 2.1. */
var checks=0,failures=0;function check(n,f){checks++;try{if(f()!==true)throw Error('wrong result')}catch(e){failures++;print('FAIL '+n+': '+e)}}
function invalid(s){check(s,function(){try{evaluate(s)}catch(e){return e instanceof SyntaxError}return false})}
invalid('()=>new.target');invalid('()=>()=>new.target');invalid('(a=new.target)=>a');invalid('({f:()=>new.target})');invalid('function valid(){};new.target');
check('global arrow direct eval',function(){var f=evaluate('()=>eval("new.target")');try{f()}catch(e){return e instanceof SyntaxError}return false});
check('nested global arrow direct eval',function(){var f=evaluate('()=>()=>eval("new.target")')();try{f()}catch(e){return e instanceof SyntaxError}return false});
check('function arrow eval',function(){function C(){return ()=>eval('new.target')}return (new C)()===C&&C()()===undefined});
check('function formals new target',function(){function C(x=new.target){return {x:x}}return (new C).x===C&&C().x===undefined});
check('arrow formals inherit new target',function(){function C(){return (x=new.target)=>x}return (new C)()===C&&C()()===undefined});
check('Function constructor new target',function(){var C=Function('x=new.target','return {x:x}');return (new C).x===C});
check('eval arrow formals',function(){function C(){return eval('(x=new.target)=>x')}return (new C)()===C});
print('NEW-TARGET-CONTEXT checks='+checks+' failures='+failures);if(failures)throw Error('new target context');
