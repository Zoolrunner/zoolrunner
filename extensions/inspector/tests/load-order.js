/* Exercise the real controller with both browser/registry event orders. */
var checks=0, removed=0, persisted=0, subjects=0;
function same(a,b,label){checks++;if(a!==b)throw Error(label+': '+a+' != '+b);}
var window={addEventListener:function(){}};
var Node={DOCUMENT_NODE:9,ELEMENT_NODE:1};
var InsUtil={persistAll:function(){persisted++;}};
var _content={document:{nodeType:9}};
var navigation={currentURI:{spec:'about:blank'}};
var input={value:''};
var document={getElementById:function(id){
    if(id==='ifBrowser')return {webNavigation:navigation};
    if(id==='bxBrowser')return {removeEventListener:function(){removed++;}};
    if(id==='tfURLBar')return input;
    throw Error('Unexpected element '+id);
}};
load(inspectorSource);
function controller(){
    var app=new InspectorApp();
    var panel={addObserver:function(){},removeObserver:function(){removed++;}};
    app.mPanelSet={getPanel:function(){return panel;},removeObserver:function(){removed++;}};
    app.addToHistory=function(){subjects++;};
    return app;
}
var app=controller();
app.documentLoaded();
same(app.mPendingDocumentLoad,true,'early load queued');
same(subjects,0,'no premature panel use');
app.initViewerPanels();
same(app.mDocPanel.subject,_content.document,'queued document delivered');
same(app.mPendingDocumentLoad,false,'queued load consumed');
same(subjects,1,'history recorded once');
same(input.value,'about:blank','location updated');
_content.document={nodeType:9};app.documentLoaded();
same(app.mDocPanel.subject,_content.document,'ready load delivered');
app.destroy();
same(removed,3,'listeners removed');
same(persisted,2,'panel persistence retained');
var old=app.mDocPanel.subject;
_content.document={nodeType:9};app.documentLoaded();app.onEvent({type:'panelsetready'});
same(app.mDocPanel.subject,old,'late callbacks ignored');
var early=controller();early.documentLoaded();early.destroy();early.initViewerPanels();
same(early.mDocPanel,undefined,'close before registry ready');
same(early.mPendingDocumentLoad,false,'closed pending load discarded');
print('INSPECTOR-LOAD-ORDER checks='+checks+' failures=0');
