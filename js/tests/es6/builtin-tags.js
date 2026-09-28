/* Built-in tag metadata, fallback and explicit legacy class naming. */
var checks = 0;
function check(ok, label) { ++checks; if (!ok) throw Error('FAIL builtin tags: ' + label); }
var objects = [Math, JSON], names = ['Math', 'JSON'];
version(170);
var legacyTag = evaluate('(function(value){return Object.prototype.toString.call(value);})', 'legacy-tag');
version(2015);
var toString = Object.prototype.toString;
for (var i = 0; i < objects.length; ++i) {
    var object = objects[i], name = names[i];
    var descriptor = Object.getOwnPropertyDescriptor(object, Symbol.toStringTag);
    check(descriptor.value === name && !descriptor.writable && !descriptor.enumerable &&
          descriptor.configurable, name + ' descriptor');
    check(toString.call(object) === '[object ' + name + ']', name + ' initial tag');
    check(Object.keys(object).length === 0 && Object.getOwnPropertySymbols(object)[0] === Symbol.toStringTag,
          name + ' reflection');
    check(delete object[Symbol.toStringTag], name + ' configurable');
    check(toString.call(object) === '[object Object]', name + ' modern fallback');
    check(legacyTag(object) === '[object ' + name + ']', name + ' legacy fallback');
    Object.defineProperty(object, Symbol.toStringTag, {configurable:true, value:17});
    check(toString.call(object) === '[object Object]', name + ' non-string fallback');
    Object.defineProperty(object, Symbol.toStringTag, {configurable:true, get:function() {
        gc(); return '\u03bb\u0000tag';
    }});
    check(toString.call(object) === '[object \u03bb\u0000tag]', name + ' Unicode getter');
    Object.defineProperty(object, Symbol.toStringTag, descriptor);
}
var symbolTag = Object.getOwnPropertyDescriptor(Symbol.prototype, Symbol.toStringTag);
delete Symbol.prototype[Symbol.toStringTag];
check(toString.call(Symbol()) === '[object Object]', 'Symbol wrapper has no ordinary class tag');
Object.defineProperty(Symbol.prototype, Symbol.toStringTag, symbolTag);
var values = [[], '', (function(){return arguments;})(), function(){}, new Error(), true, 1, new Date(), /x/];
var tags = ['Array', 'String', 'Arguments', 'Function', 'Error', 'Boolean', 'Number', 'Date', 'RegExp'];
for (i = 0; i < values.length; ++i)
    check(toString.call(values[i]) === '[object ' + tags[i] + ']', 'ordinary tag ' + tags[i]);
check(toString.call(undefined) === '[object Undefined]' && toString.call(null) === '[object Null]',
      'null and undefined');
print('ES6-BUILTIN-TAGS checks=' + checks + ' failures=0');
