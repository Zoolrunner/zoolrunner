var checks=0;function check(x,label){checks++;if(!x)throw Error(label)}
function syntax(s){var e;try{new RegExp(s)}catch(x){e=x}check(e instanceof SyntaxError,s)}
var range=new RegExp('[a-\\d]+');
check(range.exec('a0123456789-')[0]==='a0123456789-','non-singleton right endpoint union');
check(new RegExp('[--\\d]+').exec(' -09AZabcdz')[0]==='-09','ES2015 class range union');
check(new RegExp('[--\\d]+').exec('.-09-.')[0]==='-09-','non-singleton range excludes periods');
check(new RegExp('[\\D-Z]+').exec('ABCDEFGXYZ')[0]==='ABCDEFGXYZ','non-singleton left endpoint union');
check(new RegExp('[\\d-z]+').exec('cdefxyz')[0]==='z','digit class union through z');
check(/\d+/.exec('ab123cd')[0]==='123','standalone digit set');
check(/[\d]+/.exec('ab123cd')[0]==='123','class digit set');
check(new RegExp('[\\d-a]+').exec(':a0123456789-:')[0]==='a0123456789-','digit set union with dash and atom');check(new RegExp('[\\s-\\d]+').exec('& \t0123456789-&')[0]===' \t0123456789-','space set union with dash and digit set');
var e;try{new RegExp('[a-\\d]','u')}catch(x){e=x}check(e instanceof SyntaxError,'Unicode range rejects non-singleton');
var lazy=new RegExp('[a-\\d]+'),saved=version();
try{version(170);check(lazy.exec('abcd')[0]==='a','compile edition survives lazy bitmap');check(new RegExp('[a-\\d]+').exec('abcd')[0]==='a','global constructor retains ES2015 class semantics');check(new RegExp('\\8').test('8'),'constructor realm identity escape');}
finally{version(saved)}
print('ORIGINAL-REGEXP-RANGES checks='+checks+' failures=0');
