/* Computed object pattern keys. MPL 1.1/GPL 2.0/LGPL 2.1. */
(function () {
    var checks=0;
    function check(ok,label){++checks;if(!ok)throw Error(label)}
    function throws(kind,fn,label){var yes=false;try{fn()}catch(e){yes=e instanceof kind}check(yes,label)}
    var key='x', x, events=[], source={x:7};
    check(({[key]:x}=source)===source&&x===7,'assignment identity');
    var {[key]:local}=source;
    check(local===7,'binding');
    var symbol=Symbol('key');source[symbol]=9;
    ({[symbol]:x}=source);check(x===9,'symbol key');
    key={};key[Symbol.toPrimitive]=function(hint){events.push(hint);gc();return symbol};
    ({[key]:x}=source);check(x===9&&events.join()==='string','one property key conversion');
    events=[];
    throws(TypeError,function(){({[key]:x}=null)},'null source');
    check(events.length===0,'coercibility before key conversion');
    var marker={};key[Symbol.toPrimitive]=function(){throw marker};
    var caught;try{({[key]:x}=source)}catch(e){caught=e}check(caught===marker,'key abrupt completion');
    ({[0]:x}=['zero']);check(x==='zero','numeric key');
    ({[-0]:x}=['zero']);check(x==='zero','negative zero key');
    ({[true]:x}={true:4});check(x===4,'boolean key');
    ({[null]:x}={null:5});check(x===5,'null key');
    var a,b;({['x']:a,['x']:b}=source);check(a===7&&b===7,'duplicate keys');
    ({['nested']:{['value']:x}}={nested:{value:11}});check(x===11,'nested keys');
    throws(TypeError,function(){({['nested']:{['value']:x}}={nested:null})},'nested null');
    var sourceReads=0;
    source={get x(){++sourceReads;gc();return 13}};
    ({['x']:x}=source);check(x===13&&sourceReads===1,'source getter once');
    Object.defineProperty(Number.prototype,'patternReceiver',{configurable:true,get:function(){'use strict';return typeof this}});
    try{({['patternReceiver']:x}=7);check(x==='number','primitive getter receiver')}
    finally{delete Number.prototype.patternReceiver}
    function pattern(o,k){var {[k]:value}=o;return value}
    function comma(o,a,b){var {[(a,b)]:value}=o;return value}
    function nested(o,k){var {[k]:{['value']:value}}=o;return value}
    [pattern,comma,nested].forEach(function(f){var restored=eval('('+f.toString()+')');
        var source={x:{value:17}};
        check(restored(source,'x','x')===f(source,'x','x'),'roundtrip '+f.name)});
    function* generator(o){var {[yield 'key']:value}=o;return value}
    var iterator=generator({x:19});check(iterator.next().value==='key'&&iterator.next('x').value===19,'yield in key');
    var restored=eval('('+generator.toString()+')');iterator=restored({x:23});
    check(iterator.next().value==='key'&&iterator.next('x').value===23,'generator roundtrip');
    var loopValues=[],loopKey='x';
    for(var {[loopKey]:entry} of [{x:1},{x:2}]) loopValues.push(entry);
    check(loopValues.join()==='1,2','computed for-of binding');
    var calls=0,parts=[];
    function tick(){++calls;return 0}
    for(var n=0;n<6000;++n) parts.push('tick()');
    parts.push('"x"');
    var wide=eval('(function(o,k){var {[k?('+parts.join(',')+'):"y"]:v}=o;return v})');
    check(wide({x:7,y:8},true)===7&&calls===6000,'wide computed key');
    restored=eval('('+wide.toString()+')');
    check(restored({x:7,y:8},false)===8&&calls===6000,'wide branch roundtrip');
    check(restored({x:9},true)===9&&calls===12000,'wide key roundtrip');
    print('ES6-COMPUTED-PATTERNS PASS checks='+checks);
}());
