var checks=0;function check(v,m){checks++;if(!v)throw Error(m);}
function syntax(s){var threw=false;try{Function(s);}catch(e){threw=e instanceof SyntaxError;}check(threw,s);}
syntax('return function*(){let\nyield 0;}');
syntax('return function*(){let yield;}');
syntax('return function*(){for(let\nyield of []); }');
syntax('return function*(){let\nyield=1;}');
check(Function('let\nyield=3;return yield;')()===3,'ordinary sloppy yield binding');
check(Function('return function*(){let x=3;yield x;}')()().next().value===3,'generator ordinary lexical declaration');
check(Function('var let=3;return function*(){yield let;}')()().next().value===3,'generator may reference sloppy let identifier');
print('GENERATOR-LET-NEWLINE checks='+checks+' failures=0');
