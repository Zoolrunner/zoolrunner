/* GConf-free package contract and protocol fallback regression.
 * Run only against a --disable-gconf Linux package, in a disposable profile.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
var Cc = Components.classes, Ci = Components.interfaces, checks = 0;
function check(value, label) {
  if (!value) throw new Error(label);
  ++checks;
  print('PASS ' + label);
}
var contracts = ['@mozilla.org/gnome-gconf-service;1',
                 '@mozilla.org/gnome-vfs-service;1',
                 '@mozilla.org/system-preference-service;1',
                 '@mozilla.org/system-preferences;1',
                 '@mozilla.org/browser/shell-service;1'];
for (var i = 0; i < contracts.length; ++i)
  check(!(contracts[i] in Cc), 'absent ' + contracts[i]);
check(!('nsIGConfService' in Ci), 'GConf typelib absent');
var prefs = Cc['@mozilla.org/preferences-service;1'].getService(Ci.nsIPrefBranch);
var external = Cc['@mozilla.org/uriloader/external-protocol-service;1']
                 .getService(Ci.nsIExternalProtocolService);
var scheme = 'zoolrunner-gconf-probe';
check(!external.externalProtocolHandlerExists(scheme), 'unconfigured protocol has no handler');
prefs.setCharPref('network.protocol-handler.app.' + scheme, '/bin/sh');
check(external.externalProtocolHandlerExists(scheme), 'explicit executable handler works');
prefs.clearUserPref('network.protocol-handler.app.' + scheme);
check(!external.externalProtocolHandlerExists(scheme), 'cleared handler is unavailable');
print('GCONF-DISABLED checks=' + checks + ' failures=0');
