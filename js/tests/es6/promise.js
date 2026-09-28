/* Promise reactions, thenable reentrancy, species and iterator cleanup.
 * Run in an ES2015 shell with the real engine job checkpoint.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
(function () {
    var checks = 0, failures = 0;
    function check(value, label) {
        checks++;
        if (!value) { failures++; print('FAIL Promise: ' + label); }
    }
    function throwsTypeError(callback, label) {
        var correct = false;
        try { callback(); } catch (error) { correct = error instanceof TypeError; }
        check(correct, label);
    }
    check(typeof Promise === 'function', 'constructor exists');
    if (typeof Promise !== 'function') throw Error('Promise is not implemented');
    check(Promise.length === 1, 'constructor length');
    throwsTypeError(function () { Promise(function () {}); }, 'requires new');
    throwsTypeError(function () { new Promise(null); }, 'requires callable executor');
    throwsTypeError(function () { Promise.prototype.then.call({}); }, 'then brand');
    throwsTypeError(function () { Promise.prototype.then.call(Promise.prototype); }, 'prototype is not initialized');
    var order = [], resolve, reject;
    var promise = new Promise(function (yes, no) {
        'use strict';
        check(this === undefined, 'executor raw receiver');
        check(yes.length === 1 && no.length === 1, 'resolving function length');
        check(!yes.hasOwnProperty('name') && !no.hasOwnProperty('name'), 'ES2015 anonymous resolvers');
        throwsTypeError(function () { new yes(); }, 'resolve not a constructor');
        resolve = yes; reject = no; order.push('executor');
    });
    promise.then(function (value) {
        'use strict';
        check(this === undefined, 'handler raw receiver');
        check(value === 42, 'fulfilled value');
        order.push('first');
        promise.then(function () { order.push('nested'); });
        drainJobQueue();
        order.push('first-end');
        gc();
    });
    promise.then(function () { order.push('second'); });
    resolve(42); reject('ignored'); resolve('ignored');
    check(order.join(',') === 'executor', 'handlers deferred');
    gc(); drainJobQueue();
    check(order.join(',') === 'executor,first,first-end,second,nested', 'FIFO and nested checkpoint');
    check(Promise.resolve(promise) === promise, 'resolve identity');
    var failure = {}, caught;
    new Promise(function () { throw failure; }).catch(function (error) { caught = error; });
    drainJobQueue(); check(caught === failure, 'executor throw rejects');
    var thenOrder = [], thenable = {};
    Object.defineProperty(thenable, 'then', {get: function () {
        thenOrder.push('get'); gc();
        return function (yes, no) {
            check(this === thenable, 'thenable receiver');
            thenOrder.push('call'); yes(7); no('ignored'); throw failure;
        };
    }});
    Promise.resolve(thenable).then(function (value) { thenOrder.push(value); });
    check(thenOrder.join(',') === 'get', 'then getter synchronous, invocation deferred');
    gc(); drainJobQueue();
    check(thenOrder.join(',') === 'get,call,7', 'thenable first resolution wins');
    var getterError = {};
    Object.defineProperty(getterError, 'then', {get: function () { throw failure; }});
    caught = undefined;
    Promise.resolve(getterError).catch(function (error) { caught = error; });
    drainJobQueue(); check(caught === failure, 'then getter failure rejects');
    var selfResolve, self = new Promise(function (yes) { selfResolve = yes; });
    caught = undefined; self.catch(function (error) { caught = error; });
    selfResolve(self); drainJobQueue();
    check(caught instanceof TypeError, 'self resolution rejects');
    caught = undefined;
    Promise.resolve(1).then(function () { throw failure; }).then().catch(function (error) { caught = error; });
    drainJobQueue(); check(caught === failure, 'thrower propagation');
    var identity;
    Promise.resolve(5).then(null).then(function (value) { identity = value; });
    drainJobQueue(); check(identity === 5, 'identity propagation');
    var forwarded;
    check(Promise.prototype.catch.call({then: function (yes, no) {
        forwarded = [yes, no]; return 17;
    }}, failure) === 17, 'generic catch result');
    check(forwarded[0] === undefined && forwarded[1] === failure, 'generic catch arguments');
    var speciesCalls = 0, customResult = {};
    function Species(executor) {
        speciesCalls++; gc();
        executor(function (value) { customResult.value = value; }, function (reason) { customResult.error = reason; });
        return customResult;
    }
    promise.constructor = {};
    promise.constructor[Symbol.species] = Species;
    check(promise.then(function (value) { return value + 1; }) === customResult, 'custom species result');
    drainJobQueue();
    check(speciesCalls === 1 && customResult.value === 43, 'species capability');
    var allResult, raceResult;
    Promise.all([Promise.resolve(1), 2, thenable]).then(function (value) { allResult = value; });
    Promise.race([Promise.resolve(3), Promise.resolve(4)]).then(function (value) { raceResult = value; });
    gc(); drainJobQueue();
    check(allResult.join(',') === '1,2,7', 'all ordered values');
    check(raceResult === 3, 'race first settlement');
    var empty;
    Promise.all([]).then(function (value) { empty = value; });
    drainJobQueue(); check(Array.isArray(empty) && empty.length === 0, 'all empty iterable');
    var raceEmpty = false;
    Promise.race([]).then(function () { raceEmpty = true; }, function () { raceEmpty = true; });
    drainJobQueue(); check(!raceEmpty, 'empty race remains pending');
    function iteratorFailures(method) {
        var closed = 0, reason, iterable = {}, iterator = {};
        iterable[Symbol.iterator] = function () { return iterator; };
        iterator['return'] = function () { closed++; throw Error('close failure'); };
        iterator.next = function () { throw failure; };
        Promise[method](iterable).catch(function (error) { reason = error; });
        drainJobQueue();
        check(reason === failure && closed === 0, method + ' next failure marks done');
        var step = {done: false};
        Object.defineProperty(step, 'value', {get: function () { throw failure; }});
        iterator.next = function () { return step; };
        reason = undefined;
        Promise[method](iterable).catch(function (error) { reason = error; });
        drainJobQueue();
        check(reason === failure && closed === 0, method + ' value failure marks done');
        function Constructor(executor) { return new Promise(executor); }
        Object.defineProperty(Constructor, 'resolve', {get: function () { throw failure; }});
        iterator.next = function () { return {done: false, value: 1}; };
        reason = undefined;
        Promise[method].call(Constructor, iterable).catch(function (error) { reason = error; });
        drainJobQueue();
        check(reason === failure && closed === 1, method + ' resolve failure closes, original throw wins');
    }
    iteratorFailures('all'); iteratorFailures('race');
    var revoked = Proxy.revocable(function () {}, {}), revokedReason;
    revoked.revoke();
    Promise.resolve(1).then(revoked.proxy).catch(function (reason) { revokedReason = reason; });
    drainJobQueue();
    check(revokedReason instanceof TypeError, 'revoked handler rejects asynchronously');
    revokedReason = undefined;
    Promise.resolve({then: revoked.proxy}).catch(function (reason) { revokedReason = reason; });
    drainJobQueue();
    check(revokedReason instanceof TypeError, 'revoked thenable callback rejects asynchronously');
    print('ES6-PROMISE checks=' + checks + ' failures=' + failures);
    if (failures) throw Error('Promise regressions');
})();
