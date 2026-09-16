function zrWrite(text) {
    var C = Components.classes, I = Components.interfaces;
    var file = C['@mozilla.org/file/local;1'].createInstance(I.nsILocalFile);
    file.initWithPath(LOGFILE);
    var stream = C['@mozilla.org/network/file-output-stream;1']
        .createInstance(I.nsIFileOutputStream);
    stream.init(file, 0x02 | 0x08 | 0x10, 0600, 0);
    stream.write(text, text.length);
    stream.close();
}
