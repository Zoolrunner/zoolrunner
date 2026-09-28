/* Decimal quantifier magnitudes, matching and exact range validation. */
var checks=0;
function check(ok,label){++checks;if(!ok)throw Error(label);}
function syntax(pattern,label){var threw=false;try{new RegExp(pattern);}catch(e){threw=e instanceof SyntaxError;}check(threw,label);}
var counts=['65536','4294967296','9007199254740991','18446744073709551615','18446744073709551616','10000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000'];
for(var i=0;i<counts.length;++i){
 var n=counts[i];
 for(var j=0;j<2;++j){
  var flag=j?'u':'';
  check(!new RegExp('a{'+n+'}',flag).test(''),'exact empty '+n+flag);
  check(new RegExp('(?:){'+n+'}',flag).test(''),'pure empty child '+n+flag);
  check(new RegExp('(?:){'+n+'}?',flag).test(''),'minimal pure empty child '+n+flag);
  check(!new RegExp('a{'+n+',}?',flag).test('b'),'open bound '+n+flag);
  check(!new RegExp('(?:a){'+n+','+n+'}',flag).test('a'),'range short input '+n+flag);
  check(new RegExp('a{0,'+n+'}',flag).exec('aaa')[0]==='aaa','greedy maximum '+n+flag);
  check(new RegExp('^a{0,'+n+'}?a$',flag).exec('a')[0]==='a','minimal maximum '+n+flag);
 }
}
syntax('a{65537,65536}','16-bit boundary reversed');
syntax('a{4294967297,4294967296}','32-bit boundary reversed');
syntax('a{9007199254740993,9007199254740992}','exact comparison above double precision');
syntax('a{18446744073709551616,18446744073709551615}','64-bit boundary reversed');
syntax('a{18446744073709551617,18446744073709551616}','above 64-bit reversed');
syntax('a{10000000000000000000000000000000000000000000000000001,10000000000000000000000000000000000000000000000000000}','large decimal reversed');
syntax('a{000000000000000000000065537,0000065536}','leading zeros reversed');
check(!new RegExp('a{000000000000000065536,000065536}').test('a'),'leading zeros equality');
check(/a{65536}/.test('a')===false,'literal grammar');
check(/(?:){65536}/.test(''),'empty finite child');
check(/^(a?){65536}$/.exec('a')[1]==='','greedy final empty capture');
check(/^(a?){65536}?$/.exec('a')[1]==='','minimal final empty capture');
check(/^((?:a|)){65536}b$/.exec('ab')[1]==='','alternative empty captures');
var text=new Array(65537).join('a');
check(/^a{65536}$/.test(text),'wide exact successful match');
check(/^a{65536,65537}$/.test(text+'a'),'wide upper bound successful match');
check(!/^a{65536,65537}$/.test(text+'aa'),'wide finite maximum exhausted');
check(/^a{65536,65537}?a$/.test(text+'a'),'wide minimal bound');
check(new RegExp('a{1,'+counts[5]+'}').exec('aaa')[0]==='aaa','huge finite maximum');
check(/(a){65536}\1/.test(text+'a'),'backreference after large count');
check(new RegExp('a{70000x').test('a{70000x'),'non-quantifier Annex B text');
var rejected=false;try{new RegExp('a{70000x','u');}catch(e){rejected=e instanceof SyntaxError;}check(rejected,'unicode malformed quantifier');
gc();check(new RegExp('a{'+counts[3]+'}').source==='a{'+counts[3]+'}','source survives collection');
print('REGEXP-LARGE-QUANTIFIERS checks='+checks+' failures=0');
