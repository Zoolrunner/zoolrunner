// Xlib without an initialized application display must fail safely.
var enumerator = Components.classes['@mozilla.org/gfx/fontenumerator;1']
                          .createInstance(Components.interfaces.nsIFontEnumerator);
var failures = 0, checks = 0;
function unavailable(callback, label) {
  var error = null;
  try { callback(); } catch (caught) { error = caught; }
  ++checks;
  if (!error || error.result != Components.results.NS_ERROR_NOT_AVAILABLE) {
    ++failures;
    print('FAIL ' + label + ': ' + error);
  }
}
unavailable(function() { enumerator.EnumerateAllFonts({}); }, 'all fonts before display');
unavailable(function() { enumerator.EnumerateFonts('x-western', 'serif', {}); },
            'filtered fonts before display');
print('FONT-ENUMERATION-HEADLESS checks=' + checks + ' failures=' + failures);
if (failures) quit(1);
