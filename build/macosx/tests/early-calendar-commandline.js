/* Test-only startup hook: Calendar's own handler does not implement -chrome. */
var Cc = Components.classes;
var Ci = Components.interfaces;
var Cr = Components.results;
var classID = Components.ID("{bf6fa13c-36c8-4ef0-9308-68f5a135e655}");
var contractID = "@zoolrunner.org/tests/calendar-command-line;1";
var category = "a-zoolrunner-test";

var handler = {
    QueryInterface: function(iid) {
        if (iid.equals(Ci.nsISupports) || iid.equals(Ci.nsICommandLineHandler))
            return this;
        throw Cr.NS_ERROR_NO_INTERFACE;
    },
    handle: function(commandLine) {
        if (!commandLine.handleFlag("zoolrunner-test", false)) return;
        commandLine.preventDefault = true;
        Cc["@mozilla.org/embedcomp/window-watcher;1"].getService(Ci.nsIWindowWatcher)
            .openWindow(null, "chrome://zooltest/content/early-application.xul",
                        "_blank", "chrome,dialog=no,all", null);
    },
    helpInfo: "  -zoolrunner-test    Run the packaged platform GUI test.\n"
};
var factory = {
    QueryInterface: function(iid) {
        if (iid.equals(Ci.nsISupports) || iid.equals(Ci.nsIFactory)) return this;
        throw Cr.NS_ERROR_NO_INTERFACE;
    },
    createInstance: function(outer, iid) {
        if (outer) throw Cr.NS_ERROR_NO_AGGREGATION;
        return handler.QueryInterface(iid);
    },
    lockFactory: function(lock) {}
};
var module = {
    QueryInterface: function(iid) {
        if (iid.equals(Ci.nsISupports) || iid.equals(Ci.nsIModule)) return this;
        throw Cr.NS_ERROR_NO_INTERFACE;
    },
    registerSelf: function(manager, file, location, type) {
        manager.QueryInterface(Ci.nsIComponentRegistrar).registerFactoryLocation(
            classID, "Calendar platform test launcher", contractID, file, location, type);
        Cc["@mozilla.org/categorymanager;1"].getService(Ci.nsICategoryManager)
            .addCategoryEntry("command-line-handler", category, contractID, true, true);
    },
    unregisterSelf: function(manager, file, location) {
        manager.QueryInterface(Ci.nsIComponentRegistrar)
            .unregisterFactoryLocation(classID, file);
        Cc["@mozilla.org/categorymanager;1"].getService(Ci.nsICategoryManager)
            .deleteCategoryEntry("command-line-handler", category, true);
    },
    getClassObject: function(manager, cid, iid) {
        if (!cid.equals(classID)) throw Cr.NS_ERROR_NO_INTERFACE;
        return factory.QueryInterface(iid);
    },
    canUnload: function(manager) { return true; }
};
function NSGetModule(manager, file) { return module; }
