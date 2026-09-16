/* Run the existing regressions with an isolated legacy Suite profile. */
var C = Components.classes, I = Components.interfaces;
var base = 'C:\\ZRREG916\\', name = 'ZoolRegression916';
var checks = 0, failures = 0, created = false, original = null;
function file(path) {
    var f = C['@mozilla.org/file/local;1'].createInstance(I.nsILocalFile);
    f.initWithPath(path); return f;
}
function write(path, text, append) {
    var s = C['@mozilla.org/network/file-output-stream;1'].createInstance(I.nsIFileOutputStream);
    s.init(file(path), 0x02 | 0x08 | (append ? 0x10 : 0x20), 0600, 0);
    s.write(text, text.length); s.close();
}
function read(path) {
    var s = C['@mozilla.org/network/file-input-stream;1'].createInstance(I.nsIFileInputStream);
    s.init(file(path), 1, 0, 0);
    var t = C['@mozilla.org/scriptableinputstream;1'].createInstance(I.nsIScriptableInputStream);
    t.init(s); var text = t.read(t.available()); t.close(); return text;
}
function record(ok, text) {
    ++checks; if (!ok) ++failures;
    text = (ok ? 'PASS ' : 'FAIL ') + text + '\r\n';
    print(text); write(base + 'report.txt', text, true);
}
function run(exe, args) {
    var p = C['@mozilla.org/process/util;1'].createInstance(I.nsIProcess);
    p.init(file(base + 'app\\' + exe)); p.run(true, args, args.length);
    return p.exitValue;
}
if (file(base + 'report.txt').exists()) throw Error('Refusing to reuse previous results');
write(base + 'report.txt', 'Windows Suite regression\r\n' + read(base + 'version.txt'), false);
var profiles = C['@mozilla.org/profile/manager;1'].getService(I.nsIProfile);
try {
    var requiredInterfaces = ['nsIAppStartup', 'nsIEditingSession',
                              'nsIFileProtocolHandler', 'nsIWindowDataSource'];
    for (var n = 0; n < requiredInterfaces.length; ++n) {
        if (!I[requiredInterfaces[n]])
            throw Error('Package missing interface: ' + requiredInterfaces[n]);
    }
    if (I.nsIAppStartup.eForceQuit != 3)
        throw Error('Invalid application startup typelib');
    record(true, 'packaged application interfaces');
    var nativeTests = [['expat','30 checks passed'], ['regexp','REGEXP-ABORT checks=3 failures=0'],
                       ['embedding','ES5-EMBEDDING checks=18 failures=0']];
    for (var n = 0; n < nativeTests.length; ++n) {
        var nativeLog = read(base + nativeTests[n][0] + '.log');
        record(nativeLog.indexOf(nativeTests[n][1]) >= 0 && nativeLog.indexOf('FAIL') < 0, nativeTests[n][0]);
    }
    var cases = ['object-reflection', 'object-descriptors', 'json-bind-string', 'array-date',
                 'library-edge-cases', 'strict-mode', 'legacy-application', 'debugger-lifecycle'];
    for (var i = 0; i < cases.length; ++i) {
        var test = cases[i];
        var result = run('xpcshell.exe', ['-f', base + 'tests\\' + test + '-log.js',
                                         '-f', base + 'tests\\' + test + '.js']);
        var log = file(base + test + '.log').exists() ? read(base + test + '.log') : '';
        record(result == 0 && log.indexOf('failures=0') >= 0, test + ' exit=' + result);
    }
    result = run('xpcshell.exe', ['-f', base + 'tests\\inline-log.js',
                                 '-e', 'print("INLINE-PASS")']);
    log = file(base + 'inline.log').exists() ? read(base + 'inline.log') : '';
    record(result == 0 && log.indexOf('INLINE-PASS') >= 0, 'inline evaluation');
    // A new Wine prefix or Windows installation has no current profile yet.
    // Keep failures for an existing registry visible rather than catching all
    // currentProfile errors and treating them as first-run state.
    if (profiles.profileCount > 0) original = profiles.currentProfile;
    if (profiles.profileExists(name)) throw Error('Regression profile already exists');
    profiles.createNewProfile(name, base, null, false); created = true;
    var profile = profiles.QueryInterface(I.nsIProfileInternal).getProfileDir(name);
    var prefs = {'browser.dom.window.dump.enabled':true,
                 'browser.shell.checkDefaultBrowser':false,
                 'browser.startup.homepage_override.mstone':'ignore',
                 'nglayout.debug.disable_xul_cache':true,
                 'nglayout.debug.disable_xul_fastload':true,
                 'zoolrunner.test.application':'suite',
                 'zoolrunner.test.profile':profile.path};
    var text = '';
    for (var key in prefs) text += 'user_pref(' + JSON.stringify(key) + ',' + JSON.stringify(prefs[key]) + ');\n';
    write(profile.path + '\\user.js', text, false);
    var chrome = file(profile.path + '\\chrome');
    if (!chrome.exists()) chrome.create(I.nsIFile.DIRECTORY_TYPE, 0700);
    write(chrome.path + '\\chrome.rdf', '<?xml version="1.0"?><RDF:RDF xmlns:RDF="http://www.w3.org/1999/02/22-rdf-syntax-ns#" xmlns:c="http://www.mozilla.org/rdf/chrome#"><RDF:Seq RDF:about="urn:mozilla:package:root"><RDF:li RDF:resource="urn:mozilla:package:zooltest"/></RDF:Seq><RDF:Description RDF:about="urn:mozilla:package:zooltest" c:name="zooltest" c:locType="profile" c:baseURL="file:///C:/ZRREG916/tests/"/></RDF:RDF>', false);
    var gui = [['application','APPLICATION PASS:'], ['window','WINDOW-BOOTSTRAP checks=17 failures=0'],
               ['lifecycle','PLATFORM-LIFECYCLE checks=24 failures=0'], ['chatzilla','SUITE-CHATZILLA initialized=true']];
    for (i = 0; i < gui.length; ++i) {
        test = gui[i][0];
        result = run('zoolrunner.exe', ['-P',name,'-chrome','chrome://zooltest/content/' + test + '.xul']);
        log = file(base + test + '.log').exists() ? read(base + test + '.log') : '';
        record(result == 0 && log.indexOf(gui[i][1]) >= 0 && log.indexOf('FAIL') < 0, test + ' exit=' + result);
    }
} catch (error) { record(false, String(error)); }
finally {
    if (created) {
        try {
            var r = C['@mozilla.org/registry;1'].createInstance(I.nsIRegistry);
            r.open(C['@mozilla.org/file/directory_service;1'].getService(I.nsIProperties).get('AppRegF',I.nsIFile));
            var k = r.getKey(I.nsIRegistry.Common,'Profiles');
            if (r.getString(k,'CurrentProfile') == name) {
                if (original === null) r.deleteValue(k,'CurrentProfile');
                else r.setString(k,'CurrentProfile',original);
                r.flush();
            }
            profiles.deleteProfile(name,false);
        } catch (error) { record(false,'Profile cleanup: ' + error); }
    }
}
write(base + 'report.txt', 'RESULT checks=' + checks + ' failures=' + failures + '\r\n', true);
if (failures) quit(1);
