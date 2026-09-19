/* Idle JSD hooks must not disable tail calls; active hooks remain observable.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
'use strict';
var checks = 0;
function check(value, label) {
    ++checks;
    if (!value) throw Error(label);
}
var jsd = Components.classes['@mozilla.org/js/jsd/debugger-service;1']
    .getService(Components.interfaces.jsdIDebuggerService);
var tailDebugCount;
var entries = 0, exits = 0, starts = 0, ends = 0, scriptTag, collected = false;
try {
    jsd.on();
    /* Applications need not start JSD before this fixture is compiled. */
    tailDebugCount = eval('(function tailDebugCount(n) {"use strict";' +
                         'return n ? tailDebugCount(n - 1) : true;})');
    jsd.functionHook = null;
    jsd.topLevelHook = null;
    jsd.flags = jsd.DISABLE_OBJECT_TRACE;
    check(tailDebugCount(100000), 'idle debugger permits tail calls');
    jsd.functionHook = { onCall: function(frame, type) {
        if (frame.functionName === 'tailDebugCount') {
            scriptTag = frame.script.tag;
            if (!collected) { collected = true; jsd.GC(); }
            if (type === 2) ++entries;
            if (type === 3) ++exits;
        }
    }};
    check(tailDebugCount(20), 'active function callback');
    jsd.functionHook = null;
    check(entries === 21 && exits === 21, 'balanced function entry and return');
    check(collected, 'collection during debugger callback');
    check(tailDebugCount(100000), 'cleared function callback');
    jsd.topLevelHook = { onCall: function(frame, type) {
        if (type === 0) ++starts;
        if (type === 1) ++ends;
    }};
    eval('1 + 1;');
    jsd.topLevelHook = null;
    check(starts > 0 && starts === ends, 'balanced top-level callbacks');
    check(tailDebugCount(100000), 'cleared top-level callback');
    jsd.flags = jsd.DISABLE_OBJECT_TRACE | jsd.COLLECT_PROFILE_DATA;
    jsd.clearProfileData();
    check(tailDebugCount(10), 'profiled calls run');
    var calls = 0;
    jsd.enumerateScripts({enumerateScript: function(script) {
        if (script.tag === scriptTag) calls = script.callCount;
    }});
    check(calls === 11, 'profile contains every recursive call');
    jsd.flags = jsd.DISABLE_OBJECT_TRACE;
    check(tailDebugCount(100000), 'disabled profiling restores tail calls');
    jsd.flags = 0;
    function Constructor() { this.answer = 42; }
    check(new Constructor().answer === 42, 'object tracing constructor');
    jsd.flags = jsd.DISABLE_OBJECT_TRACE;
    check(tailDebugCount(100000), 'disabled object tracing restores tail calls');
    print('TAIL-CALL-DEBUGGER checks=' + checks + ' failures=0');
} finally {
    jsd.functionHook = null;
    jsd.topLevelHook = null;
    if (jsd.isOn) jsd.off();
}
