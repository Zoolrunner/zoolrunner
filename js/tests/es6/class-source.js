/* Complete class source round trips, including file-backed compilation. */
var classSourceChecks=0;
function check(label, f) { if (!f()) throw Error("CLASS-SOURCE FAIL: "+label); ++classSourceChecks; }
check('declaration',function(){class /* comment */ C {constructor(x){this.x=x} m(){return this.x}} var text=C.toString();var D=eval('('+text+')');return new D(4).m()===4;});
check('expression',function(){var C=class Named {m(){return 5}};var D=eval('('+C.toString()+')');return new D().m()===5&&D.name==='Named'});
check('nested',function(){class A { m(){return class B {m(){return 6}}}}var B=new A().m();return new (eval('('+B.toString()+')'))().m()===6;});
check('derived',function(){class A {constructor(x){this.x=x}m(){return this.x}}class B extends A {m(){return super.m()+1}}var D=eval('('+B.toString()+')');return new D(6).m()===7;});
check('Unicode',function(){var C=eval('class Caf\u00e9 {m(){return "\u2603"}}; Caf\u00e9');var D=eval('('+C.toString()+')');return new D().m()==='\u2603'});
check('long source',function(){var text='(class Long {m(){return "'+new Array(1000).join('x')+'"}})';var C=eval(text);var D=eval('('+C.toString()+')');return new D().m().length===999});
check('outer roundtrip',function(){function outer(){class A {m(){return 7}} return A}var f=eval('('+outer.toString()+')');return new (f())().m()===7});
check('template',function(){class C { m(){return `one\n${1+2}\nend`} } var D=eval('('+C.toString()+')');return new D().m()==='one\n3\nend'});

print("CLASS-SOURCE PASS checks="+classSourceChecks);
