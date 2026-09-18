/* Property reference evaluation order, callback lifetime and raw receivers.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
(function () {
    var checks=0, failures=0;
    function test(label, body) {
        ++checks;
        try { if (body() === true) return; }
        catch (e) { print('DETAIL ' + label + ': ' + e); }
        ++failures;
        print('FAIL assignment reference: ' + label);
    }
    function throws(type, body) {
        try { body(); } catch (e) { return e instanceof type; }
        return false;
    }
    var operations=['=', '+=', '-=', '*=', '/=', '%=', '<<=', '>>=', '>>>=', '&=', '|=', '^='];
    for (var i=0;i<operations.length;++i) (function(op) {
        test(op + ' key converted once before rhs', function() {
            var order=[], object={}, key={toString:function(){order.push('key');gc();return 'x';}};
            Object.defineProperty(object,'x',{
                get:function(){order.push('get');gc();return 3;},
                set:function(value){order.push('set');gc();}
            });
            function rhs(){order.push('rhs');gc();return 2;}
            eval('object[key] '+op+' rhs()');
            return order.join() === (op==='=' ? 'key,rhs,set' : 'key,get,rhs,set');
        });
        test(op + ' null rejects before rhs and key conversion', function() {
            var order=[], key={toString:function(){order.push('key');return 'x';}}, object=null;
            function rhs(){order.push('rhs');return 2;}
            return throws(TypeError,function(){eval('object[key] '+op+' rhs()');}) && order.length===0;
        });
        test(op + ' undefined dot rejects before rhs', function() {
            var order=[], object=undefined;
            function rhs(){order.push('rhs');return 2;}
            return throws(TypeError,function(){eval('object.x '+op+' rhs()');}) && order.length===0;
        });
    })(operations[i]);
    test('key expression runs before null check', function() {
        var marker={}, ran=false;
        function key(){ran=true;throw marker;}
        try { null[key()] = 1; } catch(e) { return ran && e===marker; }
        return false;
    });
    test('key conversion exception prevents rhs', function() {
        var marker={}, ran=false, object={}, key={toString:function(){throw marker;}};
        try { object[key] = (ran=true); } catch(e) { return e===marker && !ran; }
        return false;
    });
    test('rhs cannot change captured key', function() {
        var property='before', object={}, key={toString:function(){return property;}};
        object[key] = (property='after', 7);
        return object.before===7 && !object.hasOwnProperty('after');
    });
    test('symbol key conversion precedes rhs', function() {
        var order=[], symbol=Symbol('assignment'), object={}, key={};
        key[Symbol.toPrimitive]=function(hint){order.push(hint);gc();return symbol;};
        object[key]=(order.push('rhs'),8);
        return order.join()==='string,rhs' && object[symbol]===8;
    });
    test('primitive setter receives raw receiver', function() {
        var receiver, written, key={toString:function(){gc();return 'assignmentProbe';}};
        Object.defineProperty(Number.prototype,'assignmentProbe',{
            configurable:true,set:function(value){'use strict';receiver=this;written=value;gc();}
        });
        try { (3)[key]=17; return receiver===3 && written===17; }
        finally {delete Number.prototype.assignmentProbe;}
    });
    test('assignment result remains rhs', function() {
        var object={}, key={toString:function(){gc();return 'x';}}, value={};
        return (object[key]=value)===value && object.x===value;
    });
    test('converted key retained across callback GC', function() {
        var object={}, key={toString:function(){return 'generated-'+Math.random();}};
        object[key]=(key=null,gc(),19);
        var names=Object.keys(object);
        return names.length===1 && names[0].indexOf('generated-')===0 && object[names[0]]===19;
    });
    test('decompilation retains reference ordering', function() {
        var original=function(object,key,rhs){return object[key]=rhs();};
        var copy=eval('('+original.toString()+')'), order=[], object={};
        copy(object,{toString:function(){order.push('key');return 'x';}},function(){order.push('rhs');return 4;});
        return order.join()==='key,rhs' && object.x===4;
    });
    print('ES6-ASSIGNMENT-REFERENCE checks='+checks+' failures='+failures);
    if(failures)throw Error('assignment reference regressions: '+failures);
})();
