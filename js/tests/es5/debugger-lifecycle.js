/* Debugger callbacks may collect scripts or stop debugging during enumeration. */
var debuggerService = Components.classes['@mozilla.org/js/jsd/debugger-service;1']
    .getService(Components.interfaces.jsdIDebuggerService);
var debuggerChecks = 0;
function debuggerCheck(ok, label) {
    ++debuggerChecks;
    if (!ok) throw Error(label);
}
try {
    debuggerService.on();
    var rejectedNull = false;
    try { debuggerService.enumerateScripts(null); } catch (e) { rejectedNull = true; }
    debuggerCheck(rejectedNull, 'null enumerator rejected');
    var debuggerTemporary = [];
    for (var debuggerIndex = 0; debuggerIndex < 300; ++debuggerIndex)
        debuggerTemporary.push(eval('(function debuggerTemporary' + debuggerIndex + '(){return 1;})'));
    var debuggerSurvivor = eval('(function debuggerSurvivor(){return 99;})');
    var debuggerEnumerated = 0, debuggerSawSurvivor = false;
    debuggerService.enumerateScripts({enumerateScript: function(script) {
        ++debuggerEnumerated;
        if (debuggerEnumerated == 1) {
            debuggerTemporary = null;
            debuggerService.GC();
        }
        if (script.functionName == 'debuggerSurvivor') debuggerSawSurvivor = true;
        if (!(script.lineExtent > 0)) throw Error('invalid live script metadata');
    }});
    debuggerCheck(debuggerEnumerated > 0, 'live script metadata');
    debuggerCheck(debuggerSawSurvivor && debuggerSurvivor() == 99, 'surviving script enumerated');
    var debuggerStopped = 0;
    debuggerService.enumerateScripts({enumerateScript: function(script) {
        ++debuggerStopped;
        debuggerService.off();
    }});
    debuggerCheck(debuggerStopped == 1 && !debuggerService.isOn, 'stop during enumeration');
    debuggerService.on();
    var debuggerRestarted = eval('(function debuggerRestarted(){return 7;})');
    var debuggerSawRestart = false;
    debuggerService.enumerateScripts({enumerateScript: function(script) {
        if (script.functionName == 'debuggerRestarted') debuggerSawRestart = true;
    }});
    debuggerCheck(debuggerSawRestart && debuggerRestarted() == 7, 'restart after callback shutdown');
    print('DEBUGGER-LIFECYCLE checks=' + debuggerChecks + ' failures=0');
} finally {
    if (debuggerService.isOn) debuggerService.off();
}
