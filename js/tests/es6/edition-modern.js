/* Loaded with an explicit ES2015 MIME version in XUL and HTML. */
"use strict";
function modernWindowEdition() {
    return ({value:1,value:2}).value === 2 &&
           eval("({value:1,value:2}).value") === 2 &&
           typeof /a/ === "object";
}
var modernWindowLoaded = modernWindowEdition();
