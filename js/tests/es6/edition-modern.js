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
    var length = Object.getOwnPropertyDescriptor(modernWindowEdition, "length");
    var name = Object.getOwnPropertyDescriptor(modernWindowEdition, "name");
    var metadata = length.value === 0 && length.configurable && !length.writable &&
                   name.value === "modernWindowEdition" && name.configurable;
    var inferred = function() { return 42; };
    var accessor = Object.getOwnPropertyDescriptor({get value() {return 42;}}, "value").get;
    var inferredMetadata = inferred.name === "inferred" && accessor.name === "get value" &&
                           !accessor.hasOwnProperty("prototype");
    const fixed = 1;
    var immutable = false;
    try { fixed++; } catch (error) { immutable = error instanceof TypeError; }
    return radixValue === 20 && Number("0b101") === 5 && caught && conflict && ({value:1,value:2}).value === 2 &&
           metadata && inferredMetadata && immutable && modernWindowEdition.bind(null).name === "bound modernWindowEdition" &&
           eval("({value:1,value:2}).value") === 2 &&
           typeof /a/ === "object";
}
var modernWindowLoaded = modernWindowEdition();
