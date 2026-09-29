/* Additive session-entry storage regression; run with native xpcshell.
 * MPL 1.1/GPL 2.0/LGPL 2.1. */
var Cc=Components.classes,Ci=Components.interfaces,checks=0;
function check(value,label){if(!value)throw new Error(label);++checks;print('PASS '+label);}
var entry=Cc['@mozilla.org/browser/session-history-entry;1'].createInstance(Ci.nsISHEntry);
var state=entry.QueryInterface(Ci.nsISHEntryState);
check(state.historyState===null,'new entry has no state');
var io=Cc['@mozilla.org/network/io-service;1'].getService(Ci.nsIIOService);
entry.setURI(io.newURI('http://example.test/history',null,null));
entry.setTitle('Original title');entry.pageIdentifier=987;
var data=Cc['@mozilla.org/supports-string;1'].createInstance(Ci.nsISupportsString);
data.data='immutable serialized payload\u0000\ud800';
state.historyState=data;
check(state.historyState.QueryInterface(Ci.nsISupportsString).data===data.data,'payload retained without string conversion');
var copy=entry.clone(),copyState=copy.QueryInterface(Ci.nsISHEntryState);
check(copyState.historyState.QueryInterface(Ci.nsISupportsString)===data,'clone shares immutable payload');
check(copy.URI.spec===entry.URI.spec,'clone preserves legacy URI');
check(copy.title===entry.title,'clone preserves legacy title');
check(copy.pageIdentifier===entry.pageIdentifier,'clone preserves same-document identifier');
check(copy.ID===entry.ID,'ordinary clone preserves legacy entry identifier');
state.historyState=null;
check(copyState.historyState.QueryInterface(Ci.nsISupportsString)===data,'clearing original leaves clone intact');
var second=Cc['@mozilla.org/supports-string;1'].createInstance(Ci.nsISupportsString);second.data='replacement';
state.historyState=second;
check(copyState.historyState.QueryInterface(Ci.nsISupportsString)===data,'replacing original leaves clone intact');
copyState.historyState=null;
check(state.historyState.QueryInterface(Ci.nsISupportsString)===second,'clearing clone leaves original intact');
print('HISTORY-ENTRY-STATE checks='+checks+' failures=0');
