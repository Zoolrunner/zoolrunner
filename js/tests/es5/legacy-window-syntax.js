/* Unversioned XUL scripts retain classic application syntax. */
var legacyWindowValue;
var legacyWindowObject = {
    set value() { legacyWindowValue = arguments[0]; },
    overridden: 0,
    get overridden() { return 7; }
};
var legacyWindowRegexp = /^http:|\
    ^ftp:/i;
