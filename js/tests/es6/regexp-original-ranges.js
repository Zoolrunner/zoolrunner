var checks=0;function check(x,label){checks++;if(!x)throw Error(label)}
function syntax(s){var e;try{new RegExp(s)}catch(x){e=x}check(e instanceof SyntaxError,s)}
var range=new RegExp('[a-\\d]+');
check(range.exec(' abcd- ')[0]==='abcd','original endpoint identity escape');
check(new RegExp('[--\\d]+').exec(' -09AZabcdz')[0]==='-09AZabcd','original ASCII range');
check(new RegExp('[--\\d]+').exec('.-09-.')[0]==='.-09-.','original range includes period');
check(new RegExp('[\\D-Z]+').exec('ABCDEFGXYZ')[0]==='DEFGXYZ','left endpoint identity escape');
check(new RegExp('[\\d-z]+').exec('cdefxyz')[0]==='defxyz','left range to z');
check(/\d+/.exec('ab123cd')[0]==='123','standalone digit set');
check(/[\d]+/.exec('ab123cd')[0]==='123','class digit set');
syntax('[\\d-a]');syntax('[\\s-\\d]');
var e;try{new RegExp('[a-\\d]','u')}catch(x){e=x}check(e instanceof SyntaxError,'Unicode range rejects non-singleton');
var lazy=new RegExp('[a-\\d]+'),saved=version();
try{version(170);check(lazy.exec('abcd')[0]==='abcd','compile edition survives lazy bitmap');check(new RegExp('[a-\\d]+').exec('abcd')[0]==='abcd','constructor realm grammar');check(new RegExp('\\8').test('8'),'constructor realm identity escape');}
finally{version(saved)}
print('ORIGINAL-REGEXP-RANGES checks='+checks+' failures=0');
