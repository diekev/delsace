test("-x", function () {
    const x = 4;
    const y = -x;
    vérifieQue(y).doitÊtre(-4);

    const a = '4';
    const b = -a;
    vérifieQue(b).doitÊtre(-4);
});

test("+x", function () {
    const x = 1;
    const y = -1;
    vérifieQue(+x).doitÊtre(1);
    vérifieQue(+y).doitÊtre(-1);
    vérifieQue(+'').doitÊtre(0);
    vérifieQue(+true).doitÊtre(1);
    vérifieQue(+false).doitÊtre(0);
    vérifieQue(+'hello').doitÊtreNaN();
});

test("!x", function () {
    const a = 3;
    const b = -2;
    vérifieQue(!(a > 0 || b > 0)).doitÊtre(false);
});

test("~x", function () {
    const a = 5; // 00000000000000000000000000000101
    const b = -3; // 11111111111111111111111111111101
    vérifieQue(~a).doitÊtre(-6); // 11111111111111111111111111111010
    vérifieQue(~b).doitÊtre(2); // 00000000000000000000000000000010
});

test("typeof x", function () {
    vérifieQue(typeof 42).doitÊtre("number");
    vérifieQue(typeof 'blubber').doitÊtre("string");
    vérifieQue(typeof true).doitÊtre("boolean");
    vérifieQue(typeof undeclaredVariable).doitÊtre("undefined");
});
