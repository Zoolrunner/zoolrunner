/* Loaded with an explicit ES2015 MIME version in XUL and HTML. */
"use strict";
function modernWindowEdition() {
    let radixValue = 0b101 + 0o17;
    var caught = false;
    try { var nested; [[nested]] = [null]; }
    catch (error) { caught = error instanceof TypeError; }
    var conflict = false;
    try { eval("(function(parameter){let parameter;})"); }
    catch (error) { conflict = error instanceof SyntaxError; }
    return radixValue === 20 && Number("0b101") === 5 && caught && conflict && ({value:1,value:2}).value === 2 &&
           eval("({value:1,value:2}).value") === 2 &&
           typeof /a/ === "object";
}
var modernWindowLoaded = modernWindowEdition();
